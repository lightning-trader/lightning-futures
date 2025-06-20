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
#include <basic_define.h>
#include <market_api.h>
#include <event_center.hpp>
#include <mutex>
#include <sstream>
#include <condition_variable>
#include <params.hpp>
#include <library_helper.hpp>
#include <TAP_V9_20200808/TapQuoteAPI.h>
#include <basic_utils.hpp>

namespace lt::driver
{
	class tap_api_market : public asyn_actual_market, public ITapQuoteAPINotify
	{
	public:

		tap_api_market(std::unordered_map<std::string, std::string>& id_excg_map, const params& config);

		virtual ~tap_api_market();


		//market_api 接口
	public:

		virtual bool login() override;

		virtual void logout()override;

		virtual void subscribe(const std::set<code_t>& codes) override;

		virtual void unsubscribe(const std::set<code_t>& codes) override;


		//CThostFtdcMdSpi 接口
	public:

		virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo* info)noexcept;
		virtual void TAP_CDECL OnAPIReady()noexcept;
		virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode)noexcept;
		virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo* info)noexcept {};
		virtual void TAP_CDECL OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole* info)noexcept;
		virtual void TAP_CDECL OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract* info)noexcept;
		virtual void TAP_CDECL OnRtnQuote(const TapAPIQuoteWhole* info)noexcept;

	private:

		inline lt::code_t wrap_code(const TapAPIContract& contract) const
		{
			std::stringstream ids;
			ids << contract.Commodity.CommodityNo << contract.ContractNo1;
			if (contract.CallOrPutFlag1 == 'C')
			{
				ids << "C" << contract.StrikePrice1;
			}
			else if (contract.CallOrPutFlag1 == 'P')
			{
				ids << "P" << contract.StrikePrice1;
			}
			if (std::strlen(contract.ContractNo1) > 0)
			{
				ids << contract.Commodity.CommodityNo << contract.ContractNo2;
				if (contract.CallOrPutFlag2 == 'C')
				{
					ids << "C" << contract.StrikePrice2;
				}
				else if (contract.CallOrPutFlag2 == 'P')
				{
					ids << "P" << contract.StrikePrice2;
				}
			}
			return lt::make_code(contract.Commodity.ExchangeNo, ids.str());
		}
		inline void convert_code(TapAPIContract& contract,const code_t& code)
		{
			symbol_t id1 = code.extract_symbol(1);
			strcpy(contract.Commodity.CommodityNo, id1.family.c_str());
			snprintf(contract.ContractNo1, 11, "%d", id1.number);
			if(id1.option_type== symbol_t::OPT_CALL)
			{
				contract.CallOrPutFlag1 = 'C';
				snprintf(contract.StrikePrice1, 11, "%.0lf", id1.strike_price);
			}else if (id1.option_type == symbol_t::OPT_PUT)
			{
				contract.CallOrPutFlag1 = 'P';
				snprintf(contract.StrikePrice1, 11, "%.0lf", id1.strike_price);
			}else
			{
				contract.CallOrPutFlag1 = 'N';
			}
			symbol_t id2 = code.extract_symbol(2);
			snprintf(contract.ContractNo2, 11, "%d", id2.number);
			if (id2.option_type == symbol_t::OPT_CALL)
			{
				contract.CallOrPutFlag2 = 'C';
				snprintf(contract.StrikePrice2, 11, "%.0lf", id2.strike_price);
			}
			else if (id2.option_type == symbol_t::OPT_PUT)
			{
				contract.CallOrPutFlag2 = 'P';
				snprintf(contract.StrikePrice2, 11, "%.0lf", id2.strike_price);
			}
			else
			{
				contract.CallOrPutFlag2 = 'N';
			}
		}

	private:

		ITapQuoteAPI* _md_api;

		std::string			_ip;
		uint16_t			_port;
		std::string			_userid;
		std::string			_password;
		std::string			_authcode;
		TAPIUINT32			_reqid;

		std::mutex _mutex;
		std::unique_lock<std::mutex> _process_mutex;
		std::condition_variable _process_signal;

		typedef ITapQuoteAPI* (*market_creator_function)(const TapAPIApplicationInfo*, TAPIINT32&);
		market_creator_function			_tap_creator;

		typedef void (*market_destroyer_function)(ITapQuoteAPI*);
		market_destroyer_function		_tap_destroyer;

		dll_handle						_market_handle;

		bool _is_inited;
		uint32_t _trading_day;
	};


}

