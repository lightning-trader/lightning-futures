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
#include "tap_api_market.h"
#include <filesystem>
#include <time_utils.hpp>
#include <params.hpp>
#include <log_define.hpp>
#include <../../api/TAP_V9_20200808/TapAPIError.h>

using namespace lt;
using namespace lt::driver;

tap_api_market::tap_api_market(std::unordered_map<std::string, std::string>& id_excg_map, const params& config)
	:asyn_actual_market(id_excg_map)
	,_md_api(nullptr)
	, _port(0)
	,_process_mutex(_mutex)
	, _is_inited(false)
	, _trading_day(0)
	, _market_handle(NULL)
	, _tap_creator(NULL)
	, _tap_destroyer(NULL)
{
	try
	{
		_ip = config.get<std::string>("ip");
		_port = config.get<uint16_t>("port");
		_userid = config.get<std::string>("userid");
		_password = config.get<std::string>("passwd");
		_authcode = config.get<std::string>("authcode");
	}
	catch (...)
	{
		LTLOG_ERROR("tap_api_market init error ");
	}
	_market_handle = library_helper::load_library("TapQuoteAPI");
	if (_market_handle)
	{
		_tap_creator = (market_creator_function)library_helper::get_symbol(_market_handle, "CreateTapQuoteAPI");
		_tap_destroyer = (market_destroyer_function)library_helper::get_symbol(_market_handle, "FreeTapQuoteAPI");
	}
	else
	{
		LTLOG_ERROR("tap_api_market TapQuoteAPI load error ");
	}

	LTLOG_INFO("tap market init");
	//SetTapQuoteAPILogLevel(APILOGLEVEL_DEBUG);
	//SetTapQuoteAPIDataPath("md_log");
}


tap_api_market::~tap_api_market()
{

}

bool tap_api_market::login()
{
	TapAPIApplicationInfo stAppInfo;
	strcpy(stAppInfo.AuthCode, _authcode.c_str());
	const char* log_path = "./log/api";
	if (!std::filesystem::exists(log_path))
	{
		std::filesystem::create_directories(log_path);
	}
	strcpy(stAppInfo.KeyOperationLogPath, log_path);
	TAPIINT32 iResult = TAPIERROR_SUCCEED;
	_md_api = _tap_creator(&stAppInfo, iResult);
	if (NULL == _md_api) {
		LTLOG_FATAL("tap_api_market login error :", iResult);
		return false;
	}
	_md_api->SetAPINotify(this);
	
	//设定服务器IP、端口
	TAPIINT32 iErr = _md_api->SetHostAddress(_ip.c_str(),_port);
	if (TAPIERROR_SUCCEED != iErr) {
		LTLOG_FATAL("SetHostAddress Error:" , iErr );
		return false;
	}
	//登录服务器
	TapAPIQuoteLoginAuth stLoginAuth;
	memset(&stLoginAuth, 0, sizeof(stLoginAuth));
	strcpy(stLoginAuth.UserNo, _userid.c_str());
	strcpy(stLoginAuth.Password, _password.c_str());
	stLoginAuth.ISModifyPassword = APIYNFLAG_NO;
	stLoginAuth.ISDDA = APIYNFLAG_NO;
	iErr = _md_api->Login(&stLoginAuth);
	if (TAPIERROR_SUCCEED != iErr) {
		LTLOG_FATAL("Login Error:", iErr);
		return false;
	}
	_process_signal.wait(_process_mutex);
	_is_inited = true ;
	return true ;
}

void tap_api_market::logout()
{
	//do_logout();
	_id_excg_map.clear();
	if (_md_api)
	{
		_tap_destroyer(_md_api);
		_md_api = nullptr;
	}
	_is_inited = false;
}

void tap_api_market::OnAPIReady()noexcept
{
	LTLOG_INFO("OnAPIReady :", _ip.c_str(),_port);
	_process_signal.notify_all();
}

void tap_api_market::OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo* info)noexcept
{
	if (TAPIERROR_SUCCEED == errorCode) {
		LTLOG_INFO("登录成功，等待API初始化...");
		if(info)
		{
			_trading_day = date_to_uint(info->TradeDate);
		}
	}
	else {
		LTLOG_ERROR("登录失败，错误码:" , errorCode);
		//_process_signal.notify_all();
	}
}


