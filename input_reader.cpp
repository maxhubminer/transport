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
		
		for (string line; query_amount && getline(input, line); --query_amount) {

			auto parse_line = Lstrip(std::move(line));
			auto command = Split(parse_line, ' ');

			if (command.first == "Stop") {
				auto stop_info = ParseStopCommand(command.second);
				transport_cat.AddStop( string(get<0>(stop_info)), get<1>(stop_info), get<2>(stop_info) );
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

	}

	StopInfo InputReader::ParseStopCommand(string_view line) {
		
		line = Lstrip(line);
		auto parse_struct = Split(line, ':');
		auto stopname = LRstrip(parse_struct.first);

		parse_struct = Split(parse_struct.second, ',');

		double latitude = stod(string(LRstrip(parse_struct.first)));
		double longitude = stod(string(LRstrip(parse_struct.second)));

		return { stopname, latitude, longitude};

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