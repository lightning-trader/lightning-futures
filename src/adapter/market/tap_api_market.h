#pragma once
#include <define.h>
#include <market_api.h>
#include <event_center.hpp>
#include <mutex>
#include <condition_variable>
#include <params.hpp>
#include <../../api/TAP_V9_20200808/TapQuoteAPI.h>
#include <dll_helper.hpp>



class tap_api_market :	public asyn_actual_market,public ITapQuoteAPINotify
{
public:

	tap_api_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config);

	virtual ~tap_api_market();
	
	
//market_api 接口
public:

	virtual bool login() override;

	virtual void logout()override;

	virtual void subscribe(const std::set<code_t>& codes) override;

	virtual void unsubscribe(const std::set<code_t>& codes) override;


//CThostFtdcMdSpi 接口
public:
	
	virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo* info);
	virtual void TAP_CDECL OnAPIReady();
	virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode);
	virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo* info){};
	virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo* info){};
	virtual void TAP_CDECL OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole* info);
	virtual void TAP_CDECL OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract* info);
	virtual void TAP_CDECL OnRtnQuote(const TapAPIQuoteWhole* info);

private:


private:
	
	ITapQuoteAPI*	_md_api;

	std::string			_ip;
	uint16_t			_port;
	std::string			_userid;
	std::string			_password;
	std::string			_authcode;
	TAPIUINT32			_reqid;

	std::mutex _mutex;
	std::unique_lock<std::mutex> _process_mutex;
	std::condition_variable _process_signal;

	typedef ITapQuoteAPI* (*market_creator)(const TapAPIApplicationInfo* , TAPIINT32& );
	market_creator					_tap_creator;
	
	typedef void (*market_destroyer)(ITapQuoteAPI*);
	market_destroyer				_tap_destroyer ;

	dll_handle						_market_handle;

	bool _is_inited ;
	uint32_t _trading_day ;
};

