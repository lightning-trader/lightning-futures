
#include "ctp_market.h"
#include <filesystem>
#include <time_utils.hpp>
#include <params.hpp>
#include <log_wapper.hpp>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment (lib,"../api/CTP_V6.6.9_20220920/win64/thostmduserapi_se.lib")
#else
#pragma comment (lib,"../api/CTP_V6.6.9_20220920/win32/thostmduserapi_se.lib")
#endif
#endif
ctp_market::ctp_market()
	:_md_api(nullptr)
	,_reqid(0)
	,_process_mutex(_mutex)
	, _is_inited(false)
{
}


ctp_market::~ctp_market()
{
	if (_md_api)
	{
		_md_api->RegisterSpi(nullptr);
		//_md_api->Join();
		_md_api->Release();
		_md_api = nullptr;
	}
}

bool ctp_market::init(const params& config)
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
		LOG_ERROR("ctp_trader init error ");
		return false;
	}

	char path_buff[64] = {0};
	sprintf(path_buff, "md_flow/%s/%s/", _broker_id.c_str(), _userid.c_str());
	if (!std::filesystem::exists(path_buff))
	{
		std::filesystem::create_directories(path_buff);
	}	
	_md_api = CThostFtdcMdApi::CreateFtdcMdApi(path_buff);
	_md_api->RegisterSpi(this);
	_md_api->RegisterFront((char*)_front_addr.c_str());
	_md_api->Init();
	_process_signal.wait(_process_mutex);
	_is_inited = true ;
	//_md_api->Join();
	return true;
}


void ctp_market::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	if(pRspInfo)
	{
		LOG_ERROR("Error:%d->%s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_market::OnFrontConnected()
{
	LOG_INFO("Connected : %s", _front_addr.c_str());
	do_userlogin();
}

void ctp_market::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	if(pRspInfo)
	{
		LOG_DEBUG("UserLogin : %d -> %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
	if(bIsLast)
	{
		LOG_INFO("UserLogin : Market data server logined, {%s} {%s}", pRspUserLogin->TradingDay, pRspUserLogin->UserID);
		//订阅行情数据
		do_subscribe();
		if(!_is_inited)
		{
			_process_signal.notify_all();
		}
	}
}

void ctp_market::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo)
	{
		LOG_DEBUG("UserLogout : %d -> %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_market::OnFrontDisconnected( int nReason )
{
	LOG_INFO("FrontDisconnected : Reason -> %d", nReason);
}


void ctp_market::OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pDepthMarketData )
{	
	if (pDepthMarketData == nullptr)
	{
		return;
	}
	
	LOG_PROFILE(pDepthMarketData->InstrumentID);
	LOG_DEBUG("MarketData =", pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice);
	code_t code ;
	auto excg_it = _instrument_id_list.find(pDepthMarketData->InstrumentID);
	if (excg_it != _instrument_id_list.end())
	{
		code = code_t(pDepthMarketData->InstrumentID, excg_it->second.c_str());
	}
	else
	{
		code = code_t(pDepthMarketData->InstrumentID, pDepthMarketData->ExchangeID);
	}

	tick_info tick(
		code,
		get_day_begin(get_now()) + make_time(pDepthMarketData->UpdateTime),
		pDepthMarketData->UpdateMillisec,
		pDepthMarketData->OpenPrice,
		pDepthMarketData->ClosePrice,
		pDepthMarketData->HighestPrice,
		pDepthMarketData->LowestPrice,
		pDepthMarketData->LastPrice,
		pDepthMarketData->PreSettlementPrice,
		pDepthMarketData->Volume,
		std::atoi(pDepthMarketData->TradingDay),
		pDepthMarketData->OpenInterest,
		{
			std::make_pair(pDepthMarketData->BidPrice1, pDepthMarketData->BidVolume1),
			std::make_pair(pDepthMarketData->BidPrice2, pDepthMarketData->BidVolume2),
			std::make_pair(pDepthMarketData->BidPrice3, pDepthMarketData->BidVolume3),
			std::make_pair(pDepthMarketData->BidPrice4, pDepthMarketData->BidVolume4),
			std::make_pair(pDepthMarketData->BidPrice5, pDepthMarketData->BidVolume5)
		},
		{
			std::make_pair(pDepthMarketData->AskPrice1, pDepthMarketData->AskVolume1),
			std::make_pair(pDepthMarketData->AskPrice2, pDepthMarketData->AskVolume2),
			std::make_pair(pDepthMarketData->AskPrice3, pDepthMarketData->AskVolume3),
			std::make_pair(pDepthMarketData->AskPrice4, pDepthMarketData->AskVolume4),
			std::make_pair(pDepthMarketData->AskPrice5, pDepthMarketData->AskVolume5)
		}
	);
	

	//业务日期返回的是空，所以这里自己获取本地日期加上更新时间来计算业务日期时间

	LOG_PROFILE(pDepthMarketData->InstrumentID);
	this->fire_event(market_event_type::MET_TickReceived, tick);
	
}

void ctp_market::OnRspSubMarketData( CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	if(pRspInfo)
	{
		LOG_INFO("SubMarketData : code -> %d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_market::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo)
	{
		LOG_INFO("SubMarketData : code -> %d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_market::do_userlogin()
{
	if(_md_api == nullptr)
	{
		return;
	}

	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.UserID, _userid.c_str());
	strcpy(req.Password, _password.c_str());
	int iResult = _md_api->ReqUserLogin(&req, ++_reqid);
	if(iResult != 0)
	{
		LOG_ERROR("do_userlogin : % d",iResult);
	}
}

void ctp_market::do_subscribe()
{
	char* id_list[500];
	int num = 0;
	for (auto& it : _instrument_id_list)
	{
		id_list[num] = const_cast<char*>(it.first.c_str());
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

void ctp_market::do_unsubscribe(const std::vector<code_t>& code_list)
{
	char* id_list[500];
	int num = 0;
	for (size_t i = 0; i < code_list.size(); i++)
	{
		id_list[num] = const_cast<char*>(code_list[i].get_id());
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

void ctp_market::subscribe(const std::set<code_t>& code_list)
{
	for(auto& it : code_list)
	{
		_instrument_id_list[it.get_id()] = it.get_excg();
	}
	do_subscribe();
}

void ctp_market::unsubscribe(const std::set<code_t>& code_list)
{
	std::vector<code_t> delete_code_list ;
	for (auto& it : code_list)
	{
		auto n = _instrument_id_list.find(it.get_id());
		if(n != _instrument_id_list.end())
		{
			delete_code_list.emplace_back(it);
			_instrument_id_list.erase(n);
		}
	}
	do_unsubscribe(delete_code_list);
}


