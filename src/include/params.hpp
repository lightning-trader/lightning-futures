#pragma once
#include <define.h>
#include <data_types.hpp>
#include <string_helper.hpp>
#include <stdexcept>
#include <regex>

class params
{
	std::map<std::string, std::string> _params;

public:
	params() = default;

	params(const std::map<std::string, std::string>& params) :_params(params) {}
	
	void set_data(const std::map<std::string, std::string>& params) 
	{
		_params = params;
	}
	std::map<std::string, std::string> data() const
	{
		return (_params);
	}
	void from_file(const char* filename)
	{
		FILE* fp = fopen(filename, "r");
		if (!fp) { return; }
		_params.clear();
		char buf[128];
		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			if (char* r = strchr(buf, '\r')) { r[0] = '\0'; }
			if (char* r = strchr(buf, '\n')) { r[0] = '\0'; }

			std::string line{ buf };
			std::smatch sm;
			std::regex r{ "([[:alpha:]_]+)[ = ]+(.+)" };
			if (std::regex_match(line, sm, r)) { _params[sm[1].str()] = sm[2].str(); }
		}
		fclose(fp);
	}
	void from_string(const std::string& param)
	{
		_params.clear();
		auto param_pair = string_helper::split(param, '&');
		for (auto it : param_pair)
		{
			auto param = string_helper::split(it, '=');
			_params[param[0]] = param[1];
		}
	}

	template <typename T>
	T get(const char key[]) const
	{
		return get<T>(std::string(key));
	}

private:
	template <typename T>
	typename std::enable_if<std::is_same<T, const char*>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return it->second.c_str();
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, std::string>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return it->second;
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, code_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return code_t(it->second.c_str());
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, int8_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return static_cast<int8_t>(std::atoi(it->second.c_str()));
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, uint8_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return static_cast<uint8_t>(std::atoi(it->second.c_str()));
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, int16_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return static_cast<int16_t>(std::atoi(it->second.c_str()));
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, uint16_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return static_cast<uint16_t>(std::atoi(it->second.c_str()));
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, int32_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return std::atoi(it->second.c_str());
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, uint32_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return static_cast<uint32_t>(std::atoi(it->second.c_str()));
	}

	template <typename T>
	typename std::enable_if<std::is_same<T, double_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return std::atof(it->second.c_str());
	}
	template <typename T>
	typename std::enable_if<std::is_same<T, int64_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return std::atoll(it->second.c_str());
	}
	
	template <typename T>
	typename std::enable_if<std::is_same<T, uint64_t>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return static_cast<uint64_t>(std::atoll(it->second.c_str()));
	}
	
	template <typename T>
	typename std::enable_if<std::is_same<T, bool>::value, T>::type get(const std::string& key)const
	{
		auto it = _params.find(key);
		if (it == _params.end())
		{
			throw std::invalid_argument("key not find : " + key);
		}
		return "true"==it->second || "True" == it->second || "TRUE" == it->second || std::atoll(it->second.c_str()) > 0;
	}
};

