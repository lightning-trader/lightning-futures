#pragma once

#include<string>
#include<ctime>

static std::string datetime_to_string(time_t timestamp,const char* format = "%Y-%m-%d %H:%M:%S")
{
	char buffer[64] = { 0 };
	struct tm info;
	localtime_s(&info, &timestamp);
	strftime(buffer, sizeof buffer, format, &info);
	return std::string(buffer);
}

static time_t make_datetime(int year, int month, int day, int hour, int minute, int second)
{
	tm t;
	t.tm_year = year - 1900;
	t.tm_mon = month-1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
	t.tm_sec = second;
	return mktime(&t);
}

static time_t make_datetime(const char* date, const char* time)
{
	if (date != nullptr && time != nullptr && date != "" && time != "")
	{
		int year, month, day;
		sscanf_s(date, "%4d%2d%2d", &year, &month, &day);
		int hour, minute, second;
		sscanf_s(time, "%2d:%2d:%2d", &hour, &minute, &second);
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
		sscanf_s(time, "%2d:%2d:%2d", &hour, &minute, &second);
		time_t t = make_datetime(year, month, day, hour, minute, second);
		return t;
	}
	return -1;
}

static time_t make_time(const char* time)
{
	int hour, minute, second;
	sscanf_s(time, "%2d:%2d:%2d", &hour, &minute, &second);
	return hour*3600 + minute*60+ second;
}

static time_t get_now()
{
	time_t t;
	time(&t);
	return t;
}

static time_t get_day_begin(time_t cur)
{
	if (cur < 86400)
		return 0;
	int _0 = (int)cur / 86400 * 86400 - 28800;
	if (_0 <= (cur - 86400))
		_0 += 86400;
	return _0;
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