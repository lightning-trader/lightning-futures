#pragma once
#include <queue>
#include <stdint.h>
#include <thread>
#include <define.h>
#include <trader_api.h>
#include <data_types.hpp>
#include <ThostFtdcTraderApi.h>
#include <boost/pool/pool_alloc.hpp>
#include <boost/property_tree/ptree.hpp>
#include <condition_variable>

/*
 *	订单操作类型
 */
enum class action_flag
{
	AF_CANCEL = '0',	//撤销
	AF_MODIFY = '3',	//修改
};


class ctp_trader : public actual_trader, public CThostFtdcTraderSpi
{
public:
	ctp_trader();
	
	virtual ~ctp_trader();


	bool init(const boost::property_tree::ptree& config);

	//////////////////////////////////////////////////////////////////////////
	//trader_api接口
public:


	virtual bool is_usable() const override;

	virtual por_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

	virtual void cancel_order(estid_t order_id) override ;

	virtual void submit_settlement() override;

	virtual uint32_t get_trading_day()const override;

	virtual std::shared_ptr<trader_data> get_trader_data()const override;

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

	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo) override;

	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus) override;


private:

	//认证
	bool do_auth();
	//登录
	bool do_login();

	estid_t do_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag,bool is_close_history);

	bool logout();

	void query_account(bool is_sync);

	void query_positions(bool is_sync);

	void query_orders(bool is_sync);

	void query_trades(bool is_sync);

	void calculate_position(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume, double_t price, bool is_today);
	void frozen_deduction(const code_t& code, direction_type dir_type, uint32_t volume, bool is_today);
	void thawing_deduction(const code_t& code, direction_type dir_type, uint32_t volume, bool is_today);

private:
	

	inline int wrap_direction_offset(direction_type dir_type, offset_type offset_type)
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

	inline direction_type convert_direction_offset(TThostFtdcDirectionType dir_type, TThostFtdcOffsetFlagType offset_type)
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

	inline direction_type convert_direction(TThostFtdcPosiDirectionType dirType)
	{
		if (THOST_FTDC_PD_Long == dirType)
			return direction_type::DT_LONG;
		else
			return direction_type::DT_SHORT;
	}

	inline int wrap_offset(const code_t& code,offset_type offset, bool is_close_history)
	{
		if (offset_type::OT_OPEN == offset)
		{
			return THOST_FTDC_OF_Open;
		}
		else if (offset_type::OT_CLOSE == offset)
		{
			if (code.is_split_position())
			{
				if (is_close_history)
				{
					return THOST_FTDC_OF_CloseYesterday;
				}
				else
				{
					return THOST_FTDC_OF_CloseToday;
				}
			}
			return THOST_FTDC_OF_Close;
		}
		return THOST_FTDC_OF_ForceClose;
	}

	inline offset_type convert_offset(TThostFtdcOffsetFlagType offset_type)
	{
		if (THOST_FTDC_OF_Open == offset_type)
			return offset_type::OT_OPEN;
		else
			return offset_type::OT_CLOSE;
	}

	inline int wrap_action_flag(action_flag action_flag)
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

	inline estid_t generate_estid(uint32_t front_id,uint32_t session_id,uint32_t order_ref)
	{
		uint64_t p1 = (uint64_t)session_id<<32;
		uint64_t p2 = (uint64_t)front_id<<16;
		uint64_t p3 = (uint64_t)order_ref;
		uint64_t v1 = p1 & 0xFFFFFFFF00000000LLU;
		uint64_t v2 = p2 & 0x00000000FFFF0000LLU;
		uint64_t v3 = p3 & 0x000000000000FFFFLLU;
		return v1+v2+v3;
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


	inline void print_position(const char* title)
	{
		LOG_INFO("print_position %s \n", title);
		for (const auto& it : _today_position)
		{
			const auto& pos = it.second;
			LOG_INFO("position : %s today_long(%d %d) today_short(%d %d) ", pos.id.get_id(),
				pos.long_cell.volume, pos.long_cell.frozen,pos.short_cell.volume, pos.short_cell.frozen);
		}
		for (const auto& it : _history_position)
		{
			const auto& pos = it.second;
			LOG_INFO("position : %s yestoday_long(%d %d) yestoday_short(%d %d)", pos.id.get_id(),
				pos.long_cell.volume, pos.long_cell.frozen,pos.short_cell.volume, pos.short_cell.frozen);
		}
	}

	inline position_info get_merge_position(const code_t& code)const 
	{
		auto it = _today_position.find(code);
		if (it == _today_position.end())
		{
			auto yit = _history_position.find(code);
			if (yit == _history_position.end())
			{
				return position_info(code);
			}
			return yit->second;
		}
		auto yit = _history_position.find(code);
		if (yit == _history_position.end())
		{
			return it->second;
		}
		position_info result = it->second;
		result.long_cell.volume += yit->second.long_cell.volume;
		result.long_cell.frozen += yit->second.long_cell.frozen;
		result.short_cell.volume += yit->second.short_cell.volume;
		result.short_cell.frozen += yit->second.short_cell.frozen;
		return result;
	}

protected:

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


	position_map			_today_position;
	position_map			_history_position;

	//
	entrust_map				_order_info;

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
	std::atomic<bool>				_is_sync_wait;
};

