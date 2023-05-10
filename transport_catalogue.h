#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>

namespace transport {

	struct Stop {
		std::string name;
		distance::Coordinates coords;
	};
	
	struct Bus { // aka Route
		std::string name;
		std::vector<const Stop*> stops;
	};	

	class TransportCatalogue {
	public:
		void AddRoute(std::string name, const std::vector<std::string>& stopnames, bool isRing);
		void AddStop(std::string name, double latitude, double longitude);
		const Bus& GetRoute(std::string_view busname) const;
		const Stop& GetStop(std::string_view stopname) const;
		void GetRouteInfo();

	private:
		std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;

		std::deque<Stop> stops_; // queue does not have iterators; and list is over-functional for the task
		std::deque<Bus> routes_;
	};

}