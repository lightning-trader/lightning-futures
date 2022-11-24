#include "ctp_trader.h"
#include <file_wapper.hpp>
#include <time_utils.hpp>

#pragma comment (lib,"thosttraderapi_se.lib")

ctp_trader::ctp_trader()
	: _td_api(nullptr)
	, _reqid(0)
	, _last_query_time(0)
	, _front_id(0)
	, _session_id(0)
	, _order_ref(0)
	, _is_runing(false)
	, _process_mutex(_mutex)
	, _work_thread(nullptr)
	, _is_inited(false)
	, _is_connected(false)
{
}


ctp_trader::~ctp_trader()
{
	_is_runing = false;
	if (_work_thread)
	{
		_work_thread->join();
		delete _work_thread;
		_work_thread = nullptr;
	}
	if (_td_api)
	{
		//m_pUserAPI->RegisterSpi(NULL);
		_td_api->Release();
		//_td_api->Join();
		_td_api = nullptr;
	}
	_position_info.clear();
	_order_info.clear();
	_trade_info.clear();
}

bool ctp_trader::init(const boost::property_tree::ptree& config)
{
	try
	{
		_front_addr = config.get<std::string>("front");
		_broker_id = config.get<std::string>("broker");
		_userid = config.get<std::string>("userid");
		_password = config.get<std::string>("passwd");
		_appid = config.get<std::string>("appid");
		_authcode = config.get<std::string>("authcode");
		_prodict_info = config.get<std::string>("prodict");
	}
	catch (...)
	{
		LOG_ERROR("ctp_trader init error ");
		return false;
	}
	
	char path_buff[64];
	sprintf_s(path_buff,64,"td_flow/%s/%s/", _broker_id.c_str(), _userid.c_str());
	if (!file_wapper::exists(path_buff))
	{
		file_wapper::create_directories(path_buff);
	}
	_td_api = CThostFtdcTraderApi::CreateFtdcTraderApi(path_buff);
	_td_api->RegisterSpi(this);
	//_td_api->SubscribePrivateTopic(THOST_TERT_RESTART);
	//_td_api->SubscribePublicTopic(THOST_TERT_RESTART);
	_td_api->SubscribePrivateTopic(THOST_TERT_RESUME);
	_td_api->SubscribePublicTopic(THOST_TERT_RESUME);
	_td_api->RegisterFront(const_cast<char*>(_front_addr.c_str()));
	_td_api->Init();
	LOG_INFO("ctp_trader init %s ", _td_api->GetApiVersion());
	_process_signal.wait(_process_mutex);
	_is_runing = true ;
	//启动查询线程去同步账户信息
	if (_work_thread == nullptr)
	{
		_work_thread = new std::thread([this]() {
			while (_is_runing)
			{
				if (!_query_queue.empty())
				{
					auto& handler = _query_queue.front();
					if (_is_in_query)
					{
						continue ;
					}
					while (!_is_in_query.exchange(true));
					handler();
					_query_mutex.lock();
					_query_queue.pop();
					_query_mutex.unlock();
					_last_query_time = get_now();
				}
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			});
		//_work_thread->join();
	}
	_is_inited = true ;
	return true;
}


bool ctp_trader::do_login()
{
	if (_td_api == nullptr)
	{
		return false;
	}
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.UserID, _userid.c_str());
	strcpy_s(req.Password, _password.c_str());
	strcpy_s(req.UserProductInfo, _prodict_info.c_str());
	int iResult = _td_api->ReqUserLogin(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader do_login request failed: %d", iResult);
		return false ;
	}
	return true;
}

bool ctp_trader::logout()
{
	if (_td_api == nullptr)
	{
		return false;
	}

	CThostFtdcUserLogoutField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.UserID, _userid.c_str());
	int iResult = _td_api->ReqUserLogout(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader logout request failed: %d", iResult);
		return false ;
	}

	return true;
}


void ctp_trader::query_account()
{
	if (_td_api == nullptr)
	{
		return ;
	}
	_query_mutex.lock();
	_query_queue.push([this]() {
		CThostFtdcQryTradingAccountField req;
		memset(&req, 0, sizeof(req));
		strcpy_s(req.BrokerID, _broker_id.c_str());
		strcpy_s(req.InvestorID, _userid.c_str());
		_td_api->ReqQryTradingAccount(&req, genreqid());
		});
	_query_mutex.unlock();
}

