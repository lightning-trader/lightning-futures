#include "lightning.h"
#include "runtime_context.h"
#include "evaluate_context.h"
#include "context.h"

extern "C"
{
	ltobj* create_runtime_context(const char* config_path)
	{
		auto context = new runtime_context();
		if (!context->init_from_file(config_path))
		{
			delete context;
			context = nullptr ;
		}
		return context;
	}

	void destory_runtime_context(ltobj* ctx)
	{
		if(ctx)
		{
			runtime_context* runtime = (runtime_context*)ctx;
			delete runtime;
			ctx = nullptr;
		}
	}

	ltobj* create_evaluate_context(const char* config_path)
	{
		auto evaluate = new evaluate_context();
		if (!evaluate->init_from_file(config_path))
		{
			delete evaluate;
			evaluate = nullptr;
		}
		return evaluate;
	}

	void destory_evaluate_context(ltobj* ctx)
	{
		if (ctx)
		{
			evaluate_context* evaluate = (evaluate_context*)ctx;
			delete evaluate;
			ctx = nullptr;
		}
	}

	void start(ltobj* ctx)
	{
		context* c = dynamic_cast<context*>(ctx);
		if(c)
		{
			c->start();
		}
	}

	void stop(ltobj* ctx)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->stop();
		}
	}

	estid_t place_order(ltobj* ctx, offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			return c->place_order(offset, direction, code, count, price, flag);
		}
		return estid_t();
	}

	void cancel_order(ltobj* ctx, estid_t order_id)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->cancel_order(order_id);
		}
	}

	
	void set_trading_optimize(ltobj* ctx, uint32_t max_position, trading_optimal opt, bool flag)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->set_trading_optimize(max_position, opt, flag);
		}
	}

	
	const position_info* get_position(ltobj* ctx, code_t code)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			return c->get_position(code);
		}
		return nullptr ;
	}

	
	const account_info* get_account(ltobj* ctx)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			return c->get_account();
		}
		return nullptr ;
	}


	const order_info* get_order(ltobj* ctx, estid_t order_id)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			return c->get_order(order_id);
		}
		return nullptr ;
	}


	void subscribe(ltobj* ctx, code_t code)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->subscribe({code});
		}
	}

	void unsubscribe(ltobj* ctx, code_t code)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->unsubscribe({code});
		}
	}

	time_t get_last_time(ltobj* ctx)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->get_last_time();
		}
		return -1;
	}

	void set_cancel_condition(ltobj* ctx, estid_t order_id, on_condition_callback callback)
	{
		context* c = dynamic_cast<context*>(ctx);
		if (c)
		{
			c->set_cancel_condition(order_id, callback);
		}
	}
}