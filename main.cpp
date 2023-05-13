
#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>
#include <sstream>

int main() {

	using namespace transport;

	TransportCatalogue transport_cat;
	input::InputReader reader;
    output::StatReader output_reader(transport_cat);

    //reader.ParseInput(std::cin, transport_cat);
    //output_reader.ParseInput(std::cin);

    std::istringstream input{
        "13\n"
        "Stop Tolstopaltsevo : 55.611087, 37.20829\n"
        "Stop Marushkino : 55.595884, 37.209755\n"
        "Bus 256 : Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
        "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
        "Stop Rasskazovka : 55.632761, 37.333324\n"
        "Stop Biryulyovo Zapadnoye : 55.574371, 37.6517\n"
        "Stop Biryusinka : 55.581065, 37.64839\n"
        "Stop Universam : 55.587655, 37.645687\n"
        "Stop Biryulyovo Tovarnaya : 55.592028, 37.653656\n"
        "Stop Biryulyovo Passazhirskaya : 55.580999, 37.659164\n"
        "Bus 828 : Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
        "Stop Rossoshanskaya ulitsa : 55.595579, 37.605757\n"
        "Stop Prazhskaya : 55.611678, 37.603831\n"
        "6\n"
        "Bus 256\n"
        "Bus 750\n"
        "Bus 751\n"
        "Stop Samara\n"
        "Stop Prazhskaya\n"
        "Stop Biryulyovo Zapadnoye\n" };

	reader.ParseInput(input, transport_cat);    
    output_reader.ParseInput(input);

}