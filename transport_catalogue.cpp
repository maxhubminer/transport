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
		vector<Stop*> stop_ptrs(stop_amount);

		transform(stopnames.cbegin(), stopnames.cend(), stop_ptrs.begin(), [this](const auto& stopname) {
			return &GetStop(stopname);
			});

		if (!isRing) {
			auto it = stop_ptrs.begin();
			advance(it, stopnames.size());
			transform(++stopnames.crbegin(), stopnames.crend(), it, [this](const auto& stopname) {
				return &GetStop(stopname);
				});
		}

		routes_.push_front({ name, std::move(stop_ptrs) });
		busname_to_bus_[routes_.front().name] = &routes_.front();

		for (auto stop_ptr : routes_.front().stops) {
			stop_ptr->buses.insert(&routes_.front());
		}
	}

	void TransportCatalogue::AddStop(std::string name, double latitude, double longitude)
	{
		distance::Coordinates coords{ latitude, longitude };
		stops_.push_front({ name, coords, {} });
		stopname_to_stop_[stops_.front().name] = &stops_.front();
	}

	const Bus& TransportCatalogue::GetRoute(std::string_view busname) const
	{
		auto it = busname_to_bus_.find(busname);

		if (it == busname_to_bus_.end()) {
			static Bus nobus;
			nobus.name = string(busname);
			nobus.isFound = false;
			return nobus;
		}

		return *it->second;
	}

	Stop& TransportCatalogue::GetStop(std::string_view stopname) const
	{
		auto it = stopname_to_stop_.find(stopname);

		if (it == stopname_to_stop_.end()) {
			static Stop nostop;
			nostop.name = string(stopname);
			nostop.isFound = false;
			return nostop;
		}

		return *it->second;
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

	StopInfo TransportCatalogue::GetStopInfo(const Stop& stop) const
	{
		return { stop.name, stop.buses };
	}

	void TransportCatalogue::SetDistance(std::string_view stopname1, std::string_view stopname2, unsigned int distance) {
		stops_distance_[{ &GetStop(stopname1), &GetStop(stopname2) }] = distance;
	}

	unsigned int TransportCatalogue::GetDistance(std::string_view stopname1, std::string_view stopname2) const {
		auto key = std::make_pair<const Stop*, const Stop*>(&GetStop(stopname1), &GetStop(stopname2));
		if (!stops_distance_.count(key)) {
			key = std::make_pair<const Stop*, const Stop*>(&GetStop(stopname2), &GetStop(stopname1));
		}

		if (!stops_distance_.count(key)) {
			return 0;
		}

		return stops_distance_.at(key);
	}

	namespace tests {
		void tests::TestCommonCases()
		{
			TransportCatalogue transport_cat;

			auto stop = transport_cat.GetStop("unknown");
			assert(stop.isFound == false);
			
			auto route = transport_cat.GetRoute("unknown");
			assert(route.isFound == false);
			
			transport_cat.AddStop("stop A", 53.199489, -105.759253);
			transport_cat.AddStop("stop B", 54.840504, 46.591607);
			transport_cat.AddStop("stop C", -23.354995, 119.732057);
			transport_cat.AddStop("stop D", 48.071613, 114.524894);
			transport_cat.AddStop("stop E", 64.136986, -21.872559);
			transport_cat.AddStop("stop F", -36.851350, 174.762452);

			transport_cat.AddRoute("bus 001", { "stop C", "stop A", "stop A", "stop F", "stop D", "stop C"}, true);
			auto route001 = transport_cat.GetRouteInfo(transport_cat.GetRoute("bus 001"));
			assert(route001.stops_amount == 6);
			assert(route001.unique_stops_amount == 4);
			
			transport_cat.SetDistance("stop A", "stop B", 100);
			transport_cat.AddRoute("bus 999", { "stop A", "stop B", "stop E" }, false);
			auto route999 = transport_cat.GetRouteInfo(transport_cat.GetRoute("bus 999"));
			assert(route999.stops_amount == 5);
			assert(route999.length == 200);
			assert(route999.curvature == 8.6733320167853890e-06);

		}

		void tests::TestCornerCases()
		{
			TransportCatalogue transport_cat;

			transport_cat.AddStop("A", 53.199489, -105.759253);
			transport_cat.AddStop("B", 54.840504, 46.591607);
			transport_cat.AddStop("C", -23.354995, 119.732057);
			transport_cat.AddStop("  ", .0, .0);

			transport_cat.SetDistance("  ", "A", 1000);
			assert(transport_cat.GetDistance("A", "  ") == 1000);

			transport_cat.SetDistance("A", "A", 300);
			transport_cat.AddRoute("cyclic", {"A", "A", "A"}, false);
			auto cyclic_info = transport_cat.GetRouteInfo(transport_cat.GetRoute("cyclic"));
			assert(cyclic_info.length == 1200);
			assert(cyclic_info.unique_stops_amount == 1);

			transport_cat.AddRoute("empty", {  }, true);
			assert(transport_cat.GetRouteInfo(transport_cat.GetRoute("empty")).stops_amount == 0);
			transport_cat.AddRoute("empty ring", {  }, true);
			assert(transport_cat.GetRouteInfo(transport_cat.GetRoute("empty ring")).stops_amount == 0);
			transport_cat.AddRoute(" ", { "  ", "B", "C" }, true);
			assert(transport_cat.GetRouteInfo(transport_cat.GetRoute(" ")).name == " ");

			// overflow check
			unsigned int max_value = std::numeric_limits<unsigned int>::max();
			transport_cat.SetDistance("B", "C", max_value);			
			transport_cat.AddRoute("B2C_and_back", { "B", "C" }, false);
			auto overflow_info = transport_cat.GetRouteInfo(transport_cat.GetRoute("B2C_and_back"));
			assert(overflow_info.length == max_value*2.0);
			
		}
	}

}