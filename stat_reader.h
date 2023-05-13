#pragma once

#include "transport_catalogue.h"

#include <string>
#include <string_view>
#include <tuple>

namespace transport {
	namespace output {

		class StatReader {
		public:
			explicit StatReader(const TransportCatalogue& transport_cat)
				: transport_cat_(transport_cat) {};
			void ParseInput(std::istream& input);
		private:
			const TransportCatalogue& transport_cat_;
		private:
			const Bus& ParseBusCommand(std::string_view line);
			void OutputRoute(const Bus&);
		};

	}
}