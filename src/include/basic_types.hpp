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
#include "basic_define.h"
#include <ostream>
#include <utility>

constexpr size_t ID_DATA_LEN = 24;
constexpr size_t EXCG_DATA_LEN = 8;
constexpr size_t CODE_DATA_LEN = EXCG_DATA_LEN + ID_DATA_LEN * 2;

#if defined(WIN32)
#pragma  warning(disable:4996)
#endif
namespace lt
{

	//支持格式
	//rb2010 MA501 等期货普通合约
	//rb2010P3200 m1707-P-2600 等期权合约
	struct symbol_t
	{
		std::string family;		//族代码rb 注意这个不同于product
		uint32_t number;			//合约号2010
		double_t strike_price;		//期权目标价格20950
		enum
		{
			OPT_PUT = -1,
			OPT_INVALID = 0,
			OPT_CALL = 1,
		}option_type;					//1看涨期权（C）-1看跌期权（P）

		symbol_t() :option_type(OPT_INVALID), number(0U), strike_price(.0)
		{
		}

		symbol_t(const char* cd) :option_type(OPT_INVALID), number(0U), strike_price(.0)
		{
			size_t cursor = 0;
			char pi_str[6] = { 0 };
			char cn_str[8] = { 0 };
			char ot_str[2] = { 0 };
			char sp_str[8] = { 0 };
			size_t p_len = 0;
			char state = 0;
			for (size_t i = 0; i < ID_DATA_LEN && cd[i] != '\0'; i++)
			{
				auto c = cd[i];
				if(c == '-')
				{
					continue;
				}
				switch (state)
				{
				case 0: {
					if (!std::isdigit(c))
					{
						pi_str[p_len++] = c;
					}
					else
					{
						pi_str[p_len] = '\0';
						p_len = 0;
						state = 1;
						cn_str[p_len++] = c;
					}
				}break;
				case 1: {
					if (std::isdigit(c))
					{
						cn_str[p_len++] = c;
					}
					else
					{
						cn_str[p_len] = '\0';
						p_len = 0;
						state = 2;
						ot_str[p_len++] = c;
					}
				}break;
				case 2: {
					if (!std::isdigit(c))
					{
						ot_str[p_len++] = c;
					}
					else
					{
						ot_str[p_len] = '\0';
						p_len = 0;
						state = 3;
						sp_str[p_len++] = c;
					}
				}break;
				case 3: {
					if (std::isdigit(c))
					{
						sp_str[p_len++] = c;
					}
					else
					{
						sp_str[p_len] = '\0';
						p_len = 0;
						state = 4;
					}
				}break;
				}
			}
			family = pi_str;
			if(std::strlen(cn_str)>0)
			{
				number = std::stoi(cn_str);
			}
			if (std::strcmp("C", ot_str) == 0)
			{
				option_type = OPT_CALL;
			}
			if (std::strcmp("P", ot_str) == 0)
			{
				option_type = OPT_PUT;
			}
			if(option_type != OPT_INVALID)
			{
				strike_price = std::stod(sp_str);
			}
		}

	};
	/*
	* 目前支持以下四种类型
	* 1、期货普通合约 SHFE.rb2010
	* 2、期货套利合约 CZCE.AP411&AP412
	* 3、期货期权合约 SHFE.al2010P20950 DEC.m1707-P-2600
	* 4、产品族代码 SHFE.rb
	*/
	struct code_t
	{
	
		enum class code_type:uint8_t
		{
			CT_NORMAL = 0,
			CT_SP_ARBITRAGE = 1,	//同品种跨期套利
			CT_DP_ARBITRAGE = 2,	//跨品种套利
		};
	private:
		
		char _id[2][ID_DATA_LEN];
		char _excg[EXCG_DATA_LEN];

	public:

		code_t() {
			std::memset(_id, 0, sizeof(_id));
			std::memset(_excg, 0, sizeof(_excg));
		}

		code_t(const std::string& cd):code_t(cd.c_str()){ }

		code_t(const char* cd)
		{
			size_t cursor = 0;
			std::memset(_id, 0, sizeof(_id));
			std::memset(_excg, 0, sizeof(_excg));
			bool abg = false; //套利合约的第二个合约
			for (size_t i = 0; cd[i] != '\0'; i++)
			{
				char c = cd[i];
				if (c == '.')
				{
					cursor = i+1;
					continue;
				}
				if (cursor == 0)
				{
					if(i < EXCG_DATA_LEN)
					{
						_excg[i] = cd[i];
					}
				}
				else
				{
					auto c = cd[i];
					
					if (c == '&')
					{
						abg = true;
						cursor = i+1;
						continue;
					}
					if(abg)
					{
						if (i - cursor < ID_DATA_LEN)
						{
							_id[1][i - cursor] = c;
						}
					}
					else
					{
						if (i - cursor < ID_DATA_LEN)
						{
							_id[0][i - cursor] = c;
						}
					}
				}
			}
		}

