#pragma once
#include <define.h>
#include <data_types.hpp>
#include <lightning.h>
#include <functional>


class strategy
{

private:

	straid_t _id;

	class strategy_manager* _manager;

public:

	strategy();
	
	virtual ~strategy();

	/*
	*	初始化
	*/
	void init(straid_t id,class strategy_manager* manager);

	//回调函数
private:

	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init() {};

public:
	/*
	 *	tick推送
	 */
	virtual void on_tick(const tick_info& tick) {}

	
	/*
	 *	订单接收回报
	 *  @is_success	是否成功
	 *	@order	本地订单
	 */
	virtual void on_entrust(const order_info& order) {};

	/*
	 *	成交回报
	 *
	 *	@localid	本地订单id
	*/
	virtual void on_deal(estid_t localid,uint32_t deal_volume, uint32_t total_volume) {}

	/*
	 *	成交完成回报
	 *
	 *	@localid	本地订单id
	 */
	virtual void on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume) {}


	/*
	 *	撤单
	 *	@localid	本地订单id
	 */
	virtual void on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume) {}

	/*
	 *	错误
	 *	@localid	本地订单id
	 *	@error 错误代码
	 */
	virtual void on_error(estid_t localid, const uint32_t error) {}


protected:
	//功能函数
	/*
	 *	开多单
	 *	code 期货代码 SHFF.rb2301
	 *  price 如果是0表示市价单，其他表示现价单
	 *  flag 默认为正常单
	 *	@localid	本地订单id
	 */
	estid_t buy_for_open(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = OF_NOR);

	/*
	 *	平多单
	 *	code 期货代码 SHFF.rb2301
	 *  price 如果是0表示市价单，其他表示现价单
	 *  flag 默认为正常单
	 *	@localid	本地订单id
	 */
	estid_t sell_for_close(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = OF_NOR);

	/*
	 *	开空单
	 *	code 期货代码 SHFF.rb2301
	 *  price 如果是0表示市价单，其他表示现价单
	 *  flag 默认为正常单
	 *	@localid	本地订单id
	 */
	estid_t sell_for_open(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = OF_NOR);

	/*
	 *	平空单
	 *	code 期货代码 SHFF.rb2301
	 *  price 如果是0表示市价单，其他表示现价单
	 *  flag 默认为正常单
	 *	@localid	本地订单id
	 */
	estid_t buy_for_close(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = OF_NOR);

	/*
	*	下单单
	*	order_id 下单返回的id
	*/
	estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price = 0, order_flag flag = OF_NOR);
	/*
	 *	撤单
	 *	order_id 下单返回的id
	 */
	void cancel_order(estid_t order_id);

	/**  
	* 获取仓位信息
	*/
	const position_info& get_position(const code_t& code) const;

	/**
	* 获取账户资金
	*/
	const account_info& get_account() const;

	/**  
	* 获取委托订单
	**/
	const order_info& get_order(estid_t order_id) const;


	/**
	* 订阅行情
	**/
	void subscribe(const code_t& code) ;

	/**
	* 取消订阅行情
	**/
	void unsubscribe(const code_t& code) ;

	/**
	* 获取时间
	* 
	*/
	time_t get_last_time() const ;

	/*
	* 设置撤销条件(返回true时候撤销)
	*/
	void set_cancel_condition(estid_t order_id, std::function<bool(const tick_info&)> callback);


	/**
	* 获取最后一次下单时间
	*	跨交易日返回0
	*/
	time_t last_order_time();



	inline straid_t get_id()
	{
		return _id;
	}

	/**
	* 获取用户数据，直接写入会被保存到共享内存中
	*	注意多个策略时候id不能改变
	*	ID最大值与localdb配置中的userdata_block对应
	*/

	void* get_username(size_t size);
	

};


