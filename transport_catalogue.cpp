#include "transport_catalogue.h"

#include <algorithm>
#include <iterator>

#include <cassert>
#include <limits>

namespace transport {

	using namespace std;

	void TransportCatalogue::AddRoute(std::string name, const std::vector<std::string>& stopnames, bool isRing)
	{
		size_t stop_amount = stopnames.size() * (isRing ? 1 : 2) - (isRing ? 0 : 1);
		vector<const Stop*> stop_ptrs(stop_amount);

		transform(stopnames.cbegin(), stopnames.cend(), stop_ptrs.begin(), [this](const auto& stopname) {
			return GetStop(stopname);
			});

		if (!isRing) {
			auto it = stop_ptrs.begin();
			advance(it, stopnames.size());
			transform(++stopnames.crbegin(), stopnames.crend(), it, [this](const auto& stopname) {
				return GetStop(stopname);
				});
		}

		routes_.push_front({ name, std::move(stop_ptrs) });
		busname_to_bus_[routes_.front().name] = &routes_.front();

		for (auto stop_ptr : routes_.front().stops) {
			stopname_to_buses_[stop_ptr->name].insert(&routes_.front());
		}
	}

	void TransportCatalogue::AddStop(std::string name, distance::Coordinates coords)
	{
		stops_.push_front({ name, coords });
		stopname_to_stop_[stops_.front().name] = &stops_.front();
	}

	const Bus* TransportCatalogue::GetRoute(std::string_view busname) const
	{
		auto it = busname_to_bus_.find(busname);

		if (it == busname_to_bus_.end()) {
			return nullptr;
		}

		return it->second;
	}

	const Stop* TransportCatalogue::GetStop(std::string_view stopname) const
	{
		auto it = stopname_to_stop_.find(stopname);

		if (it == stopname_to_stop_.end()) {
			return nullptr;
		}

		return it->second;
	}

	RouteInfo TransportCatalogue::GetRouteInfo(string_view routename) const
	{
		return GetRouteInfo(*GetRoute(routename));
	}
	
	RouteInfo TransportCatalogue::GetRouteInfo(const Bus& route) const
	{
		unordered_set<string_view> unique_stops;
		double route_length = 0;
		double geo_route_length = 0;

		if (route.stops.size()) {

			for (int i = 1; i < route.stops.size(); ++i) {
				unique_stops.insert(route.stops[i]->name);
				geo_route_length += distance::ComputeDistance(route.stops[i - 1]->coords, route.stops[i]->coords);
				route_length += GetDistance(route.stops[i - 1]->name, route.stops[i]->name);
			}

		}

		double curvature = route_length / geo_route_length;

		return { route.name, route.stops.size(), unique_stops.size(), route_length, curvature };
	}

	StopInfo TransportCatalogue::GetStopInfo(string_view stopname) const
	{
		return GetStopInfo(*GetStop(stopname));
	}

	StopInfo TransportCatalogue::GetStopInfo(const Stop& stop) const
	{
		return { stop.name, 
			stopname_to_buses_.count(stop.name) ? 
			std::optional<BusSet>{ stopname_to_buses_.at(stop.name) } : std::nullopt };
	}

	void TransportCatalogue::SetDistance(std::string_view stopname_from, std::string_view stopname_to, unsigned int distance) {
		stops_distance_[{ GetStop(stopname_from), GetStop(stopname_to) }] = distance;
	}

	unsigned int TransportCatalogue::GetDistance(std::string_view stopname_from, std::string_view stopname_to) const {
		auto key = std::make_pair<const Stop*, const Stop*>(GetStop(stopname_from), GetStop(stopname_to));
		if (!stops_distance_.count(key)) {
			key = std::make_pair<const Stop*, const Stop*>(GetStop(stopname_to), GetStop(stopname_from));
		}

		if (!stops_distance_.count(key)) {
			return 0;
		}

		return stops_distance_.at(key);
	}

	namespace tests {
		void TestCommonCases()
		{
			TransportCatalogue catalogue;

			auto stop = catalogue.GetStop("unknown");
			assert(!stop);
			
			auto route = catalogue.GetRoute("unknown");
			assert(!route);
			
			catalogue.AddStop("stop A", { 53.199489, -105.759253 });
			catalogue.AddStop("stop B", { 54.840504, 46.591607 });
			catalogue.AddStop("stop C", { -23.354995, 119.732057 });
			catalogue.AddStop("stop D", { 48.071613, 114.524894 });
			catalogue.AddStop("stop E", { 64.136986, -21.872559 });
			catalogue.AddStop("stop F", { -36.851350, 174.762452 });

			catalogue.AddRoute("bus 001", { "stop C", "stop A", "stop A", "stop F", "stop D", "stop C"}, true);
			auto route001 = catalogue.GetRouteInfo("bus 001");
			assert(route001.stops_amount == 6);
			assert(route001.unique_stops_amount == 4);
			
			catalogue.SetDistance("stop A", "stop B", 100);
			catalogue.AddRoute("bus 999", { "stop A", "stop B", "stop E" }, false);
			auto route999 = catalogue.GetRouteInfo("bus 999");
			assert(route999.stops_amount == 5);
			assert(route999.length == 200);
			assert(route999.curvature == 8.6733320170381233e-06);

		}

		void TestCornerCases()
		{
			TransportCatalogue catalogue;

			catalogue.AddStop("A", { 53.199489, -105.759253 });
			catalogue.AddStop("B", { 54.840504, 46.591607 });
			catalogue.AddStop("C", { -23.354995, 119.732057 });
			catalogue.AddStop("  ", { .0, .0 });

			catalogue.SetDistance("  ", "A", 1000);
			assert(catalogue.GetDistance("A", "  ") == 1000);

			catalogue.SetDistance("A", "A", 300);
			catalogue.AddRoute("cyclic", {"A", "A", "A"}, false);
			auto cyclic_info = catalogue.GetRouteInfo("cyclic");
			assert(cyclic_info.length == 1200);
			assert(cyclic_info.unique_stops_amount == 1);

			catalogue.AddRoute("empty", {  }, true);
			assert(catalogue.GetRouteInfo("empty").stops_amount == 0);
			catalogue.AddRoute("empty ring", {  }, true);
			assert(catalogue.GetRouteInfo("empty ring").stops_amount == 0);
			catalogue.AddRoute(" ", { "  ", "B", "C" }, true);
			assert(catalogue.GetRouteInfo(" ").name == " ");

			// overflow check
			unsigned int max_value = std::numeric_limits<unsigned int>::max();
			catalogue.SetDistance("B", "C", max_value);			
			catalogue.AddRoute("B2C_and_back", { "B", "C" }, false);
			auto overflow_info = catalogue.GetRouteInfo("B2C_and_back");
			assert(overflow_info.length == max_value*2.0);
			
		}
	}

}