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
#include <../../api/CTP_V6.6.9_20220920/ThostFtdcMdApi.h>
#include <dll_helper.hpp>

namespace lt::driver
{
	class ctp_api_market : public asyn_actual_market, public CThostFtdcMdSpi
	{
	public:

		ctp_api_market(std::unordered_map<std::string, std::string>& id_excg_map, const params& config);

		virtual ~ctp_api_market();

		//IMarketAPI 接口
	public:

		virtual bool login() override;

		virtual void logout() override;

		virtual void subscribe(const std::set<code_t>& codes) override;

		virtual void unsubscribe(const std::set<code_t>& codes) override;


		//CThostFtdcMdSpi 接口
	public:
		virtual void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) noexcept;

		virtual void OnFrontConnected() noexcept;

		virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)noexcept;

		///登出请求响应
		virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)noexcept;

		virtual void OnFrontDisconnected(int nReason)noexcept;

		virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)noexcept;

		virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)noexcept;

		virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)noexcept;

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

		CThostFtdcMdApi* _md_api;

		std::string			_front_addr;
		std::string			_broker_id;
		std::string			_userid;
		std::string			_password;

		int					_reqid;

		std::mutex _mutex;
		std::unique_lock<std::mutex> _process_mutex;
		std::condition_variable _process_signal;

		bool _is_inited;

		typedef CThostFtdcMdApi* (*market_creator)(const char*, const bool, const bool);
		market_creator					_ctp_creator;
		dll_handle						_market_handle;

	};
}