		code_t(const code_t& obj)
		{
			std::memcpy(_id, obj._id, sizeof(_id));
			std::strcpy(_excg, obj._excg);
		}

		code_t(const char* id, const char* excg_id)
		{
			std::memset(_excg, 0, sizeof(_excg));
			if(std::strlen(excg_id)< EXCG_DATA_LEN)
			{
				std::strcpy(_excg, excg_id);
			}
			size_t cursor = 0;
			std::memset(_id, 0, sizeof(_id));
			bool abg = false; //套利合约的第二个合约
			for (size_t i = 0; i < ID_DATA_LEN && id[i] != '\0'; i++)
			{
				char c = id[i];
				if (c == '&')
				{
					abg = true;
					cursor = i+1;
					continue;
				}
				if (abg)
				{
					_id[1][i - cursor] = c;
				}
				else
				{
					_id[0][i] = c;
				}
			}
		}

		// 比较运算符
		bool operator<(const code_t& other) const 
		{
			auto excg_cmp = std::strcmp(_excg, other._excg);
			return excg_cmp < 0||(excg_cmp ==0&&std::memcmp(_id, other._id, sizeof(_id)) < 0);
		}

		bool operator==(const code_t& other) const 
		{
			return std::memcmp(_id, other._id, sizeof(_id)) == 0 &&
				std::strcmp(_excg, other._excg) == 0;
		}

		bool operator!=(const code_t& other) const 
		{
			return !(*this == other);
		}

		const char* get_symbol(size_t index=0)const
		{
			return _id[index];
		}

		const char* get_exchange()const
		{
			return _excg;
		}

		const symbol_t extract_symbol(size_t index = 0)const
		{
			return symbol_t(_id[index]);
		}

		const std::string to_string()const
		{
			return get_exchange() + std::string(".") + get_symbol();
		}

		code_type get_type()const
		{
			if(std::strlen(_id[1])==0)
			{
				return code_type::CT_NORMAL;
			}
			for (size_t i = 0; i < ID_DATA_LEN && _id[0][i] != '\0'&& _id[1][i] != '\0'; i++)
			{
				if(std::isdigit(_id[0][i]) && std::isdigit(_id[1][i]))
				{
					break;
				}
				if(_id[0][i]!=_id[1][i])
				{
					return code_type::CT_DP_ARBITRAGE;
				}
			}
			return code_type::CT_SP_ARBITRAGE;
		}

		bool is_distinct() const {
			return strcmp(EXCHANGE_ID_SHFE, get_exchange()) == 0 ||
				strcmp(EXCHANGE_ID_INE, get_exchange()) == 0;
		}
	};

	const code_t default_code;

	//
	
	struct tick_info
	{
		code_t id; //合约ID

		daytm_t time; //日内时间（毫秒数）

		double_t price;  //pDepthMarketData->LastPrice

		uint32_t volume; //当前tick成交量（累计量）

		double_t open_interest; //持仓（截面量）

		double_t average_price; //当日均价

		uint32_t trading_day;

		price_volume_array bid_order;

		price_volume_array ask_order;

		double_t buy_price()const
		{
			if (bid_order.empty())
			{
				return price;
			}
			return bid_order[0].first;
		}
		double_t sell_price()const
		{
			if (ask_order.empty())
			{
				return price;
			}
			return ask_order[0].first;
		}


		tick_info()
			:time(0),
			price(0),
			volume(0LLU),
			open_interest(0),
			average_price(.0),
			trading_day(0U)
		{}

		tick_info(const code_t& cod, daytm_t dtm, double_t prs, uint32_t val,double_t oit, double_t ap, uint32_t td, price_volume_array&& buy_ord, price_volume_array&& sell_ord)
			:id(cod),
			time(dtm),
			price(prs),
			volume(val),
			open_interest(oit),
			average_price(ap),
			trading_day(td),
			bid_order(buy_ord),
			ask_order(sell_ord)
		{}

		uint32_t total_buy_valume()const
		{
			uint32_t reuslt = 0;
			for (auto& it : bid_order)
			{
				reuslt += it.second;
			}
			return reuslt;
		}

		uint32_t total_sell_valume()const
		{
			uint32_t reuslt = 0;
			for (auto& it : ask_order)
			{
				reuslt += it.second;
			}
			return reuslt;
		}

		bool invalid()const
		{
			return id == default_code;
		}

	};

	const tick_info default_tick;

