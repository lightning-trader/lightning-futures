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
#include "define.h"
#include <ostream>
#include <utility>

constexpr size_t CODE_DATA_LEN = 20;
//合约id起始位置（rb2010）
#define CONTID_BEGIN	0
//交易所id起始位置（SHFE）
#define EXCGID_BEGIN	8
//品种ID起始位置（rb）
#define CMDTID_BEGIN	14
//合约NO起始位置（2010）
#define CMDTNO_BEGIN	18

#if defined(WIN32)
#pragma  warning(disable:4996)
#endif

struct code_t
{
private:
	char _data[CODE_DATA_LEN];

public:

	code_t(const char* cd)
	{
		size_t ed = 0;
		std::memset(&_data, 0, sizeof(_data));
		size_t cmdtno_index = 0U;
		for (size_t i = 0; i < CODE_DATA_LEN && cd[i] != '\0'; i++)
		{
			char c = cd[i];
			if (c == '.')
			{
				ed = i;
				continue;
			}
			if (ed == 0)
			{
				_data[i + EXCGID_BEGIN] = cd[i];
			}
			else
			{
				auto c = cd[i];
				_data[i - ed - 1] = c;
				if (c >= '0' && c <= '9')
				{
					if(cmdtno_index==0U)
					{
						cmdtno_index = i - ed - 1;
					}
				}
				else
				{
					_data[i - ed - 1 + CMDTID_BEGIN] = c;
				}
			}
		}
		*reinterpret_cast<uint16_t*>(_data + CMDTNO_BEGIN) = static_cast<uint16_t>(std::atoi(_data+ cmdtno_index));
	}


	code_t()
	{
		std::memset(&_data, 0, sizeof(_data));
	}

	code_t(const code_t& obj)
	{
		std::memcpy(_data, obj._data, CODE_DATA_LEN);
	}
	
	code_t(const char* id, const char* excg_id)
	{
		std::memset(&_data, 0, sizeof(_data));
		size_t cmdtno_index = 0U;
		for (size_t i = 0; i < EXCGID_BEGIN && id[i] != '\0'; i++)
		{
			auto c = id[i];
			_data[i] = c;
			if (c >= '0' && c <= '9')
			{
				if (cmdtno_index == 0)
				{
					cmdtno_index = i;
				}
			}
			else
			{
				_data[i + CMDTID_BEGIN] = c;
			}
		}
		*reinterpret_cast<uint16_t*>(_data + CMDTNO_BEGIN) = static_cast<uint16_t>(std::atoi(_data + cmdtno_index));
		if (std::strlen(excg_id) < CMDTID_BEGIN - EXCGID_BEGIN)
		{
			std::strcpy(_data + EXCGID_BEGIN,excg_id);
		}
	}

	code_t(const char* cmdt_id, const char* cmdt_no, const char* excg_id)
	{
		std::memset(&_data, 0, sizeof(_data));
		if (std::strlen(cmdt_id) + std::strlen(cmdt_no) < EXCGID_BEGIN)
		{
			std::strcpy(_data, cmdt_id);
			size_t cmdtno_index = strlen(_data);
			std::strcpy(_data + cmdtno_index, cmdt_no);
			std::strcpy(_data + CMDTID_BEGIN, cmdt_id);
			*reinterpret_cast<uint16_t*>(_data + CMDTNO_BEGIN) = static_cast<uint16_t>(std::atoi(_data + cmdtno_index));
		}
		if (std::strlen(excg_id) < CMDTID_BEGIN - EXCGID_BEGIN)
		{
			std::strcpy(_data + EXCGID_BEGIN, excg_id);
		}
	}
	
	bool operator < (const code_t& other)const
	{
		return std::memcmp(_data, other._data, CODE_DATA_LEN) < 0;
	}
	bool operator == (const code_t& other)const
	{
		return std::memcmp(_data, other._data, CODE_DATA_LEN) == 0;
	}
	bool operator != (const code_t& other)const
	{
		return std::memcmp(_data, other._data, CODE_DATA_LEN) != 0;
	}