void tap_api_market::OnDisconnect(TAPIINT32 reasonCode)noexcept
{
	LTLOG_INFO("tap_api_market OnDisconnect : Reason -> %d", reasonCode);
}

void tap_api_market::OnRtnQuote(const TapAPIQuoteWhole* info)noexcept
{	
	if (info == nullptr)
	{
		return;
	}
	
	PROFILE_INFO(info->Contract.Commodity.CommodityNo);

	auto tick_data = tick_info(
		wrap_code(info->Contract),
		make_daytm(info->DateTimeStamp + 11, true),
		info->QLastPrice,
		info->QTotalQty,
		static_cast<double_t>(info->QPositionQty),
		info->QAveragePrice,
		_trading_day,//
		{
			std::make_pair(info->QBidPrice[0], static_cast<uint32_t>(info->QBidQty[0])),
			std::make_pair(info->QBidPrice[1], static_cast<uint32_t>(info->QBidQty[1])),
			std::make_pair(info->QBidPrice[2], static_cast<uint32_t>(info->QBidQty[2])),
			std::make_pair(info->QBidPrice[3], static_cast<uint32_t>(info->QBidQty[3])),
			std::make_pair(info->QBidPrice[4], static_cast<uint32_t>(info->QBidQty[4]))
		},
		{
			std::make_pair(info->QAskPrice[0], static_cast<uint32_t>(info->QAskQty[0])),
			std::make_pair(info->QAskPrice[1], static_cast<uint32_t>(info->QAskQty[1])),
			std::make_pair(info->QAskPrice[2], static_cast<uint32_t>(info->QAskQty[2])),
			std::make_pair(info->QAskPrice[3], static_cast<uint32_t>(info->QAskQty[3])),
			std::make_pair(info->QAskPrice[4], static_cast<uint32_t>(info->QAskQty[4]))
		}
	);
	auto extend_data = std::make_tuple(
		info->QOpeningPrice,
		info->QClosingPrice,
		info->QHighPrice,
		info->QLowPrice,
		info->QLimitUpPrice,
		info->QLimitDownPrice,
		info->QPreSettlePrice
	);
	PROFILE_DEBUG(tick_data.id.get_symbol());
	this->fire_event(market_event_type::MET_TickReceived, tick_data);
}

void tap_api_market::OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole* info)noexcept
{
	LTLOG_INFO("SubMarketData : code ", errorCode);
}

void tap_api_market::OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract* info)noexcept
{
	LTLOG_INFO("UnSubMarketData : code ", errorCode);
}

void tap_api_market::subscribe(const std::set<code_t>& code_list)
{
	//订阅行情
	for(const auto& it : code_list)
	{
		TapAPIContract stContract;
		memset(&stContract, 0, sizeof(stContract));
		
		strcpy(stContract.Commodity.ExchangeNo, it.get_exchange());
		stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;

		convert_code(stContract,it);

		auto iErr = _md_api->SubscribeQuote(&_reqid, &stContract);
		if (TAPIERROR_SUCCEED != iErr) {
			LTLOG_ERROR( "SubscribeQuote Error:" , iErr);
		}
	}
	
}

void tap_api_market::unsubscribe(const std::set<code_t>& code_list)
{
	for (const auto& it : code_list)
	{
		TapAPIContract stContract;
		memset(&stContract, 0, sizeof(stContract));
		strcpy(stContract.Commodity.ExchangeNo, it.get_exchange());
		stContract.Commodity.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
		//strcpy(stContract.Commodity.CommodityNo, DEFAULT_COMMODITY_NO);
		strcpy(stContract.ContractNo1, it.get_symbol());
		stContract.CallOrPutFlag1 = TAPI_CALLPUT_FLAG_NONE;
		stContract.CallOrPutFlag2 = TAPI_CALLPUT_FLAG_NONE;
		auto iErr = _md_api->UnSubscribeQuote(&_reqid, &stContract);
		if (TAPIERROR_SUCCEED != iErr) {
			LTLOG_ERROR("UnSubscribeQuote Error:", iErr);
		}
	}
}


