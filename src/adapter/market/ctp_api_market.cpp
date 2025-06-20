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
#include "ctp_api_market.h"
#include <filesystem>
#include <time_utils.hpp>
#include <params.hpp>
#include <log_define.hpp>

using namespace lt;
using namespace lt::driver;


ctp_api_market::ctp_api_market(std::unordered_map<std::string, std::string>& id_excg_map, const params& config)
	:asyn_actual_market(id_excg_map)
	,_md_api(nullptr)
	,_reqid(0)
	,_is_runing(false)
	,_process_mutex(_mutex)
	, _is_inited(false)
	, _ctp_creator(NULL)
	, _market_handle(NULL)
{
	try
	{
		_front_addr = config.get<std::string>("front");
		_broker_id = config.get<std::string>("broker");
		_userid = config.get<std::string>("userid");
		_password = config.get<std::string>("passwd");
	}
	catch (...)
	{
		LTLOG_ERROR("market config error ");
	}
	_market_handle = library_helper::load_library("thostmduserapi_se");
	if (_market_handle)
	{
#ifdef _WIN32
#	ifdef _WIN64
		const char* creator_name = "?CreateFtdcMdApi@CThostFtdcMdApi@@SAPEAV1@PEBD_N1@Z";
#	else
		const char* creator_name = "?CreateFtdcMdApi@CThostFtdcMdApi@@SAPAV1@PBD_N1@Z";
#	endif
#else
		const char* creator_name = "_ZN15CThostFtdcMdApi15CreateFtdcMdApiEPKcbb";
#endif
		_ctp_creator = (market_creator_function)library_helper::get_symbol(_market_handle, creator_name);
	}
	else
	{
		LTLOG_ERROR("ctp_api_trader thosttraderapi_se load error ");
	}
}


ctp_api_market::~ctp_api_market()
{
	library_helper::free_library(_market_handle);
	_market_handle = nullptr;
}

bool ctp_api_market::login()
{
	if(_is_inited)
	{
		LTLOG_ERROR("ctp_api_market already login");
		return false;
	}
	_is_runing = true;
	char path_buff[64] = {0};
	sprintf(path_buff, "data/md_flow/%s/%s/", _broker_id.c_str(), _userid.c_str());
	if (!std::filesystem::exists(path_buff))
	{
		std::filesystem::create_directories(path_buff);
	}	
	_md_api = _ctp_creator(path_buff,false,false);
	_md_api->RegisterSpi(this);
	_md_api->RegisterFront((char*)_front_addr.c_str());
	_md_api->Init();
	_process_signal.wait(_process_mutex);
	
	return _is_inited;
}

void ctp_api_market::logout()
{
	_is_runing = false;
	do_logout();
	_reqid = 0;
	_id_excg_map.clear();
	_tick_time_map.clear();

	if (_md_api)
	{
		_md_api->RegisterSpi(nullptr);
		//_md_api->Join();
		_md_api->Release();
		_md_api = nullptr;
	}
	_is_inited.exchange(false);
}