	const char* get_id()const
	{
		return _data;
	}
	const char* get_excg()const
	{
		return _data + EXCGID_BEGIN;
	}
	const char* get_cmdtid()const
	{
		return (_data + CMDTID_BEGIN);
	}
	uint16_t get_cmdtno()const
	{
		return *reinterpret_cast<const uint16_t*>(_data + CMDTNO_BEGIN);
	}
	std::ostream& operator<<(std::ostream& os)
	{
		return os << get_id();
	}
	bool is_distinct()const
	{
		if (strcmp(EXCHANGE_ID_SHFE, get_excg())==0)
		{
			return true ;
		}
		if (strcmp(EXCHANGE_ID_INE, get_excg())==0)
		{
			return true;
		}
		if (strcmp(EXCHANGE_ID_SGE, get_excg())==0)
		{
			return true;
		}
		if (strcmp(EXCHANGE_ID_CFFEX, get_excg())==0)
		{
			return true;
		}
		return false;
	}
};

const code_t default_code;

struct tick_info
{
	code_t id; //合约ID

	daytm_t time; //日内时间（毫秒数）

	double_t price;  //pDepthMarketData->LastPrice

	double_t open;
	
	double_t close;

	double_t high;

	double_t low;

	double_t standard;

	uint64_t volume;

	uint32_t trading_day;

	double_t open_interest;

	std::array<std::pair<double_t, uint32_t>, 5> buy_order;
	
	std::array<std::pair<double_t, uint32_t>, 5> sell_order;

	double_t buy_price()const
	{
		if (buy_order.empty())
		{
			return 0;
		}
		return buy_order[0].first;
	}
	double_t sell_price()const
	{
		if (sell_order.empty())
		{
			return 0;
		}
		return sell_order[0].first;
	}
	tick_info()
		:time(0),
		open(0),
		close(0),
		high(0),
		low(0),
		price(0),
		standard(0),
		volume(0LLU),
		trading_day(0),
		open_interest(.0F)
	{}

	tick_info(const code_t& cod,daytm_t dtm,double_t op, double_t cls, double_t hi, double_t lo, double_t prs, double_t std, uint64_t vlm,uint32_t td,double_t ist, std::array<std::pair<double_t, uint32_t>, 5>&& buy_ord, std::array<std::pair<double_t, uint32_t>, 5>&& sell_ord)
		:id(cod),
		time(dtm),
		open(op),
		close(cls),
		high(hi),
		low(lo),
		price(prs),
		standard(std),
		volume(vlm),
		trading_day(td),
		open_interest(ist),
		buy_order(buy_ord),
		sell_order(sell_ord)
	{}

	uint32_t total_buy_valume()const
	{
		uint32_t reuslt = 0;
		for (auto& it : buy_order)
		{
			reuslt += it.second;
		}
		return reuslt;
	}

	uint32_t total_sell_valume()const
	{
		uint32_t reuslt = 0;
		for (auto& it : sell_order)
		{
			reuslt += it.second;
		}
		return reuslt;
	}

	bool invalid()const
	{
		return id==default_code;
	}

};

const tick_info default_tick;


enum class error_code : uint8_t
{
	EC_Success,
	EC_Failure,
	EC_OrderFieldError = 23U, //字段错误
	EC_PositionNotEnough = 30U, //仓位不足
	EC_MarginNotEnough = 31U,		//保证金不足
	EC_StateNotReady = 32, //状态不对
};

struct position_cell
{
	//仓位
	uint32_t	postion;
	//冻结（平仓未成交）
	uint32_t	frozen;

	position_cell() :
		postion(0),
		frozen(0)
	{}

	uint32_t usable()const
	{
		return postion - frozen;
	}

	bool empty()const
	{
		return postion == 0;
	}

	void clear()
	{
		postion = 0;
		frozen = 0;
	}
};
struct position_info
{
	code_t id; //合约ID
	position_info(const code_t& code) :id(code), long_pending(0), short_pending(0){}
	//今仓
	position_cell today_long;
	position_cell today_short;

	//昨仓
	position_cell history_long;
	position_cell history_short;

	//开仓还未成交
	uint32_t	long_pending;
	uint32_t	short_pending;

