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
#include<basic_define.h>

//#include "stringbuilder.h"
#define ONE_DAY_SECONDS 86400
#define ONE_MINUTE_SECONDS 60
#define ONE_HOUR_SECONDS 3600
#define ONE_HOUR_MILLISECONDS 3600000
#define ONE_DAY_MILLISECONDS 86400000
#define ONE_MINUTE_MILLISECONDS 60000
#define ONE_SECOND_MILLISECONDS 1000
#define ONE_WEEK_DAYS 7

#define SEQTM_SEPARATION_COEFFICIENT 1000000000LLU


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
	//time_t
	static daytm_t make_daytm(time_t time, uint32_t tick)
	{
		return daytm_sequence(static_cast<daytm_t>(time-get_day_begin(time)) * ONE_SECOND_MILLISECONDS + tick);
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
				return make_daytm(static_cast<uint32_t>(std::atoi(tmp)), std::atoi(time + p));
			}
		}
		return -1;
	}
	static seqtm_t make_seqtm(uint32_t date, daytm_t daytm)
	{
		return static_cast<seqtm_t>(static_cast<uint64_t>(date) * SEQTM_SEPARATION_COEFFICIENT + daytm );
	}

	static seqtm_t make_seqtm(uint32_t date, const char* tmstr)
	{
		return make_seqtm(date, make_daytm(tmstr,true));
	}

	static uint32_t get_uint_day(seqtm_t seqtm)
	{
		return static_cast<uint32_t>(static_cast<uint64_t>(seqtm) / SEQTM_SEPARATION_COEFFICIENT);
	}
	
	static daytm_t get_daytm(seqtm_t seqtm)
	{
		return static_cast<daytm_t>(static_cast<uint64_t>(seqtm) % SEQTM_SEPARATION_COEFFICIENT);
	}

	static time_t make_datetime(time_t date_begin, const char* time)
	{
		return date_begin + make_time(time);
	}

	static time_t make_datetime(uint32_t day, daytm_t dtm)
	{
		return make_date(day) + daytm_really(dtm) / ONE_SECOND_MILLISECONDS;
	}

	static std::string seqtm_to_string(seqtm_t seqtm, const char* format = "%Y-%m-%d %H:%M:%S")
	{
		uint32_t trading_day = get_uint_day(seqtm);
		daytm_t daytm = get_daytm(seqtm);
		return datetime_to_string(make_datetime(trading_day, daytm), format);
	}

	static time_t make_datetime(seqtm_t seqtm)
	{
		return make_date(get_uint_day(seqtm)) + daytm_really(get_daytm(seqtm)) / ONE_SECOND_MILLISECONDS;
	}
	// 计算本周一0点(ISO标准周)
	static time_t get_week_begin(time_t cur)
	{
		time_t day_start = get_day_begin(cur);
		int weekday = (day_start / ONE_DAY_SECONDS + 4) % ONE_WEEK_DAYS; // 1970-1-1是周四
		return day_start - weekday * ONE_DAY_SECONDS;
	}
	// 获取当前月开始时间(1号0点)
	static time_t get_month_begin(time_t cur) {
		struct tm tm = *localtime(&cur);
		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
		tm.tm_mday = 1;
		return mktime(&tm);
	}

	// 获取当前年开始时间(1月1日0点)
	static time_t get_year_begin(time_t cur) {
		struct tm tm = *localtime(&cur);
		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
		tm.tm_mon = 0;
		tm.tm_mday = 1;
		return mktime(&tm);
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

	static daytm_t section_daytm_snap(daytm_t tm, daytm_t delta=3000)
	{
		std::vector<daytm_t> base_time = {
			18000000, //21:00:00
			25200000,	//23:00:00
			32400000,	//01:00:00
			37800000,	//02:30:00
			61200000,	//09:00:00
			63000000,	//09:30:00
			65700000,	//10:15:00
			66600000,	//10:30:00
			70200000,	//11:30:00
			75600000,	//13:00:00
			77400000,	//13:30:00
			82800000,	//15:00:00
			83700000,	//15:15:00
		};
		for(auto it : base_time)
		{
			if ((it - delta) <= tm && tm <= (it + delta))
			{
				return it;
			}
		}
		
		return tm;
	}

	static bool is_same_day(time_t t1, time_t t2) 
	{
		struct tm* tm1 = std::localtime(&t1);
		struct tm* tm2 = std::localtime(&t2);
		return (tm1->tm_year == tm2->tm_year) && (tm1->tm_mon == tm2->tm_mon) && (tm1->tm_mday == tm2->tm_mday);
	}
	static bool is_same_week(time_t t1, time_t t2)
	{
		struct tm tm1 = *localtime(&t1);
		struct tm tm2 = *localtime(&t2);
		int y1, w1, y2, w2;
		y1 = tm1.tm_year + 1900;
		w1 = (tm1.tm_yday + 7 - tm1.tm_wday) / 7;
		y2 = tm2.tm_year + 1900;
		w2 = (tm2.tm_yday + 7 - tm2.tm_wday) / 7;
		return y1 == y2 && w1 == w2;
	}
	static bool is_same_month(time_t t1, time_t t2)
	{
		struct tm tm1 = *localtime(&t1);
		struct tm tm2 = *localtime(&t2);
		return tm1.tm_year == tm2.tm_year &&
			tm1.tm_mon == tm2.tm_mon;
	}
	static bool is_same_year(time_t t1, time_t t2)
	{
		struct tm tm1 = *localtime(&t1);
		struct tm tm2 = *localtime(&t2);
		return tm1.tm_year == tm2.tm_year;
	}
	//回到过去
	static seqtm_t time_back(seqtm_t tm, uint32_t seconds)
	{
		return tm - seconds * ONE_SECOND_MILLISECONDS;
	}
	static daytm_t time_back(daytm_t tm, uint32_t seconds)
	{
		return tm - seconds * ONE_SECOND_MILLISECONDS;
	}
	//未来
	static seqtm_t time_forward(seqtm_t tm, uint32_t seconds)
	{
		return tm + seconds * ONE_SECOND_MILLISECONDS;
	}
	static daytm_t time_forward(daytm_t tm, uint32_t seconds)
	{
		return tm + seconds * ONE_SECOND_MILLISECONDS;
	}
}
