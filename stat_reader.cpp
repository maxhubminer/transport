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

	void StatReader::OutputRoute(const Bus& route)
	{
		if (!route.isFound) {
			std::cout << "Bus "s << route.name << ": not found"s << std::endl;
			return;
		}

		
		std::unordered_set<string_view> unique_stops;
		double route_length = 0;

		if (route.stops.size()) {

			for (int i = 1; i < route.stops.size(); ++i) {
				unique_stops.insert(route.stops[i]->name);
				route_length += distance::ComputeDistance(route.stops[i - 1]->coords, route.stops[i]->coords);
			}

		}

		std::cout << "Bus "s << route.name << ": "s
			<< route.stops.size() << " stops on route, "s
			<< unique_stops.size() << " unique stops, "s
			<< std::setprecision(6) << route_length << " route length"s << std::endl;
	}

	void StatReader::OutputStop(const Stop& stop)
	{
		if (!stop.isFound) {
			std::cout << "Stop "s << stop.name << ": not found"s << std::endl;
			return;
		}

		if (!stop.buses.size()) {
			std::cout << "Stop "s << stop.name << ": no buses"s << std::endl;
			return;
		}

		std::cout << "Stop "s << stop.name << ": buses "s;
		for (auto it = stop.buses.cbegin(); it != stop.buses.cend(); ++it) {			
			std::cout << (*it)->name;
			auto next_it = it;
			;
			if (next(next_it) != stop.buses.cend()) {
				std::cout << " "s;
			}
		}
		std::cout << std::endl;
	}

}