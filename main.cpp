
#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>
#include <sstream>

int main() {

	using namespace transport;

	transport::tests::TestCommonCases();
	transport::tests::TestCornerCases();

	/*TransportCatalogue transport_cat;
	input::InputReader reader;
    output::StatReader output_reader(transport_cat);

    reader.ParseInput(std::cin, transport_cat);
    output_reader.ParseInput(std::cin);*/

}