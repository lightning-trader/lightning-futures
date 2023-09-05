#include "ctp_trader.h"
#include <filesystem>
#include <time_utils.hpp>

ctp_trader::ctp_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config)
	:actual_trader(id_excg_map)
	, _td_api(nullptr)
	, _reqid(0)
	, _front_id(0)
	, _session_id(0)
	, _order_ref(0)
	, _is_runing(false)
	, _process_mutex(_mutex)
	, _is_inited(false)
	, _is_connected(false)
	, _is_sync_wait(false)
	, _trader_handle(nullptr)
	, _is_in_query(false)
{
	try
	{
		_front_addr = config.get<std::string>("front");
		_broker_id = config.get<std::string>("broker");
		_userid = config.get<std::string>("userid");
		_password = config.get<std::string>("passwd");
		_appid = config.get<std::string>("appid");
		_authcode = config.get<std::string>("authcode");
		_product_info = config.get<std::string>("product");
	}
	catch (...)
	{
		LOG_ERROR("ctp_trader config error ");
	}
	_trader_handle = dll_helper::load_library("thosttraderapi_se");
	if(_trader_handle)
	{
#ifdef _WIN32
#	ifdef _WIN64
		const char* creator_name = "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPEAV1@PEBD@Z";
#	else
		const char* creator_name = "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPAV1@PBD@Z";
#	endif
#else
		const char* creator_name = "_ZN19CThostFtdcTraderApi19CreateFtdcTraderApiEPKc";
#endif
		_ctp_creator = (trader_creator)dll_helper::get_symbol(_trader_handle, creator_name);
	}
	else
	{
		LOG_ERROR("ctp_trader thosttraderapi_se load error ");
	}
}


ctp_trader::~ctp_trader()
{
	dll_helper::free_library(_trader_handle);
	_trader_handle = nullptr ;
}

void ctp_trader::login()
{
	_is_runing = true;
	char path_buff[64];
	sprintf(path_buff,"td_flow/%s/%s/", _broker_id.c_str(), _userid.c_str());
	if (!std::filesystem::exists(path_buff))
	{
		std::filesystem::create_directories(path_buff);
	}
	_td_api = _ctp_creator(path_buff);
	_td_api->RegisterSpi(this);
	//_td_api->SubscribePrivateTopic(THOST_TERT_RESTART);
	//_td_api->SubscribePublicTopic(THOST_TERT_RESTART);
	_td_api->SubscribePrivateTopic(THOST_TERT_RESUME);
	_td_api->SubscribePublicTopic(THOST_TERT_RESUME);
	_td_api->RegisterFront(const_cast<char*>(_front_addr.c_str()));
	_td_api->Init();
	LOG_INFO("ctp_trader init ");
	_process_signal.wait(_process_mutex);
	
	
	_is_inited = true ;
	submit_settlement();
	LOG_INFO("ctp_trader login");
}

void ctp_trader::logout()
{
	_is_runing = false;
	do_logout();
	
	_reqid = 0;
	
	_front_id = 0;		//前置编号
	_session_id = 0;	//会话编号
	_order_ref.exchange(0);		//报单引用


	_position_info.clear();
	_order_info.clear();
	
	_is_in_query.exchange(false);
	_is_inited = false;
	_is_connected = false;
	_is_sync_wait.exchange(false);

	if (_td_api)
	{
		_td_api->RegisterSpi(nullptr);
		//_td_api->Join();
		_td_api->Release();
		_td_api = nullptr;
	}
	LOG_INFO("ctp_trader logout");
}

bool ctp_trader::do_login()
{
	if (_td_api == nullptr)
	{
		return false;
	}
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.UserID, _userid.c_str());
	strcpy(req.Password, _password.c_str());
	strcpy(req.UserProductInfo, _product_info.c_str());
	int iResult = _td_api->ReqUserLogin(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader do_login request failed: %d", iResult);
		return false ;
	}
	return true;
}

bool ctp_trader::do_logout()
{
	if (_td_api == nullptr)
	{
		return false;
	}

	CThostFtdcUserLogoutField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.UserID, _userid.c_str());
	int iResult = _td_api->ReqUserLogout(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader logout request failed: %d", iResult);
		return false ;
	}

	return true;
}

