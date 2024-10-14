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
#include "csv_tick_loader.h"
#include <fstream>
#include <filesystem>
#include <define_types.hpp>
#include <string_helper.hpp>
#include <time_utils.hpp>
#include <log_wapper.hpp>

using namespace lt::driver;

bool csv_tick_loader::init(const std::string& root_path)
{
	_root_path = root_path;
	return true ;
}


void csv_tick_loader::load_tick(std::vector<tick_detail>& result , const code_t& code, uint32_t trade_day)
{
	char filename[128]={0};
	sprintf(filename, _root_path.c_str(), code.get_id(), trade_day);
	if (!std::filesystem::exists(filename))
	{
		LOG_ERROR("cant find file in path:", filename);
		return ;
	}
	std::ifstream file(filename);
	if (!file.is_open()) {
		LOG_ERROR("cant open file :", filename);
		return ;
	}
	time_t last_second = 0;
	std::string line;
	while (std::getline(file, line))
	{
		const auto& cell = string_helper::split(line,',');
		if(cell.size()<44 || cell[1] != code.get_id())
		{
			continue;
		}
		tick_detail tick;
		tick.id = code;
		//const std::string& date_str = doc.GetCell<std::string>("业务日期",i);
		const std::string& time_str = cell[20];
		uint32_t current_tick = 0;
		time_t current_second = make_time(time_str.c_str());
		if (std::strcmp(code.get_excg(), "ZEC") && current_second == last_second)
		{
			//郑商所 没有tick问题，后一个填上500和上期所一致
			current_tick = 500;
		}
		else
		{
			current_tick = std::stoi(cell[21]);
			last_second = current_second;
		}
		tick.time = make_daytm(time_str.c_str(), current_tick);
		tick.price = std::stod(cell[4]);
		tick.volume = std::stoi(cell[11]);
		tick.open_interest = std::stoi(cell[13]);
		tick.trading_day = std::stoi(cell[0]);

		tick.bid_order[0] = std::make_pair(std::stod(cell[22]), std::stoi(cell[23]));
		tick.bid_order[1] = std::make_pair(std::stod(cell[26]), std::stoi(cell[27]));
		tick.bid_order[2] = std::make_pair(std::stod(cell[30]), std::stoi(cell[31]));
		tick.bid_order[3] = std::make_pair(std::stod(cell[34]), std::stoi(cell[35]));
		tick.bid_order[4] = std::make_pair(std::stod(cell[38]), std::stoi(cell[39]));

		tick.ask_order[0] = std::make_pair(std::stod(cell[24]), std::stoi(cell[25]));
		tick.ask_order[1] = std::make_pair(std::stod(cell[28]), std::stoi(cell[29]));
		tick.ask_order[2] = std::make_pair(std::stod(cell[32]), std::stoi(cell[33]));
		tick.ask_order[3] = std::make_pair(std::stod(cell[36]), std::stoi(cell[37]));
		tick.ask_order[4] = std::make_pair(std::stod(cell[40]), std::stoi(cell[41]));

		tick.extend = std::make_tuple(
			std::stod(cell[8]),		//open
			std::stod(cell[14]),	//close
			std::stod(cell[5]),		//standard
			std::stod(cell[9]),		//high
			std::stod(cell[10]),	//low
			std::stod(cell[16]),	//max
			std::stod(cell[17])		//min
		);

		result.emplace_back(tick);
	}

	std::sort(result.begin(), result.end(), [](const auto& lh, const auto& rh)->bool {
		
		if (lh.time < rh.time)
		{
			return true;
		}
		if (lh.time > rh.time)
		{
			return false;
		}
		return lh.id < rh.id;
		});
	
}