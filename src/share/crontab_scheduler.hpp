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
#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <iomanip>

namespace lt
{
    class crontab_scheduler {
    public:
        using time_point = std::chrono::system_clock::time_point;
        using duration = std::chrono::seconds;

        int add_schedule(const std::string& begin_time_str, std::chrono::seconds duration_seconds) {
            schedule s{ parse_time(begin_time_str), duration_seconds };
            _schedules.push_back(s);
            return static_cast<int>(_schedules.size()) - 1;
        }

        void set_work_days(const std::vector<int>& days) {
            _work_days.clear();
            for (int day : days) {
                if (day >= 0 && day <= 6) {
                    _work_days.insert(day);
                }
            }
        }

        void set_callback(std::function<void(int)> begin_cb, std::function<void(int)> end_cb) {
            _begin_cb = begin_cb;
            _end_cb = end_cb;
        }

        void polling() {
            const auto now = std::chrono::system_clock::now();
            const std::time_t now_t = std::chrono::system_clock::to_time_t(now);

            // ================== 关键修改1：先处理结束事件 ==================
            auto it = _active_schedules.begin();
            while (it != _active_schedules.end()) {
                if (now >= it->second) {
                    if (_end_cb) _end_cb(it->first);
                    it = _active_schedules.erase(it);
                }
                else {
                    ++it;
                }
            }

            // ================== 关键修改2：双日期检查 ==================
            std::tm today_tm = *std::localtime(&now_t);

            // 生成昨日时间结构
            std::tm yesterday_tm = today_tm;
            yesterday_tm.tm_mday -= 1;  // 减去1天
            mktime(&yesterday_tm);       // 标准化时间结构

            // 检查两个可能的工作日
            const bool today_is_workday = _work_days.count(today_tm.tm_wday);
            const bool yesterday_is_workday = _work_days.count(yesterday_tm.tm_wday);

            for (int i = 0; i < static_cast<int>(_schedules.size()); ++i) {
                if (_active_schedules.count(i)) continue;

                const auto& s = _schedules[i];

                // 计算今日和昨日的开始时间
                const auto schedule_today = get_schedule_time(today_tm, s.begin_time);
                const auto end_today = schedule_today + s.foretime;

                const auto schedule_yesterday = get_schedule_time(yesterday_tm, s.begin_time);
                const auto end_yesterday = schedule_yesterday + s.foretime;

                // 双重检查逻辑
                if (yesterday_is_workday && now >= schedule_yesterday && now < end_yesterday)
                {
                    _active_schedules[i] = end_yesterday;
                    if (_begin_cb) _begin_cb(i);
                }
                if (today_is_workday && now >= schedule_today && now < end_today)
                {
                    _active_schedules[i] = end_today;
                    if (_begin_cb) _begin_cb(i);
                }
            }
        }

    private:
        struct schedule {
            duration begin_time;
            duration foretime;
        };

        std::unordered_set<int> _work_days;
        std::vector<schedule> _schedules;
        std::unordered_map<int, time_point> _active_schedules;
        std::function<void(int)> _begin_cb;
        std::function<void(int)> _end_cb;

        static duration parse_time(const std::string& time_str) {
            std::tm tm = {};
            std::istringstream ss(time_str);
            ss >> std::get_time(&tm, "%H:%M:%S");
            return std::chrono::hours(tm.tm_hour) +
                std::chrono::minutes(tm.tm_min) +
                std::chrono::seconds(tm.tm_sec);
        }

        time_point get_schedule_time(const std::tm& base_tm, duration begin_time) const {
            std::tm tm = base_tm;
            const auto total_sec = begin_time.count();
            tm.tm_hour = static_cast<int>(total_sec / 3600);
            tm.tm_min = static_cast<int>((total_sec % 3600) / 60);
            tm.tm_sec = static_cast<int>(total_sec % 60);
            return std::chrono::system_clock::from_time_t(std::mktime(&tm));
        }
    };
}