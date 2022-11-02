#pragma once

#include<string>
#include<ctime>

static std::string time_to_string(time_t timestamp)
{
	char buffer[64] = { 0 };
	struct tm info;
	localtime_s(&info, &timestamp);
	strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S", &info);
	return std::string(buffer);
}

static time_t make_time(int year, int month, int day, int hour, int minute, int second)
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

static time_t make_time(const char* date, const char* time)
{
	if (date != nullptr && time != nullptr && date != "" && time != "")
	{
		int year, month, day;
		sscanf_s(date, "%4d%2d%2d", &year, &month, &day);
		int hour, minute, second;
		sscanf_s(time, "%2d:%2d:%2d", &hour, &minute, &second);
		time_t t = make_time(year, month, day, hour, minute, second);
		return t;
	}
	return -1;
}

static time_t get_now()
{
	time_t t;
	time(&t);
	return t;
}

static std::string time_to_string(const char* date, const char* time)
{
	time_t t = make_time(date, time);
	if (t > 0)
	{
		return time_to_string(t);
	}
	return "";
}