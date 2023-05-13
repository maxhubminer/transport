#include "transport_catalogue.h"

#include <algorithm>
#include <iterator>

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

		if (route.stops.size()) {

			for (int i = 1; i < route.stops.size(); ++i) {
				unique_stops.insert(route.stops[i]->name);
				route_length += distance::ComputeDistance(route.stops[i - 1]->coords, route.stops[i]->coords);
			}

		}

		return { route.name, route.stops.size(), unique_stops.size(), route_length };
	}

	StopInfo TransportCatalogue::GetStopInfo(const Stop& stop) const
	{
		return { stop.name, stop.buses };
	}

}