void ctp_trader::query_positions()
{
	if (_td_api == nullptr)
	{
		return;
	}

	_query_mutex.lock();

	_query_queue.push([this]() {
		
		CThostFtdcQryInvestorPositionField req;
		memset(&req, 0, sizeof(req));
		strcpy_s(req.BrokerID, _broker_id.c_str());
		strcpy_s(req.InvestorID, _userid.c_str());
		_td_api->ReqQryInvestorPosition(&req, genreqid());
	});
	_query_mutex.unlock();
}

void ctp_trader::query_orders()
{
	if (_td_api == nullptr)
	{
		return;
	}
	_query_mutex.lock();
	
	_query_queue.push([this]() {
		CThostFtdcQryOrderField req;
		memset(&req, 0, sizeof(req));
		strcpy_s(req.BrokerID, _broker_id.c_str());
		strcpy_s(req.InvestorID, _userid.c_str());
		_td_api->ReqQryOrder(&req, genreqid());
	});
	_query_mutex.unlock();
}

void ctp_trader::query_trades()
{
	if (_td_api == nullptr)
	{
		return;
	}

	_query_mutex.lock();

	_query_queue.push([this]() {
		CThostFtdcQryTradeField req;
		memset(&req, 0, sizeof(req));
		strcpy_s(req.BrokerID, _broker_id.c_str());
		strcpy_s(req.InvestorID, _userid.c_str());
		_td_api->ReqQryTrade(&req, genreqid());
	});
	_query_mutex.unlock();
	return ;
}

void ctp_trader::OnFrontConnected()
{
	LOG_INFO("ctp_trader OnFrontConnected ");
	do_auth();
	_is_connected = true ;
}

void ctp_trader::OnFrontDisconnected(int nReason)
{
	LOG_INFO("ctp_trader FrontDisconnected : Reason -> %d", nReason);
	_is_connected = false ;
}

void ctp_trader::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo)
	{
		LOG_ERROR("ctp_trader OnRspAuthenticate Error : %d", pRspInfo->ErrorID);
	}
	do_login();
}

void ctp_trader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!pRspInfo)
	{
		LOG_ERROR("ctp_trader OnRspAuthenticate Error : %d", pRspInfo->ErrorID);
	}

	if (pRspUserLogin)
	{

		LOG_INFO("[%s][用户登录] UserID:%s AppID:%s SessionID:%d FrontID:%d\n",
			datetime_to_string(pRspUserLogin->TradingDay, pRspUserLogin->LoginTime).c_str(),
			pRspUserLogin->UserID, _appid.c_str(), pRspUserLogin->SessionID, pRspUserLogin->FrontID);
		//LOG("OnRspUserLogin\tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			// 保存会话参数
		_front_id = pRspUserLogin->FrontID;
		_session_id = pRspUserLogin->SessionID;
		_order_ref = atoi(pRspUserLogin->MaxOrderRef);
	}

	if (bIsLast&&!_is_inited)
	{
		_process_signal.notify_all();
	}

}

void ctp_trader::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo)
	{
		LOG_DEBUG("UserLogout : %d -> %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
}

void ctp_trader::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo)
	{
		LOG_DEBUG("OnRspSettlementInfoConfirm\tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
	this->fire_event(ET_TradingReady);
}

void ctp_trader::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspOrderInsert \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
	
}

void ctp_trader::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspOrderAction \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	}
	
}

void ctp_trader::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while(!_is_in_query.exchange(false));
	}
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspQryTradingAccount \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		return;
	}
	
	if (bIsLast&& pTradingAccount)
	{
		_account_info.money = pTradingAccount->CurrMargin;
		_account_info.frozen_monery = pTradingAccount->FrozenMargin;
		this->fire_event(ET_AccountChange, _account_info);
	}
}

