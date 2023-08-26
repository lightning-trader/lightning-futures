#pragma once
#include <define.h>
#include <market_api.h>
#include <event_center.hpp>
#include <mutex>
#include <condition_variable>
#include <params.hpp>
#include <../../api/CTP_V6.6.9_20220920/ThostFtdcMdApi.h>
#include <dll_helper.hpp>



class ctp_market :	public actual_market,public CThostFtdcMdSpi
{
public:

	ctp_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config);

	virtual ~ctp_market();
	
//IMarketAPI 接口
public:

	virtual void login() override;

	virtual void logout()override;

	virtual void subscribe(const std::set<code_t>& codes) override;

	virtual void unsubscribe(const std::set<code_t>& codes) override;


//CThostFtdcMdSpi 接口
public:
	virtual void OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );

	virtual void OnFrontConnected();

	virtual void OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	virtual void OnFrontDisconnected( int nReason );

	virtual void OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pDepthMarketData );

	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );

	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

private:
	/*
	 *	发送登录请求
	 */
	void do_userlogin();
	/*
	 *	订阅品种行情
	 */
	void do_subscribe();
	
	void do_unsubscribe(const std::vector<code_t>& code_list);

private:
	
	CThostFtdcMdApi*	_md_api;

	std::string			_front_addr;
	std::string			_broker_id;
	std::string			_userid;
	std::string			_password;
	
	int					_reqid;

	std::mutex _mutex;
	std::unique_lock<std::mutex> _process_mutex;
	std::condition_variable _process_signal;

	bool _is_inited ;

	typedef CThostFtdcMdApi* (*market_creator)(const char*, const bool, const bool);
	market_creator					_ctp_creator;
	dll_handle						_market_handle;
	
};

