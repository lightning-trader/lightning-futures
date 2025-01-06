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
#include "strategy.h"
#include "receiver.h"
#include "engine.h"



class orderflow_strategy : public lt::hft::strategy,public lt::hft::bar_receiver
{

	struct persist_data
	{
		uint32_t trading_day;
		lt::estid_t sell_order;
		lt::estid_t buy_order;
	};
public:

	orderflow_strategy(lt::hft::straid_t id, lt::hft::engine* engine, const lt::code_t& code, uint32_t period, uint32_t open_once, uint32_t multiple, uint32_t threshold, uint32_t position_limit) :
		lt::hft::strategy(id, engine, true, true),
		_code(code),
		_open_once(open_once),
		_period(period),
		_multiple(multiple),
		_threshold(threshold),
		_position_limit(position_limit)
	{
	};

	~orderflow_strategy()
	{
		
	};


public:


	/*
	 *	��ʼ���¼�
	 *	����������ֻ��ص�һ��
	 */
	virtual void on_init(lt::hft::subscriber& suber) override;

	/*
	 *	bar����
	 */
	virtual void on_bar(const lt::bar_info& bar) override;


	/*
	 *	�������ջر�
	 *  @is_success	�Ƿ�ɹ�
	 *	@order	���ض���
	 */
	virtual void on_entrust(const lt::order_info& order) override;

	/*
	 *	�ɽ��ر�
	 *
	 *	@localid	���ض���id
	 */
	virtual void on_trade(lt::estid_t localid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double_t price, uint32_t volume)  override;


	/*
	 *	����
	 *	@localid	���ض���id
	 */
	virtual void on_cancel(lt::estid_t localid, const lt::code_t& code, lt::offset_type offset, lt::direction_type directionv, double_t price, uint32_t cancel_volume, uint32_t total_volume)  override;

	/*
	 *	����
	 *	@localid	���ض���id
	 *	@error �������
	 */
	virtual void on_error(lt::error_type type, lt::estid_t localid, const lt::error_code error) override;

	/*
	 *	����
	 */
	virtual void on_destroy(lt::hft::unsubscriber& unsuber)override;

private:

	bool is_close_coming() const;

private:

	void try_buy();

	void try_sell();

	//��Լ����
	lt::code_t _code;

	//һ�ο��ֶ�����
	uint32_t _open_once;

	//k�����ڣ����ӣ�
	uint32_t _period;

	//ʧ��delta
	uint32_t _multiple;

	//ʧ�ⴥ���źŵ���ֵ
	uint32_t _threshold;

	//��������
	uint32_t _position_limit;

	persist_data _order_data;

};

