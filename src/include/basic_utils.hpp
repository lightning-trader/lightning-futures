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
#include <charconv>
#include <basic_types.hpp>
#include <string_helper.hpp>

namespace lt{

	
	inline code_t make_code(const std::string& exchange, const std::string& symbol) {
		return code_t(symbol.c_str(), exchange.c_str());
	}
	inline code_t make_code(const std::string& exchange, const char* symbol) {
		return code_t(symbol, exchange.c_str());
	}
	inline code_t make_code(const char* exchange, const std::string& symbol) {
        return code_t(symbol.c_str(), exchange);
	}
	inline code_t make_code(const char* exchange, const char* symbol) {
        return code_t(symbol, exchange);
	}
    template<typename T>
    static std::string value_to_string(const T& value) {
        if constexpr (std::is_floating_point_v<T>) {
            return string_helper::to_string(value, 2);
        }
        else {
            return std::to_string(value);
        }
    }
    template<typename F, typename S, size_t N>
    static std::string pairarr_to_string(const std::array<std::pair<F, S>, N>& pairs,std::function<bool(F, S)> filter_callback=nullptr)
    {
        std::vector<std::string> result;
        for (auto it : pairs)
        {
            if(filter_callback == nullptr || filter_callback(it.first, it.second))
            {
                result.emplace_back(value_to_string(it.first) + ":" + value_to_string(it.second));
            }
        }
        return string_helper::join(',', result);
    }

    template<typename F, typename S, size_t N>
    static std::array<std::pair<F, S>, N> string_to_pairarr(const std::string& string)
    {
        std::array<std::pair<F, S>, N> result;
        std::vector<std::string> pairstr = string_helper::split(string, ',');
        for (auto i = 0; i < pairstr.size(); i++)
        {
            auto strs = string_helper::split(string, ':');
            F key; S value;
            auto [key_ptr, key_ec] = std::from_chars(strs[0].data(), strs[0].data() + strs[0].size(), key);
            auto [val_ptr, val_ec] = std::from_chars(strs[1].data(), strs[1].data() + strs[1].size(), value);
            if (key_ec == std::errc() && val_ec == std::errc())
            {
                result[i] = std::make_pair(key, value);
            }
        }
        return result;
    }

    template<typename F, typename S>
    static std::string valmap_to_string(const std::map<F, S>& pairs, std::function<bool(F, S)> filter_callback = nullptr)
    {
        std::vector<std::string> result;
        for (auto it : pairs)
        {
            if (filter_callback == nullptr || filter_callback(it.first, it.second))
            {
                result.emplace_back(value_to_string(it.first) + ":" + value_to_string(it.second));
            }
        }
        return string_helper::join(',', result);
    }


    template<typename F, typename S>
    static std::map<F, S> string_to_valmap(const std::string& string)
    {
        std::map<F, S> result;
        std::vector<std::string> pairstr = string_helper::split(string, ',');
        for (auto i = 0; i < pairstr.size(); i++)
        {
            auto strs = string_helper::split(string, ':');
            
            F key;S value;
            auto [key_ptr, key_ec] = std::from_chars(strs[0].data(), strs[0].data() + strs[0].size(), key);
            auto [val_ptr, val_ec] = std::from_chars(strs[1].data(), strs[1].data() + strs[1].size(), value);
            if (key_ec == std::errc() && val_ec == std::errc())
            {
                result.insert(std::make_pair(key,value));
            }

        }
        return result;
    }

    inline deal_direction get_deal_direction(const lt::tick_info& prev, const lt::tick_info& tick)
    {
        if (tick.price >= prev.sell_price() || tick.price >= tick.sell_price())
        {
            return deal_direction::DD_UP;
        }
        if (tick.price <= prev.buy_price() || tick.price <= tick.buy_price())
        {
            return deal_direction::DD_DOWN;
        }
        return deal_direction::DD_FLAT;
    }

}


