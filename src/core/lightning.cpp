#include "lightning.h"
#include "runtime.h"
#include "evaluate.h"
#include "context.h"

extern "C"
{

	ltobj lt_create_context(context_type ctx_type, const char* config_path)
	{
		ltobj lt;
		lt.obj_type = ctx_type;
		if(ctx_type == CT_RUNTIME)
		{
			auto obj = new runtime();
			if (!obj->init_from_file(config_path))
			{
				delete obj;
				obj = nullptr;
			}
			
			lt.obj_ptr = obj;
			return lt;
		}
		if(ctx_type == CT_EVALUATE)
		{
			auto obj = new evaluate();
			if (!obj->init_from_file(config_path))
			{
				delete obj;
				obj = nullptr;
			}
			lt.obj_ptr = obj;
			return lt;
		}
		lt.obj_ptr = nullptr;
		return lt;
	}

	void lt_destory_context(ltobj& lt)
	{
		if(lt.obj_ptr)
		{
			if(lt.obj_type == CT_RUNTIME)
			{
				runtime* obj = (runtime*)lt.obj_ptr;
				delete obj;
				lt.obj_ptr = nullptr;
			}
			else if (lt.obj_type == CT_EVALUATE)
			{
				evaluate* obj = (evaluate*)lt.obj_ptr;
				delete obj;
				lt.obj_ptr = nullptr;
			}
		}
	}

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, start_service, (const ltobj& lt),());
	
	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, stop_service, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(estid_t, INVALID_ESTID, context, place_order, (const ltobj& lt, untid_t untid, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag), (untid, offset, direction, code, count, price, flag));
	
	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, cancel_order, (const ltobj& lt, estid_t est_id), (est_id));
	
	LT_INTERFACE_IMPLEMENTATION(const position_info&, default_position, context, get_position, (const ltobj& lt, const code_t& code), (code));
	
	LT_INTERFACE_IMPLEMENTATION(const account_info&, default_account, context, get_account, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(const order_info&, default_order, context, get_order, (const ltobj& lt, estid_t est_id), (est_id));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, subscribe, (const ltobj& lt, const std::set<code_t>& tick_data, tick_callback tick_cb, const std::map<code_t, std::set<uint32_t>>& bar_data,bar_callback bar_cb), (tick_data, tick_cb, bar_data, bar_cb));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, unsubscribe, (const ltobj& lt, const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data), (tick_data, bar_data));

	LT_INTERFACE_IMPLEMENTATION(daytm_t, 0, context, get_last_time, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, set_cancel_condition, (const ltobj& lt, estid_t est_id, condition_callback callback), (est_id, callback));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, set_trading_filter, (const ltobj& lt, filter_callback callback), (callback));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, bind_realtime_event, (const ltobj& lt, const order_event& od_evt,ready_callback ready_cb,update_callback update_cb), (od_evt, ready_cb, update_cb));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, bind_delayed_event, (const ltobj& lt, const order_event& od_evt,account_callback account_cb, position_callback position_cb), (od_evt, account_cb, position_cb));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, evaluate, playback_history, (const ltobj& lt, uint32_t trading_day), (trading_day));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, evaluate, crossday_settlement, (const ltobj& lt, uint32_t trading_day), (trading_day));

	LT_INTERFACE_IMPLEMENTATION(daytm_t, 0, context, last_order_time, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(const order_statistic&, default_statistic, context, get_order_statistic, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(bool, false, context, is_trading_ready, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(uint32_t, 0U, context, get_trading_day, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(daytm_t, 0, context, get_close_time, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, use_custom_chain, (const ltobj& lt, untid_t untid, bool flag), (untid, flag));

	LT_INTERFACE_IMPLEMENTATION(const today_market_info&, default_today_market_info, context, get_today_market_info, (const ltobj& lt, const code_t& code), (code));

	LT_INTERFACE_IMPLEMENTATION(uint32_t, 0U, context, get_pending_position, (const ltobj& lt, const code_t& code, offset_type offset, direction_type direction), (code, offset, direction));

}