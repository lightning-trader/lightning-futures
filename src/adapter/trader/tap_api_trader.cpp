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
#include "tap_api_trader.h"
#include <filesystem>
#include <time_utils.hpp>
#include "../../api/TAP_V9_20200808/TapAPIError.h"

using namespace lt;
using namespace lt::driver;

tap_api_trader::tap_api_trader(std::unordered_map<std::string, std::string>& id_excg_map, const params& config)
	: asyn_actual_trader(id_excg_map)
	, _td_api(nullptr)
	, _reqid(0)
	, _order_ref(0)
	, _login_time(0)
	, _is_runing(false)
	, _process_mutex(_mutex)
	, _is_inited(false)
	, _is_connected(false)
	, _is_sync_wait(false)
	, _is_in_query(false)
	, _trader_handle(NULL)
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
		_appid = config.get<std::string>("appid");
	}
	catch (...)
	{
		LTLOG_ERROR("tap trader init error ");
	}
	_trader_handle = library_helper::load_library("TapTradeAPI");
	if (_trader_handle)
	{
		_tap_creator = (trader_creator_function)library_helper::get_symbol(_trader_handle, "CreateTapTradeAPI");
		_tap_destroyer = (trader_destroyer_function)library_helper::get_symbol(_trader_handle, "FreeTapTradeAPI");
	}
	else
	{
		LTLOG_ERROR("tap trader TapQuoteAPI load error ");
	}

	LTLOG_INFO("tap market init");
	
}


tap_api_trader::~tap_api_trader()
{
	library_helper::free_library(_trader_handle);
	_trader_handle = nullptr;
}

bool tap_api_trader::login()
{
	_is_runing = true;
	//创建API实例
	TAPIINT32 iResult = TAPIERROR_SUCCEED;
	TapAPIApplicationInfo stAppInfo;
	strcpy(stAppInfo.AuthCode, _authcode.c_str());
	const char* log_path = "./log/api";
	if (!std::filesystem::exists(log_path))
	{
		std::filesystem::create_directories(log_path);
	}
	strcpy(stAppInfo.KeyOperationLogPath, log_path);
	_td_api = _tap_creator(&stAppInfo, iResult);
	if (NULL == _td_api) {
		LTLOG_FATAL("创建API实例失败，错误码：" , iResult);
		return false;
	}

	//设定ITapTradeAPINotify的实现类，用于异步消息的接收
	
	_td_api->SetAPINotify(this);

	TAPIINT32 iErr = TAPIERROR_SUCCEED;


	//设定服务器IP、端口
	iErr = _td_api->SetHostAddress(_ip.c_str(), _port);
	if (TAPIERROR_SUCCEED != iErr) {
		LTLOG_ERROR("SetHostAddress Error:", iErr);
		return false;
	}

	//登录服务器
	TapAPITradeLoginAuth stLoginAuth;
	memset(&stLoginAuth, 0, sizeof(stLoginAuth));
	strcpy(stLoginAuth.UserNo, _userid.c_str());
	strcpy(stLoginAuth.Password, _password.c_str());
	strcpy(stLoginAuth.AuthCode, _authcode.c_str());
	strcpy(stLoginAuth.AppID, _appid.c_str());
	stLoginAuth.ISModifyPassword = APIYNFLAG_NO;
	stLoginAuth.ISDDA = APIYNFLAG_NO;
	stLoginAuth.NoticeIgnoreFlag = TAPI_NOTICE_IGNORE_FUND | TAPI_NOTICE_IGNORE_FILL | TAPI_NOTICE_IGNORE_POSITION | TAPI_NOTICE_IGNORE_CLOSE | TAPI_NOTICE_IGNORE_POSITIONPROFIT;
	iErr = _td_api->Login(&stLoginAuth);
	if (TAPIERROR_SUCCEED != iErr)
	{
		LTLOG_ERROR( "Login Error:" ,iErr);
		return false;
	}

	LTLOG_INFO("ctp_api_trader init ");
	_process_signal.wait(_process_mutex);

	_is_inited = true;
	return true ;
}

void tap_api_trader::logout()
{
	
	_is_runing = false;
	
	_reqid = 0;
	_order_ref = 0;
	_login_time = 0;	//登录本地时间戳

	_position_info.clear();
	_order_info.clear();

	_is_inited = false;
	_is_connected = false;
	_is_sync_wait.exchange(false);

	if (_td_api)
	{
		_tap_destroyer(_td_api);
		_td_api = nullptr;
	}
}



