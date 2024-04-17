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

	tap_api_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config)noexcept;

	virtual ~tap_api_market()noexcept;
	
	
//market_api 接口
public:

	virtual bool login() noexcept override;

	virtual void logout()noexcept override;

	virtual void subscribe(const std::set<code_t>& codes) noexcept override;

	virtual void unsubscribe(const std::set<code_t>& codes) noexcept override;


//CThostFtdcMdSpi 接口
public:
	
	virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo* info)noexcept;
	virtual void TAP_CDECL OnAPIReady()noexcept;
	virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode)noexcept;
	virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo* info)noexcept{};
	virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo* info)noexcept{};
	virtual void TAP_CDECL OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole* info)noexcept;
	virtual void TAP_CDECL OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract* info)noexcept;
	virtual void TAP_CDECL OnRtnQuote(const TapAPIQuoteWhole* info)noexcept;

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