void ctp_trader::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_position_info.clear();
		_yestoday_position_info.clear();
	}
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspQryInvestorPosition \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		return;
	}
	
	if (pInvestorPosition)
	{	
		
		auto& position = _position_info[pInvestorPosition->InstrumentID];
		auto& yestoday_position = _yestoday_position_info[pInvestorPosition->InstrumentID];

		position.id = pInvestorPosition->InstrumentID;
		if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
		{
			position.long_postion = pInvestorPosition->Position;
			position.long_frozen = pInvestorPosition->LongFrozen;
			yestoday_position.long_postion = pInvestorPosition->YdPosition;
		}
		else if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short)
		{
			position.short_postion = pInvestorPosition->Position;
			position.short_frozen = pInvestorPosition->ShortFrozen;
			yestoday_position.short_postion = pInvestorPosition->YdPosition;
		}
	}

}


void ctp_trader::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_trade_info.clear();
	}
	if (pRspInfo&& pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspQryTrade \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		return;
	}
	if(pTrade)
	{
		//auto& trade = _trade_info[pTrade->TradeID];
	}
	
	if (bIsLast)
	{
		
	}
}

void ctp_trader::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_order_info.clear();
	}
	if (pOrder)
	{
		estid_t estid = generate_estid(pOrder->FrontID, pOrder->SessionID,strtoul(pOrder->OrderRef,NULL,10));
		auto& order = _order_info[estid];
		order.code = pOrder->InstrumentID ;
		order.create_time = make_datetime(pOrder->InsertDate, pOrder->InsertTime);
		order.est_id = estid;
		order.direction = wrap_position_direction(pOrder->Direction);
		order.offset = wrap_offset_type(pOrder->CombOffsetFlag[0]);
		order.last_volume = pOrder->VolumeTotal;
		order.total_volume = pOrder->VolumeTotal + pOrder->VolumeTraded;
		order.price = pOrder->LimitPrice;
	}

	if (bIsLast)
	{
		
	}
}

void ctp_trader::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	LOG_ERROR("OnRspError \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
}

void ctp_trader::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	if(pOrder == nullptr)
	{
		LOG_ERROR("OnRtnOrder null order ");
		return ;
	}
	auto estid = generate_estid(pOrder->FrontID, pOrder->SessionID, strtoul(pOrder->OrderRef, NULL, 10));
	auto code = code_t(pOrder->InstrumentID, pOrder->ExchangeID);
	auto direction = wrap_direction_offset(pOrder->Direction, pOrder->CombOffsetFlag[0]);
	auto offset = wrap_offset_type(pOrder->CombOffsetFlag[0]);
	
	if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled || pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
	{
		if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
		{
			LOG_INFO("[%s][订单撤销] UserID:%s SessionID:%d 合约:%s 开平标记:%c 方向:%c 价格:%f 数量:%d\n"
				, datetime_to_string(get_now()).c_str(), pOrder->UserID, pOrder->SessionID
				, pOrder->InstrumentID, pOrder->CombOffsetFlag[0], pOrder->CombHedgeFlag[0], pOrder->LimitPrice, pOrder->VolumeTotal);
			//撤销解冻仓位
			thawing_deduction(code, direction, pOrder->VolumeTotal + pOrder->VolumeTraded);
			this->fire_event(ET_OrderCancel, estid, code, offset, direction, pOrder->LimitPrice, (uint32_t)pOrder->VolumeTotal, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
		}
		if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
		{
			LOG_INFO("[%s][订单成交] UserID:%s SessionID:%d 合约:%s 开平标记:%c 方向:%c 价格:%f 数量:%d\n"
				, datetime_to_string(get_now()).c_str(), pOrder->UserID, pOrder->SessionID
				, pOrder->InstrumentID, pOrder->CombOffsetFlag[0], pOrder->Direction, pOrder->LimitPrice, pOrder->VolumeTraded);
			calculate_position(code, direction, offset, pOrder->VolumeTotal + pOrder->VolumeTraded);
			this->fire_event(ET_OrderTrade, estid, code, offset, direction, pOrder->LimitPrice, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
		}
		auto it = _order_info.find(estid);
		if (it != _order_info.end())
		{
			_order_info.erase(it);
		}
		
	}
	else
	{
		auto order = _order_info.find(estid);
		if (order == _order_info.end())
		{
			order_info entrust;
			entrust.code = code;
			entrust.create_time = make_datetime(pOrder->InsertDate, pOrder->InsertTime);
			entrust.est_id = estid;
			entrust.direction = direction;
			entrust.last_volume = pOrder->VolumeTotal;
			entrust.total_volume = pOrder->VolumeTotal + pOrder->VolumeTraded;
			entrust.offset = offset;
			entrust.price = pOrder->LimitPrice;
			order = _order_info.insert(std::make_pair(estid, entrust)).first;
			this->fire_event(ET_OrderPlace, order->second);
			if(offset == OT_OPEN)
			{
				//开仓冻结资金
				query_account();
			}
			else
			{
				//平仓冻结仓位
				frozen_deduction(code, direction, entrust.total_volume);
			}
		}
		else
		{
			auto& entrust = _order_info[estid];
			entrust.last_volume = pOrder->VolumeTotal;
		}

		if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing || pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing)
		{
			//触发 deal 事件
			this->fire_event(ET_OrderDeal, estid, pOrder->VolumeTotal, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
		}
	}
}

void ctp_trader::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	if(pTrade&& pTrade->OffsetFlag != THOST_FTDC_OF_Open)
	{
		//平仓计算盈亏
		query_account();
	}

	
}