bool tap_api_trader::query_positions(bool is_sync)
{
	if (_td_api == nullptr)
	{
		LTLOG_ERROR("tap trader api nullptr");
		return false;
	}
	if(_is_in_query)
	{
		LTLOG_ERROR("tap trader query_positions _is_in_query true");
		return false;
	}
	LTLOG_INFO("tap trader query_positions :", is_sync);
	TapAPIPositionQryReq qryReq;
	auto err = _td_api->QryPosition(&_reqid, &qryReq);
	if(err != TAPIERROR_SUCCEED)
	{
		LTLOG_ERROR("tap trader QryPosition Error:", err);
		return false;
	}
	while (_is_in_query.exchange(true));
	if (is_sync)
	{
		while (!_is_sync_wait.exchange(true));
		_process_signal.wait(_process_mutex);
	}
	return true;
}

bool tap_api_trader::query_orders(bool is_sync)
{
	if (_td_api == nullptr)
	{
		LTLOG_ERROR("tap trader api nullptr");
		return false;
	}
	if (_is_in_query)
	{
		LTLOG_ERROR("tap trader query_orders _is_in_query true");
		return false;
	}
	LTLOG_INFO("tap trader query_orders :", is_sync);
	TapAPIOrderQryReq qryReq;
	qryReq.OrderQryType = TAPI_ORDER_QRY_TYPE_UNENDED;
	auto err = _td_api->QryOrder(&_reqid,&qryReq);
	if (err != TAPIERROR_SUCCEED)
	{
		LTLOG_ERROR("tap trader QryOrder Error:", err);
		return false;
	}
	while (_is_in_query.exchange(true));
	if (is_sync)
	{
		while (!_is_sync_wait.exchange(true));
		_process_signal.wait(_process_mutex);
	}
	return true;
}

bool tap_api_trader::query_instruments(bool is_sync)
{
	return false ;
}

void tap_api_trader::OnConnect()noexcept
{
	_is_connected = true ;
}
void tap_api_trader::OnRspLogin(TAPIINT32 errorCode, const TapAPITradeLoginRspInfo* loginRspInfo)noexcept
{
	if (TAPIERROR_SUCCEED == errorCode) {
		LTLOG_INFO("登录成功，等待API初始化...");
		if (loginRspInfo)
		{
			_trading_day = date_to_uint(loginRspInfo->TradeDate);
			auto now = std::chrono::system_clock::now().time_since_epoch();
			_login_time = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now).count());

		}
	}
	else {
		LTLOG_ERROR("登录失败，错误码:", errorCode);
		//_process_signal.notify_all();
	}
}
void tap_api_trader::OnAPIReady()noexcept
{
	LTLOG_INFO("OnAPIReady :", _ip.c_str(), _port);
	_process_signal.notify_all();
}
void tap_api_trader::OnDisconnect(TAPIINT32 reasonCode)noexcept
{
	_is_connected = false;
}

