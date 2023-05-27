#include "json_reader.h"
#include "request_handler.h"

#include <sstream>

namespace transport {

	void JsonReader::Input(std::istream & input) {

		Document doc = Load(input);

		// "base_requests": [...] , — массив с описанием автобусных маршрутов и остановок,
		// "stat_requests" : [...] — массив с запросами к транспортному справочнику.
		const Array& base_requests = doc.GetRoot().AsMap().at("base_requests").AsArray();
		const Array& stat_requests = doc.GetRoot().AsMap().at("stat_requests").AsArray();
		// "render_settings": { ... }
		const Dict& render_settings = doc.GetRoot().AsMap().at("render_settings").AsMap();

		HandleBaseRequests(base_requests);
		HandleRenderSettings(render_settings);
		HandleStatRequests(stat_requests);

	}

	void JsonReader::Output(std::ostream& output) {
		Document doc{ answers_ };
		Print(doc, output);
		answers_.clear();
	}

	void JsonReader::HandleBaseRequests(const Array& base_requests) {

		std::vector<const Dict*> bus_requests;
		std::unordered_map<std::string_view, std::vector<DistanceToStop> > stop_distances;

		for (const Node& request_node : base_requests) {
			const Dict& request = request_node.AsMap();
			if (request.at("type").AsString() == "Bus"sv) {
				bus_requests.push_back(&request);
			}
			else if (request.at("type").AsString() == "Stop"sv) {
				/*
				{
					"type": "Stop",
					"name" : "Электросети",
					"latitude" : 43.598701,
					"longitude" : 39.730623,
					"road_distances" : {  // Словарь, задающий дорожное расстояние от этой остановки до соседних.
										  // Каждый ключ в этом словаре — название соседней остановки,
										  // значение — целочисленное расстояние в метрах.
						"Улица Докучаева": 3000,
						"Улица Лизы Чайкиной" : 4300
					}
				}
				*/

				std::string_view stopname = request.at("name").AsString();
				catalogue_.AddStop(std::string(stopname),
					{ request.at("latitude").AsDouble(), request.at("longitude").AsDouble() });

				const Dict& distances = request.at("road_distances").AsMap();
				for (const auto& [key_stopname, value_distance] : distances) {
					DistanceToStop dist{ key_stopname, static_cast<unsigned int>(value_distance.AsInt()) };
					stop_distances[catalogue_.GetStop(stopname)->name].push_back(std::move(dist));
				}
			}
		}

		for (auto& query : bus_requests) {
			/*
			{
				"type": "Bus",
				"name" : "14",
				"stops" : [ // массив с названиями остановок, через которые проходит маршрут.
							// У кольцевого маршрута название последней остановки дублирует название первой.
							// Например: ["stop1", "stop2", "stop3", "stop1"]
					"Улица Лизы Чайкиной",
					"Электросети",
					"Улица Докучаева",
					"Улица Лизы Чайкиной"
				] ,
				"is_roundtrip": true // значение типа bool. true, если маршрут кольцевой
			}
			*/
			std::vector<std::string> bus_stopnames;
			for (const auto& busstop : query->at("stops").AsArray()) {
				bus_stopnames.push_back(busstop.AsString());
			}
			catalogue_.AddRoute(query->at("name").AsString(), bus_stopnames, query->at("is_roundtrip").AsBool());
		}

		for (auto& [stopname, dest_info_vector] : stop_distances) {
			for (auto& dest_info : dest_info_vector) {
				catalogue_.SetDistance(stopname, dest_info.dest_stopname, dest_info.distance);
			}
		}

	}

