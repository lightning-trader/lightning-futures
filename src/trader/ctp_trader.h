#pragma once
#include <queue>
#include <stdint.h>
#include <functional>
#include <thread>
#include <define.h>
#include <trader_api.h>
#include <data_types.hpp>
#include <ThostFtdcTraderApi.h>
#include <boost/pool/pool_alloc.hpp>
#include <boost/property_tree/ptree.hpp>


/*
 *	订单操作类型
 */
typedef enum action_flag
{
	AF_CANCEL = '0',	//撤销
	AF_MODIFY = '3',	//修改
} action_flag;


class ctp_trader : public trader_api, public CThostFtdcTraderSpi
{
public:
	ctp_trader(event_source* evt);
	
	virtual ~ctp_trader();


	bool init(const boost::property_tree::ptree& config);

	void release();

	//////////////////////////////////////////////////////////////////////////
	//trader_api接口
public:


	virtual bool is_usable() const override;

	virtual estid_t place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag) override;

	virtual void cancel_order(estid_t order_id) override ;

	virtual const account_info* get_account() const override ;

	virtual const position_info* get_position(code_t code) const override ;

	virtual const order_info* get_order(estid_t order_id) const override ;

	virtual void find_orders(std::vector<const order_info*>& order_result, std::function<bool(const order_info&)> func) const override;

	virtual void submit_settlement() override;

	//////////////////////////////////////////////////////////////////////////
	//CTP交易接口实现
public:
	virtual void OnFrontConnected() override;

	virtual void OnFrontDisconnected(int nReason) override;

	
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	
	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder) override;

	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade) override;

	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) override;

	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) override;



private:

	//认证
	bool do_auth();
	//登录
	bool do_login();

	bool logout();

	void query_account();

	void query_positions();

	void query_orders();

	void query_trades();

private:
	

	inline int wrap_direction_offset(direction_type dir_type, offset_type offset_type)
	{
		if (DT_LONG == dir_type)
			if (offset_type == OT_OPEN)
				return THOST_FTDC_D_Buy;
			else
				return THOST_FTDC_D_Sell;
		else
			if (offset_type == OT_OPEN)
				return THOST_FTDC_D_Sell;
			else
				return THOST_FTDC_D_Buy;
	}

	inline direction_type wrap_direction_offset(TThostFtdcDirectionType dir_type, TThostFtdcOffsetFlagType offset_type)
	{
		if (THOST_FTDC_D_Buy == dir_type)
			if (offset_type == THOST_FTDC_OF_Open)
				return DT_LONG;
			else
				return DT_SHORT;
		else
			if (offset_type == THOST_FTDC_OF_Open)
				return DT_SHORT;
			else
				return DT_LONG;
	}

	inline direction_type wrap_position_direction(TThostFtdcPosiDirectionType dirType)
	{
		if (THOST_FTDC_PD_Long == dirType)
			return DT_LONG;
		else
			return DT_SHORT;
	}

	inline int wrap_offset_type(code_t code,uint32_t volume,offset_type offset, direction_type direction)
	{
		if (OT_OPEN == offset)
		{
			return THOST_FTDC_OF_Open;
		}
		else if (OT_CLOSE == offset)
		{
			auto it = _yestoday_position_info.find(code);
			if (it != _yestoday_position_info.end())
			{
				//TODO 先不处理分仓
				if (direction == DT_LONG)
				{
					if (it->second.long_postion > volume)
					{
						return THOST_FTDC_OF_CloseYesterday;
					}
				}
				else
				{
					if (it->second.short_postion > volume)
					{
						return THOST_FTDC_OF_CloseYesterday;
					}
					
				}
			}
			
		}
		
		return THOST_FTDC_OF_CloseToday;
	}

	inline offset_type wrap_offset_type(TThostFtdcOffsetFlagType offset_type)
	{
		if (THOST_FTDC_OF_Open == offset_type)
			return OT_OPEN;
		else
			return OT_CLOSE;
	}

	inline int wrap_action_flag(action_flag action_flag)
	{
		if (AF_CANCEL == action_flag)
			return THOST_FTDC_AF_Delete;
		else
			return THOST_FTDC_AF_Modify;
	}
	inline estid_t generate_estid()
	{
		_order_ref.fetch_add(1);
		return generate_estid(_front_id, _session_id, _order_ref);
	}

	inline estid_t generate_estid(uint32_t front_id,uint32_t session_id,uint32_t order_ref)
	{
		thread_local static char buffer[32];
		sprintf_s(buffer, "%u.%u.%u", front_id, session_id, order_ref);
		const char * est_id = buffer;
		return est_id;
	}
	
	inline bool	extract_estid(estid_t estid, uint32_t& front_id, uint32_t& session_id, uint32_t& order_ref)
	{
		std::string estid_str = std::string(estid.id);
		auto front_id_end = estid_str.find('.');
		if (front_id_end == std::string::npos)
			return false;
		auto front_id_str = estid_str.substr(0, front_id_end);
		front_id = strtoul(front_id_str.c_str(), nullptr, 10);
		
		auto session_id_end = estid_str.find('.',front_id_end+1);
		if (session_id_end == std::string::npos)
			return false;
		auto session_id_str = estid_str.substr(front_id_end+1, session_id_end - front_id_end - 1);
		session_id = strtoul(session_id_str.c_str(), nullptr, 10);

		auto order_ref_str = estid_str.substr(session_id_end+1, estid_str.size() - session_id_end - 1);
		order_ref = strtoul(order_ref_str.c_str(), nullptr, 10);
		return true ;
	}

	inline uint32_t genreqid()
	{
		return _reqid.fetch_add(1);
	}

protected:

	event_source*			_event ;
	CThostFtdcTraderApi*	_td_api;
	std::atomic<uint32_t>	_reqid;

	std::string				_front_addr;
	std::string				_broker_id;
	std::string				_userid;
	std::string				_password;
	std::string				_appid;
	std::string				_authcode;
	std::string				_prodict_info;

	std::string				_usernick;

	time_t					_last_query_time;
	uint32_t				_front_id;		//前置编号
	uint32_t				_session_id;	//会话编号
	std::atomic<uint32_t>	_order_ref;		//报单引用
	typedef std::function<void()>	common_executer;
	typedef std::queue<common_executer>	query_queue; //查询队列
	query_queue				_query_queue;

	//boost::pool_allocator<code_t, position_info> a ;
	//std::vector<int, boost::pool_allocator<int>> v;
	//
	typedef std::map<code_t,position_info, std::less<code_t>, boost::fast_pool_allocator<std::pair<code_t const, position_info>>> position_map;
	position_map			_position_info;
	position_map			_yestoday_position_info;

	//
	typedef std::map<estid_t, order_info, std::less<estid_t>, boost::fast_pool_allocator<std::pair<estid_t const, order_info>>> entrust_map;
	entrust_map				_order_info;
	
	//
	typedef std::map<estid_t, trade_info, std::less<estid_t>, boost::fast_pool_allocator<std::pair<estid_t const, trade_info>>> trade_map;
	trade_map				_trade_info;

	account_info			_account_info ;

	bool					_is_runing ;
	std::thread*			_work_thread ;

	std::mutex				_query_mutex ;
	std::mutex _mutex;
	std::unique_lock<std::mutex>	_process_mutex;
	std::condition_variable			_process_signal;
	std::atomic<bool>				_is_in_query ;
	bool							_is_inited ;
	bool							_is_connected ;
};

