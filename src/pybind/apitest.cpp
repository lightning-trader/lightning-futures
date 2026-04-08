#include <log_define.hpp>
#include <bridge.h>
#include <fstream>

using namespace lt::cta ;

int main(int argc,char* argv[])
{
	std::ifstream file("config.txt", std::ios::binary);
	if (!file.is_open()) {
		return -1;
	}
	std::string json_config = std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
	bridge* app = new bridge(json_config);
	app->start_trading();
	return 0;
}
