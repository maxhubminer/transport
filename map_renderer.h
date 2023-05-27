#pragma once

#include "svg.h"
#include "domain.h"
#include "geo.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace transport {

	namespace renderer {

		struct RendererSettings {

			using ArrayDouble = std::vector<double>;
			using ArrayColor = std::vector<svg::Color>;
            // В типе не должно быть int, т.к. значения double в json могут быть записаны БЕЗ точки,
            // и в случае наличия типа int - при загрузке трактовались бы как int, а не double
			using RenderSettings = std::variant<std::monostate, double, std::string, ArrayDouble, svg::Color, ArrayColor>;

			RendererSettings() {
				render_settings_["nullvalue"];
			}
		
			void SetSetting(std::string name, RenderSettings value) {
				render_settings_[std::move(name)] = value;
			}

			const RenderSettings& GetSetting(const std::string& name) const {
				if (render_settings_.count(name)) {
					return render_settings_.at(name);
				}
				else {
					return render_settings_.at("nullvalue");
				}				
			}

		private:
			std::unordered_map<std::string, RenderSettings> render_settings_;
		};

        class SphereProjector;
        
        class MapRenderer {
		public:
			void SetSettings(RendererSettings settings);
            void AddStop(Stop stop);
			void AddRoute(Bus route);
			svg::Document Render() const;
		private:
			RendererSettings settings_;
            std::vector<Stop> stops_;
            std::vector<Bus> buses_;

            void DrawRouteLines(svg::Document& doc, const SphereProjector& proj) const;
            void DrawRouteLabels(svg::Document& doc, const SphereProjector& proj) const;
            void DrawStopShapes(svg::Document& doc, const SphereProjector& proj) const;
            void DrawStopLabels(svg::Document& doc, const SphereProjector& proj) const;
            svg::Text GetStopLabel(svg::Point pos, std::string name) const;
            svg::Text GetRouteLabel(svg::Point stop_pos, std::string stop_name) const;
		};


        
        class SphereProjector {
        public:
            inline static const double EPSILON = 1e-6;

            // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
            template <typename PointInputIt>
            SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                double max_width, double max_height, double padding)
                : padding_(padding) //
            {
                // Если точки поверхности сферы не заданы, вычислять нечего
                if (points_begin == points_end) {
                    return;
                }

                // Находим точки с минимальной и максимальной долготой
                const auto [left_it, right_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
                min_lon_ = left_it->lng;
                const double max_lon = right_it->lng;

                // Находим точки с минимальной и максимальной широтой
                const auto [bottom_it, top_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
                const double min_lat = bottom_it->lat;
                max_lat_ = top_it->lat;

                // Вычисляем коэффициент масштабирования вдоль координаты x
                std::optional<double> width_zoom;
                if (!IsZero(max_lon - min_lon_)) {
                    width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
                }

                // Вычисляем коэффициент масштабирования вдоль координаты y
                std::optional<double> height_zoom;
                if (!IsZero(max_lat_ - min_lat)) {
                    height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
                }

                if (width_zoom && height_zoom) {
                    // Коэффициенты масштабирования по ширине и высоте ненулевые,
                    // берём минимальный из них
                    zoom_coeff_ = std::min(*width_zoom, *height_zoom);
                }
                else if (width_zoom) {
                    // Коэффициент масштабирования по ширине ненулевой, используем его
                    zoom_coeff_ = *width_zoom;
                }
                else if (height_zoom) {
                    // Коэффициент масштабирования по высоте ненулевой, используем его
                    zoom_coeff_ = *height_zoom;
                }
            }

            // Проецирует широту и долготу в координаты внутри SVG-изображения
            svg::Point operator()(geo::Coordinates coords) const {
                return {
                    (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_
                };
            }

            static bool IsZero(double value) {
                return std::abs(value) < EPSILON;
            }

        private:
            double padding_;
            double min_lon_ = 0;
            double max_lat_ = 0;
            double zoom_coeff_ = 0;
        };

	}

}