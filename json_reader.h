#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

using namespace json;
using namespace transport;
using namespace renderer;
using namespace std::literals;

namespace transport {

	class JsonReader {
	public:
		explicit JsonReader(TransportCatalogue& catalogue, MapRenderer& renderer)
			: catalogue_(catalogue), renderer_(renderer) {};
	
		void Input(std::istream& input);

		void Output(std::ostream& output);

	private:
		TransportCatalogue& catalogue_;
		MapRenderer& renderer_;
		Array answers_;

		struct DistanceToStop {
			std::string dest_stopname;
			unsigned int distance;
		};

		void HandleBaseRequests(const Array& base_requests);
		void HandleRenderSettings(const Dict& render_settings);
		void HandleStatRequests(const Array& stat_requests);

		svg::Color ParseColor(const Node& color_node);

	};

	namespace jsonreader_tests {
		void TestCornerCases();
		void TestColorParsing();
	}

}