bool ctp_trader::query_positions(bool is_sync)
{
	if (_td_api == nullptr)
	{
		return false;
	}
	if (_is_in_query)
	{
		LOG_ERROR("ctp trader _is_in_query not return");
		return false;
	}
	CThostFtdcQryInvestorPositionField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.InvestorID, _userid.c_str());
	auto iResult = _td_api->ReqQryInvestorPosition(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ReqQryInvestorPosition failed: %d", iResult);
		return false;
	}
	while (!_is_in_query.exchange(true));
	if(is_sync)
	{
		while (!_is_sync_wait.exchange(true));
		_process_signal.wait(_process_mutex);
	}
	return true ;
}

bool ctp_trader::query_orders(bool is_sync)
{
	if (_td_api == nullptr)
	{
		return false;
	}
	if (_is_in_query)
	{
		LOG_ERROR("ctp mini trader _is_in_query not return");
		return false;
	}
	CThostFtdcQryOrderField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.InvestorID, _userid.c_str());
	auto iResult = _td_api->ReqQryOrder(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ReqQryOrder failed: %d", iResult);
		return false;
	}
	while (!_is_in_query.exchange(true));
	if (is_sync)
	{
		while (!_is_sync_wait.exchange(true));
		_process_signal.wait(_process_mutex);
	}
	return true ;
}

void ctp_trader::OnFrontConnected()
{
	LOG_INFO("ctp_trader OnFrontConnected ");
	_is_connected = true ;
	if (_is_runing)
	{
		do_auth();
	}
}

void ctp_trader::OnFrontDisconnected(int nReason)
{
	LOG_INFO("ctp_trader FrontDisconnected : Reason ->", nReason);
	_is_connected = false ;
}

void ctp_trader::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID)
	{
		LOG_ERROR("ctp_trader OnRspAuthenticate Error :", pRspInfo->ErrorID);
		return ;
	}
	do_login();
}

void ctp_trader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID)
	{
		LOG_ERROR("ctp_trader OnRspUserLogin Error : ", pRspInfo->ErrorID);
		return;
	}

	if (pRspUserLogin)
	{

		LOG_INFO("[%s][用户登录] UserID:%s AppID:%s SessionID:%d FrontID:%d",
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
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_DEBUG("OnRspSettlementInfoConfirm\tErrorID =", pRspInfo->ErrorID, "ErrorMsg =", pRspInfo->ErrorMsg);
	}
	if (bIsLast)
	{
		_process_signal.notify_all();
	}
}

void ctp_trader::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspOrderInsert \tErrorID =",pRspInfo->ErrorID,"ErrorMsg =", pRspInfo->ErrorMsg);
	}
	if (pInputOrder && pRspInfo)
	{
		estid_t estid = generate_estid(_front_id, _session_id, strtol(pInputOrder->OrderRef, NULL, 10));
		this->fire_event(trader_event_type::TET_OrderError, error_type::ET_PLACE_ORDER, estid, (uint32_t)pRspInfo->ErrorID);
	}
}

void ctp_trader::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspOrderAction \tErrorID =", pRspInfo->ErrorID,"ErrorMsg =", pRspInfo->ErrorMsg);
	}
	if (pInputOrderAction && pRspInfo)
	{
		estid_t estid = generate_estid(pInputOrderAction->FrontID, pInputOrderAction->SessionID, strtol(pInputOrderAction->OrderRef, NULL, 10));
		this->fire_event(trader_event_type::TET_OrderError, error_type::ET_CANCEL_ORDER, estid, (uint32_t)pRspInfo->ErrorID);
	}
}

void ctp_trader::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_position_info.clear();
	}
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspQryInvestorPosition \tErrorID = [%d] ErrorMsg = [%s]", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		return;
	}
	
	if (pInvestorPosition)
	{	
		LOG_DEBUG("OnRspQryInvestorPosition ", pInvestorPosition->InstrumentID, pInvestorPosition->TodayPosition, pInvestorPosition->Position, pInvestorPosition->YdPosition);
		code_t code(pInvestorPosition->InstrumentID, pInvestorPosition->ExchangeID);
		position_seed pos;
		auto it = _position_info.find(code);
		if(it != _position_info.end())
		{
			pos = it->second;
		}
		pos.id = code;
		if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
		{

			if(pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
			{

				pos.today_long += pInvestorPosition->TodayPosition;
				if (std::strcmp(pInvestorPosition->ExchangeID, EXCHANGE_ID_SHFE) != 0)
				{
					uint32_t yestoday_position = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
					pos.today_long += yestoday_position;
				}
				
			}
			else
			{
				uint32_t yestoday_position = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
				pos.history_long += yestoday_position;
			}
		}
		else if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short)
		{
			if (pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today)
			{
				pos.today_short += pInvestorPosition->TodayPosition;
				if(std::strcmp(pInvestorPosition->ExchangeID, EXCHANGE_ID_SHFE) != 0)
				{
					uint32_t yestoday_position = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
					pos.today_short += yestoday_position;
				}
			}
			else
			{
				uint32_t yestoday_position = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
				pos.history_short += yestoday_position;
			}
		}
		_position_info[code] = pos;
	}
	if (bIsLast && _is_sync_wait)
	{
		while (!_is_sync_wait.exchange(false));
		_process_signal.notify_all();
	}
}


