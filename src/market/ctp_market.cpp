
#include "ctp_market.h"
#include <file_wapper.hpp>
#include <time_utils.hpp>

#pragma comment (lib,"thostmduserapi_se.lib")

ctp_market::ctp_market()
	:_md_api(nullptr)
	,_reqid(0)
	,_process_mutex(_mutex)
	,_last_tick_time(0)
	, _current_trading_day(0)
	, _is_inited(false)
{
}


ctp_market::~ctp_market()
{
	if (_md_api)
	{
		_md_api->RegisterSpi(nullptr);
		_md_api->Release();
		//_md_api->Join();
		_md_api = nullptr;
	}
}

bool ctp_market::init(const boost::property_tree::ptree& config)
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
	sprintf_s(path_buff, 64, "md_flow/%s/%s/", _broker_id.c_str(), _userid.c_str());
	if (!file_wapper::exists(path_buff))
	{
		file_wapper::create_directories(path_buff);
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
	LOG_ERROR("Connected : %s", _front_addr.c_str());
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
	LOG_DEBUG("MarketData = [%s] [%f]\n", pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice);
	
	tick_info tick ;
	auto& excg_it = _instrument_id_list.find(pDepthMarketData->InstrumentID);
	if(excg_it != _instrument_id_list.end())
	{
		tick.id = code_t(pDepthMarketData->InstrumentID, excg_it->second.c_str());
	}
	else
	{
		tick.id = code_t(pDepthMarketData->InstrumentID, pDepthMarketData->ExchangeID);
	}
	//业务日期返回的是空，所以这里自己获取本地日期加上更新时间来计算业务日期时间
	tick.time = get_day_begin(get_now()) + make_time(pDepthMarketData->UpdateTime);
	tick.tick = pDepthMarketData->UpdateMillisec;
	tick.price = pDepthMarketData->LastPrice;
	tick.standard = pDepthMarketData->PreSettlementPrice ;
	tick.volume = pDepthMarketData->Volume;
	tick.open = pDepthMarketData->OpenPrice;
	tick.close = pDepthMarketData->ClosePrice;
	tick.high = pDepthMarketData->HighestPrice;
	tick.low = pDepthMarketData->LowestPrice;
	tick.high_limit = pDepthMarketData->UpperLimitPrice;
	tick.low_limit = pDepthMarketData->LowerLimitPrice;

	tick.buy_order[0] = std::make_pair(pDepthMarketData->BidPrice1, pDepthMarketData->BidVolume1);
	tick.buy_order[1] = std::make_pair(pDepthMarketData->BidPrice2, pDepthMarketData->BidVolume2);
	tick.buy_order[2] = std::make_pair(pDepthMarketData->BidPrice3, pDepthMarketData->BidVolume3);
	tick.buy_order[3] = std::make_pair(pDepthMarketData->BidPrice4, pDepthMarketData->BidVolume4);
	tick.buy_order[4] = std::make_pair(pDepthMarketData->BidPrice5, pDepthMarketData->BidVolume5);

	tick.sell_order[0] = std::make_pair(pDepthMarketData->AskPrice1, pDepthMarketData->AskVolume1);
	tick.sell_order[1] = std::make_pair(pDepthMarketData->AskPrice2, pDepthMarketData->AskVolume2);
	tick.sell_order[2] = std::make_pair(pDepthMarketData->AskPrice3, pDepthMarketData->AskVolume3);
	tick.sell_order[3] = std::make_pair(pDepthMarketData->AskPrice4, pDepthMarketData->AskVolume4);
	tick.sell_order[4] = std::make_pair(pDepthMarketData->AskPrice5, pDepthMarketData->AskVolume5);
	tick.trading_day = std::atoi(pDepthMarketData->TradingDay);
	if (_current_trading_day != tick.trading_day)
	{
		_current_trading_day = tick.trading_day;
		this->fire_event(ET_CrossDay, _current_trading_day);
	}
	this->fire_event(ET_TickReceived, tick);

	_last_tick_time = tick.time;

	if(pDepthMarketData->SettlementPrice == tick.close && pDepthMarketData->SettlementPrice == tick.price && pDepthMarketData->SettlementPrice > 0)
	{
		this->fire_event(ET_EndTrading);
	}
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
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.UserID, _userid.c_str());
	strcpy_s(req.Password, _password.c_str());
	int iResult = _md_api->ReqUserLogin(&req, ++_reqid);
	if(iResult != 0)
	{
		LOG_ERROR("do_userlogin : % d",iResult);
	}
}

void ctp_market::do_subscribe()
{
	char* id_list[500];
	int i = 0, num = 0;
	for (auto& it : _instrument_id_list)
	{
		id_list[i] = const_cast<char*>(it.first.c_str());
		num++;
		if (num == 500)
		{
			_md_api->SubscribeMarketData(id_list, num);//订阅行情
			num = 0;
		}
		i++;
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
		id_list[i] = const_cast<char*>(code_list[i].get_id());
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


