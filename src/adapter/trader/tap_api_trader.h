﻿/*
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
#include <queue>
#include <stdint.h>
#include <thread>
#include <define.h>
#include <log_define.hpp>
#include <trader_api.h>
#include <params.hpp>
#include <condition_variable>
#include <TAP_V9_20200808/TapTradeAPI.h>
#include <library_helper.hpp>


namespace lt::driver
{
	class tap_api_trader : public asyn_actual_trader, public ITapTradeAPINotify
	{
		struct order_index
		{
			TAPICHAR					ServerFlag;						///< 服务器标识
			TAPISTR_20					OrderNo;						///< 委托编码
		};

	public:

		tap_api_trader(std::unordered_map<std::string, std::string>& id_excg_map, const params& config);

		virtual ~tap_api_trader();

		//////////////////////////////////////////////////////////////////////////
		//trader_api接口
	public:

		virtual bool login() override;

		virtual void logout()override;

		virtual bool is_usable() const override;

		virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

		virtual bool cancel_order(estid_t estid) override;

		virtual uint32_t get_trading_day()const override;

		virtual std::shared_ptr<trader_data> get_trader_data() override;

		//////////////////////////////////////////////////////////////////////////

	public:
		//对ITapTradeAPINotify的实现
		virtual void TAP_CDECL OnConnect()noexcept;
		virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPITradeLoginRspInfo* loginRspInfo)noexcept;
		virtual void TAP_CDECL OnAPIReady()noexcept;
		virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode)noexcept;
		virtual void TAP_CDECL OnRspChangePassword(TAPIUINT32 sessionID, TAPIINT32 errorCode)noexcept {};
		virtual void TAP_CDECL OnRspSetReservedInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, const TAPISTR_50 info)noexcept {};
		virtual void TAP_CDECL OnRspQryAccount(TAPIUINT32 sessionID, TAPIUINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspQryFund(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFundData* info) noexcept {};
		virtual void TAP_CDECL OnRtnFund(const TapAPIFundData* info) noexcept {};
		virtual void TAP_CDECL OnRspQryExchange(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeInfo* info) noexcept {};
		virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICommodityInfo* info) noexcept {};
		virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPITradeContractInfo* info) noexcept {};
		virtual void TAP_CDECL OnRtnContract(const TapAPITradeContractInfo* info) noexcept {};
		virtual void TAP_CDECL OnRtnOrder(const TapAPIOrderInfoNotice* info)noexcept;
		virtual void TAP_CDECL OnRspOrderAction(TAPIUINT32 sessionID, TAPIUINT32 errorCode, const TapAPIOrderActionRsp* info)noexcept;
		virtual void TAP_CDECL OnRspQryOrder(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo* info)noexcept;
		virtual void TAP_CDECL OnRspQryOrderProcess(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspQryFill(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFillInfo* info)noexcept {};
		virtual void TAP_CDECL OnRtnFill(const TapAPIFillInfo* info) noexcept {};
		virtual void TAP_CDECL OnRspQryPosition(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIPositionInfo* info)noexcept;
		virtual void TAP_CDECL OnRtnPosition(const TapAPIPositionInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspQryClose(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICloseInfo* info) noexcept {};
		virtual void TAP_CDECL OnRtnClose(const TapAPICloseInfo* info)noexcept {};
		virtual void TAP_CDECL OnRtnPositionProfit(const TapAPIPositionProfitNotice* info) noexcept {};
		virtual void TAP_CDECL OnRspQryDeepQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIDeepQuoteQryRsp* info) noexcept {};
		virtual void TAP_CDECL OnRspQryExchangeStateInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeStateInfo* info) noexcept {};
		virtual void TAP_CDECL OnRtnExchangeStateInfo(const TapAPIExchangeStateInfoNotice* info) noexcept {};
		virtual void TAP_CDECL OnRtnReqQuoteNotice(const TapAPIReqQuoteNotice* info) noexcept {}; //V9.0.2.0 20150520
		virtual void TAP_CDECL OnRspUpperChannelInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIUpperChannelInfo* info) noexcept {};
		virtual void TAP_CDECL OnRspAccountRentInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountRentInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspSubmitUserLoginInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPISubmitUserLoginRspInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspQryBill(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIBillQryRsp* info) noexcept {};
		virtual void TAP_CDECL OnRspQryAccountStorage(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountStorageInfo* info)noexcept {};
		virtual void TAP_CDECL OnRtnAccountStorage(const TapAPIAccountStorageInfo* info)noexcept {};

	private:

		bool query_positions(bool is_sync);

		bool query_orders(bool is_sync);

	private:


		inline int convert_direction_offset(direction_type dir_type, offset_type offset_type)
		{
			if (direction_type::DT_LONG == dir_type)
				if (offset_type == offset_type::OT_OPEN)
					return TAPI_SIDE_BUY;
				else
					return TAPI_SIDE_SELL;
			else
				if (offset_type == offset_type::OT_OPEN)
					return TAPI_SIDE_SELL;
				else
					return TAPI_SIDE_BUY;
		}

		inline direction_type wrap_direction_offset(TAPISideType dir_type, TAPIPositionEffectType offset_type)
		{
			if (TAPI_SIDE_BUY == dir_type)
				if (offset_type == TAPI_PositionEffect_OPEN)
					return direction_type::DT_LONG;
				else
					return direction_type::DT_SHORT;
			else
				if (offset_type == TAPI_PositionEffect_OPEN)
					return direction_type::DT_SHORT;
				else
					return direction_type::DT_LONG;
		}

		inline int convert_offset_type(const code_t& code, uint32_t volume, offset_type offset, direction_type direction)
		{
			if (offset_type::OT_OPEN == offset)
			{
				return TAPI_PositionEffect_OPEN;
			}
			else
			{
				if (code.is_distinct())
				{
					if (offset_type::OT_CLOSE == offset)
					{
						return TAPI_PositionEffect_COVER;
					}
					else
					{
						return TAPI_PositionEffect_COVER_TODAY;
					}
				}
				else
				{
					return TAPI_PositionEffect_COVER;
				}
			}
		}

		inline offset_type wrap_offset_type(const code_t& code, TAPIPositionEffectType offset_type)
		{
			if (TAPI_PositionEffect_OPEN == offset_type)
			{
				return offset_type::OT_OPEN;
			}
			else
			{
				if (code.is_distinct())
				{
					if (TAPI_PositionEffect_COVER_TODAY == offset_type)
						return offset_type::OT_CLSTD;
					else
						return offset_type::OT_CLOSE;
				}
				else
				{
					return offset_type::OT_CLSTD;
				}
			}
		}

		inline estid_t generate_estid()
		{
			_order_ref++;
			uint64_t p1 = (uint64_t)_login_time << 32;
			uint64_t p2 = (uint64_t)_order_ref;
			uint64_t v1 = p1 & 0xFFFFFFFF00000000LLU;
			uint64_t v2 = p2 & 0x00000000FFFFFFFFLLU;
			return v1 + v2;
		}


	protected:

		ITapTradeAPI* _td_api;
		uint32_t		_reqid;

		std::string				_ip;
		uint16_t				_port;
		std::string				_userid;
		std::string				_password;
		std::string				_appid;
		std::string				_authcode;

		uint32_t				_order_ref;
		uint32_t				_login_time;	//会话编号

		std::map<code_t, position_seed>	_position_info;

		entrust_map						_order_info;
		//
		std::map<estid_t, order_index>	_order_index;

		bool					_is_runing;


		std::mutex _mutex;
		std::unique_lock<std::mutex>	_process_mutex;
		std::condition_variable			_process_signal;

		bool							_is_inited;
		bool							_is_connected;
		std::atomic<bool>				_is_sync_wait;
		std::atomic<bool>				_is_in_query;

		typedef ITapTradeAPI* (*trader_creator)(const TapAPIApplicationInfo* appInfo, TAPIINT32& iResult);
		trader_creator					_tap_creator;

		typedef void (*trader_destroyer)(ITapTradeAPI*);
		trader_destroyer				_tap_destroyer;

		dll_handle						_trader_handle;
		uint32_t						_trading_day;
	};
}

