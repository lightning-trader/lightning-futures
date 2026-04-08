#include <bridge.h>
#include <market_api.h>
#include <inipp.h>
#include <interface.h>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <log_define.hpp>
#include <time_utils.hpp>
#include <basic_utils.hpp>


using namespace lt;
using namespace lt::cta;

// Helper function to parse launch_setting from JSON
launch_setting parse_launch_setting(const nlohmann::json& json_obj) {
	launch_setting setting;
	if (json_obj.contains("daily_open_limit")) {
		setting.daily_open_limit = json_obj["daily_open_limit"].get<uint32_t>();
	}
	if (json_obj.contains("daily_order_limit")) {
		setting.daily_order_limit = json_obj["daily_order_limit"].get<uint32_t>();
	}
	if (json_obj.contains("order_frequency_limit")) {
		setting.order_frequency_limit = json_obj["order_frequency_limit"].get<uint32_t>();
	}
	if (json_obj.contains("cancel_order_limit")) {
		setting.cancel_order_limit = json_obj["cancel_order_limit"].get<uint32_t>();
	}

	return setting;
}

// Helper function to parse ltds_setting from JSON
ltds_setting parse_ltds_setting(const nlohmann::json& json_obj) {
	ltds_setting setting;
	if (json_obj.contains("kline_cache")) {
		setting.kline_cache = json_obj["kline_cache"].get<uint32_t>();
	}
	if (json_obj.contains("detail_cache")) {
		setting.detail_cache = json_obj["detail_cache"].get<uint32_t>();
	}
	if (json_obj.contains("channel")) {
		setting.channel = json_obj["channel"].get<std::string>();
	}
	if (json_obj.contains("cache_path")) {
		setting.cache_path = json_obj["cache_path"].get<std::string>();
	}
	return setting;
}

// Helper function to parse contract_setting from JSON
contract_setting parse_contract_setting(const nlohmann::json& json_obj) {
	contract_setting setting;
	if (json_obj.contains("code")) {
		setting.code = lt::code_t(json_obj["code"].get<std::string>());
	}
	if (json_obj.contains("kline_length")) {
		setting.kline_length = json_obj["kline_length"].get<uint32_t>();
	}
	if (json_obj.contains("kline_period")) {
		auto periods = json_obj["kline_period"];
		if (periods.is_array()) {
			for (const auto& period : periods) {
				setting.kline_period.push_back(period.get<uint32_t>());
			}
		}
	}
	return setting;
}

