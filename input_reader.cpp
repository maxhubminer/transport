#include "input_reader.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <variant>

namespace transport::input {

	using namespace std;
	using namespace transport::utils;
	
	void InputReader::ParseInput(std::istream& input, TransportCatalogue& transport_cat) {

		string query_amount_str;
		getline(input, query_amount_str);

		unsigned int query_amount = stoi(query_amount_str);

		BusQueryQueue bus_queries;
		unordered_map<string_view, vector<DistanceToStop> > stop_distances;
		
		for (string line; query_amount && getline(input, line); --query_amount) {

			auto parse_line = Lstrip(std::move(line));
			auto command = Split(parse_line, ' ');

			if (command.first == "Stop") {
				auto stop_info = ParseStopCommand(command.second);
				string_view stopname = get<0>(stop_info);
				transport_cat.AddStop( string(stopname), get<1>(stop_info), get<2>(stop_info) );
				stop_distances[transport_cat.GetStop(stopname).name] = std::move(get<3>(stop_info));
			}
			else if (command.first == "Bus") {
				auto bus_info = ParseBusCommand(command.second);
				bus_queries.AddQuery(std::move(bus_info));				
			}
			else {
				return; // no supported commands or a number of DB queries
			}
		}

		for (auto& query : bus_queries.GetQueue()) {
			transport_cat.AddRoute(std::move(get<0>(query)), std::move(get<1>(query)), get<2>(query));
		}

		for (auto& [stopname, dest_info_vector] : stop_distances) {
			for (auto& dest_info : dest_info_vector) {
				transport_cat.SetDistance(stopname, dest_info.dest_stopname, dest_info.distance);
			}			
		}

	}

	StopInfo InputReader::ParseStopCommand(string_view line) {
		
		line = Lstrip(line);
		auto parse_struct = Split(line, ':');
		auto stopname = LRstrip(parse_struct.first);

		parse_struct = Split(parse_struct.second, ',');

		double latitude = stod(string(LRstrip(parse_struct.first)));
		
		parse_struct = Split(parse_struct.second, ',');
		double longitude = stod(string(LRstrip(parse_struct.first)));

		vector<DistanceToStop> distances;
		AddStopDistances(parse_struct.second, distances);

		return { stopname, latitude, longitude, distances };

	}

	
	void InputReader::AddStopDistances(string_view line, vector<DistanceToStop>& distances) {
		if (line.empty()) return;

		auto parse_struct = Split(line, ',');

		auto parse_distance = Split(parse_struct.first, 'm');
		int distance = stoi(string(LRstrip(parse_distance.first)));

		string_view second_part = parse_distance.second;

		second_part = Lstrip(second_part);
		second_part.remove_prefix(2); // remove "to"
		string_view dest_stopname = LRstrip(second_part);

		DistanceToStop distance_to_stop{ string(dest_stopname), static_cast<unsigned int>(distance) };
		distances.push_back( std::move(distance_to_stop) );

		AddStopDistances(parse_struct.second, distances);
	}

	BusInfo InputReader::ParseBusCommand(string_view line) {
		
		vector<string> stopnames;
		
		line = Lstrip(line);
		auto parse_struct = Split(line, ':');
		auto busname = LRstrip(parse_struct.first);

		bool isRing = false;
		char split_char = '>';
		
		auto split_result = Split(parse_struct.second, split_char);
		if (split_result.second.size()) {
			isRing = true;
		}
		else {
			split_char = '-';
			split_result = Split(parse_struct.second, split_char);
		}

		while (split_result.second.size()) {
			stopnames.push_back(string(LRstrip(split_result.first)));
			split_result = Split(split_result.second, split_char);
		}
		stopnames.push_back(string(LRstrip(split_result.first)));

		return { string(busname), stopnames, isRing }; // bus queries are processed later
	}

}

namespace transport::utils {

	using namespace std;
	
	string_view Lstrip(string_view line) {
		while (!line.empty() && isspace(line[0])) {
			line.remove_prefix(1);
		}
		return line;
	}

	string_view LRstrip(string_view line) {
		line = Lstrip(line);
		while (!line.empty() && isspace(line[line.size() - 1])) {
			line.remove_suffix(1);
		}
		return line;
	}

	pair<string_view, string_view> Split(string_view line, char by) {
		size_t pos = line.find(by);
		string_view left = line.substr(0, pos);

		if (pos < line.size() && pos + 1 < line.size()) {
			return { left, line.substr(pos + 1) };
		}
		else {
			return { left, string_view() };
		}
	}

	string_view Unbracket(string_view value, char symbol) {
		if (!value.empty() && value.front() == symbol) {
			value.remove_prefix(1);
		}
		if (!value.empty() && value.back() == symbol) {
			value.remove_suffix(1);
		}
		return value;
	}
}