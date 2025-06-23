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
#include <basic_define.h>
#include <log_define.hpp>
#include <trader_api.h>
#include <basic_types.hpp>
#include <params.hpp>
#include <condition_variable>
#include <CTP_V6.7.9_20250319/ThostFtdcTraderApi.h>
#include <thread>

namespace lt::driver
{
	class ctp_api_trader : public asyn_actual_trader, public CThostFtdcTraderSpi
	{
		/*
		 *	订单操作类型
		 */
		enum class action_flag
		{
			AF_CANCEL = '0',	//撤销
			AF_MODIFY = '3',	//修改
		};

	public:

		ctp_api_trader(std::unordered_map<std::string, std::string>& id_excg_map, const params& config);

		virtual ~ctp_api_trader();


		//////////////////////////////////////////////////////////////////////////
		//trader_api接口
	public:

		virtual bool login() override;

		virtual void logout()override;

		virtual bool is_usable() const override;

		virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

		virtual bool cancel_order(estid_t estid) override;

		virtual uint32_t get_trading_day()const override;

		virtual std::vector<order_info> get_all_orders() override;

		virtual std::vector<position_seed> get_all_positions() override;

		virtual std::vector<instrument_info> get_all_instruments() override;

		virtual daytm_t get_exchange_time(const char* exchange) const override;

		virtual std::pair<bool, daytm_t> get_product_state(const lt::code_t& product_code) const override;

		//////////////////////////////////////////////////////////////////////////
		//CTP交易接口实现
	public:
		virtual void OnFrontConnected() noexcept override;

		virtual void OnFrontDisconnected(int nReason) noexcept override;


		virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)noexcept override;

		virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;

		virtual void OnRtnOrder(CThostFtdcOrderField* pOrder) noexcept override;

		virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo) noexcept override;

		virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo) noexcept override;

		virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus) noexcept override;

		virtual void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept override;
	
	private:

		//认证
		bool do_auth();
		//登录
		bool do_login();

		bool do_logout();

		bool query_positions(bool is_sync);

		bool query_orders(bool is_sync);

		bool query_instruments(bool is_sync);

		void submit_settlement();

		bool is_product_trading(const std::string& product);

	private:


		inline int convert_direction_offset(direction_type dir_type, offset_type offset_type)
		{
			if (direction_type::DT_LONG == dir_type)
				if (offset_type == offset_type::OT_OPEN)
					return THOST_FTDC_D_Buy;
				else
					return THOST_FTDC_D_Sell;
			else
				if (offset_type == offset_type::OT_OPEN)
					return THOST_FTDC_D_Sell;
				else
					return THOST_FTDC_D_Buy;
		}

		inline direction_type wrap_direction_offset(TThostFtdcDirectionType dir_type, TThostFtdcOffsetFlagType offset_type)
		{
			if (THOST_FTDC_D_Buy == dir_type)
				if (offset_type == THOST_FTDC_OF_Open)
					return direction_type::DT_LONG;
				else
					return direction_type::DT_SHORT;
			else
				if (offset_type == THOST_FTDC_OF_Open)
					return direction_type::DT_SHORT;
				else
					return direction_type::DT_LONG;
		}

		inline int convert_offset_type(const code_t& code, uint32_t volume, offset_type offset, direction_type direction)
		{
			if (offset_type::OT_OPEN == offset)
			{
				return THOST_FTDC_OF_Open;
			}
			else if (offset_type::OT_CLOSE == offset)
			{
				return THOST_FTDC_OF_CloseYesterday;
			}

			return THOST_FTDC_OF_CloseToday;
		}

		inline offset_type wrap_offset_type(TThostFtdcOffsetFlagType offset_type)
		{
			if (THOST_FTDC_OF_Open == offset_type)
				return offset_type::OT_OPEN;
			else if (THOST_FTDC_OF_CloseToday == offset_type)
				return offset_type::OT_CLSTD;
			else
				return offset_type::OT_CLOSE;
		}

		inline int convert_action_flag(action_flag action_flag)
		{
			if (action_flag::AF_CANCEL == action_flag)
				return THOST_FTDC_AF_Delete;
			else
				return THOST_FTDC_AF_Modify;
		}
		inline estid_t generate_estid()
		{
			_order_ref.fetch_add(1);
			return generate_estid(_front_id, _session_id, _order_ref);
		}

		inline estid_t generate_estid(uint32_t front_id, uint32_t session_id, uint32_t order_ref)
		{
			uint64_t p1 = (uint64_t)session_id << 32;
			uint64_t p2 = (uint64_t)front_id << 16;
			uint64_t p3 = (uint64_t)order_ref;
			uint64_t v1 = p1 & 0xFFFFFFFF00000000LLU;
			uint64_t v2 = p2 & 0x00000000FFFF0000LLU;
			uint64_t v3 = p3 & 0x000000000000FFFFLLU;
			return v1 + v2 + v3;
		}

		inline void	extract_estid(estid_t estid, uint32_t& front_id, uint32_t& session_id, uint32_t& order_ref)
		{
			uint64_t v1 = estid & 0xFFFFFFFF00000000LLU;
			uint64_t v2 = estid & 0x00000000FFFF0000LLU;
			uint64_t v3 = estid & 0x000000000000FFFFLLU;
			session_id = static_cast<uint32_t>(v1 >> 32);
			front_id = static_cast<uint32_t>(v2 >> 16);
			order_ref = static_cast<uint32_t>(v3);
		}

		inline uint32_t genreqid()
		{
			return _reqid.fetch_add(1);
		}

	protected:

		CThostFtdcTraderApi* _td_api;
		std::atomic<uint32_t>	_reqid;

		std::string				_front_addr;
		std::string				_broker_id;
		std::string				_userid;
		std::string				_password;
		std::string				_appid;
		std::string				_authcode;
		std::string				_product_info;
		std::string				_public_topic;
		std::string				_private_topic;
		


		uint32_t				_front_id;		//前置编号
		uint32_t				_session_id;	//会话编号
		std::atomic<uint32_t>	_order_ref;		//报单引用
		//
		std::map<code_t, position_seed>		_position_info;
		//
		entrust_map							_order_info;

		std::map<code_t, instrument_info>		_instrument_info;

		bool							_is_runing;
		daytm_t							_login_time;

		std::mutex _mutex;
		std::unique_lock<std::mutex>	_process_mutex;
		std::condition_variable			_process_signal;
		std::atomic<bool>				_is_in_query;
		std::atomic<bool>				_is_inited;
		bool							_is_connected;
		std::atomic<bool>				_is_sync_wait;
		typedef CThostFtdcTraderApi* (*trader_creator_function)(const char*);
		trader_creator_function			_ctp_creator;
		dll_handle						_trader_handle;

		std::map<std::string, int32_t>  _exchange_delta_time;

		std::map<uint32_t, estid_t>		_request_estid_map;

		//正在交易中的品种 product_code 对应 begin_time
		std::map<std::string,daytm_t> _trading_product;
	};

}
