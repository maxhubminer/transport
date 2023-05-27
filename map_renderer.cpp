#include "map_renderer.h"

namespace transport::renderer {

	void MapRenderer::SetSettings(RendererSettings settings) {
		settings_ = std::move(settings);
	}

	void MapRenderer::AddStop(Stop stop) {
		stops_.push_back(stop);
	}

	void MapRenderer::AddRoute(Bus route) {
		buses_.push_back(route);
	}

	size_t NextColorIndex(size_t current_index, const RendererSettings::ArrayColor& palette) {
		size_t result = current_index + 1;
		if (palette.size() <= result) {
			result = 0;
		}
		return result;
	}

	void MapRenderer::DrawRouteLines(svg::Document& doc, const SphereProjector& proj) const {

		using namespace svg;
		using namespace std::literals;
		
		size_t color_index = 0;
		const auto& palette = std::get<RendererSettings::ArrayColor>(settings_.GetSetting("color_palette"));
		for (const auto& route : buses_) {

			if (!route.stops.size()) continue;

			Polyline line;

			line.SetFillColor("none"s);			
			line.SetStrokeWidth(std::get<double>(settings_.GetSetting("line_width")));
			line.SetStrokeLineCap(StrokeLineCap::ROUND);
			line.SetStrokeLineJoin(StrokeLineJoin::ROUND);
			
			line.SetStrokeColor(palette[color_index]);
			color_index = NextColorIndex(color_index, palette);

			for (const auto stop_ptr : route.stops) {
				const svg::Point screen_coord = proj(stop_ptr->coords);
				line.AddPoint({ screen_coord.x, screen_coord.y });
			}

			doc.Add(std::move(line));

		}

	}

	svg::Text MapRenderer::GetRouteLabel(svg::Point pos, std::string name) const {

		using namespace svg;

		Text text;
		text.SetPosition(pos);
		
		const auto& offset_arr = std::get<RendererSettings::ArrayDouble>(settings_.GetSetting("bus_label_offset"));
		text.SetOffset({ offset_arr[0], offset_arr[1] });
		text.SetFontSize(static_cast<int>(std::get<double>(settings_.GetSetting("bus_label_font_size"))));
		text.SetFontFamily("Verdana");
		text.SetFontWeight("bold");
		text.SetData(std::move(name));

		return text;
	}

	svg::Text MapRenderer::GetStopLabel(svg::Point pos, std::string name) const {

		using namespace svg;

		Text text;
		text.SetPosition(pos);

		const auto& offset_arr = std::get<RendererSettings::ArrayDouble>(settings_.GetSetting("stop_label_offset"));
		text.SetOffset({ offset_arr[0], offset_arr[1] });
		text.SetFontSize(static_cast<int>(std::get<double>(settings_.GetSetting("stop_label_font_size"))));
		text.SetFontFamily("Verdana");
		text.SetData(std::move(name));

		return text;
	}

	void MapRenderer::DrawRouteLabels(svg::Document& doc, const SphereProjector& proj) const {

		using namespace svg;		

		size_t color_index = 0;
		const auto& palette = std::get<RendererSettings::ArrayColor>(settings_.GetSetting("color_palette"));
		for (const auto& route : buses_) {

			std::vector<const Stop*> labeled_stops;
			if (route.stops.size()) {
				const auto start_stop = route.stops[0];
				labeled_stops.push_back(start_stop);
				
				size_t final_stop_index = 0;
				if (route.isRing) {
					final_stop_index = route.stops.size() - 1;
				}
				else {
					final_stop_index = route.stops.size()/2;
				}
				const auto final_stop = route.stops[final_stop_index];
				if (start_stop->name != final_stop->name) {
					labeled_stops.push_back(final_stop);
				}
			}
			for (const auto stop_ptr : labeled_stops) {
				
				const svg::Point screen_coord = proj(stop_ptr->coords);
				
				// underlayer:
				Text underlayer = GetRouteLabel(screen_coord, route.name);
				underlayer.SetFillColor(std::get<Color>(settings_.GetSetting("underlayer_color")));
				underlayer.SetStrokeColor(std::get<Color>(settings_.GetSetting("underlayer_color")));
				underlayer.SetStrokeWidth(std::get<double>(settings_.GetSetting("underlayer_width")));
				underlayer.SetStrokeLineCap(StrokeLineCap::ROUND);
				underlayer.SetStrokeLineJoin(StrokeLineJoin::ROUND);

				// label
				Text label = GetRouteLabel(screen_coord, route.name);
				label.SetFillColor(palette[color_index]);

				doc.Add(std::move(underlayer));
				doc.Add(std::move(label));
			}

			color_index = NextColorIndex(color_index, palette);
		}

	}

	void MapRenderer::DrawStopShapes(svg::Document& doc, const SphereProjector& proj) const {

		using namespace svg;
		
		for (const auto& stop : stops_) {
			const svg::Point screen_coord = proj(stop.coords);

			Circle stop_shape;
			stop_shape.SetCenter(screen_coord).SetRadius(std::get<double>(settings_.GetSetting("stop_radius")));
			stop_shape.SetFillColor("white");

			doc.Add(std::move(stop_shape));
		}
	}

	void MapRenderer::DrawStopLabels(svg::Document& doc, const SphereProjector& proj) const {

		using namespace svg;

		for (const auto& stop : stops_) {

			const svg::Point screen_coord = proj(stop.coords);

			// underlayer:
			Text underlayer = GetStopLabel(screen_coord, stop.name);
			underlayer.SetFillColor(std::get<Color>(settings_.GetSetting("underlayer_color")));
			underlayer.SetStrokeColor(std::get<Color>(settings_.GetSetting("underlayer_color")));
			underlayer.SetStrokeWidth(std::get<double>(settings_.GetSetting("underlayer_width")));
			underlayer.SetStrokeLineCap(StrokeLineCap::ROUND);
			underlayer.SetStrokeLineJoin(StrokeLineJoin::ROUND);

			// label
			Text label = GetStopLabel(screen_coord, stop.name);
			label.SetFillColor("black");

			doc.Add(std::move(underlayer));
			doc.Add(std::move(label));
		}
	}

	svg::Document MapRenderer::Render() const {
		
		using namespace svg;
		using namespace std::literals;

		std::vector<geo::Coordinates> geo_coords;
		
		for (const auto& route : buses_) {
			if (!route.stops.size()) continue;
			for (const auto stop_ptr : route.stops) {
				geo_coords.push_back(stop_ptr->coords);
			}
		}

		const double WIDTH = std::get<double>(settings_.GetSetting("width"));
		const double HEIGHT = std::get<double>(settings_.GetSetting("height"));
		const double PADDING = std::get<double>(settings_.GetSetting("padding"));

		// Создаём проектор сферических координат на карту
		const SphereProjector proj{
			geo_coords.begin(), geo_coords.end(), WIDTH, HEIGHT, PADDING
		};
		
		
		svg::Document doc;
		DrawRouteLines(doc, proj);
		DrawRouteLabels(doc, proj);
		DrawStopShapes(doc, proj);
		DrawStopLabels(doc, proj);

		return doc;

	}
}