#pragma once

#include<string>
//#include "stringbuilder.h"
#define ONE_DAY_SECONDS 86400
#define ONE_MINUTE_SECONDS 60
#define ONE_HOUR_SECONDS 3600
#define ONE_DAY_MILLISECONDS 86400000
#define ONE_MINUTE_MILLISECONDS 60000
#define ONE_SECOND_MILLISECONDS 1000

#if defined(WIN32)
#pragma  warning(disable:4996)
#endif

static std::string datetime_to_string(time_t timestamp,const char* format = "%Y-%m-%d %H:%M:%S")
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
	t.tm_mon = month-1;
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
	if (time != nullptr && time != "")
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
static daytm_t make_daytm(const char* time, uint32_t tick)
{
	if (time != nullptr)
	{
		return static_cast<daytm_t>(make_time(time)) * ONE_SECOND_MILLISECONDS + tick;
	}
	return -1;
}

// 21:00:00.500 
static daytm_t make_daytm(const char* time)
{
	if (time != nullptr)
	{
		char tmp[13] = {0};
		size_t p = 0 ;
		for(size_t i=0; time[i]!='\0'&& i < 13; i++)
		{
			if(time[i] == '.')
			{
				tmp[i] = '\0';
				p = i+1;
			}
			else
			{
				tmp[i] = time[i];
			}
		}
		return make_daytm(tmp,std::atoi(time+p));
	}
	return -1;
}

static time_t make_datetime(time_t date_begin, const char* time)
{
	return date_begin + make_time(time);
}


static time_t make_datetime(uint32_t day,daytm_t dtm)
{
	return make_date(day) + dtm / ONE_SECOND_MILLISECONDS;
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
	return static_cast<daytm_t>((cur - get_day_begin(cur)) * ONE_SECOND_MILLISECONDS);
}

static time_t get_next_time(time_t cur,const char* time)
{
	time_t day_begin = get_day_begin(cur);
	time_t next = day_begin + make_time(time);
	if(next < cur)
	{
		next = next+ ONE_DAY_SECONDS;
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
	size_t j = 0 ;
	for(size_t i=0;i<16&&t[i]!='\0';i++)
	{
		if(t[i]!='-')
		{
			buffer[j++] = t[i];
		}
	}
	return std::atoi(buffer);
}