void ctp_trader::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspQryTrade \tErrorID = [%d] ErrorMsg = [%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		return;
	}
	/*
	if(pInputOrder)
	{
		estid_t estid = generate_estid(_front_id, _session_id, strtol(pInputOrder->OrderRef,NULL,13));
		auto it = _order_info.find(estid);
		if(it != _order_info.end())
		{
			_order_info.erase(it);
		}
	}
	*/
}


bool ctp_trader::do_auth()
{
	if (_td_api == nullptr)
	{
		return false;
	}
	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.UserID, _userid.c_str());
	//strcpy(req.UserProductInfo, m_strProdInfo.c_str());
	strcpy_s(req.AuthCode, _authcode.c_str());
	strcpy_s(req.AppID, _appid.c_str());
	int iResult = _td_api->ReqAuthenticate(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader do_auth request failed: %d", iResult);
		return false;
	}
	return true ;
}

bool ctp_trader::is_usable()const
{
	if (_td_api == nullptr)
	{
		return false;
	}
	if(!_is_inited)
	{
		return false ;
	}
	return _is_connected;
}


estid_t ctp_trader::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t volume, double_t price, order_flag flag)
{

	if (_td_api == nullptr)
	{
		return INVALID_ESTID;
	}
	estid_t est_id = generate_estid();
	//LOG_INFO("place_order : %s", est_id);

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.InvestorID, _userid.c_str());

	strcpy_s(req.InstrumentID, code.get_id());
	strcpy_s(req.ExchangeID, code.get_excg());

	uint32_t order_ref = 0, season_id = 0, front_id = 0;
	extract_estid(est_id, front_id, season_id, order_ref);
	///报单引用
	sprintf_s(req.OrderRef, "%u", order_ref);

	if(price > 0)
	{
		///报单价格条件: 限价
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

	}
	else
	{
		req.OrderPriceType = THOST_FTDC_OPT_BestPrice;
	}
	///买卖方向: 
	req.Direction = wrap_direction_offset(direction, offset);
	///组合开平标志: 开仓
	req.CombOffsetFlag[0] = wrap_offset_type(code,volume,offset, direction);
	///组合投机套保标志
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	req.LimitPrice = price;
	///数量: 1
	req.VolumeTotalOriginal = volume;

	if (flag == OF_NOR)
	{
		req.TimeCondition = THOST_FTDC_TC_GFD;
		req.VolumeCondition = THOST_FTDC_VC_AV;
		req.MinVolume = 1;
	}
	else if (flag == OF_FAK)
	{
		req.TimeCondition = THOST_FTDC_TC_IOC;
		req.VolumeCondition = THOST_FTDC_VC_AV;
		req.MinVolume = 1;
	}
	else if (flag == OF_FOK)
	{
		req.TimeCondition = THOST_FTDC_TC_IOC;
		req.VolumeCondition = THOST_FTDC_VC_CV;
		req.MinVolume = volume;
	}


	///触发条件: 立即
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///强平原因: 非强平
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	req.IsAutoSuspend = 0;
	///用户强评标志: 否
	req.UserForceClose = 0;

	int iResult = _td_api->ReqOrderInsert(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader order_insert request failed: %d", iResult);
		return INVALID_ESTID;
	}
	return est_id;

}

