#pragma once

#include "transport_catalogue.h"

#include <string>
#include <string_view>
#include <tuple>

namespace transport {
	namespace output {

		class StatReader {
		public:
			explicit StatReader(const TransportCatalogue& catalogue)
				: catalogue_(catalogue) {};
			void ParseInput(std::istream& input, std::ostream& output);
		private:
			const TransportCatalogue& catalogue_;
		private:
			const std::pair<std::string_view, const Bus*> ParseBusCommand(std::string_view line);
			const std::pair<std::string_view, const Stop*> ParseStopCommand(std::string_view line);
			void OutputRoute(const std::pair<std::string_view, const Bus*> route_info, std::ostream& output);
			void OutputStop(const std::pair<std::string_view, const Stop*>, std::ostream& output);
		};

	}
}