void tap_api_trader::OnRtnOrder(const TapAPIOrderInfoNotice* notice)noexcept
{
	if (notice && notice->ErrorCode != TAPIERROR_SUCCEED)
	{
		LTLOG_ERROR("OnRtnOrder Error : ", notice->ErrorCode);
		return ;
	}
	
	if(notice && notice->OrderInfo)
	{
		auto info = notice->OrderInfo;
		estid_t estid = strtoll(info->RefString, NULL, 10);
		LTLOG_INFO("OnRtnOrder : ", info->OrderState, estid, info->CommodityNo, info->ContractNo, info->ExchangeNo, info->OrderSide, info->PositionEffect);
		if(info->ErrorCode != TAPIERROR_SUCCEED)
		{
			LTLOG_ERROR("OnRtnOrder info Error : ", info->ErrorCode);
			this->fire_event(trader_event_type::TET_OrderError, error_type::ET_PLACE_ORDER, estid, (uint8_t)error_code::EC_StateNotReady);
			return;
		}
		if(info->OrderState == TAPI_ORDER_STATE_FAIL)
		{
			this->fire_event(trader_event_type::TET_OrderError, error_type::ET_PLACE_ORDER, estid, (uint8_t)error_code::EC_StateNotReady);
			return;
		}
		
		code_t code = wrap_code(*info);
		auto direction = wrap_direction_offset(info->OrderSide, info->PositionEffect);
		auto offset = wrap_offset_type(code,info->PositionEffect);
		if (info->OrderState == TAPI_ORDER_STATE_FINISHED || info->OrderState == TAPI_ORDER_STATE_CANCELED || info->OrderState == TAPI_ORDER_STATE_LEFTDELETED)
		{
			auto it = _order_info.find(estid);
			if (it != _order_info.end())
			{
				auto order = it->second;
				uint32_t deal_volume = order.last_volume - (info->OrderQty - info->OrderMatchQty);
				if (deal_volume > 0)
				{
					order.last_volume = info->OrderQty - info->OrderMatchQty;
					//触发 deal 事件
					this->fire_event(trader_event_type::TET_OrderDeal, estid, deal_volume, order.last_volume);
				}
				if (info->OrderState == TAPI_ORDER_STATE_CANCELED || info->OrderState == TAPI_ORDER_STATE_LEFTDELETED)
				{
					LTLOG_INFO("OnRtnOrder fire_event ET_OrderCancel", estid, code.get_symbol(), direction, offset);
					this->fire_event(trader_event_type::TET_OrderCancel, estid, code, offset, direction, info->OrderPrice, order.last_volume, info->OrderQty);
				}
				if (info->OrderState == TAPI_ORDER_STATE_FINISHED)
				{
					LTLOG_INFO("OnRtnOrder fire_event ET_OrderTrade", estid, code.get_symbol(), direction, offset);
					this->fire_event(trader_event_type::TET_OrderTrade, estid, code, offset, direction, info->OrderPrice, info->OrderQty);
				}
				_order_info.erase(it);
			}
			auto idx = _order_index.find(estid);
			if (idx != _order_index.end())
			{
				_order_index.erase(idx);
			}
		}
		else
		{
			auto it = _order_index.find(estid);
			if (it == _order_index.end())
			{
				order_index index;
				index.ServerFlag = info->ServerFlag;
				strcpy(index.OrderNo, info->OrderNo);
				_order_index[estid] = index;
			}
			auto ordit = _order_info.find(estid);
			if(ordit == _order_info.end())
			{
				order_info order;
				order.code = code;
				order.create_time = make_daytm(info->OrderInsertTime + 6);
				order.estid = estid;
				order.direction = direction;
				order.offset = offset;
				order.last_volume = info->OrderQty - info->OrderMatchQty;
				order.total_volume = info->OrderQty;
				order.price = info->OrderPrice;
				_order_info[estid] = order;
				this->fire_event(trader_event_type::TET_OrderPlace, order);
				if(info->OrderMatchQty > 0)
				{
					//触发 deal 事件
					this->fire_event(trader_event_type::TET_OrderDeal, estid, info->OrderMatchQty, order.last_volume);
				}
				
			}
			else
			{
				uint32_t deal_volume = ordit->second.last_volume - (info->OrderQty - info->OrderMatchQty);
				if (deal_volume > 0)
				{
					
					ordit->second.last_volume = info->OrderQty - info->OrderMatchQty;
					//触发 deal 事件
					this->fire_event(trader_event_type::TET_OrderDeal, estid, deal_volume, ordit->second.last_volume);
				}
				else
				{
					LTLOG_WARNING("return order ", ordit->second.last_volume, info->OrderMatchQty, info->OrderQty);
				}
				
			}
		}
	}
}

void tap_api_trader::OnRspOrderAction(TAPIUINT32 sessionID, TAPIUINT32 errorCode, const TapAPIOrderActionRsp* info)noexcept
{
	if (errorCode != TAPIERROR_SUCCEED)
	{
		LTLOG_ERROR("OnRspOrderAction : ", errorCode);
	}
	if(info)
	{
		if (info->OrderInfo)
		{
			LTLOG_INFO("OnRspOrderAction : ", info->ActionType, info->OrderInfo->CommodityNo, info->OrderInfo->ContractNo, info->OrderInfo->ExchangeNo, info->OrderInfo->OrderSide, info->OrderInfo->PositionEffect);
		}
		
	}
}

