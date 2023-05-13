#include "stat_reader.h"

#include "input_reader.h" // utils namespace

#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <iterator>

namespace transport::output {

	using namespace std;
	using namespace transport::utils;
	
	void StatReader::ParseInput(std::istream& input)
	{
		string query_amount_str;
		getline(input, query_amount_str);

		unsigned int query_amount = stoi(query_amount_str);

		for (string line; query_amount && getline(input, line); --query_amount) {

			auto parse_line = Lstrip(std::move(line));
			auto command = Split(parse_line, ' ');

			if (command.first == "Bus") {
				const auto& bus_info = ParseBusCommand(command.second);
				OutputRoute(bus_info);
			} else if (command.first == "Stop") {
				const auto& stop_info = ParseStopCommand(command.second);
				OutputStop(stop_info);
			}
		}
	}

	const Bus& StatReader::ParseBusCommand(string_view line) {

		auto routename = LRstrip(line);
		return transport_cat_.GetRoute(routename);
	}

	const Stop& StatReader::ParseStopCommand(string_view line) {

		auto stopname = LRstrip(line);
		return transport_cat_.GetStop(stopname);
	}

	ostream& operator<<(ostream& os, const RouteInfo& route_info)
	{
		os << "Bus "s << route_info.name << ": "s
			<< route_info.stops_amount << " stops on route, "s
			<< route_info.unique_stops_amount << " unique stops, "s
			<< std::setprecision(6) << route_info.length << " route length"s;

		return os;
	}

	void StatReader::OutputRoute(const Bus& route)
	{
		if (!route.isFound) {
			std::cout << "Bus "s << route.name << ": not found"s << std::endl;
			return;
		}
		
		std::cout << transport_cat_.GetRouteInfo(route) << std::endl;
		
	}

	ostream& operator<<(ostream& os, const StopInfo& stop_info)
	{
		if (!stop_info.buses.size()) {
			os << "Stop "s << stop_info.name << ": no buses"s;
			return os;
		}

		os << "Stop "s << stop_info.name << ": buses "s;
		for (auto it = stop_info.buses.cbegin(); it != stop_info.buses.cend(); ++it) {
			os << (*it)->name;
			auto next_it = it;
			;
			if (next(next_it) != stop_info.buses.cend()) {
				os << " "s;
			}
		}

		return os;
	}

	void StatReader::OutputStop(const Stop& stop)
	{
		if (!stop.isFound) {
			std::cout << "Stop "s << stop.name << ": not found"s << std::endl;
			return;
		}
		
		std::cout << transport_cat_.GetStopInfo(stop) << std::endl;
	}

}