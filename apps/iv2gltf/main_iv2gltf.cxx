#include <Inventor/SoDB.h>
#include <iostream>
#include <cxxopts.hpp>

int main(int argc, char* argv[])
{
	cxxopts::Options options("iv2gltf", "a converter for open inventor to gltf");
	options.add_options()
		//("d,debug", "Enable debugging") // a bool parameter
		//("o,integer", "Int param", cxxopts::value<int>())
		("o,gltf", "gltf file", cxxopts::value<std::string>()->default_value(""))
		("i,iv", "inventor file", cxxopts::value<std::string>())
		("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
		("h,help", "Print usage")
		;
	
	cxxopts::ParseResult result; 
	try {
		result = options.parse(argc, argv);		
	}
	catch (const cxxopts::OptionParseException& x) {
		std::cerr << "error: " << x.what() << '\n';
		std::cout << options.help() << std::endl;
		return EXIT_FAILURE;
	}
	if (result.count("help"))
	{
		std::cout << options.help() << std::endl;
		return EXIT_SUCCESS;
	}

	SoDB::init();
	if (result.count("")) {

	}
	return EXIT_SUCCESS; 
}