bridge::bridge(const std::string& json_config)
	: _trader(nullptr), _ctx(nullptr), _dc(nullptr), _market(nullptr), _trading_day(0U)
	, _is_runing(false)
{
	try {
		nlohmann::json config = nlohmann::json::parse(json_config);

		// Parse account_config (INI file path)
		std::string account_config;
		if (config.contains("account_config")) {
			account_config = config["account_config"].get<std::string>();
		}
		else {
			throw std::runtime_error("JSON config missing 'account_config' field");
		}

		// Parse section_config
		std::string section_config;
		if (config.contains("section_config")) {
			section_config = config["section_config"].get<std::string>();
		}
		else {
			throw std::runtime_error("JSON config missing 'section_config' field");
		}

		// Parse launch_setting
		launch_setting launch_settings;
		if (config.contains("launch_setting")) {
			launch_settings = parse_launch_setting(config["launch_setting"]);
		}

		// Parse ltds_setting
		ltds_setting ltds_settings;
		if (config.contains("ltds_setting")) {
			ltds_settings = parse_ltds_setting(config["ltds_setting"]);
		}

		// Parse contract_settings
		if (config.contains("contract_settings")) {
			auto contract_settings_json = config["contract_settings"];
			if (contract_settings_json.is_array()) {
				for (const auto& setting_json : contract_settings_json) {
					contract_setting setting = parse_contract_setting(setting_json);
					contract_settings[setting.code] = setting;
				}
			}
		}

		if (!std::filesystem::exists(account_config))
		{
			throw std::runtime_error("account_config does not exist: " + account_config);
		}

		// Load INI config for market and trader
		inipp::Ini<char> ini;
		std::ifstream is(account_config);
		ini.parse(is);

		auto it = ini.sections.find("actual_market");
		if (it == ini.sections.end())
		{
			throw std::runtime_error("account_config missing [actual_market] section: " + account_config);
		}
		//market
		_market = create_actual_market(it->second);
		if (_market == nullptr)
		{
			throw std::runtime_error("failed to create actual_market from: " + account_config);
		}
		it = ini.sections.find("actual_trader");
		if (it == ini.sections.end())
		{
			throw std::runtime_error("account_config missing [actual_trader] section: " + account_config);
		}
		//trader
		_trader = create_actual_trader(it->second);
		if (_trader == nullptr)
		{
			throw std::runtime_error("failed to create actual_trader from: " + account_config);
		}

		_ctx = new trading_context(_market, _trader, section_config.c_str(), false);
		_ctx->set_trading_filter([this, launch_settings](const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, uint32_t count, double_t price, lt::order_flag flag)->bool {
			const auto& statisic = _ctx->get_order_statistic(code);
			if (offset == lt::offset_type::OT_OPEN && statisic.entrust_amount >= launch_settings.daily_open_limit)
			{
				PRINT_WARNING("trading filter open amount limit", statisic.entrust_amount, statisic.cancel_amount, statisic.trade_amount);
				return false;
			}
			if (statisic.entrust_amount >= launch_settings.daily_order_limit)
			{
				PRINT_WARNING("trading filter entrust amount limit", statisic.entrust_amount, statisic.cancel_amount, statisic.trade_amount);
				return false;
			}
			auto order_time = _ctx->last_order_time();
			auto last_time = _ctx->get_last_time();
			if (order_time > 0)
			{
				if (last_time < order_time || 1.0 / (last_time - order_time) > launch_settings.order_frequency_limit)
				{
					PRINT_WARNING("trading filter entrust order time limit", last_time, order_time);
					return false;
				}
			}

			PRINT_DEBUG("trading filter ", last_time, order_time, last_time - order_time);
			return true;
			},
			[this, launch_settings](estid_t estid)->bool {
				const auto& order = _ctx->get_order(estid);
				const auto& statisic = _ctx->get_order_statistic(order.code);
				if (statisic.cancel_amount >= launch_settings.cancel_order_limit)
				{
					EVALUATE_INFO("[撤单预警] 合约:", order.code.to_string(), " 撤单限制:", launch_settings.cancel_order_limit, " 当前撤单数量:", statisic.cancel_amount);
					PRINT_WARNING("trading filter cancel limit", statisic.cancel_amount, launch_settings.cancel_order_limit);
					return false;
				}
				return true;
			});
		this->_dc = new data_channel(_ctx, ltds_settings.channel.c_str(), ltds_settings.cache_path.c_str(), ltds_settings.detail_cache, ltds_settings.kline_cache);

	}
	catch (const std::exception& e) {
		PRINT_ERROR("Failed to parse JSON config:", e.what());
		throw;
	}
	if (_ctx == nullptr || _dc == nullptr)
	{
		throw std::runtime_error("Controller is not initialized");
	}
	
	// 设置周一到周五为工作日
	_manager.set_work_days({ 1,2,3,4,5 });
	//夜盘
	_manager.add_schedule("20:45:00", std::chrono::hours(6));
	//日盘
	_manager.add_schedule("08:45:00", std::chrono::hours(7));

	_manager.set_callback([&](int index) {

		if (!this->start_trading())
		{
			std::cerr << "启动同步失败" << std::endl;
			return;
		}
		},
		[&](int index) {
			this->stop_trading();
		});

}

bridge::~bridge()
{

	if (this->_ctx)
	{
		delete this->_ctx;
		this->_ctx = nullptr;
	}
	if (_dc)
	{
		delete _dc;
		_dc = nullptr;
	}
	if (_market)
	{
		_market->clear_event();
		destory_actual_market(_market);
	}

	if (_trader)
	{
		_trader->clear_event();
		destory_actual_trader(_trader);
	}

}