	void JsonReader::HandleStatRequests(const Array& stat_requests) {

		/*
		{
			"id": 12345678,
			"type" : "Bus", / "Stop"
			"name" : "14"
		}*/
		for (const Node& request_node : stat_requests) {
			const Dict& request = request_node.AsMap();
			if (request.at("type").AsString() == "Bus"sv) {
				
				const auto route_ptr = catalogue_.GetRoute(request.at("name").AsString());

				if (route_ptr) {

					const auto& route_info = catalogue_.GetRouteInfo(*route_ptr);
					/*
					{
						"curvature": 2.18604,
						"request_id" : 12345678,
						"route_length" : 9300,
						"stop_count" : 4,
						"unique_stop_count" : 3
					}*/
					Dict answer{
						{"curvature"s, route_info.curvature},
						{"request_id", request.at("id").AsInt()},
						{"route_length", route_info.length},
						{"stop_count", static_cast<int>(route_info.stops_amount)},
						{"unique_stop_count", static_cast<int>(route_info.unique_stops_amount)}
					};
					answers_.push_back(std::move(answer));

				}
				else {
					Dict answer{
						{"request_id", request.at("id").AsInt()},
						{"error_message", "not found"s},
					};
					answers_.push_back(std::move(answer));
				}
			}
			else if (request.at("type").AsString() == "Stop"sv) {

				const auto stop_ptr = catalogue_.GetStop(request.at("name").AsString());

				if (stop_ptr) {

					const auto& stop_info = catalogue_.GetStopInfo(*stop_ptr);
					/*
					{
					  "buses": [
						  "14", "22к"
					  ],
					  "request_id": 12345
					}*/
					Array buses;

					if (stop_info.buses.has_value()) {

						buses.reserve(stop_info.buses.value().get().size());

						for (auto it = stop_info.buses.value().get().cbegin(); it != stop_info.buses.value().get().cend(); ++it) {
							buses.push_back((*it)->name);
						}

					}

					Dict answer{
						{"buses"s, buses},
						{"request_id", request.at("id").AsInt()},
					};
					answers_.push_back(std::move(answer));
				}
				else {
					Dict answer{						
						{"request_id", request.at("id").AsInt()},
						{"error_message"s, "not found"s},
					};
					answers_.push_back(std::move(answer));
				}				
			}
			else if (request.at("type").AsString() == "Map"sv) {

				RequestHandler handler(catalogue_, renderer_);

				for (const std::string& name : catalogue_.GetStopNames()) {
					const auto& stop_info = catalogue_.GetStopInfo(name);
					if (stop_info.buses.has_value()) {
						const auto stop = handler.GetStopStat(name);
						renderer_.AddStop(*stop);
					}
				}

				for (const std::string& name : catalogue_.GetRouteNames()) {
					const auto bus = handler.GetBusStat(name);
					if (bus.has_value()) {
						renderer_.AddRoute(*bus);
					}
				}

				std::stringstream ss;
				handler.RenderMap().Render(ss);

				Dict answer{						
						{"map"s, ss.str()},
						{"request_id", request.at("id").AsInt()},
				};
				answers_.push_back(std::move(answer));
			}
		}
	}

	svg::Color JsonReader::ParseColor(const Node& color_node) {

		svg::Color result;

		if (color_node.IsString()) {
			result = color_node.AsString();			
		}
		else if (color_node.IsArray()) {
			
			int red = color_node.AsArray()[0].AsInt();
			int green = color_node.AsArray()[1].AsInt();
			int blue = color_node.AsArray()[2].AsInt();
			
			if (color_node.AsArray().size() == 4) {
				result = svg::Rgba(red, green, blue, color_node.AsArray()[3].AsDouble());
			}
			else {
				result = svg::Rgb(red, green, blue);				
			}
		}

		return result;

	}

