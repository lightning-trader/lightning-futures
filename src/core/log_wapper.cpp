#include <define.h>
#include <fstream>
#include <filesystem>
#include <boost/scoped_array.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/sources/record_ostream.hpp>

bool _is_log_inited = false ;

bool _is_log_ready = false ;

bool init_log_environment(std::string _cfg)
{
	
	if (!std::filesystem::exists(_cfg))
	{
		return false;
	}

	namespace logging = boost::log;
	using namespace logging::trivial;

	if (!std::filesystem::exists("./log/"))
	{
		std::filesystem::create_directory("./log/");
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
		return false;
	}
	return true;
}
/*
#define LOG_TRACE \
   BOOST_LOG_STREAM_WITH_PARAMS( \
      *(crush::common::get_glog()), \
      (set_get_attrib("File", path_to_filename(__FILE__))) \
      (set_get_attrib("Line", __LINE__)) \
      (set_get_attrib("Function", __func__)) \
      (::boost::log::keywords::severity = (crush::common::SL_TRACE)) \
   )
*/

void log_format(log_level lv, const char* format, ...)
{
	if(!_is_log_inited)
	{
		_is_log_ready = init_log_environment("./log.ini");
		_is_log_inited = true ;
	}
	if(!_is_log_ready)
	{
		return ;
	}
	va_list arg_list;
	va_start(arg_list, format);
	char formatted_string[10240] = {0};
	vsnprintf(formatted_string, 10240, format, arg_list);
	//printf(formatted_string.get());
	//BOOST_LOG(crush::common::SL_TRACE) << formatted_string;
	switch(lv)
	{
		case LLV_TRACE:
			BOOST_LOG_TRIVIAL(trace) << formatted_string;
		break;
		case LLV_DEBUG:
			BOOST_LOG_TRIVIAL(debug) << formatted_string;
			break;
		case LLV_INFO:
			BOOST_LOG_TRIVIAL(info) << formatted_string;
			break;
		case LLV_WARNING:
			BOOST_LOG_TRIVIAL(warning) << formatted_string;
			break;
		case LLV_ERROR:
			BOOST_LOG_TRIVIAL(error) << formatted_string;
			break;
		case LLV_FATAL:
			BOOST_LOG_TRIVIAL(fatal) << formatted_string;
			break;
	}
	va_end(arg_list);
}
