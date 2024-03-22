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
#include <define_types.hpp>
namespace lt
{
	enum class deal_direction
	{
		DD_DOWN = -1,	//向下
		DD_FLAT = 0,	//平
		DD_UP = 1,		//向上
	};

	enum class deal_status
	{
		DS_INVALID,
		DS_DOUBLE_OPEN, //双开
		DS_OPEN,		//开仓
		DS_CHANGE,		//换手
		DS_CLOSE,		//平仓
		DS_DOUBLE_CLOSE,//双平

	};

	//盘口信息
	struct tape_info
	{
		code_t		id;

		double_t	price;

		daytm_t		time;
		//现手
		uint32_t	volume_delta;
		//增仓
		double_t	interest_delta;
		//方向
		deal_direction	direction;

		tape_info(const code_t& cod, daytm_t dtm, double_t prc) :
			id(cod),
			time(dtm),
			price(prc),
			volume_delta(0),
			interest_delta(.0),
			direction(deal_direction::DD_FLAT)
		{}

		deal_status get_status()
		{
			if (volume_delta == interest_delta && interest_delta > 0)
			{
				return deal_status::DS_DOUBLE_OPEN;
			}
			if (volume_delta > interest_delta && interest_delta > 0)
			{
				return deal_status::DS_OPEN;
			}
			if (volume_delta > interest_delta && interest_delta == 0)
			{
				return deal_status::DS_CHANGE;
			}
			if (volume_delta > -interest_delta && -interest_delta > 0)
			{
				return deal_status::DS_CLOSE;
			}
			if (volume_delta == -interest_delta && -interest_delta > 0)
			{
				return deal_status::DS_DOUBLE_CLOSE;
			}
		}
	};


	struct bar_info
	{
		code_t id; //合约ID

		daytm_t time; //时间(分钟毫秒数)

		uint32_t period;

		double_t open;

		double_t close;

		double_t high;

		double_t low;

		//成交量
		uint32_t volume;

		//订单流中delta（price_buy_volume - price_sell_volume）
		int32_t delta;

		//订单流中poc (最大成交量的价格，表示bar重心)
		double_t poc;

		double_t price_step; //价格单元

		//订单流中的明细
		std::map<double_t, uint32_t> price_buy_volume;
		std::map<double_t, uint32_t> price_sell_volume;

		uint32_t get_buy_volume(double_t price)const
		{
			auto it = price_buy_volume.find(price);
			if (it == price_buy_volume.end())
			{
				return 0;
			}
			return it->second;
		}

		uint32_t get_sell_volume(double_t price)const
		{
			auto it = price_sell_volume.find(price);
			if (it == price_sell_volume.end())
			{
				return 0;
			}
			return it->second;
		}

		std::vector<std::tuple<double_t, uint32_t, uint32_t>> get_order_book()const
		{
			std::vector < std::tuple<double_t, uint32_t, uint32_t>> result;
			for (double_t price = low; price <= high; price += price_step)
			{
				result.emplace_back(std::make_tuple(price, get_buy_volume(price), get_sell_volume(price)));
			}
			return result;
		}

		//获取不平衡订单
		std::pair<std::shared_ptr<std::vector<double_t>>, std::shared_ptr<std::vector<double_t>>> get_unbalance(uint32_t multiple)const
		{
			//需求失衡(供大于求)
			auto demand_unbalance = std::make_shared<std::vector<double_t>>();
			//供给失衡(供不应求)
			auto supply_unbalance = std::make_shared<std::vector<double_t>>();
			//构建订单薄
			auto order_book = get_order_book();

			for (size_t i = 0; i < order_book.size() - 1; i++)
			{
				auto demand_cell = order_book[i];
				auto supply_cell = order_book[i + 1];
				if (std::get<1>(demand_cell) * multiple > std::get<2>(supply_cell))
				{
					demand_unbalance->emplace_back(std::get<0>(demand_cell));
				}
				else if (std::get<2>(supply_cell) * multiple > std::get<1>(demand_cell))
				{
					supply_unbalance->emplace_back(std::get<0>(supply_cell));
				}
			}

			return std::make_pair(demand_unbalance, supply_unbalance);
		}


		void clear()
		{
			time = 0;
			period = 0U;
			open = .0;
			high = .0;
			low = .0;
			close = .0;
			volume = 0;
			delta = 0;
			poc = 0;
			price_step = .0;
			price_buy_volume.clear();
			price_sell_volume.clear();
		}

		bar_info()
			:time(0),
			period(0U),
			open(0),
			close(0),
			high(0),
			low(0),
			volume(0),
			delta(0),
			poc(.0),
			price_step(.0)
		{}
	};

	const bar_info default_bar_info;
	const std::vector<bar_info> default_bar_vector;

}