void ctp_trader::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
	}
	if (pRspInfo&& pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnRspQryTrade \tErrorID =", pRspInfo->ErrorID,"ErrorMsg = ", pRspInfo->ErrorMsg);
		return;
	}
	if (bIsLast && _is_sync_wait)
	{
		while (!_is_sync_wait.exchange(false));
		_process_signal.notify_all();
	}
}

void ctp_trader::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (_is_in_query)
	{
		while (!_is_in_query.exchange(false));
		_order_info.clear();
	}
	if (pOrder&& pOrder->VolumeTotal>0&& pOrder->OrderStatus!= THOST_FTDC_OST_Canceled&& pOrder->OrderStatus != THOST_FTDC_OST_AllTraded)
	{
		estid_t estid = generate_estid(pOrder->FrontID, pOrder->SessionID,strtoul(pOrder->OrderRef,NULL,10));
		auto order = _order_info[estid];
		order.code = code_t(pOrder->InstrumentID , pOrder->ExchangeID);
		order.create_time = make_daytm(pOrder->InsertTime,0);
		order.est_id = estid;
		order.direction = wrap_position_direction(pOrder->Direction);
		order.offset = wrap_offset_type(pOrder->CombOffsetFlag[0]);
		order.last_volume = pOrder->VolumeTotal;
		order.total_volume = pOrder->VolumeTotal + pOrder->VolumeTraded;
		order.price = pOrder->LimitPrice;
		_order_info[estid] = order;
		LOG_INFO("OnRspQryOrder", pOrder->InstrumentID, estid, pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef, pOrder->LimitPrice);
	}

	if (bIsLast && _is_sync_wait)
	{
		while (!_is_sync_wait.exchange(false));
		_process_signal.notify_all();
	}
}

void ctp_trader::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(!_is_inited)
	{
		return ;
	}
	if (pRspInfo)
	{
		LOG_ERROR("OnRspError \tErrorID = ", pRspInfo->ErrorID, " ErrorMsg =",  pRspInfo->ErrorMsg);
		this->fire_event(trader_event_type::TET_OrderError,error_type::ET_OTHER_ERROR, INVALID_ESTID, (uint32_t)pRspInfo->ErrorID);
	}

}

