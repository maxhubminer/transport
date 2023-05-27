#pragma once

#include "geo.h"

#include <string>
#include <vector>
#include <set>

namespace transport {

	struct Stop;

	struct Bus { // aka Route		

		std::string name;
		std::vector<const Stop*> stops;
		bool isRing;

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
		::geo::Coordinates coords;
	};

}