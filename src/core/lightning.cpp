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

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, subscribe, (const ltobj& lt, const code_t& code), ({ code }));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, unsubscribe, (const ltobj& lt, const code_t& code), ({ code }));

	LT_INTERFACE_IMPLEMENTATION(time_t, 0, context, get_last_time, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, set_cancel_condition, (const ltobj& lt, estid_t est_id, condition_callback callback), (est_id, callback));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, set_trading_filter, (const ltobj& lt, filter_callback callback), (callback));


	void lt_bind_callback(const ltobj& lt, tick_callback tick_cb, entrust_callback entrust_cb, deal_callback deal_cb
		, trade_callback trade_cb, cancel_callback cancel_cb, error_callback error_cb, ready_callback ready_cb)
	{
		LT_INTERFACE_CHECK(context, VOID_DEFAULT)
		c->on_tick = tick_cb;
		c->on_entrust = entrust_cb;
		c->on_deal = deal_cb;
		c->on_trade = trade_cb;
		c->on_cancel = cancel_cb;
		c->on_error = error_cb;
		c->on_ready = ready_cb;
	}

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, evaluate, playback_history, (const ltobj& lt, uint32_t trading_day), (trading_day));

	LT_INTERFACE_IMPLEMENTATION(time_t, 0, context, last_order_time, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(const order_statistic&, default_statistic, context, get_order_statistic, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(void*, nullptr, context, get_userdata, (const ltobj& lt, uint32_t index, size_t size), (index, size));

	LT_INTERFACE_IMPLEMENTATION(bool, false, context, is_trading_ready, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(uint32_t, 0U, context, get_trading_day, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(time_t, 0, context, get_close_time, (const ltobj& lt), ());

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, use_custom_chain, (const ltobj& lt, untid_t untid, trading_optimal opt, bool flag), (untid, opt, flag));

	LT_INTERFACE_IMPLEMENTATION(void, VOID_DEFAULT, context, bind_transfer_info, (const ltobj& lt, const code_t& code, const code_t& expire, double_t offset), (code, expire, offset));

}