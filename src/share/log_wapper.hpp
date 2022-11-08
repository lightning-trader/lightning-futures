#pragma once
#include <fstream>
/*
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
*/
/*
bool init_log_environment(std::string _cfg)
{
	namespace logging = boost::log;
	using namespace logging::trivial;

	if (!boost::filesystem::exists("./log/"))
	{
		boost::filesystem::create_directory("./log/");
	}
	logging::add_common_attributes();

	logging::register_simple_formatter_factory<severity_level, char>("Severity");
	logging::register_simple_filter_factory<severity_level, char>("Severity");

	std::ifstream file(_cfg);
	try
	{
		logging::init_from_stream(file);
	}
	catch (const std::exception& e)
	{
		std::cout << "init_logger is fail, read log config file fail. curse: " << e.what() << std::endl;
		exit(-2);
	}
	return true;
}
*/
#define LOG_DEBUG(format, ...) printf(format, __VA_ARGS__);
#define LOG_INFO(format, ...) printf(format, __VA_ARGS__);
#define LOG_WARNING(format, ...) //printf(format, __VA_ARGS__);
#define LOG_ERROR(format, ...) //printf(format, __VA_ARGS__);
#define LOG_OPTIMIZE(format, ...) printf(format, __VA_ARGS__);