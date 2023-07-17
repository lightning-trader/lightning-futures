#pragma once

#include<string>
#include<ctime>

#define ONE_DAY_SECONDS 86400
#define ONE_MINUTE_SECONDS 60
#define ONE_HOUR_SECONDS 3600

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

static time_t make_datetime(int year, int month, int day, int hour=0, int minute=0, int second=0)
{
	tm t;
	t.tm_year = year - 1900;
	t.tm_mon = month-1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = second;
    t.tm_isdst = -1;//添加这一行，初始化tm_isdst，不然mktime返回值恒为-1
	return mktime(&t);
}

static time_t make_datetime(const char* date, const char* time)
{
	if (date != nullptr && time != nullptr)
	{
		int year, month, day;
		sscanf(date, "%4d%2d%2d", &year, &month, &day);
		int hour, minute, second;
		sscanf(time, "%2d:%2d:%2d", &hour, &minute, &second);
		time_t t = make_datetime(year, month, day, hour, minute, second);
		return t;
	}
	return -1;
}
static time_t make_datetime(uint32_t date, const char* time)
{
	if (time != nullptr && time != "")
	{
		int year, month, day;
		year = date / 10000;
		month = date % 10000 / 100;
		day = date % 100;
		int hour, minute, second;
		sscanf(time, "%2d:%2d:%2d", &hour, &minute, &second);
		time_t t = make_datetime(year, month, day, hour, minute, second);
		return t;
	}
	return -1;
}

static time_t make_time(const char* time)
{
	int hour, minute, second;
	sscanf(time, "%2d:%2d:%2d", &hour, &minute, &second);
	return hour * ONE_HOUR_SECONDS + minute * ONE_MINUTE_SECONDS + second;
}
static time_t make_datetime(time_t date_begin, const char* time)
{
	return date_begin + make_time(time);
}
static time_t make_date(uint32_t date)
{
	int year, month, day;
	year = date / 10000;
	month = date % 10000 / 100;
	day = date % 100;
	return make_datetime(year, month, day);
}

static time_t get_now()
{
	time_t t;
	time(&t);
	return t;
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

static uint32_t time_to_uint(time_t t)
{
	char buffer[16] = { 0 };
	struct tm* info = localtime(&t);
	strftime(buffer, sizeof buffer, "%Y%m%d", info);
	return std::atoi(buffer);
}