void tap_api_trader::OnRspQryOrder(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo* info)noexcept
{
	if(errorCode != TAPIERROR_SUCCEED)
	{
		LTLOG_ERROR("OnRspQryOrder : ", errorCode);
	}
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_order_info.clear();
	}
	if(info && info->OrderState != TAPI_ORDER_STATE_FAIL && info->OrderState != TAPI_ORDER_STATE_CANCELED && info->OrderState != TAPI_ORDER_STATE_FINISHED)
	{
		estid_t estid = strtoll(info->RefString, NULL, 10);
		auto it = _order_index.find(estid);
		if(it == _order_index.end())
		{
			order_index index ;
			index.ServerFlag = info->ServerFlag;
			strcpy(index.OrderNo, info->OrderNo);
			_order_index[estid] = index;

			order_info order;
			order.code = wrap_code(*info);
			order.create_time = make_daytm(info->OrderInsertTime+6);
			order.estid = estid;
			order.direction = wrap_direction_offset(info->OrderSide, info->PositionEffect);
			order.offset = wrap_offset_type(order.code,info->PositionEffect);
			order.last_volume = info->OrderQty - info->OrderMatchQty;
			order.total_volume = info->OrderQty;
			order.price = info->OrderPrice;
			_order_info[estid] = order;
		}
		
	}
	
	if(isLast == APIYNFLAG_YES && _is_sync_wait)
	{
		_process_signal.notify_all();
	}
}

void tap_api_trader::OnRspQryPosition(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIPositionInfo* info)noexcept
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_position_info.clear();
	}
	if(info)
	{
		code_t code = wrap_code(*info);
		position_seed& pos = _position_info[code];
		pos.id = code;
		if (info->MatchSide == TAPI_SIDE_BUY)
		{
			pos.total_long += info->PositionQty;
		}
		else if (info->MatchSide == TAPI_SIDE_SELL)
		{
			pos.total_short += info->PositionQty;
		}
		else if (info->MatchSide == TAPI_SIDE_ALL)
		{
			pos.total_long += info->PositionQty;
			pos.total_short += info->PositionQty;
		}
		if(info->IsHistory == APIYNFLAG_YES&&code.is_distinct())
		{
			if(info->MatchSide == TAPI_SIDE_BUY)
			{
				pos.history_long += info->PositionQty ;
			}
			else if (info->MatchSide == TAPI_SIDE_SELL)
			{
				pos.history_short += info->PositionQty;
			}
			else if (info->MatchSide == TAPI_SIDE_ALL)
			{
				pos.history_long += info->PositionQty;
				pos.history_short += info->PositionQty;
			}
		}
	}
	if (isLast == APIYNFLAG_YES && _is_sync_wait)
	{
		_process_signal.notify_all();
	}
}

void tap_api_trader::OnRtnExchangeStateInfo(const TapAPIExchangeStateInfoNotice* info) noexcept 
{

};

bool tap_api_trader::is_usable()const
{
	if (_td_api == nullptr)
	{
		return false;
	}
	if (!_is_inited)
	{
		return false;
	}
	return _is_connected;
}