void ctp_trader::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	if(pOrder == nullptr||! _is_inited)
	{
		return ;
	}

	auto estid = generate_estid(pOrder->FrontID, pOrder->SessionID, strtoul(pOrder->OrderRef, NULL, 10));
	auto code = code_t(pOrder->InstrumentID, pOrder->ExchangeID);
	auto direction = wrap_direction_offset(pOrder->Direction, pOrder->CombOffsetFlag[0]);
	auto offset = wrap_offset_type(pOrder->CombOffsetFlag[0]);
	auto is_today = (THOST_FTDC_OF_CloseToday == pOrder->CombOffsetFlag[0]);
	LOG_INFO("OnRtnOrder", estid, pOrder->FrontID, pOrder->SessionID, pOrder->InstrumentID, direction, offset, pOrder->OrderStatus);

	if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled || pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
	{
		auto it = _order_info.find(estid);
		if (it != _order_info.end())
		{	
			auto order = it->second;
			if (order.last_volume > static_cast<uint32_t>(pOrder->VolumeTotal))
			{
				uint32_t deal_valume = order.last_volume - pOrder->VolumeTotal;
				if(deal_valume>0)
				{
					order.last_volume = pOrder->VolumeTotal;
					//触发 deal 事件
					this->fire_event(trader_event_type::TET_OrderDeal, estid, deal_valume, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
				}
			}
			if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			{
				LOG_INFO("OnRtnOrder fire_event ET_OrderCancel", estid, code.get_id(), direction, offset);
				this->fire_event(trader_event_type::TET_OrderCancel, estid, code, offset, direction, pOrder->LimitPrice, (uint32_t)pOrder->VolumeTotal, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
			}
			if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)
			{
				LOG_INFO("OnRtnOrder fire_event ET_OrderTrade", estid, code.get_id(), direction, offset);
				this->fire_event(trader_event_type::TET_OrderTrade, estid, code, offset, direction, pOrder->LimitPrice, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
			}
			_order_info.erase(it);
		}
	}
	else
	{
		auto it = _order_info.find(estid);
		if (it == _order_info.end())
		{
			order_info entrust;
			entrust.code = code;
			entrust.create_time = make_daytm(pOrder->InsertTime,0);
			entrust.est_id = estid;
			entrust.direction = direction;
			entrust.last_volume = pOrder->VolumeTotal;
			entrust.total_volume = pOrder->VolumeTotal + pOrder->VolumeTraded;
			entrust.offset = offset;
			entrust.price = pOrder->LimitPrice;
			_order_info.insert(std::make_pair(estid, entrust));
			this->fire_event(trader_event_type::TET_OrderPlace, entrust);
			if (pOrder->VolumeTraded > 0)
			{
				//触发 deal 事件
				this->fire_event(trader_event_type::TET_OrderDeal, estid, (uint32_t)pOrder->VolumeTotal, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
			}
		}
		else
		{
			auto entrust = it->second;
			if(entrust.last_volume > static_cast<uint32_t>(pOrder->VolumeTotal))
			{
				uint32_t deal_volume = entrust.last_volume - pOrder->VolumeTotal;
				if(deal_volume > 0)
				{
					entrust.last_volume = pOrder->VolumeTotal;
					//触发 deal 事件
					this->fire_event(trader_event_type::TET_OrderDeal, estid, deal_volume, (uint32_t)(pOrder->VolumeTraded + pOrder->VolumeTotal));
				}
			}
			else
			{
				if (entrust.last_volume < static_cast<uint32_t>(pOrder->VolumeTotal))
				{
					LOG_ERROR("OnRtnOrder Error", estid, code.get_id(), entrust.last_volume, pOrder->VolumeTotal);
				}
			}
			_order_info[estid] = entrust;
		}

	}
}

void ctp_trader::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	if (!_is_inited)
	{
		return;
	}
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnErrRtnOrderInsert \tErrorID =", pRspInfo->ErrorID," ErrorMsg =", pRspInfo->ErrorMsg);
	}
	if(pInputOrder && pRspInfo)
	{
		LOG_ERROR("OnErrRtnOrderInsert", pInputOrder->InstrumentID, pInputOrder->VolumeTotalOriginal, pRspInfo->ErrorMsg);
		estid_t estid = generate_estid(_front_id, _session_id, strtol(pInputOrder->OrderRef,NULL,10));
		auto it = _order_info.find(estid);
		if(it != _order_info.end())
		{
			_order_info.erase(it);
		}
		this->fire_event(trader_event_type::TET_OrderError, error_type::ET_PLACE_ORDER,estid, (uint32_t)pRspInfo->ErrorID);
	}
}
void ctp_trader::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
{
	if (!_is_inited)
	{
		return;
	}
	if (pRspInfo && pRspInfo->ErrorID != 0)
	{
		LOG_ERROR("OnErrRtnOrderAction \tErrorID = ",pRspInfo->ErrorID," ErrorMsg =", pRspInfo->ErrorMsg);
		if(pOrderAction)
		{
			LOG_ERROR("OnErrRtnOrderAction ", pOrderAction->OrderRef, pOrderAction->RequestID, pOrderAction->SessionID, pOrderAction->FrontID);
		}
		return ;
	}
	if (pOrderAction && pRspInfo)
	{
		estid_t estid = generate_estid(pOrderAction->FrontID, pOrderAction->SessionID, strtol(pOrderAction->OrderRef, NULL, 10));
		
		this->fire_event(trader_event_type::TET_OrderError, error_type::ET_CANCEL_ORDER, estid, (uint32_t)pRspInfo->ErrorID);
	}
}

void ctp_trader::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus)
{
	
}

