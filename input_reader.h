#pragma once

#include "transport_catalogue.h"

#include <string>
#include <string_view>
#include <tuple>

namespace transport {
	namespace input {

		typedef std::tuple<std::string_view, double, double> StopInfo;
		typedef std::tuple<std::string, std::vector<std::string>, bool> BusInfo;

		class BusQueryQueue {
		public:
			void AddQuery(BusInfo&& query) {
				queue_.push_back(std::move(query));
			}

			void Clear() {
				queue_.clear();
			}

			const std::vector<BusInfo>& GetQueue() const {
				return queue_;
			}

		private:
			std::vector<BusInfo> queue_;
		};
		
		class InputReader {
		public:
			void ParseInput(std::istream& input, TransportCatalogue& transport_cat);
		private:
			StopInfo ParseStopCommand(std::string_view line);
			BusInfo ParseBusCommand(std::string_view line);
			std::string_view Lstrip(std::string_view line);
			std::string_view LRstrip(std::string_view line);
			std::pair<std::string_view, std::string_view> Split(std::string_view line, char by);
			std::string_view Unbracket(std::string_view value, char symbol);
		};

	}
}