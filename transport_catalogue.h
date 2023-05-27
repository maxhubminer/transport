#pragma once

#include "domain.h"
#include "geo.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <deque>
#include <vector>
#include <optional>
#include <functional>

namespace transport {

	struct RouteInfo {
		std::string_view name;
		size_t stops_amount;
		size_t unique_stops_amount;
		double length;
		double curvature;
	};

	struct StopInfo {
		std::string_view name;
		const std::optional<std::reference_wrapper<const BusSet>> buses;
	};

	class TransportCatalogue {
	public:
		void AddRoute(std::string name, const std::vector<std::string>& stopnames, bool isRing);
		void AddStop(std::string name, ::geo::Coordinates coords);

		const Bus* GetRoute(std::string_view busname) const;
		const Stop* GetStop(std::string_view stopname) const;

		RouteInfo GetRouteInfo(std::string_view routename) const;
		RouteInfo GetRouteInfo(const Bus& route) const;

		StopInfo GetStopInfo(std::string_view stopname) const;
		StopInfo GetStopInfo(const Stop& stop) const;

		void SetDistance(std::string_view stopname1, std::string_view stopname2, unsigned int distance);
		unsigned int GetDistance(std::string_view stopname1, std::string_view stopname2) const;

		std::set<std::string> GetStopNames() const;
		std::set<std::string> GetRouteNames() const;

	private:
		struct PtrPairHasher {
			size_t operator()(std::pair<const Stop*, const Stop*> pair_) const noexcept {
				return std::hash<const void*>{}(pair_.first) + 3 * std::hash<const void*>{}(pair_.second);
			}
		};

	private:
		std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, unsigned int, PtrPairHasher> stops_distance_;
		std::unordered_map<std::string_view, BusSet> stopname_to_buses_;

		std::deque<Stop> stops_; // queue does not have iterators; and list is over-functional for the task
		std::deque<Bus> routes_;
	};

	namespace tests {
		void TestCommonCases();
		void TestCornerCases();
	}

}