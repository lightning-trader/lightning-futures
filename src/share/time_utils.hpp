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

#include<string>
#include<define.h>
//#include "stringbuilder.h"
#define ONE_DAY_SECONDS 86400
#define ONE_MINUTE_SECONDS 60
#define ONE_HOUR_SECONDS 3600
#define ONE_HOUR_MILLISECONDS 3600000
#define ONE_DAY_MILLISECONDS 86400000
#define ONE_MINUTE_MILLISECONDS 60000
#define ONE_SECOND_MILLISECONDS 1000

#if defined(WIN32)
#pragma  warning(disable:4996)
#endif
namespace lt
{
	static std::string datetime_to_string(time_t timestamp, const char* format = "%Y-%m-%d %H:%M:%S")
	{
		char buffer[64] = { 0 };
		struct tm* info = localtime(&timestamp);
		strftime(buffer, sizeof buffer, format, info);
		return std::string(buffer);
	}

	static time_t make_date(int year, int month, int day)
	{
		tm t;
		t.tm_year = year - 1900;
		t.tm_mon = month - 1;
		t.tm_mday = day;
		t.tm_hour = 0;
		t.tm_min = 0;
		t.tm_sec = 0;
		t.tm_isdst = -1;//添加这一行，初始化tm_isdst，不然mktime返回值恒为-1
		return mktime(&t);
	}

	static time_t make_date(uint32_t date)
	{
		int year, month, day;
		year = date / 10000;
		month = date % 10000 / 100;
		day = date % 100;
		return make_date(year, month, day);
	}
	static time_t make_time(uint32_t time)
	{
		int hour, minute, second;
		hour = time / 10000;
		minute = time % 10000 / 100;
		second = time % 100;
		return hour * ONE_HOUR_SECONDS + minute * ONE_MINUTE_SECONDS + second;
	}
	static time_t make_time(const char* time)
	{
		int time_value[3] = { 0 };
		char tmp[3] = { '\0' };
		size_t i = 0;
		size_t q = 0;
		size_t p = 0;
		while (time[p] != '\0')
		{
			if (time[p] == ':')
			{
				tmp[p - q] = '\0';
				time_value[i++] = atoi(tmp);
				q = p + 1;
			}
			else
			{
				tmp[p - q] = time[p];
			}
			p++;
		}
		tmp[p - q] = '\0';
		time_value[i++] = atoi(tmp);
		return time_value[0] * ONE_HOUR_SECONDS + time_value[1] * ONE_MINUTE_SECONDS + time_value[2];
	}

	static time_t make_datetime(uint32_t date, const char* time)
	{
		if (time != nullptr && strcmp(time, "") != 0)
		{
			int year = date / 10000;
			int month = date % 10000 / 100;
			int day = date % 100;
			return  make_date(year, month, day) + make_time(time);
		}
		return -1;
	}

	static time_t make_datetime(const char* date, const char* time)
	{
		if (date != nullptr && time != nullptr)
		{
			uint32_t date_value = std::atoi(date);
			return make_datetime(date_value, time);
		}
		return -1;
	}

	static daytm_t daytm_offset(daytm_t tm, int32_t milliseconds)
	{
		int32_t result = (static_cast<int32_t>(tm) + milliseconds) % ONE_DAY_MILLISECONDS;
		if (result < 0)
		{
			result += ONE_DAY_MILLISECONDS;
		}
		return static_cast<daytm_t>(result);
	}

	static daytm_t daytm_sequence(daytm_t tm)
	{
		//21点开盘，向前偏移16小时 下午16点记作0点
		if (tm < 16 * ONE_HOUR_MILLISECONDS)
		{
			return tm + ONE_DAY_MILLISECONDS - 16 * ONE_HOUR_MILLISECONDS;
		}
		else
		{
			return tm - 16 * ONE_HOUR_MILLISECONDS;
		}
	}
	static daytm_t daytm_really(daytm_t tm)
	{
		//21点开盘，向后偏移16小时
		return (tm + 16 * ONE_HOUR_MILLISECONDS) % ONE_DAY_MILLISECONDS;
	}

	static daytm_t make_daytm(const char* time, uint32_t tick)
	{
		if (time != nullptr)
		{
			return daytm_sequence(static_cast<daytm_t>(make_time(time)) * ONE_SECOND_MILLISECONDS + tick);
		}
		return -1;
	}
	//121212
	static daytm_t make_daytm(uint32_t time, uint32_t tick)
	{
		return daytm_sequence(static_cast<daytm_t>(make_time(time)) * ONE_SECOND_MILLISECONDS + tick);
	}
	//is_str==true: 21:12:30.500
	//is_str==false:  211230.500 
	static daytm_t make_daytm(const char* time, bool is_str = false)
	{
		if (time != nullptr)
		{
			char tmp[13] = { 0 };
			size_t p = 0;
			for (size_t i = 0; i < 13 && time[i] != '\0'; i++)
			{
				if (time[i] == '.')
				{
					tmp[i] = '\0';
					p = i + 1;
				}
				else
				{
					tmp[i] = time[i];
				}
			}
			if (is_str)
			{
				return make_daytm(tmp, static_cast<uint32_t>(std::atoi(time + p)));
			}
			else
			{
				return make_daytm(std::atoi(tmp), std::atoi(time + p));
			}
		}
		return -1;
	}


	static time_t make_datetime(time_t date_begin, const char* time)
	{
		return date_begin + make_time(time);
	}


	static time_t make_datetime(uint32_t day, daytm_t dtm)
	{
		return make_date(day) + daytm_really(dtm) / ONE_SECOND_MILLISECONDS;
	}

	static time_t get_day_begin(time_t cur)
	{
		if (cur < ONE_DAY_SECONDS)
			return 0;
		int _0 = (int)cur / ONE_DAY_SECONDS * ONE_DAY_SECONDS - 28800;
		if (_0 <= (cur - ONE_DAY_SECONDS))
			_0 += ONE_DAY_SECONDS;
		return _0;
	}

	static daytm_t get_day_time(time_t cur)
	{
		return daytm_sequence(static_cast<daytm_t>(cur - get_day_begin(cur)) * ONE_SECOND_MILLISECONDS);
	}

	static time_t get_next_time(time_t cur, const char* time)
	{
		time_t day_begin = get_day_begin(cur);
		time_t next = day_begin + make_time(time);
		if (next < cur)
		{
			next = next + ONE_DAY_SECONDS;
		}
		return next;
	}

	static std::string datetime_to_string(const char* date, const char* time)
	{
		time_t t = make_datetime(date, time);
		if (t > 0)
		{
			return datetime_to_string(t);
		}
		return "";
	}

	static uint32_t date_to_uint(time_t t)
	{
		char buffer[16] = { 0 };
		struct tm* info = localtime(&t);
		strftime(buffer, sizeof buffer, "%Y%m%d", info);
		return std::atoi(buffer);
	}
	static uint32_t date_to_uint(const char* t)
	{
		char buffer[16] = { 0 };
		size_t j = 0;
		for (size_t i = 0; i < 16 && t[i] != '\0'; i++)
		{
			if (t[i] != '-')
			{
				buffer[j++] = t[i];
			}
		}
		return std::atoi(buffer);
	}
}