	void JsonReader::HandleRenderSettings(const Dict& render_settings) {
		/*
		{
			"width": 1200.0, // — ширина и высота изображения в пикселях. Вещественное число в диапазоне от 0 до 100000.
			"height" : 1200.0,

			"padding" : 50.0, //  — отступ краёв карты от границ SVG - документа.
							  // Вещественное число не меньше 0 и меньше min(width, height) / 2.

			"line_width" : 14.0, //  — толщина линий, которыми рисуются автобусные маршруты. Вещественное число в диапазоне от 0 до 100000.
			"stop_radius" : 5.0, //  — радиус окружностей, которыми обозначаются остановки.Вещественное число в диапазоне от 0 до 100000.

			"bus_label_font_size" : 20, //  — размер текста, которым написаны названия автобусных маршрутов. Целое число в диапазоне от 0 до 100000.
			"bus_label_offset" : [7.0, 15.0] , //  — смещение надписи с названием маршрута относительно координат конечной остановки на карте.
											   // Массив из двух элементов типа double. 
											   // Задаёт значения свойств dx и dy SVG - элемента <text>.
											   // Элементы массива — числа в диапазоне от –100000 до 100000.

			"stop_label_font_size" : 20, //  — размер текста, которым отображаются названия остановок. Целое число в диапазоне от 0 до 100000.
			"stop_label_offset" : [7.0, -3.0] , //  — смещение названия остановки относительно её координат на карте.
												// Массив из двух элементов типа double.
												// Задаёт значения свойств dx и dy SVG - элемента <text>.
												// Числа в диапазоне от –100000 до 100000.

			"underlayer_color" : [255, 255, 255, 0.85] , //  — цвет подложки под названиями остановок и маршрутов.
														 // Формат хранения цвета будет ниже.
			"underlayer_width" : 3.0, // — толщина подложки под названиями остановок и маршрутов.
									  // Задаёт значение атрибута stroke - width элемента <text>.
									  // Вещественное число в диапазоне от 0 до 100000.

			"color_palette" : [ //  — цветовая палитра. Непустой массив.
				"green",
					[255, 160, 0],
					"red"
			]
		}*/		
		/*
			Цвет можно указать в одном из следующих форматов:
				- в виде строки, например, "red" или "black";
				- в массиве из трёх целых чисел диапазона[0, 255]. Они определяют r, g и b компоненты цвета в формате svg::Rgb. Цвет[255, 16, 12] нужно вывести в SVG как rgb(255, 16, 12);
				- в массиве из четырёх элементов: три целых числа в диапазоне от[0, 255] и одно вещественное число в диапазоне от[0.0, 1.0].Они задают составляющие red, green, blue и opacity цвета формата svg::Rgba.Цвет, заданный как[255, 200, 23, 0.85], должен быть выведен в SVG как rgba(255, 200, 23, 0.85).
		*/
		using namespace renderer;

		RendererSettings settings;

		for (const auto& [key, val] : render_settings) {
			
			if (key == "underlayer_color"s) {
				settings.SetSetting(key, ParseColor(val));
			} else if (key == "color_palette"s) {

				RendererSettings::ArrayColor colors_arr;
				for (const auto& color_node : val.AsArray()) {
					colors_arr.push_back(ParseColor(color_node));
				}
				settings.SetSetting(key, colors_arr);

			} else if (val.IsInt()) {
				settings.SetSetting(key, val.AsDouble());
			} else if (val.IsDouble()) {
				settings.SetSetting(key, val.AsDouble());
			} else if (val.IsString()) {
				settings.SetSetting(key, val.AsString());
			} else if (val.IsArray()) {
				RendererSettings::ArrayDouble arr;
				for (const auto& element : val.AsArray()) {
					arr.push_back(element.AsDouble());
				}
				settings.SetSetting(key, arr);
			}
		}

		renderer_.SetSettings(std::move(settings));
	}

	namespace jsonreader_tests {

		using namespace json;
		
		void TestCornerCases() {
			Array base_requests_arr{
				Dict{					
					{"type", "Stop"s},
					{"name", "Achtung, baby!"s},
					{"latitude", 50.0},
					{"longitude", 10.0},
					{"road_distances", Dict{}}
				}				
			};

			Array stat_requests_arr{
				Dict{
					{"id", 100},
					{"type", "Stop"s},
					{"name", "Neverstop"s }
				},
				Dict{
					{"id", 200},
					{"type", "Bus"s},
					{"name", "Neverbus"s }
				},
				Dict{
					{"id", 300},
					{"type", "Stop"s},
					{"name", "Achtung, baby!"s } // no buses
				}
			};

			Dict render_settings_dict{};

			Dict root{
					{"base_requests", base_requests_arr},
					{"stat_requests", stat_requests_arr},
					{"render_settings", render_settings_dict}
			};

			TransportCatalogue catalogue;
			MapRenderer renderer;
			JsonReader reader(catalogue, renderer);
			
			std::stringstream strm;
			json::Print(Document{ root }, strm);
			reader.Input(strm);
			reader.Output(std::cout);
			
		}

		void TestColorParsing() {
			
			Node color1{ "magenta"s };
			Node color2{ Array{156, 200, 13} };
			Node color3{ Array{156, 200, 13, 0.5} };

			Array base_requests_arr;

			Array stat_requests_arr;

			Dict render_settings_dict{
				{"underlayer_color", color1},
				{"underlayer_color", color2},
				{"underlayer_color", color3},
			};

			Dict root{
					{"base_requests", base_requests_arr},
					{"stat_requests", stat_requests_arr},
					{"render_settings", render_settings_dict}
			};

			TransportCatalogue catalogue;
			MapRenderer renderer;
			JsonReader reader(catalogue, renderer);

			std::stringstream strm;
			json::Print(Document{ root }, strm);
			reader.Input(strm);
			reader.Output(std::cout);

		}

	}

}