bool ctp_trader::do_auth()
{
	if (_td_api == nullptr)
	{
		return false;
	}
	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.UserID, _userid.c_str());
	//strcpy(req.UserProductInfo, m_strProdInfo.c_str());
	strcpy(req.AuthCode, _authcode.c_str());
	strcpy(req.AppID, _appid.c_str());
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
	PROFILE_DEBUG(code.get_id());
	LOG_INFO("ctp_trader place_order %s %d",code.get_id(), volume);

	if (_td_api == nullptr)
	{
		return INVALID_ESTID;
	}
	estid_t est_id = generate_estid();
	
	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.InvestorID, _userid.c_str());

	strcpy(req.InstrumentID, code.get_id());
	strcpy(req.ExchangeID, code.get_excg());

	uint32_t order_ref = 0, season_id = 0, front_id = 0;
	extract_estid(est_id, front_id, season_id, order_ref);
	///报单引用
	sprintf(req.OrderRef, "%u", order_ref);

	if(price != .0F)
	{
		///报单价格条件: 限价
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

	}
	else
	{
		req.OrderPriceType = THOST_FTDC_OPT_BestPrice;
	}
	///买卖方向: 
	req.Direction = convert_direction_offset(direction, offset);
	///组合开平标志: 开仓
	req.CombOffsetFlag[0] = convert_offset_type(code,volume,offset, direction);
	///组合投机套保标志
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	req.LimitPrice = price;
	///数量: 1
	req.VolumeTotalOriginal = volume;

	if (flag == order_flag::OF_NOR)
	{
		req.TimeCondition = THOST_FTDC_TC_GFD;
		req.VolumeCondition = THOST_FTDC_VC_AV;
		req.MinVolume = 1;
	}
	else if (flag == order_flag::OF_FAK)
	{
		req.TimeCondition = THOST_FTDC_TC_IOC;
		req.VolumeCondition = THOST_FTDC_VC_AV;
		req.MinVolume = 1;
	}
	else if (flag == order_flag::OF_FOK)
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
	PROFILE_DEBUG(code.get_id());
	int iResult = _td_api->ReqOrderInsert(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader order_insert request failed: %d", iResult);
		return INVALID_ESTID;
	}
	PROFILE_INFO(code.get_id());
	return est_id;
}

void ctp_trader::cancel_order(estid_t order_id)
{
	if (_td_api == nullptr)
	{
		LOG_ERROR("ctp_trader cancel_order _td_api nullptr : %llu", order_id);
		return ;
	}
	auto it = _order_info.find(order_id);
	if (it == _order_info.end())
	{
		LOG_ERROR("ctp_trader cancel_order order invalid : %llu", order_id);
		return;
	}
	auto& order = it->second;
	uint32_t frontid = 0, sessionid = 0, orderref = 0;
	extract_estid(order_id, frontid, sessionid, orderref);
	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.InvestorID, _userid.c_str());
	strcpy(req.UserID, _userid.c_str());
	///报单引用
	sprintf(req.OrderRef, "%u", orderref);
	///请求编号
	///前置编号
	req.FrontID = frontid;
	///会话编号
	req.SessionID = sessionid;
	///操作标志
	req.ActionFlag = convert_action_flag(action_flag::AF_CANCEL);
	///合约代码
	strcpy(req.InstrumentID, order.code.get_id());

	//req.LimitPrice = change.price;

	//req.VolumeChange = (int)change.volume;

	//strcpy_s(req.OrderSysID, change.order_id.c_str());
	strcpy(req.ExchangeID, order.code.get_excg());
	LOG_INFO("ctp_trader ReqOrderAction :", req.ExchangeID, req.InstrumentID, req.FrontID, req.SessionID, req.OrderRef, req.BrokerID, req.InvestorID, req.UserID, order_id);
	int iResult = _td_api->ReqOrderAction(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader order_action request failed:", iResult);
	}
}


uint32_t ctp_trader::get_trading_day()const
{
	if(_td_api)
	{
		return static_cast<uint32_t>(std::atoi(_td_api->GetTradingDay()));
	}
	return 0X0U;
}

std::shared_ptr<trader_data> ctp_trader::get_trader_data()
{
	auto result = std::make_shared<trader_data>();
	if (!query_positions(true))
	{
		LOG_ERROR("query_positions error");
		return result;
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	if (!query_orders(true))
	{
		LOG_ERROR("query_orders error");
		return result;
	}
	for (auto it : _order_info)
	{
		result->orders.emplace_back(it.second);
	}
	for (auto it : _position_info)
	{
		result->positions.emplace_back(it.second);
	}
	return result ;
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
	strcpy(req.BrokerID, _broker_id.c_str());
	strcpy(req.InvestorID, _userid.c_str());

	//fmt::format_to(req.ConfirmDate, "{}", TimeUtils::getCurDate());
	//memcpy(req.ConfirmTime, TimeUtils::getLocalTime().c_str(), 8);

	int iResult = _td_api->ReqSettlementInfoConfirm(&req, genreqid());
	if (iResult != 0)
	{
		LOG_ERROR("ctp_trader submit_settlement request failed: %d", iResult);
	}
	while (!_is_sync_wait.exchange(true));
	_process_signal.wait(_process_mutex);
}