void ctp_api_market::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )noexcept
{
	if(pRspInfo)
	{
		LTLOG_ERROR("Error:%d->%s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_api_market::OnFrontConnected()noexcept
{
	LTLOG_INFO("Connected : %s", _front_addr.c_str());
	if (_is_runing)
	{
		if(!do_login())
		{
			_process_signal.notify_all();
		}
	}
	
}

void ctp_api_market::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )noexcept
{
	if(pRspInfo)
	{
		LTLOG_DEBUG("UserLogin : %d -> %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
	if(bIsLast)
	{
		LTLOG_INFO("UserLogin : Market data server logined, {%s} {%s}", pRspUserLogin->TradingDay, pRspUserLogin->UserID);
		_is_inited.exchange(true);
		_process_signal.notify_all();

	}
}

void ctp_api_market::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)noexcept
{
	if (pRspInfo)
	{
		LTLOG_DEBUG("UserLogout : %d -> %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_api_market::OnFrontDisconnected( int nReason )noexcept
{
	LTLOG_INFO("FrontDisconnected : Reason -> %d", nReason);
}


void ctp_api_market::OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pDepthMarketData )noexcept
{	
	if (pDepthMarketData == nullptr)
	{
		return;
	}
	
	PROFILE_INFO(pDepthMarketData->InstrumentID);
	LTLOG_DEBUG("MarketData =", pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice, pDepthMarketData->UpdateTime, pDepthMarketData->UpdateMillisec);
	const char * excg_id = pDepthMarketData->ExchangeID;
	auto excg_it = _id_excg_map.find(pDepthMarketData->InstrumentID);
	if (excg_it != _id_excg_map.end())
	{
		excg_id = excg_it->second.c_str();
	}
	code_t code(pDepthMarketData->InstrumentID, excg_id);
	daytm_t time = make_daytm(pDepthMarketData->UpdateTime, static_cast<uint32_t>(pDepthMarketData->UpdateMillisec));
	auto tm_it = _tick_time_map.find(code);
	if(tm_it == _tick_time_map.end())
	{
		tm_it = _tick_time_map.insert(std::make_pair(code,time)).first;
	}
	else
	{
		tm_it->second = std::max<uint32_t>(time, tm_it->second+250);
	}
	PROFILE_DEBUG(pDepthMarketData->InstrumentID);
	tick_info tick_data(
		code,
		tm_it->second,
		price_adaptation(pDepthMarketData->LastPrice),
		pDepthMarketData->Volume,
		pDepthMarketData->OpenInterest,
		pDepthMarketData->AveragePrice,
		std::atoi(pDepthMarketData->TradingDay),
		{
			std::make_pair(price_adaptation(pDepthMarketData->BidPrice1), pDepthMarketData->BidVolume1),
			std::make_pair(price_adaptation(pDepthMarketData->BidPrice2), pDepthMarketData->BidVolume2),
			std::make_pair(price_adaptation(pDepthMarketData->BidPrice3), pDepthMarketData->BidVolume3),
			std::make_pair(price_adaptation(pDepthMarketData->BidPrice4), pDepthMarketData->BidVolume4),
			std::make_pair(price_adaptation(pDepthMarketData->BidPrice5), pDepthMarketData->BidVolume5)
		},
		{
			std::make_pair(price_adaptation(pDepthMarketData->AskPrice1), pDepthMarketData->AskVolume1),
			std::make_pair(price_adaptation(pDepthMarketData->AskPrice2), pDepthMarketData->AskVolume2),
			std::make_pair(price_adaptation(pDepthMarketData->AskPrice3), pDepthMarketData->AskVolume3),
			std::make_pair(price_adaptation(pDepthMarketData->AskPrice4), pDepthMarketData->AskVolume4),
			std::make_pair(price_adaptation(pDepthMarketData->AskPrice5), pDepthMarketData->AskVolume5)
		}
	);
	double_t standard_price = price_adaptation(pDepthMarketData->PreSettlementPrice);
	auto extend_data = std::make_tuple(
		price_adaptation(pDepthMarketData->OpenPrice, standard_price),
		price_adaptation(pDepthMarketData->ClosePrice, standard_price),
		price_adaptation(pDepthMarketData->HighestPrice, standard_price),
		price_adaptation(pDepthMarketData->LowestPrice, standard_price),
		price_adaptation(pDepthMarketData->UpperLimitPrice),
		price_adaptation(pDepthMarketData->LowerLimitPrice),
		standard_price
	);

	PROFILE_DEBUG(pDepthMarketData->InstrumentID);
	this->fire_event(market_event_type::MET_TickReceived, tick_data, extend_data);
	PROFILE_DEBUG(pDepthMarketData->InstrumentID);
}

void ctp_api_market::OnRspSubMarketData( CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )noexcept
{
	if(pRspInfo)
	{
		LTLOG_INFO("SubMarketData : code -> %d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_api_market::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)noexcept
{
	if (pRspInfo)
	{
		LTLOG_INFO("UnSubMarketData : code -> %d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

bool ctp_api_market::do_login()
{
	if(_md_api == nullptr)
	{
		LTLOG_ERROR("do_login : _md_api nullptr");
		return false;
	}

	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.UserID, _userid.c_str());
	strcpy(req.Password, _password.c_str());
	int iResult = _md_api->ReqUserLogin(&req, ++_reqid);
	if(iResult != 0)
	{
		LTLOG_ERROR("do_login : % d",iResult);
		return false;
	}
	return true;
}

bool ctp_api_market::do_logout()
{
	if (_md_api == nullptr)
	{
		return false;
	}

	CThostFtdcUserLogoutField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.UserID, _userid.c_str());
	int iResult = _md_api->ReqUserLogout(&req, ++_reqid);
	if (iResult != 0)
	{
		LTLOG_ERROR("ctp_api_market logout request failed:", iResult);
		return false;
	}

	return true;
}

void ctp_api_market::do_subscribe(const std::set<code_t>& codes)
{
	char* id_list[500];
	int num = 0;
	for (const auto& it : codes)
	{
		id_list[num] = const_cast<char*>(it.get_symbol());
		num++;
		if (num == 500)
		{
			_md_api->SubscribeMarketData(id_list, num);//订阅行情
			num = 0;
		}
	}
	if (num > 0)
	{
		_md_api->SubscribeMarketData(id_list, num);//订阅行情
		num = 0;
	}
}

void ctp_api_market::do_unsubscribe(const std::vector<code_t>& code_list)
{
	char* id_list[500];
	int num = 0;
	for (size_t i = 0; i < code_list.size(); i++)
	{
		id_list[num] = const_cast<char*>(code_list[i].get_symbol());
		num++;
		if (num == 500)
		{
			_md_api->UnSubscribeMarketData(id_list, num);//订阅行情
			num = 0;
		}
	}
	if (num > 0)
	{
		_md_api->UnSubscribeMarketData(id_list, num);//订阅行情
		num = 0;
	}
}

void ctp_api_market::subscribe(const std::set<code_t>& code_list)
{
	for(auto& it : code_list)
	{
		(_id_excg_map)[it.get_symbol()] = it.get_exchange();
	}
	do_subscribe(code_list);
}

void ctp_api_market::unsubscribe(const std::set<code_t>& code_list)
{
	std::vector<code_t> delete_code_list ;
	for (auto& it : code_list)
	{
		auto n = _id_excg_map.find(it.get_symbol());
		if(n != _id_excg_map.end())
		{
			delete_code_list.emplace_back(it);
			_id_excg_map.erase(n);
		}
	}
	do_unsubscribe(delete_code_list);
}


