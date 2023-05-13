#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <deque>
#include <vector>

namespace transport {

	struct Stop;
	
	struct Bus { // aka Route		

		std::string name;
		std::vector<Stop*> stops;
		bool isFound = true;

		bool operator<(const Bus& other) const {
			return name < other.name;
		}
	};

	struct BusComparator
	{
		bool operator()(const Bus* lhs, const Bus* rhs) const
		{
			return *lhs < *rhs;
		}
	};

	using BusSet = std::set<const Bus*, BusComparator>;
	
	struct Stop {
		std::string name;
		distance::Coordinates coords;
		BusSet buses;
		bool isFound = true;
	};

	struct RouteInfo {
		std::string_view name;
		size_t stops_amount;
		size_t unique_stops_amount;
		double length;
	};

	struct StopInfo {
		std::string_view name;
		const BusSet& buses;
	};

	class TransportCatalogue {
	public:
		void AddRoute(std::string name, const std::vector<std::string>& stopnames, bool isRing);
		void AddStop(std::string name, double latitude, double longitude);
		const Bus& GetRoute(std::string_view busname) const;
		Stop& GetStop(std::string_view stopname) const;
		RouteInfo GetRouteInfo(const Bus& route) const;
		StopInfo GetStopInfo(const Stop& stop) const;

	private:
		std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;

		std::deque<Stop> stops_; // queue does not have iterators; and list is over-functional for the task
		std::deque<Bus> routes_;
	};

}