	struct tick_detail : public tick_info {
		std::tuple<double_t, double_t, double_t, double_t, double_t, double_t, double_t> extend;
	};

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
		position_info(const code_t& code) :id(code), long_pending(0), short_pending(0) {}
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
			return today_long.empty() && today_short.empty() && history_long.empty() && history_short.empty() && long_pending == 0U && short_pending == 0U;
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
		position_info() :long_pending(0U), short_pending(0U)
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
	};

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

		std::string		unit_id;

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
			return false;
		}

		bool is_sell()const
		{
			if (direction == direction_type::DT_SHORT && offset == offset_type::OT_OPEN)
			{
				return true;
			}
			if (direction == direction_type::DT_LONG && (offset == offset_type::OT_CLOSE || offset == offset_type::OT_CLSTD))
			{
				return true;
			}
			return false;
		}

	};
	const order_info default_order;

	/*
	*	标的类型
	*/
	enum class product_type:uint8_t
	{
		PT_INVALID = 0,
		PT_FUTURE ,		//期货
		PT_OPTION		//期权
	};
	struct instrument_info
	{
		code_t code;
		product_type classtype;
		std::string product;
		std::string underlying;
		double_t price_step;
		double_t multiple; //乘数
		uint32_t begin_day;
		uint32_t end_day;
		
		instrument_info() :
			code(),
			price_step(.0),
			multiple(.0),
			classtype(product_type::PT_INVALID),
			underlying(),
			begin_day(0U),
			end_day(0U)
		{}

	};

	const instrument_info default_instrument;

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

		order_statistic() :
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

	struct market_info
	{
		code_t code;

		double_t open_price;

		double_t close_price;

		double_t standard_price;

		double_t high_price;

		double_t low_price;

		double_t max_price;

		double_t min_price;

		uint32_t trading_day;

		std::map<double_t, uint32_t> volume_distribution;

		tick_info last_tick_info;

		market_info()
			:open_price(.0),
			close_price(.0),
			standard_price(.0),
			high_price(.0),
			low_price(.0),
			max_price(.0),
			min_price(.0),
			trading_day(0)
		{}
		double_t control_price()const
		{
			double_t control_price = last_tick_info.price;
			uint32_t max_volume = 0;
			for (const auto& it : volume_distribution)
			{
				if (it.second > max_volume)
				{
					max_volume = it.second;
					control_price = it.first;
				}
			}
			return control_price;
		}

		double_t middle_price()const
		{
			return (high_price + low_price) / 2.0;
		}

		void clear()
		{
			volume_distribution.clear();
		}

		bool invalid()const
		{
			return code == default_code;
		}
	};
	const market_info default_market;

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

		tape_info()
			:time(0U),
			price(.0),
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

		seqtm_t time; //时间

		uint32_t period;

		double_t open;

		double_t close;

		double_t high;

		double_t low;

		//成交量
		uint32_t volume;

		double_t detail_density; //价格详细系数（会根据这个系数合并details数据）

		//订单流中的明细
		std::map<double_t, uint32_t> sell_details;
		std::map<double_t, uint32_t> buy_details;
		std::map<double_t, uint32_t> other_details;

		uint32_t get_buy_volume(double_t price)const
		{
			double_t bucket_price = std::round(price / detail_density) * detail_density;
			auto it = buy_details.find(bucket_price);
			if (it == buy_details.end())
			{
				return 0;
			}
			return it->second;
		}

		uint32_t get_sell_volume(double_t price)const
		{
			double_t bucket_price = std::round(price / detail_density) * detail_density;
			auto it = sell_details.find(bucket_price);
			if (it == sell_details.end())
			{
				return 0;
			}
			return it->second;
		}

		//订单流中delta
		int32_t get_delta()const{
			int32_t delta = 0;
			for(const auto& it : sell_details)
			{
				delta += it.second;
			}
			for (const auto& it : sell_details)
			{
				delta -= it.second;
			}
			return delta;
		}

		double_t get_poc()const{

			double_t poc = .0;
			uint32_t max_volume = 0U;
			for(const auto& it : sell_details)
			{
				if(it.second>max_volume)
				{
					max_volume = it.second;
					poc = it.first;
				}
			}
			for (const auto& it : buy_details)
			{
				if (it.second > max_volume)
				{
					max_volume = it.second;
					poc = it.first;
				}
			}
			for (const auto& it : other_details)
			{
				if (it.second > max_volume)
				{
					max_volume = it.second;
					poc = it.first;
				}
			}
			return poc;
		}

		//（price_buy_volume - price_sell_volume）
		int32_t get_price_delta(double_t price)const {

			return static_cast<int32_t>(get_buy_volume(price) - get_sell_volume(price));
		}

		std::vector<std::tuple<double_t, uint32_t, uint32_t>> get_order_book()const
		{
			std::vector < std::tuple<double_t, uint32_t, uint32_t>> result;
			if (low == .0 || high == .0 || detail_density == .0) {
				return result;
			}
			for (double_t price = low; price <= high; price += detail_density)
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
			if (order_book.size() > 0)
			{
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
			detail_density = .0;
			buy_details.clear();
			sell_details.clear();
		}

		bar_info()
			:time(0),
			period(0U),
			open(0),
			close(0),
			high(0),
			low(0),
			volume(0),
			detail_density(.0)
		{}
	};

	const bar_info default_bar_info;
	const std::vector<bar_info> default_bar_vector;
}

