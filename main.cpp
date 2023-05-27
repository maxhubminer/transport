#include "json_reader.h"
#include "request_handler.h"

using namespace std;

int main() {
    
    /*jsonreader_tests::TestCornerCases();
    jsonreader_tests::TestColorParsing();*/
    /*tests::TestCommonCases();
    tests::TestCornerCases();*/

    TransportCatalogue catalogue;
    MapRenderer renderer;

    JsonReader reader(catalogue, renderer);
    reader.Input(std::cin);
    reader.Output(std::cout);

}