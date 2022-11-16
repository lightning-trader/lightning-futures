#include "lightning.h"
#include "runtime.h"
#include "evaluate.h"
#include "context.h"

extern "C"
{
	ltobj lt_create_context(context_type ctx_type, const char* config_path)
	{
		ltobj lt;
		lt.obj_type = CT_INVALID;
		lt.obj_ptr = nullptr;
		if(ctx_type == CT_RUNTIME)
		{
			auto obj = new runtime();
			if (!obj->init_from_file(config_path))
			{
				delete obj;
				obj = nullptr;
			}
			lt.obj_type = ctx_type;
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
			lt.obj_type = ctx_type;
			lt.obj_ptr = obj;
			return lt;
		}
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


	void lt_start_service(const ltobj& lt)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return;
		}
		c->start();
	}

	void lt_stop_service(const ltobj& lt)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return ;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return ;
		}
		c->stop();
	}

	estid_t lt_place_order(const ltobj& lt, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return INVALID_ESTID;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return INVALID_ESTID;
		}
		return c->place_order(offset, direction, code, count, price, flag);
	}

	void lt_cancel_order(const ltobj& lt, estid_t order_id)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return;
		}
		c->cancel_order(order_id);
	}

	
	void lt_set_trading_optimize(const ltobj& lt, uint32_t max_position, trading_optimal opt, bool flag)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return;
		}
		c->set_trading_optimize(max_position, opt, flag);
	}

	
	const position_info& lt_get_position(const ltobj& lt, const code_t& code)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return default_position;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return default_position;
		}
		return c->get_position(code);
	}

	
	const account_info& lt_get_account(const ltobj& lt)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return default_account;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return default_account;
		}
		return c->get_account();
	}


	const order_info& lt_get_order(const ltobj& lt, estid_t order_id)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return default_order;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return default_order;
		}
		return c->get_order(order_id);
	}


	void lt_subscribe(const ltobj& lt, const code_t& code)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return;
		}
		c->subscribe({ code });
	}

	void lt_unsubscribe(const ltobj& lt, const code_t& code)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return ;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return ;
		}
		c->unsubscribe({ code });
		
	}

	time_t lt_get_last_time(const ltobj& lt)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return -1;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return -1;
		}
		return c->get_last_time();
	}

	void lt_set_cancel_condition(const ltobj& lt, estid_t order_id, condition_callback callback)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return ;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return ;
		}
		c->set_cancel_condition(order_id, callback);
	}

	void lt_bind_callback(const ltobj& lt, tick_callback tick_cb, entrust_callback entrust_cb, deal_callback deal_cb
		, trade_callback trade_cb, cancel_callback cancel_cb)
	{
		if (lt.obj_type != CT_RUNTIME && lt.obj_type != CT_EVALUATE)
		{
			return;
		}
		context* c = (context*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return;
		}
		c->on_tick = tick_cb;
		c->on_entrust = entrust_cb;
		c->on_deal = deal_cb;
		c->on_trade = trade_cb;
		c->on_cancel = cancel_cb;
	}

	void lt_playback_history(const ltobj& lt, uint32_t trading_day)
	{
		if (lt.obj_type != CT_EVALUATE)
		{
			return;
		}
		evaluate* c = (evaluate*)(lt.obj_ptr);
		if (c == nullptr)
		{
			return;
		}
		c->play(trading_day);

	}
}