estid_t tap_api_trader::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t volume, double_t price, order_flag flag)
{
	if (_td_api == nullptr)
	{
		LTLOG_ERROR("tap trader api nullptr");
		return INVALID_ESTID;
	}
	PROFILE_DEBUG(code.get_symbol());
	LTLOG_INFO("ctp_api_trader place_order %s %d", code.get_symbol(), volume);

	TapAPINewOrder stNewOrder;
	memset(&stNewOrder, 0, sizeof(stNewOrder));
	auto extid = generate_estid();
	strcpy(stNewOrder.AccountNo, _userid.c_str());
	stNewOrder.CommodityType = TAPI_COMMODITY_TYPE_FUTURES;
	convert_code(stNewOrder, code);
	
	if(price > .0)
	{
		stNewOrder.OrderType = TAPI_ORDER_TYPE_LIMIT;
	}
	else
	{
		stNewOrder.OrderType = TAPI_ORDER_TYPE_MARKET;
	}
	
	stNewOrder.OrderSource = TAPI_ORDER_SOURCE_ESUNNY_API;
	if(flag == order_flag::OF_FAK)
	{
		stNewOrder.TimeInForce = TAPI_ORDER_TIMEINFORCE_FAK;
	}
	else if(flag == order_flag::OF_FOK)
	{
		stNewOrder.TimeInForce = TAPI_ORDER_TIMEINFORCE_FOK;
	}
	else
	{
		stNewOrder.TimeInForce = TAPI_ORDER_TIMEINFORCE_GFD;
	}
	strcpy(stNewOrder.ExpireTime, "");
	stNewOrder.IsRiskOrder = APIYNFLAG_NO;
	stNewOrder.OrderSide = convert_direction_offset(direction, offset);
	stNewOrder.PositionEffect = convert_offset_type(code, volume,offset, direction);
	stNewOrder.PositionEffect2 = TAPI_PositionEffect_NONE;
	strcpy(stNewOrder.InquiryNo, "");
	stNewOrder.HedgeFlag = TAPI_HEDGEFLAG_T;
	stNewOrder.OrderPrice = price;
	stNewOrder.OrderQty = volume;
	snprintf(stNewOrder.RefString, 51, "%llu", extid);
	stNewOrder.TacticsType = TAPI_TACTICS_TYPE_NONE;
	stNewOrder.TriggerCondition = TAPI_TRIGGER_CONDITION_NONE;
	stNewOrder.TriggerPriceType = TAPI_TRIGGER_PRICE_NONE;
	stNewOrder.AddOneIsValid = APIYNFLAG_NO;
	stNewOrder.OrderQty2;
	stNewOrder.HedgeFlag2 = TAPI_HEDGEFLAG_NONE;
	stNewOrder.MarketLevel = TAPI_MARKET_LEVEL_0;
	stNewOrder.FutureAutoCloseFlag = APIYNFLAG_NO; // V9.0.2.0 20150520

	auto iErr = _td_api->InsertOrder(&_reqid, &stNewOrder);
	if (TAPIERROR_SUCCEED != iErr) {
		LTLOG_INFO("InsertOrder Error:" , iErr);
		return INVALID_ESTID;
	}
	
	LTLOG_INFO("ctp_api_trader place_order end", code.get_symbol(), extid);
	PROFILE_INFO(code.get_symbol());
	return extid;
}

bool tap_api_trader::cancel_order(estid_t estid)
{
	if (_td_api == nullptr)
	{
		LTLOG_ERROR("cancel_order _td_api nullptr : %llu", estid);
		return false;
	}
	auto it = _order_index.find(estid);
	if (it == _order_index.end())
	{
		LTLOG_ERROR("cancel_order order invalid : %llu", estid);
		return false;
	}
	TapAPIOrderCancelReq cancel ;
	auto& order = it->second;
	strcpy(cancel.OrderNo, order.OrderNo);
	cancel.ServerFlag = order.ServerFlag;
	snprintf(cancel.RefString, 51, "%llu", estid);
	
	LTLOG_INFO("CancelOrder :", cancel.OrderNo, cancel.ServerFlag, cancel.RefString);
	auto iResult = _td_api->CancelOrder(&_reqid, &cancel);
	if (iResult != 0)
	{
		LTLOG_ERROR("cancel_order request failed:", iResult);
		return false;
	}
	return true;
}

uint32_t tap_api_trader::get_trading_day()const
{
	if (_td_api)
	{
		return _trading_day;
	}
	return 0X0U;
}

std::vector<order_info> tap_api_trader::get_all_orders()
{
	std::vector<order_info> result;
	if (!query_orders(true))
	{
		LTLOG_ERROR("query_orders error");
		return result;
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	for (auto it : _order_info)
	{
		result.emplace_back(it.second);
	}
	return result;
}

std::vector<position_seed> tap_api_trader::get_all_positions()
{
	std::vector<position_seed> result;
	if (!query_positions(true))
	{
		LTLOG_ERROR("query_positions error");
		return result;
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));

	for (auto it : _position_info)
	{
		result.emplace_back(it.second);
	}
	return result;
}

std::vector<instrument_info> tap_api_trader::get_all_instruments()
{
	std::vector<instrument_info> result;
	/*
	if (!query_instruments(true))
	{
		LTLOG_ERROR("query_positions error");
		return result;
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));

	for (auto it : _instrument_info)
	{
		result.emplace_back(it.second);
	}
*/
	return result;
}

daytm_t tap_api_trader::get_exchange_time(const char* exchange) const
{
	//找不到返回本地时间
	return make_daytm(time(0), 0U);
}

std::pair<bool, daytm_t> tap_api_trader::get_product_state(const code_t& product_code) const
{
	return std::make_pair(false,0U);
}
