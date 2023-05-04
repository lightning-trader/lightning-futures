// example.cpp: 目标的源文件。
//

#include "example.h"
#include <define.h>
#include "lightning.h"
#include <thread>
#include <time_utils.hpp>
struct persist_data
{
	uint32_t trading_day;
	estid_t sell_order;
	estid_t buy_order;

	persist_data() :
		trading_day(0x0U),
		sell_order(INVALID_ESTID),
		buy_order(INVALID_ESTID)
	{}
};

ltobj _ctx;
untid_t _unit_id ;
persist_data* _order_data ;
double_t _delta ;


/*
*	撤单条件检查
*/
bool on_cancel_check(estid_t estid, const tick_info& tick)
{
	auto& order = lt_get_order(_ctx,estid);
	if(tick.tick - order.create_time>60)
	{
		return true ;
	}
	return false ;
}

/*
*	撤单条件检查
*/
bool on_order_filter(const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)
{
	//auto& status = lt_get_order_statistic(_ctx);
	auto now = lt_get_last_time(_ctx);
	auto last_order = lt_last_order_time(_ctx);
	if(now - last_order<1)
	{
		return false;
	}
	return true;
}

/*
*	交易日初始化完成
*/
void on_ready()
{
	_unit_id = 1;
	_delta = 2;
	auto trading_day = lt_get_trading_day(_ctx);
	_order_data = (persist_data*)lt_get_userdata(_ctx, _unit_id,sizeof(persist_data));
	if (_order_data->trading_day != trading_day)
	{
		_order_data->trading_day = trading_day;
		_order_data->buy_order = INVALID_ESTID;
		_order_data->sell_order = INVALID_ESTID;
	}
	else
	{
		auto& buy_order = lt_get_order(_ctx,_order_data->buy_order);
		if (buy_order.est_id != INVALID_ESTID)
		{
			lt_set_cancel_condition(_ctx,buy_order.est_id, on_cancel_check);
		}
		else
		{
			_order_data->buy_order = INVALID_ESTID;
		}
		auto& sell_order = lt_get_order(_ctx, _order_data->sell_order);
		if (sell_order.est_id != INVALID_ESTID)
		{
			lt_set_cancel_condition(_ctx, sell_order.est_id, on_cancel_check);
		}
		else
		{
			_order_data->sell_order = INVALID_ESTID;
		}
	}
}

/*
*	收到tick行情
*/
void on_tick(const tick_info& tick, const deal_info& deal)
{
	//还没有准备完成收到tick数据忽略
	if(!lt_is_trading_ready(_ctx))
	{
		return ;
	}
	//buy_order已经成交或者已经被撤销
	if(_order_data->buy_order == INVALID_ESTID)
	{
		auto pos = lt_get_position(_ctx,tick.id);
		if(pos.today_short.usable()>0|| pos.yestoday_short.usable())
		{
			_order_data->buy_order = lt_place_order(_ctx, 1, offset_type::OT_CLOSE, direction_type::DT_SHORT, tick.id, 1,tick.buy_price()- _delta,order_flag::OF_NOR);
		}
		else
		{
			_order_data->buy_order = lt_place_order(_ctx, 1, offset_type::OT_OPEN, direction_type::DT_LONG, tick.id, 1, tick.buy_price() - _delta, order_flag::OF_NOR);
		}
	}
	//sell_order已经成交或者已经被撤销
	if(_order_data->sell_order == INVALID_ESTID)
	{
		auto pos = lt_get_position(_ctx, tick.id);
		if (pos.today_long.usable() > 0 || pos.yestoday_long.usable())
		{
			_order_data->sell_order = lt_place_order(_ctx, 1, offset_type::OT_CLOSE, direction_type::DT_LONG, tick.id, 1, tick.buy_price() - _delta, order_flag::OF_NOR);
		}
		else
		{
			_order_data->sell_order = lt_place_order(_ctx, 1, offset_type::OT_OPEN, direction_type::DT_SHORT, tick.id, 1, tick.buy_price() - _delta, order_flag::OF_NOR);
		}
	}
}

/*
 *	订单接收回报
 *  @is_success	是否成功
 *	@order	本地订单
 */
void on_entrust(const order_info& order)
{
	lt_set_cancel_condition(_ctx,order.est_id, on_cancel_check);
}

/*
 *	成交回报
 *
 *	@localid	本地订单id
 */
void on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	if (_order_data->sell_order == localid)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
	if(_order_data->buy_order == localid)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
}


/*
 *	撤单
 *	@localid	本地订单id
 */
void on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type directionv, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	if (_order_data->sell_order == localid)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
	if (_order_data->buy_order == localid)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
}

/*
 *	错误
 *	@localid	本地订单id
 *	@error 错误代码
 */
void on_error(error_type type, estid_t localid, const uint32_t error)
{
	if (_order_data->sell_order == localid)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
	if (_order_data->buy_order == localid)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
}

//开始实盘
void start_runtime(const char* config)
{
	//1、创建实盘环境
	_ctx = lt_create_context(CT_RUNTIME, config);
	//2、启动服务
	lt_start_service(_ctx);
	//3、注册事件
	order_event evt;
	evt.on_entrust = on_entrust;
	evt.on_deal = nullptr;
	evt.on_trade = on_trade;
	evt.on_cancel = on_cancel;
	evt.on_error = on_error;
	lt_bind_realtime_event(_ctx, evt, on_ready,nullptr);
	//设置拦截函数
	lt_set_trading_filter(_ctx, on_order_filter);
	//4、订阅行情
	std::map<code_t, std::set<uint32_t>> bar_sub;
	lt_subscribe(_ctx,{"SHFE.rb2210"}, on_tick, bar_sub,nullptr);
	//5、等待收盘
	while (!lt_is_trading_ready(_ctx))
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	LOG_INFO("runtime_engine run in trading ready");
	time_t close_time = lt_get_close_time(_ctx);
	time_t delta_seconds = close_time - get_now();
	if (delta_seconds > 0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(delta_seconds));
	}
	//6、停止服务
	lt_stop_service(_ctx);
	//7、销毁环境
	lt_destory_context(_ctx);
}

//开始评估
void start_evaluate(const char* config,uint32_t trading_day)
{

	//1、创建评估环境
	_ctx = lt_create_context(CT_EVALUATE, config);
	//2、启动服务
	lt_start_service(_ctx);
	//3、注册事件
	order_event evt;
	evt.on_entrust = on_entrust;
	evt.on_deal = nullptr;
	evt.on_trade = on_trade;
	evt.on_cancel = on_cancel;
	evt.on_error = on_error;
	lt_bind_realtime_event(_ctx, evt, on_ready, nullptr);
	//4、订阅行情
	std::map<code_t, std::set<uint32_t>> bar_sub;
	lt_subscribe(_ctx, { "SHFE.rb2210" }, on_tick, bar_sub, nullptr);
	//5、播放历史数据
	lt_playback_history(_ctx, trading_day);
	//6、停止服务
	lt_stop_service(_ctx);
	//7、销毁环境
	lt_destory_context(_ctx);
}

int main()
{
	//start_evaluate("evaluate.ini", 20220801);
	start_runtime("runtime.ini");
	return 0;
}
