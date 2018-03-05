#include "jsonpp.h"
#include <iostream>
using namespace jsonpp;


int main() {
	Json my_json = Json::object{
		{ "key1", "value1" },
	{ "key2", false },
	{ "key3", Json::array { 1, 2, 3 } },
	};
	std::string json_str = my_json.dump();
	std::cout << json_str << std::endl;

	return 0;
}