	bool empty()const
	{
		return today_long.empty() && today_short.empty() && history_long.empty() && history_short.empty() && long_pending==0U && short_pending==0U;
	}

	uint32_t get_total()const
	{
		return today_long.postion + today_short.postion + history_long.postion + history_short.postion;
	}

	int32_t get_real()const
	{
		return today_long.postion + history_long.postion - (today_short.postion + history_short.postion);
	}

	uint32_t get_long_position()const
	{
		return today_long.postion + history_long.postion;
	}

	uint32_t get_short_position()const
	{
		return today_short.postion + history_short.postion;
	}
	uint32_t get_long_frozen()const
	{
		return today_long.frozen + history_long.frozen;
	}

	uint32_t get_short_frozen()const
	{
		return today_short.frozen + history_short.frozen;
	}
	position_info():long_pending(0U), short_pending(0U)
	{}
};

const position_info default_position;
/*
 *	订单标志
 */
enum class order_flag
{
	OF_NOR = '0',		//普通订单
	OF_FAK,			//部成部撤，不等待自动撤销
	OF_FOK,			//全成全撤，不等待自动撤销
} ;

/*
 *	开平方向
 */
enum class offset_type
{
	OT_OPEN = '0',	//开仓
	OT_CLOSE,		//平仓,上期为平昨
	OT_CLSTD		//平今
};

/*
 *	多空方向
 */
enum class direction_type
{
	DT_LONG = '0',	//做多
	DT_SHORT		//做空
};

struct order_info
{
	
	estid_t		estid;

	code_t			code;

	std::string		unit_id ;

	uint32_t		total_volume;

	uint32_t		last_volume;

	daytm_t			create_time;

	offset_type		offset;

	direction_type	direction;

	double_t		price;

	order_info() :
		estid(INVALID_ESTID),
		offset(offset_type::OT_OPEN),
		direction(direction_type::DT_LONG),
		total_volume(0),
		last_volume(0),
		create_time(0),
		price(.0f)
	{}

	bool invalid()const
	{
		return estid == INVALID_ESTID;
	}

	bool is_buy()const
	{
		if (direction == direction_type::DT_LONG && offset == offset_type::OT_OPEN)
		{
			return true;
		}
		if (direction == direction_type::DT_SHORT && (offset == offset_type::OT_CLSTD || offset == offset_type::OT_CLOSE))
		{
			return true;
		}
		return false ;
	}

	bool is_sell()const
	{
		if (direction == direction_type::DT_SHORT && offset == offset_type::OT_OPEN)
		{
			return true;
		}
		if (direction == direction_type::DT_LONG && (offset == offset_type::OT_CLOSE|| offset == offset_type::OT_CLSTD))
		{
			return true;
		}
		return false;
	}
	
};
const order_info default_order ;


//订单统计数据
struct order_statistic
{
	//下单数量
	uint32_t place_order_amount;
	//委托数量
	uint32_t entrust_amount;
	//成交数量
	uint32_t trade_amount;
	//撤单数量
	uint32_t cancel_amount;
	//错误数量
	uint32_t error_amount;

	order_statistic():
		place_order_amount(0),
		entrust_amount(0),
		trade_amount(0),
		cancel_amount(0),
		error_amount(0)
	{}

public:


	std::ostream& operator>>(std::ostream& os)
	{
		os << place_order_amount << entrust_amount;
		os << trade_amount << cancel_amount << error_amount;
		return os;
	}
};

const order_statistic default_statistic;


enum class error_type
{
	ET_PLACE_ORDER,
	ET_CANCEL_ORDER,
	ET_OTHER_ERROR
};

struct today_market_info
{
	code_t code ;
	
	std::map<double_t, uint32_t> volume_distribution;

	tick_info last_tick_info;

	double_t get_control_price()const
	{
		double_t control_price = last_tick_info.price;
		uint32_t max_volume = 0;
		for(const auto& it : volume_distribution)
		{
			if(it.second > max_volume)
			{
				max_volume = it.second;
				control_price = it.first;
			}
		}
		return control_price;
	}

	void clear()
	{
		volume_distribution.clear();
	}
};
const today_market_info default_today_market;