void ctp_trader::cancel_order(estid_t order_id)
{
	if (_td_api == nullptr)
	{
		return ;
	}
	auto order = get_order(order_id);
	if(order.est_id == INVALID_ESTID)
	{
		return;
	}
	uint32_t frontid = 0, sessionid = 0, orderref = 0;
	extract_estid(order_id, frontid, sessionid, orderref);
	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.InvestorID, _userid.c_str());
	strcpy_s(req.UserID, _userid.c_str());
	///报单引用
	sprintf_s(req.OrderRef, "%d", orderref);
	///请求编号
	///前置编号
	req.FrontID = frontid;
	///会话编号
	req.SessionID = sessionid;
	///操作标志
	req.ActionFlag = wrap_action_flag(AF_CANCEL);
	///合约代码
	strcpy_s(req.InstrumentID, order.code.get_id());

	//req.LimitPrice = change.price;

	//req.VolumeChange = (int)change.volume;

	//strcpy_s(req.OrderSysID, change.order_id.c_str());
	strcpy_s(req.ExchangeID, order.code.get_excg());

	int iResult = _td_api->ReqOrderAction(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader order_action request failed: %d", iResult);
	}
}

const account_info& ctp_trader::get_account() const
{
	return (_account_info);
}

const position_info& ctp_trader::get_position(const code_t& code) const
{
	auto it = _position_info.find(code);
	if (it != _position_info.end())
	{
		return (it->second);
	}
	return default_position;
}

const order_info& ctp_trader::get_order(estid_t order_id) const
{
	auto it = _order_info.find(order_id);
	if(it != _order_info.end())
	{
		return (it->second);
	}
	return default_order;
}
void ctp_trader::find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const
{
	for(auto& it : _order_info)
	{
		if(func(it.second))
		{
			order_result.emplace_back(it.second);
		}
	}
}

void ctp_trader::submit_settlement()
{
	if (_td_api == nullptr)
	{
		return;
	}

	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, _broker_id.c_str());
	strcpy_s(req.InvestorID, _userid.c_str());

	//fmt::format_to(req.ConfirmDate, "{}", TimeUtils::getCurDate());
	//memcpy(req.ConfirmTime, TimeUtils::getLocalTime().c_str(), 8);

	int iResult = _td_api->ReqSettlementInfoConfirm(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader submit_settlement request failed: %d", iResult);
	}

}

void ctp_trader::calculate_position(const code_t& code,direction_type dir_type, offset_type offset_type,uint32_t volume)
{
	auto& p = _position_info[code];
	p.id = code;
	if (dir_type == DT_LONG)
	{
		if (offset_type == OT_OPEN)
		{
			p.long_postion += volume;
		}
		else
		{
			if (p.short_postion >= volume)
			{
				p.short_postion -= volume;
				p.short_frozen -= volume;
			}
			else
			{
				p.short_postion = 0;
			}
		}
	}
	else if (dir_type == DT_SHORT)
	{
		if (offset_type == OT_OPEN)
		{
			p.short_postion += volume;
		}
		else
		{
			if (p.long_postion >= volume)
			{
				p.long_postion -= volume;
				p.long_frozen -= volume;
			}
			else
			{
				p.long_postion = 0;
			}
		}
	}
	this->fire_event(ET_PositionChange, p);
}

void ctp_trader::frozen_deduction(const code_t& code, direction_type dir_type, uint32_t volume)
{
	auto& pos = _position_info[code];
	if (dir_type == DT_LONG)
	{
		pos.long_frozen += volume;
	}
	else if (dir_type == DT_SHORT)
	{
		pos.short_frozen += volume;
	}
	this->fire_event(ET_PositionChange, pos);
}
void ctp_trader::thawing_deduction(const code_t& code, direction_type dir_type, uint32_t volume)
{
	auto& pos = _position_info[code];
	if (dir_type == DT_LONG)
	{
		pos.long_frozen -= volume;
	}
	else if (dir_type == DT_SHORT)
	{
		pos.short_frozen -= volume;
	}
	this->fire_event(ET_PositionChange, pos);
}