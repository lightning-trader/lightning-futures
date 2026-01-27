/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
#include <basic_define.h>
#include <string_helper.hpp>
#include <stdexcept>
#include <regex>
namespace lt 
{
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

		params(const std::string& param)
		{
			_params.clear();
			auto param_pair = string_helper::split(param, '&');
			for (auto it : param_pair)
			{
				auto param = string_helper::split(it, '=');
				_params[param[0]] = param[1];
			}
		}
		bool has(const char* key)const
		{
			return _params.end() != _params.find(key);
		}
		template <typename T>
		T get(const char key[]) const
		{
			return get<T>(std::string(key));
		}
		std::string to_string() const
		{
			std::string result;
			for (auto it = _params.begin(); it != _params.end(); ++it)
			{
				if (it != _params.begin())
				{
					result += '&';
				}
				result += it->first;
				result += '=';
				result += it->second;
			}
			return result;
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
			return "true" == it->second || "True" == it->second || "TRUE" == it->second || std::atoll(it->second.c_str()) > 0;
		}
	};
}