void bridge::on_tick(const lt::tick_info& tick)
{
	if (_on_tick_callback)
	{
		try
		{
			_on_tick_callback(tick);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}

void bridge::on_bar(const lt::bar_info& bar)
{

	if (_on_bar_callback)
	{
		try
		{
			_on_bar_callback(bar);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}


void bridge::on_entrust(const order_info& order)
{
	PRINT_INFO("on_entrust :", order.estid, order.code.get_symbol(), order.direction, order.offset, order.price, order.last_volume, order.total_volume);

	if (_on_entrust_callback)
	{

		try
		{
			_on_entrust_callback(order);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}

void bridge::on_deal(estid_t estid, uint32_t deal_volume)
{
	PRINT_INFO("on_deal :", estid, deal_volume);

	if (_on_deal_callback)
	{
		try
		{
			_on_deal_callback(estid, deal_volume);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}

void bridge::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	PRINT_INFO("on_trade :", localid, code.get_symbol(), direction, offset, price, volume);

	if (_on_trade_callback)
	{
		try
		{
			_on_trade_callback(localid, code, offset, direction, price, volume);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}

void bridge::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	PRINT_INFO("on_cancel :", localid, code.get_symbol(), direction, offset, price, cancel_volume);

	if (_on_cancel_callback)
	{
		try
		{
			_on_cancel_callback(localid, code, offset, direction, price, cancel_volume, total_volume);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}

void bridge::on_error(error_type type, estid_t localid, const error_code error)
{
	PRINT_ERROR("on_error :", localid, error);

	if (_on_error_callback)
	{
		try
		{
			_on_error_callback(type, localid, error);
		}
		catch (const std::exception& e)
		{
			PRINT_ERROR("Callback failed:", e.what());
		}
	}
}

void bridge::polling()
{
	_manager.polling();
	if (_is_runing)
	{
		_ctx->polling();
		_dc->polling();
	}
}

bool bridge::start_trading()
{
	if (_is_runing)
	{
		return false;
	}
	if (_trader == nullptr || !_trader->login())
	{
		return false;
	}
	if (_market == nullptr || !_market->login())
	{
		return false;
	}
	PRINT_INFO("login success start load data");
	if (!_ctx->load_data())
	{
		_trader->logout();
		_market->logout();
		return false;
	}

	// 订阅行情数据
	subscriber suber(_dc);

	// 订阅tick数据和bar数据 based on contract_settings
	for (const auto& contract_pair : contract_settings) {
		const lt::code_t& code = contract_pair.first;
		const contract_setting& setting = contract_pair.second;

		// 订阅tick数据
		suber.regist_tick_receiver(code, this);

		// 订阅bar数据
		for (const auto& period : setting.kline_period) {
			suber.regist_bar_receiver(code, period, this);
		}
	}

	if (!contract_settings.empty()) {
		suber.subscribe();
	}
	PRINT_INFO("load data success start realtime thread");
	_is_runing = true;
	this->check_crossday();
	return true;
}

bool bridge::stop_trading()
{
	if (!_is_runing)
	{
		return false;
	}

	_is_runing = false;

	// 取消订阅行情数据
	unsubscriber unsuber(_dc);
	for (const auto& contract_pair : contract_settings) {
		const lt::code_t& code = contract_pair.first;
		const contract_setting& setting = contract_pair.second;

		// 取消tick数据订阅
		unsuber.unregist_tick_receiver(code, this);

		// 取消bar数据订阅
		for (const auto& period : setting.kline_period) {
			unsuber.unregist_bar_receiver(code, period, this);
		}
	}
	unsuber.unsubscribe();
	if (_market)
	{
		_market->logout();
	}
	if (_trader)
	{
		_trader->logout();
	}

	return true;
}


void bridge::check_crossday()
{
	auto td = _ctx->get_trading_day();
	if (td != _trading_day)
	{
		this->_ctx->crossday();
		_trading_day = td;
	}
}
