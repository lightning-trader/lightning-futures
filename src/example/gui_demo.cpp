#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <afxwin.h>
#include <afxcmn.h>
#include <atlconv.h>

#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <interface.h>
#include <memory>
#include <mutex>
#include <numeric>
#include <params.hpp>
#include <queue>
#include <receiver.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <trading_context.h>
#include <data_channel.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <inipp.h>

namespace
{
	constexpr UINT_PTR UI_TIMER_ID = 2001U;
	constexpr UINT UI_REFRESH_INTERVAL_MS = 33U;
	const std::array<const char*, 3> kFavoriteContracts = {
		"SHFE.ag2606",
		"SHFE.fu2609",
		"CZCE.MA505"
	};

	enum control_id : UINT
	{
		IDC_COMBO_CODE = 1001,
		IDC_COMBO_SIDE,
		IDC_RADIO_BUY_OPEN,
		IDC_RADIO_SELL_OPEN,
		IDC_RADIO_BUY_CLOSE,
		IDC_RADIO_SELL_CLOSE,
		IDC_EDIT_VOLUME,
		IDC_EDIT_PRICE,
		IDC_CHECK_CLOSE_TODAY,
		IDC_BUTTON_ORDER,
		IDC_BUTTON_CANCEL,
		IDC_LIST_ORDERS,
		IDC_LIST_POSITIONS,
		IDC_STATIC_STATUS,
		IDC_BUTTON_BATCH_CANCEL_ALL,
		IDC_BUTTON_CLOSE_ALL,
		IDC_BUTTON_PAUSE,
		IDC_EDIT_ORDER_LIMIT,
		IDC_EDIT_CANCEL_LIMIT,
		IDC_BUTTON_SET_LIMITS,
		IDC_EDIT_LOGS,
		IDC_COMBO_LOG_LEVEL,
		IDC_STATIC_MONITOR,
		IDC_STATIC_SUMMARY,
		IDC_STATIC_HINT,
		IDC_TAB_MAIN,
		IDC_TAB_TOP_RIGHT,
		IDC_GROUP_QUICK,
		IDC_GROUP_MARKET,
		IDC_GROUP_TICKS,
		IDC_GROUP_RISK,
		IDC_GROUP_MONITOR,
		IDC_GROUP_FAVORITES,
		IDC_GROUP_ORDERS,
		IDC_GROUP_POSITIONS,
		IDC_GROUP_LOGS,
		IDC_LABEL_CONTRACT,
		IDC_LABEL_SIDE,
		IDC_LABEL_VOLUME,
		IDC_LABEL_PRICE,
		IDC_LABEL_ORDER_LIMIT,
		IDC_LABEL_CANCEL_LIMIT,
		IDC_STATIC_RISK_DIVIDER,
		IDC_LIST_FAVORITES,
		IDC_CHART_MARKET,
		IDC_LIST_TICKS
	};

	CString to_cstring(const std::string& text)
	{
		USES_CONVERSION;
		return CString(A2CT(text.c_str()));
	}

	std::string to_std_string(const CString& text)
	{
		USES_CONVERSION;
		return std::string(CT2A(text));
	}

	std::string get_window_text(CWnd& wnd)
	{
		CString text;
		wnd.GetWindowText(text);
		return to_std_string(text);
	}

	void set_window_text(CWnd& wnd, const std::string& text)
	{
		wnd.SetWindowText(to_cstring(text));
	}

	std::filesystem::path get_module_directory()
	{
		char buffer[MAX_PATH] = {};
		const DWORD size = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		if (size == 0 || size >= MAX_PATH)
		{
			return std::filesystem::current_path();
		}
		return std::filesystem::path(buffer).parent_path();
	}

	std::filesystem::path resolve_config_path(const char* relative_name)
	{
		std::vector<std::filesystem::path> roots = {
			std::filesystem::current_path(),
			get_module_directory()
		};

		for (const auto& root : roots)
		{
			auto cursor = root;
			for (int depth = 0; depth < 6; ++depth)
			{
				const auto bin_candidate = cursor / "bin" / "config" / relative_name;
				if (std::filesystem::exists(bin_candidate))
				{
					return bin_candidate;
				}
				const auto direct_candidate = cursor / "config" / relative_name;
				if (std::filesystem::exists(direct_candidate))
				{
					return direct_candidate;
				}
				if (!cursor.has_parent_path())
				{
					break;
				}
				cursor = cursor.parent_path();
			}
		}

		throw std::runtime_error(std::string("config file not found: ") + relative_name);
	}

	constexpr uint32_t kDefaultMaxSingleOrderVolume = 100U;
	constexpr uint32_t kAgOpenMaxSingleOrderVolume = 800U;

	std::string format_logic_time_text(lt::daytm_t time)
	{
		const auto hour = time / 3600000U;
		const auto minute = (time / 60000U) % 60U;
		const auto second = (time / 1000U) % 60U;
		const auto millisecond = time % 1000U;

		std::ostringstream oss;
		oss << std::setfill('0')
			<< std::setw(2) << hour << ":"
			<< std::setw(2) << minute << ":"
			<< std::setw(2) << second << "."
			<< std::setw(3) << millisecond;
		return oss.str();
	}

	uint64_t make_tick_key(const lt::tick_info& tick)
	{
		return (static_cast<uint64_t>(tick.time) << 32U) | static_cast<uint64_t>(tick.volume);
	}

	bool is_price_step_aligned(double price, double price_step)
	{
		if (price_step <= 0.0)
		{
			return true;
		}
		const double scaled = price / price_step;
		return std::fabs(scaled - std::round(scaled)) < 1e-6;
	}

	uint32_t get_max_single_order_volume(const lt::code_t& code, const std::string& side)
	{
		if ((side == "buy_open" || side == "sell_open") && code.to_string().rfind("SHFE.ag", 0) == 0)
		{
			return kAgOpenMaxSingleOrderVolume;
		}
		return kDefaultMaxSingleOrderVolume;
	}

	bool should_write_test_record(const std::string& category)
	{
		return category == "trade"
			|| category == "system"
			|| category == "monitor"
			|| category == "risk"
			|| category == "error";
	}
}

class gui_logic : public lt::trading_context::order_listener, public lt::tick_receiver
{
public:
	struct order_row
	{
		lt::estid_t estid;
		std::string code;
		std::string offset;
		std::string direction;
		double price;
		uint32_t total_volume;
		uint32_t last_volume;
		lt::daytm_t create_time;
	};

	struct position_row
	{
		std::string code;
		uint32_t current_long;
		uint32_t current_short;
		uint32_t history_long;
		uint32_t history_short;
		uint32_t long_frozen;
		uint32_t short_frozen;
		uint32_t long_pending;
		uint32_t short_pending;
	};

	struct market_level_row
	{
		std::string label;
		double price;
		uint32_t volume;
	};

	struct tick_row
	{
		std::string time_text;
		double price;
		int32_t delta_volume;
		double open_interest;
		double bid_price;
		double ask_price;
	};

	struct snapshot
	{
		std::vector<std::string> instruments;
		std::vector<order_row> orders;
		std::vector<position_row> positions;
		std::vector<std::string> logs;
		std::string status;
		std::string connection_status;
		std::string focused_code;
		uint32_t trading_day;
		lt::daytm_t last_time;
		uint32_t order_submit_count;
		uint32_t cancel_request_count;
		uint32_t duplicate_warning_count;
		uint32_t reject_count;
		uint64_t error_event_id;
		std::string error_message;
		uint32_t order_threshold;
		uint32_t cancel_threshold;
		bool order_threshold_alert;
		bool cancel_threshold_alert;
		bool trading_paused;
		double last_price;
		double average_price;
		double open_interest;
		double open_price;
		double high_price;
		double low_price;
		double control_price;
		std::vector<market_level_row> market_levels;
		std::vector<tick_row> recent_ticks;
	};

public:
	gui_logic(const char* account_config, const char* control_config, const char* section_config);
	~gui_logic();

	snapshot get_snapshot() const;
	void append_test_record(const std::string& category, const std::string& message);
	bool is_started() const;
	void start_trading();
	void stop_trading();
	void post_command(const std::string& command);

private:
	virtual void on_entrust(const lt::order_info& order) override;
	virtual void on_deal(lt::estid_t estid, uint32_t deal_volume) override;
	virtual void on_trade(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t volume) override;
	virtual void on_cancel(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t cancel_volume, uint32_t total_volume) override;
	virtual void on_error(lt::error_type type, lt::estid_t estid, const lt::error_code error) override;
	virtual void on_tick(const lt::tick_info& tick) override;

private:
	void worker_loop();
	bool process_command_queue();
	void handle_command(const lt::params& p);
	void subscribe_favorites();
	void unsubscribe_favorites();
	void refresh_snapshot();
	void update_focus(const lt::code_t& code);
	void set_status(const std::string& status);
	void publish_error(const std::string& message);
	void append_log(const std::string& category, const std::string& message);
	void open_log_file();
	void update_connection_status();
	void update_threshold_flags();
	void pad_display_ticks(std::vector<tick_row>& ticks, size_t target_count) const;

	lt::estid_t buy_open(const lt::code_t& code, uint32_t count, double price = 0.0, lt::order_flag flag = lt::order_flag::OF_NOR);
	lt::estid_t sell_open(const lt::code_t& code, uint32_t count, double price = 0.0, lt::order_flag flag = lt::order_flag::OF_NOR);
	lt::estid_t buy_close(const lt::code_t& code, uint32_t count, double price = 0.0, bool close_today = false, lt::order_flag flag = lt::order_flag::OF_NOR);
	lt::estid_t sell_close(const lt::code_t& code, uint32_t count, double price = 0.0, bool close_today = false, lt::order_flag flag = lt::order_flag::OF_NOR);

	static std::string to_string(lt::offset_type offset);
	static std::string to_string(lt::direction_type direction);
	static std::string to_string(lt::error_type type);
	static std::string to_string(lt::error_code error);
	static lt::order_flag parse_order_flag(const std::string& flag_text);

private:
	mutable std::mutex _snapshot_mutex;
	snapshot _snapshot;
	std::mutex _command_mutex;
	std::queue<std::string> _command_queue;
	lt::actual_market* _market = nullptr;
	lt::actual_trader* _trader = nullptr;
	lt::trading_context* _ctx = nullptr;
	lt::data_channel* _dc = nullptr;
	std::thread _worker;
	std::atomic<bool> _running { false };
	std::atomic<bool> _servicing { false };
	std::atomic<bool> _subscribed { false };
	std::atomic<uint32_t> _loop_interval_us { 1000U };
	std::unordered_map<std::string, lt::daytm_t> _recent_orders;
	std::unordered_map<std::string, uint64_t> _last_tick_keys;
	std::unordered_map<std::string, uint32_t> _last_tick_volumes;
	std::unordered_map<std::string, std::vector<tick_row>> _tick_history;
	std::unordered_set<std::string> _tick_seen_codes;
	std::chrono::steady_clock::time_point _start_time;
	bool _no_tick_warning_emitted = false;
	std::ofstream _log_file;
	std::ofstream _test_log_file;
	std::filesystem::path _log_path;
	std::filesystem::path _test_log_path;
	std::vector<lt::code_t> _favorites;
};

gui_logic::gui_logic(const char* account_config, const char* control_config, const char* section_config)
	: _start_time(std::chrono::steady_clock::now())
{
	for (const auto* code : kFavoriteContracts)
	{
		_favorites.emplace_back(code);
		_snapshot.instruments.emplace_back(code);
	}
	_snapshot.status = "waiting for trading service";
	_snapshot.connection_status = "starting";
	_snapshot.focused_code = _favorites.front().to_string();
	_snapshot.trading_day = 0U;
	_snapshot.last_time = 0U;
	_snapshot.order_submit_count = 0U;
	_snapshot.cancel_request_count = 0U;
	_snapshot.duplicate_warning_count = 0U;
	_snapshot.reject_count = 0U;
	_snapshot.error_event_id = 0U;
	_snapshot.error_message.clear();
	_snapshot.order_threshold = 100U;
	_snapshot.cancel_threshold = 100U;
	_snapshot.order_threshold_alert = false;
	_snapshot.cancel_threshold_alert = false;
	_snapshot.trading_paused = false;
	_snapshot.last_price = .0;
	_snapshot.average_price = .0;
	_snapshot.open_interest = .0;
	_snapshot.open_price = .0;
	_snapshot.high_price = .0;
	_snapshot.low_price = .0;
	_snapshot.control_price = .0;

	inipp::Ini<char> account_ini;
	std::ifstream account_stream(account_config);
	if (!account_stream.is_open())
	{
		throw std::runtime_error(std::string("cannot open account config: ") + account_config);
	}
	account_ini.parse(account_stream);

	auto market_it = account_ini.sections.find("actual_market");
	auto trader_it = account_ini.sections.find("actual_trader");
	if (market_it == account_ini.sections.end() || trader_it == account_ini.sections.end())
	{
		throw std::runtime_error("missing actual_market or actual_trader section");
	}

	_market = create_actual_market(lt::params(market_it->second));
	_trader = create_actual_trader(lt::params(trader_it->second));
	if (_market == nullptr || _trader == nullptr)
	{
		throw std::runtime_error("failed to create actual_market or actual_trader");
	}

	_ctx = new lt::trading_context(_market, _trader, section_config);

	inipp::Ini<char> control_ini;
	std::ifstream control_stream(control_config);
	if (control_stream.is_open())
	{
		control_ini.parse(control_stream);
		auto control_it = control_ini.sections.find("control");
		if (control_it != control_ini.sections.end())
		{
			lt::params control_params(control_it->second);
			if (control_params.has("loop_interval"))
			{
				_loop_interval_us = control_params.get<uint32_t>("loop_interval");
			}
		}
	}

	auto ltds_it = control_ini.sections.find("ltds");
	if (ltds_it == control_ini.sections.end())
	{
		throw std::runtime_error("missing [ltds] section");
	}
	lt::params ltds_params(ltds_it->second);
	const std::string channel = ltds_params.get<std::string>("channel");
	const std::string cache_path = ltds_params.get<std::string>("cache_path");
	const size_t detail_cache = ltds_params.has("detail_cache_size") ? ltds_params.get<uint32_t>("detail_cache_size") : 2U;
	const size_t bar_cache = ltds_params.has("bar_cache_size") ? ltds_params.get<uint32_t>("bar_cache_size") : 8U;
	_dc = new lt::data_channel(_ctx, channel.c_str(), cache_path.c_str(), detail_cache, bar_cache);

	open_log_file();
	append_log("system", "demo-gui logic initialized");

	_running = true;
	_worker = std::thread([this]() { worker_loop(); });
}

gui_logic::~gui_logic()
{
	stop_trading();
	_running = false;
	if (_worker.joinable())
	{
		_worker.join();
	}
	if (_dc != nullptr)
	{
		delete _dc;
		_dc = nullptr;
	}
	if (_ctx != nullptr)
	{
		delete _ctx;
		_ctx = nullptr;
	}
	if (_market != nullptr)
	{
		destory_actual_market(_market);
	}
	if (_trader != nullptr)
	{
		destory_actual_trader(_trader);
	}
}

gui_logic::snapshot gui_logic::get_snapshot() const
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	return _snapshot;
}

void gui_logic::append_test_record(const std::string& category, const std::string& message)
{
	if (!_test_log_file.is_open() || !should_write_test_record(category))
	{
		return;
	}
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_test_log_file << "[external-" << category << "] " << message << std::endl;
}

bool gui_logic::is_started() const
{
	return _servicing.load();
}

void gui_logic::start_trading()
{
	if (_servicing)
	{
		return;
	}

	_start_time = std::chrono::steady_clock::now();
	_no_tick_warning_emitted = false;
	_tick_seen_codes.clear();
	set_status("logging in");
	append_log("system", "starting trading service");

	if (!_trader || !_trader->login())
	{
		set_status("trader login failed");
		publish_error("trader login failed");
		append_log("error", "trader login failed");
		refresh_snapshot();
		return;
	}
	if (!_market || !_market->login())
	{
		set_status("market login failed");
		publish_error("market login failed");
		append_log("error", "market login failed");
		_trader->logout();
		refresh_snapshot();
		return;
	}
	if (!_ctx->load_data())
	{
		set_status("load data failed");
		publish_error("load data failed");
		append_log("error", "load data failed");
		_market->logout();
		_trader->logout();
		refresh_snapshot();
		return;
	}

	subscribe_favorites();
	_servicing = true;
	set_status("trading service started");
	append_log("system", "trading service started");
	refresh_snapshot();
}

void gui_logic::stop_trading()
{
	if (_servicing)
	{
		unsubscribe_favorites();
		_servicing = false;
	}
	if (_trader)
	{
		_trader->logout();
	}
	if (_market)
	{
		_market->logout();
	}
	set_status("trading service stopped");
	append_log("system", "trading service stopped");
	refresh_snapshot();
}

void gui_logic::post_command(const std::string& command)
{
	std::lock_guard<std::mutex> lock(_command_mutex);
	_command_queue.push(command);
}

void gui_logic::worker_loop()
{
	while (_running)
	{
		bool did_update = process_command_queue();
		if (_servicing)
		{
			did_update |= _ctx->polling();
			did_update |= _dc->polling();

			if (!_no_tick_warning_emitted &&
				std::chrono::steady_clock::now() - _start_time > std::chrono::seconds(15) &&
				_tick_seen_codes.empty())
			{
				std::ostringstream oss;
				oss << "no tick received yet for favorites: ";
				for (size_t index = 0; index < _favorites.size(); ++index)
				{
					if (index > 0)
					{
						oss << ", ";
					}
					oss << _favorites[index].to_string();
				}
				append_log("market", oss.str());
				_no_tick_warning_emitted = true;
			}

			update_connection_status();
			if (did_update)
			{
				refresh_snapshot();
			}
		}
		else if (did_update)
		{
			refresh_snapshot();
		}

		std::this_thread::sleep_for(std::chrono::microseconds(_loop_interval_us.load()));
	}
}

bool gui_logic::process_command_queue()
{
	std::queue<std::string> commands;
	{
		std::lock_guard<std::mutex> lock(_command_mutex);
		std::swap(commands, _command_queue);
	}

	bool changed = false;
	while (!commands.empty())
	{
		handle_command(lt::params(commands.front()));
		commands.pop();
		changed = true;
	}
	return changed;
}

void gui_logic::pad_display_ticks(std::vector<tick_row>& ticks, size_t target_count) const
{
	if (ticks.empty() || ticks.size() >= target_count)
	{
		return;
	}
	const auto seed = ticks.back();
	while (ticks.size() < target_count)
	{
		auto filler = seed;
		filler.time_text.clear();
		filler.delta_volume = 0;
		ticks.push_back(filler);
	}
}

void gui_logic::handle_command(const lt::params& p)
{
	try
	{
		const auto action = p.get<std::string>("action");
		if (action == "order")
		{
			const auto code = p.get<lt::code_t>("code");
			const auto side = p.get<std::string>("side");
			const auto volume = p.get<uint32_t>("volume");
			const auto price = p.get<double_t>("price");
			const auto close_today = p.has("close_today") ? p.get<bool>("close_today") : false;

			if (get_snapshot().trading_paused)
			{
				{
					std::lock_guard<std::mutex> lock(_snapshot_mutex);
					_snapshot.reject_count++;
				}
				update_threshold_flags();
				set_status("trading is paused");
				publish_error("trading is paused");
				append_log("risk", "rejected order because trading is paused");
				refresh_snapshot();
				return;
			}

			if (volume == 0U)
			{
				throw std::invalid_argument("volume must be greater than zero");
			}
			if (price < .0)
			{
				throw std::invalid_argument("price must not be negative");
			}
			const auto& instrument = _ctx->get_instrument(code);
			if (instrument.code == lt::default_code)
			{
				throw std::invalid_argument("contract not found");
			}
			if (price > 0.0 && instrument.price_step > 0.0 && !is_price_step_aligned(price, instrument.price_step))
			{
				std::ostringstream oss;
				oss << "price does not match minimum tick: step=" << instrument.price_step;
				throw std::invalid_argument(oss.str());
			}
			const uint32_t max_single_order_volume = get_max_single_order_volume(code, side);
			if (volume > max_single_order_volume)
			{
				std::ostringstream oss;
				oss << "single order volume exceeds max limit: max=" << max_single_order_volume;
				throw std::invalid_argument(oss.str());
			}
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				if (_snapshot.order_submit_count >= _snapshot.order_threshold)
				{
					throw std::invalid_argument("order limit reached");
				}
			}

			const std::string order_signature = code.to_string() + "|" + side + "|" + std::to_string(volume) + "|" + std::to_string(price);
			const auto now = _ctx->get_last_time();
			auto dup_it = _recent_orders.find(order_signature);
			if (dup_it != _recent_orders.end() && now >= dup_it->second && now - dup_it->second <= 1000U)
			{
				{
					std::lock_guard<std::mutex> lock(_snapshot_mutex);
					_snapshot.duplicate_warning_count++;
				}
				append_log("monitor", "duplicate order pattern detected: " + order_signature);
			}
			_recent_orders[order_signature] = now;

			lt::estid_t estid = INVALID_ESTID;
			if (side == "buy_open") estid = buy_open(code, volume, price);
			else if (side == "sell_open") estid = sell_open(code, volume, price);
			else if (side == "buy_close") estid = buy_close(code, volume, price, close_today);
			else if (side == "sell_close") estid = sell_close(code, volume, price, close_today);
			else throw std::invalid_argument("unsupported side");

			if (estid == INVALID_ESTID)
			{
				{
					std::lock_guard<std::mutex> lock(_snapshot_mutex);
					_snapshot.reject_count++;
				}
				update_threshold_flags();
				set_status("order rejected by framework");
				publish_error("order rejected by framework");
				append_log("error", "framework rejected order: " + order_signature);
			}
			else
			{
				{
					std::lock_guard<std::mutex> lock(_snapshot_mutex);
					_snapshot.order_submit_count++;
				}
				update_threshold_flags();
				std::ostringstream oss;
				oss << "order submitted estid=" << estid;
				set_status(oss.str());
				append_log("trade", oss.str() + " " + order_signature);
			}
		}
		else if (action == "cancel")
		{
			const auto estid = p.get<uint64_t>("estid");
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				if (_snapshot.cancel_request_count >= _snapshot.cancel_threshold)
				{
					throw std::invalid_argument("cancel limit reached");
				}
			}
			_ctx->cancel_order(estid);
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				_snapshot.cancel_request_count++;
			}
			update_threshold_flags();
			std::ostringstream oss;
			oss << "cancel requested estid=" << estid;
			set_status(oss.str());
			append_log("trade", oss.str());
		}
		else if (action == "batch_cancel")
		{
			const auto scope = p.has("scope") ? p.get<std::string>("scope") : std::string("all");
			const auto code = p.has("code") ? p.get<lt::code_t>("code") : lt::default_code;
			std::vector<lt::estid_t> estids;
			for (const auto& it : _ctx->get_orders())
			{
				if (scope == "contract" && it.second.code != code)
				{
					continue;
				}
				estids.emplace_back(it.first);
			}
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				if (_snapshot.cancel_request_count + static_cast<uint32_t>(estids.size()) > _snapshot.cancel_threshold)
				{
					throw std::invalid_argument("cancel limit would be exceeded");
				}
			}
			for (const auto estid : estids)
			{
				_ctx->cancel_order(estid);
			}
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				_snapshot.cancel_request_count += static_cast<uint32_t>(estids.size());
			}
			update_threshold_flags();
			std::ostringstream oss;
			oss << "batch cancel requested count=" << estids.size();
			set_status(oss.str());
			append_log("trade", oss.str());
		}
		else if (action == "pause")
		{
			const auto paused = p.get<bool>("value");
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				_snapshot.trading_paused = paused;
			}
			set_status(paused ? "trading paused" : "trading resumed");
			append_log("system", paused ? "trading paused" : "trading resumed");
		}
		else if (action == "close_all")
		{
			if (get_snapshot().trading_paused)
			{
				{
					std::lock_guard<std::mutex> lock(_snapshot_mutex);
					_snapshot.reject_count++;
				}
				update_threshold_flags();
				set_status("trading is paused");
				publish_error("trading is paused");
				append_log("risk", "rejected close all because trading is paused");
				refresh_snapshot();
				return;
			}

			uint32_t planned_count = 0U;
			for (const auto& it : _ctx->get_positions())
			{
				const auto& pos = it.second;
				if (pos.history_short.usable() > 0U) { planned_count++; }
				if (pos.current_short.usable() > 0U) { planned_count++; }
				if (pos.history_long.usable() > 0U) { planned_count++; }
				if (pos.current_long.usable() > 0U) { planned_count++; }
			}
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				if (_snapshot.order_submit_count + planned_count > _snapshot.order_threshold)
				{
					throw std::invalid_argument("order limit would be exceeded");
				}
			}

			uint32_t close_count = 0U;
			for (const auto& it : _ctx->get_positions())
			{
				const auto& code = it.first;
				const auto& pos = it.second;
				const auto& tick = _ctx->get_last_tick(code);
				const double buy_price = tick.sell_price() > 0.0 ? tick.sell_price() : tick.price;
				const double sell_price = tick.buy_price() > 0.0 ? tick.buy_price() : tick.price;
				if (pos.history_short.usable() > 0U && buy_close(code, pos.history_short.usable(), buy_price) != INVALID_ESTID) { close_count++; }
				if (pos.current_short.usable() > 0U && buy_close(code, pos.current_short.usable(), buy_price, true) != INVALID_ESTID) { close_count++; }
				if (pos.history_long.usable() > 0U && sell_close(code, pos.history_long.usable(), sell_price) != INVALID_ESTID) { close_count++; }
				if (pos.current_long.usable() > 0U && sell_close(code, pos.current_long.usable(), sell_price, true) != INVALID_ESTID) { close_count++; }
			}
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				_snapshot.order_submit_count += close_count;
			}
			update_threshold_flags();
			std::ostringstream oss;
			oss << "close all requested count=" << close_count;
			set_status(oss.str());
			append_log("trade", oss.str());
		}
		else if (action == "set_limits")
		{
			const auto order_threshold = p.get<uint32_t>("order_threshold");
			const auto cancel_threshold = p.get<uint32_t>("cancel_threshold");
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				_snapshot.order_threshold = order_threshold;
				_snapshot.cancel_threshold = cancel_threshold;
			}
			update_threshold_flags();
			std::ostringstream oss;
			oss << "thresholds updated order=" << order_threshold << " cancel=" << cancel_threshold;
			set_status(oss.str());
			append_log("monitor", oss.str());
		}
		else if (action == "focus")
		{
			update_focus(p.get<lt::code_t>("code"));
			set_status("focused contract updated");
		}
	}
	catch (const std::exception& ex)
	{
		{
			std::lock_guard<std::mutex> lock(_snapshot_mutex);
			_snapshot.reject_count++;
		}
		update_threshold_flags();
		set_status(std::string("command failed: ") + ex.what());
		publish_error(std::string("command failed: ") + ex.what());
		append_log("error", std::string("command failed: ") + ex.what());
	}

	refresh_snapshot();
}

void gui_logic::subscribe_favorites()
{
	if (_subscribed || _dc == nullptr)
	{
		return;
	}
	lt::subscriber suber(_dc);
	std::ostringstream oss;
	for (const auto& code : _favorites)
	{
		suber.regist_tick_receiver(code, this);
		if (oss.tellp() > 0)
		{
			oss << ", ";
		}
		oss << code.to_string();
	}
	suber.subscribe();
	_subscribed = true;
	append_log("market", "subscribed favorites: " + oss.str());
}

void gui_logic::unsubscribe_favorites()
{
	if (!_subscribed || _dc == nullptr)
	{
		return;
	}
	lt::unsubscriber unsuber(_dc);
	for (const auto& code : _favorites)
	{
		unsuber.unregist_tick_receiver(code, this);
	}
	unsuber.unsubscribe();
	_subscribed = false;
}

void gui_logic::on_entrust(const lt::order_info& order)
{
	std::ostringstream oss;
	oss << "entrusted " << order.code.to_string() << " estid=" << order.estid;
	set_status(oss.str());
	append_log("trade", oss.str());
	refresh_snapshot();
}

void gui_logic::on_deal(lt::estid_t estid, uint32_t deal_volume)
{
	std::ostringstream oss;
	oss << "dealt estid=" << estid << " volume=" << deal_volume;
	set_status(oss.str());
	append_log("trade", oss.str());
	refresh_snapshot();
}

void gui_logic::on_trade(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t volume)
{
	std::ostringstream oss;
	oss << "trade finished estid=" << estid
		<< " code=" << code.to_string()
		<< " offset=" << to_string(offset)
		<< " direction=" << to_string(direction)
		<< " price=" << price
		<< " volume=" << volume;
	set_status(oss.str());
	append_log("trade", oss.str());
	refresh_snapshot();
}

void gui_logic::on_cancel(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t cancel_volume, uint32_t total_volume)
{
	std::ostringstream oss;
	oss << "canceled estid=" << estid
		<< " code=" << code.to_string()
		<< " offset=" << to_string(offset)
		<< " direction=" << to_string(direction)
		<< " price=" << price
		<< " volume=" << cancel_volume
		<< "/" << total_volume;
	set_status(oss.str());
	append_log("trade", oss.str());
	refresh_snapshot();
}

void gui_logic::on_error(lt::error_type type, lt::estid_t estid, const lt::error_code error)
{
	const auto error_id = static_cast<uint32_t>(error);
	std::ostringstream oss;
	oss << "error type=" << to_string(type)
		<< " estid=" << estid
		<< " code=" << error_id << "(" << to_string(error) << ")";
	{
		std::lock_guard<std::mutex> lock(_snapshot_mutex);
		_snapshot.reject_count++;
	}
	update_threshold_flags();
	set_status(oss.str());
	publish_error(oss.str());
	append_log("error", oss.str());
	refresh_snapshot();
}

void gui_logic::on_tick(const lt::tick_info& tick)
{
	if (_snapshot.focused_code.empty())
	{
		update_focus(tick.code);
	}

	if (!tick.invalid())
	{
		const std::string code = tick.code.to_string();
		if (_tick_seen_codes.insert(code).second)
		{
			std::ostringstream oss;
			oss << "first tick received for " << code
				<< " price=" << tick.price
				<< " time=" << format_logic_time_text(tick.time);
			append_log("market", oss.str());
		}

		const auto tick_key = make_tick_key(tick);
		auto key_it = _last_tick_keys.find(code);
		if (key_it == _last_tick_keys.end() || key_it->second != tick_key)
		{
			const auto volume_it = _last_tick_volumes.find(code);
			const uint32_t prev_volume = volume_it == _last_tick_volumes.end() ? tick.volume : volume_it->second;
			const int32_t delta_volume = tick.volume >= prev_volume
				? static_cast<int32_t>(tick.volume - prev_volume)
				: static_cast<int32_t>(tick.volume);

			tick_row row;
			row.time_text = format_logic_time_text(tick.time);
			row.price = tick.price;
			row.delta_volume = delta_volume;
			row.open_interest = tick.open_interest;
			row.bid_price = tick.buy_price();
			row.ask_price = tick.sell_price();
			auto& history = _tick_history[code];
			history.insert(history.begin(), row);
			if (history.size() > 120U)
			{
				history.resize(120U);
			}
			_last_tick_keys[code] = tick_key;
			_last_tick_volumes[code] = tick.volume;
		}
	}

	refresh_snapshot();
}

void gui_logic::refresh_snapshot()
{
	snapshot new_snapshot;
	{
		std::lock_guard<std::mutex> lock(_snapshot_mutex);
		new_snapshot = _snapshot;
	}

	new_snapshot.trading_day = _ctx ? _ctx->get_trading_day() : 0U;
	new_snapshot.last_time = _ctx ? _ctx->get_last_time() : 0U;
	new_snapshot.instruments.clear();
	new_snapshot.orders.clear();
	new_snapshot.positions.clear();

	for (const auto& favorite : _favorites)
	{
		new_snapshot.instruments.emplace_back(favorite.to_string());
	}
	std::sort(new_snapshot.instruments.begin(), new_snapshot.instruments.end());
	if (new_snapshot.focused_code.empty() && !new_snapshot.instruments.empty())
	{
		new_snapshot.focused_code = new_snapshot.instruments.front();
	}

	if (_ctx)
	{
		for (const auto& it : _ctx->get_orders())
		{
			order_row row;
			row.estid = it.second.estid;
			row.code = it.second.code.to_string();
			row.offset = to_string(it.second.offset);
			row.direction = to_string(it.second.direction);
			row.price = it.second.price;
			row.total_volume = it.second.total_volume;
			row.last_volume = it.second.last_volume;
			row.create_time = it.second.create_time;
			new_snapshot.orders.emplace_back(row);
		}

		for (const auto& it : _ctx->get_positions())
		{
			position_row row;
			row.code = it.first.to_string();
			row.current_long = it.second.current_long.position;
			row.current_short = it.second.current_short.position;
			row.history_long = it.second.history_long.position;
			row.history_short = it.second.history_short.position;
			row.long_frozen = it.second.get_long_frozen();
			row.short_frozen = it.second.get_short_frozen();
			row.long_pending = it.second.long_pending;
			row.short_pending = it.second.short_pending;
			new_snapshot.positions.emplace_back(row);
		}
	}

	std::sort(new_snapshot.orders.begin(), new_snapshot.orders.end(), [](const order_row& lhs, const order_row& rhs) {
		return lhs.estid > rhs.estid;
	});
	std::sort(new_snapshot.positions.begin(), new_snapshot.positions.end(), [](const position_row& lhs, const position_row& rhs) {
		return lhs.code < rhs.code;
	});

	if (_ctx && !new_snapshot.focused_code.empty())
	{
		const lt::code_t focused_code(new_snapshot.focused_code.c_str());
		const auto& market = _ctx->get_market_info(focused_code);
		const auto& tick = _ctx->get_last_tick(focused_code);
		new_snapshot.last_price = tick.price;
		new_snapshot.average_price = tick.average_price;
		new_snapshot.open_interest = tick.open_interest;
		new_snapshot.open_price = market.open_price;
		new_snapshot.high_price = market.high_price;
		new_snapshot.low_price = market.low_price;
		new_snapshot.control_price = market.control_price();
		new_snapshot.market_levels.clear();

		for (size_t index = 0; index < tick.ask_order.size() && index < 5U; ++index)
		{
			market_level_row row;
			row.label = "Ask " + std::to_string(index + 1U);
			row.price = tick.ask_order[index].first;
			row.volume = tick.ask_order[index].second;
			new_snapshot.market_levels.emplace_back(row);
		}
		for (size_t reverse = std::min<size_t>(5U, tick.bid_order.size()); reverse > 0; --reverse)
		{
			const size_t index = reverse - 1U;
			market_level_row row;
			row.label = "Bid " + std::to_string(index + 1U);
			row.price = tick.bid_order[index].first;
			row.volume = tick.bid_order[index].second;
			new_snapshot.market_levels.emplace_back(row);
		}

		const auto history_it = _tick_history.find(new_snapshot.focused_code);
		new_snapshot.recent_ticks = history_it == _tick_history.end() ? std::vector<tick_row>{} : history_it->second;
		if (!tick.invalid() && new_snapshot.recent_ticks.empty())
		{
			tick_row row;
			row.time_text = format_logic_time_text(tick.time);
			row.price = tick.price;
			row.delta_volume = 0;
			row.open_interest = tick.open_interest;
			row.bid_price = tick.buy_price();
			row.ask_price = tick.sell_price();
			new_snapshot.recent_ticks.push_back(row);
		}
		pad_display_ticks(new_snapshot.recent_ticks, 32U);
	}

	{
		std::lock_guard<std::mutex> lock(_snapshot_mutex);
		_snapshot = std::move(new_snapshot);
	}
}

void gui_logic::update_focus(const lt::code_t& code)
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	if (_snapshot.focused_code != code.to_string())
	{
		_snapshot.focused_code = code.to_string();
		const auto history_it = _tick_history.find(_snapshot.focused_code);
		_snapshot.recent_ticks = history_it == _tick_history.end() ? std::vector<tick_row>{} : history_it->second;
	}
}

void gui_logic::set_status(const std::string& status)
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.status = status;
}

void gui_logic::publish_error(const std::string& message)
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.error_message = message;
	++_snapshot.error_event_id;
}

void gui_logic::append_log(const std::string& category, const std::string& message)
{
	const std::string line = format_logic_time_text(_ctx ? _ctx->get_last_time() : 0U) + " [" + category + "] " + message;
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.logs.emplace_back(line);
	if (_snapshot.logs.size() > 200U)
	{
		_snapshot.logs.erase(_snapshot.logs.begin(), _snapshot.logs.begin() + static_cast<long long>(_snapshot.logs.size() - 200U));
	}
	if (_log_file.is_open())
	{
		_log_file << line << std::endl;
	}
	if (_test_log_file.is_open() && should_write_test_record(category))
	{
		_test_log_file << line << std::endl;
	}
}

void gui_logic::open_log_file()
{
	try
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t current = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm = {};
		localtime_s(&local_tm, &current);

		std::ostringstream name;
		name << "demo-gui-" << std::put_time(&local_tm, "%Y%m%d") << ".log";
		_log_path = std::filesystem::current_path() / "logs" / name.str();

		std::ostringstream test_name;
		test_name << "demo-gui-test-record-" << std::put_time(&local_tm, "%Y%m%d") << ".log";
		_test_log_path = std::filesystem::current_path() / "logs" / test_name.str();

		std::filesystem::create_directories(_log_path.parent_path());
		_log_file.open(_log_path, std::ios::app);
		_test_log_file.open(_test_log_path, std::ios::app);
	}
	catch (...)
	{
	}
}

void gui_logic::update_connection_status()
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	if (_snapshot.trading_day != 0U)
	{
		_snapshot.connection_status = "connected";
	}
	else if (_servicing)
	{
		_snapshot.connection_status = "initializing";
	}
	else
	{
		_snapshot.connection_status = "starting";
	}
}

void gui_logic::update_threshold_flags()
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.order_threshold_alert = _snapshot.order_submit_count >= _snapshot.order_threshold;
	_snapshot.cancel_threshold_alert = _snapshot.cancel_request_count >= _snapshot.cancel_threshold;
}

lt::estid_t gui_logic::buy_open(const lt::code_t& code, uint32_t count, double price, lt::order_flag flag)
{
	return _ctx->place_order(this, lt::offset_type::OT_OPEN, lt::direction_type::DT_LONG, code, count, price, flag);
}

lt::estid_t gui_logic::sell_open(const lt::code_t& code, uint32_t count, double price, lt::order_flag flag)
{
	return _ctx->place_order(this, lt::offset_type::OT_OPEN, lt::direction_type::DT_SHORT, code, count, price, flag);
}

lt::estid_t gui_logic::buy_close(const lt::code_t& code, uint32_t count, double price, bool close_today, lt::order_flag flag)
{
	return _ctx->place_order(this, close_today ? lt::offset_type::OT_CLSTD : lt::offset_type::OT_CLOSE, lt::direction_type::DT_SHORT, code, count, price, flag);
}

lt::estid_t gui_logic::sell_close(const lt::code_t& code, uint32_t count, double price, bool close_today, lt::order_flag flag)
{
	return _ctx->place_order(this, close_today ? lt::offset_type::OT_CLSTD : lt::offset_type::OT_CLOSE, lt::direction_type::DT_LONG, code, count, price, flag);
}

std::string gui_logic::to_string(lt::offset_type offset)
{
	switch (offset)
	{
	case lt::offset_type::OT_OPEN:
		return "open";
	case lt::offset_type::OT_CLOSE:
		return "close";
	case lt::offset_type::OT_CLSTD:
		return "close_today";
	default:
		return "unknown";
	}
}

std::string gui_logic::to_string(lt::direction_type direction)
{
	switch (direction)
	{
	case lt::direction_type::DT_LONG:
		return "long";
	case lt::direction_type::DT_SHORT:
		return "short";
	default:
		return "unknown";
	}
}

std::string gui_logic::to_string(lt::error_type type)
{
	switch (type)
	{
	case lt::error_type::ET_PLACE_ORDER:
		return "place_order";
	case lt::error_type::ET_CANCEL_ORDER:
		return "cancel_order";
	case lt::error_type::ET_OTHER_ERROR:
		return "other";
	default:
		return "unknown";
	}
}

std::string gui_logic::to_string(lt::error_code error)
{
	switch (error)
	{
	case lt::error_code::EC_Success:
		return "success";
	case lt::error_code::EC_Failure:
		return "failure";
	case lt::error_code::EC_OrderFieldError:
		return "field_error";
	case lt::error_code::EC_PositionNotEnough:
		return "position_not_enough";
	case lt::error_code::EC_MarginNotEnough:
		return "margin_not_enough";
	case lt::error_code::EC_StateNotReady:
		return "state_not_ready";
	case static_cast<lt::error_code>(50):
		return "close_today_not_enough";
	case static_cast<lt::error_code>(51):
		return "close_yesterday_not_enough";
	default:
		return "unknown";
	}
}

lt::order_flag gui_logic::parse_order_flag(const std::string&)
{
	return lt::order_flag::OF_NOR;
}

class market_chart_ctrl : public CWnd
{
public:
	void set_data(const std::vector<gui_logic::tick_row>& ticks, double last_price, double average_price)
	{
		std::ostringstream oss;
		oss.setf(std::ios::fixed);
		oss.precision(4);
		oss << "L" << last_price << "|A" << average_price;
		for (const auto& tick : ticks)
		{
			oss << "|" << tick.time_text << ":" << tick.price;
		}
		const std::string signature = oss.str();
		if (signature == _signature)
		{
			return;
		}
		_signature = signature;
		_ticks = ticks;
		_last_price = last_price;
		_average_price = average_price;
		if (::IsWindow(GetSafeHwnd()))
		{
			Invalidate();
		}
	}

protected:
	afx_msg void OnPaint()
	{
		CPaintDC dc(this);
		CRect rect;
		GetClientRect(&rect);
		dc.FillSolidRect(rect, RGB(250, 250, 250));
		dc.DrawEdge(rect, EDGE_SUNKEN, BF_RECT);

		rect.DeflateRect(10, 10);
		if (rect.Width() <= 20 || rect.Height() <= 20)
		{
			return;
		}

		CPen grid_pen(PS_SOLID, 1, RGB(230, 230, 230));
		CPen* old_pen = dc.SelectObject(&grid_pen);
		for (int index = 1; index < 4; ++index)
		{
			const int y = rect.top + (rect.Height() * index) / 4;
			dc.MoveTo(rect.left, y);
			dc.LineTo(rect.right, y);
		}
		dc.SelectObject(old_pen);

		std::vector<double> prices;
		prices.reserve(_ticks.size() + (_last_price > 0.0 ? 1U : 0U));
		for (auto it = _ticks.rbegin(); it != _ticks.rend(); ++it)
		{
			if (it->price > 0.0)
			{
				prices.emplace_back(it->price);
			}
		}
		if (prices.empty() && _last_price > 0.0)
		{
			prices.emplace_back(_last_price);
		}

		if (prices.empty())
		{
			dc.SetTextColor(RGB(120, 120, 120));
			dc.SetBkMode(TRANSPARENT);
			dc.DrawText(_T("Waiting for tick data"), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			return;
		}

		double min_price = prices.front();
		double max_price = prices.front();
		for (const auto price : prices)
		{
			min_price = std::min(min_price, price);
			max_price = std::max(max_price, price);
		}
		double range = max_price - min_price;
		if (range < 0.0001)
		{
			const double reference = std::max(1.0, std::fabs(prices.back()) * 0.002);
			max_price += reference;
			min_price -= reference;
			range = max_price - min_price;
		}
		const double padding = std::max(0.5, range * 0.15);
		min_price -= padding;
		max_price += padding;

		auto price_to_y = [&](double price) {
			const double ratio = (price - min_price) / (max_price - min_price);
			return rect.bottom - static_cast<int>(ratio * rect.Height());
		};

		if (_average_price > min_price && _average_price < max_price)
		{
			CPen avg_pen(PS_DASH, 1, RGB(180, 180, 180));
			old_pen = dc.SelectObject(&avg_pen);
			const int y = price_to_y(_average_price);
			dc.MoveTo(rect.left, y);
			dc.LineTo(rect.right, y);
			dc.SelectObject(old_pen);
		}

		CPen line_pen(PS_SOLID, 2, RGB(0, 120, 215));
		old_pen = dc.SelectObject(&line_pen);
		for (size_t index = 0; index < prices.size(); ++index)
		{
			const int x = rect.left + static_cast<int>((rect.Width() * index) / std::max<size_t>(1, prices.size() - 1));
			const int y = price_to_y(prices[index]);
			if (index == 0)
			{
				dc.MoveTo(x, y);
			}
			else
			{
				dc.LineTo(x, y);
			}
		}
		dc.SelectObject(old_pen);

		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(80, 80, 80));
		CString header;
		header.Format(_T("Last %.4f   Avg %.4f"), _last_price, _average_price);
		dc.TextOut(rect.left, rect.top - 2, header);
	}

	DECLARE_MESSAGE_MAP()

private:
	std::vector<gui_logic::tick_row> _ticks;
	double _last_price = 0.0;
	double _average_price = 0.0;
	std::string _signature;
};

BEGIN_MESSAGE_MAP(market_chart_ctrl, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

class demo_gui_frame : public CFrameWnd
{
public:
	demo_gui_frame()
	{
		try
		{
			const auto runtime_config = resolve_config_path("runtime_ctpdev.ini");
			const auto control_config = resolve_config_path("control_gui.ini");
			const auto section_config = resolve_config_path("section_alltrading.csv");
			_logic = std::make_shared<gui_logic>(runtime_config.string().c_str(), control_config.string().c_str(), section_config.string().c_str());
		}
		catch (const std::exception& ex)
		{
			AfxMessageBox(to_cstring(ex.what()), MB_ICONERROR | MB_OK);
			throw;
		}
	}

	virtual ~demo_gui_frame()
	{
		stop_logic();
	}

	bool is_service_started() const
	{
		return _logic && _logic->is_started() && _trading_ready && !_trading_starting;
	}

	bool has_startup_failed() const
	{
		if (_logic == nullptr || _trading_ready || _trading_starting)
		{
			return false;
		}

		const auto snapshot = _logic->get_snapshot();
		return snapshot.error_event_id > 0 || snapshot.status.find("failed") != std::string::npos;
	}

protected:
	afx_msg int OnCreate(LPCREATESTRUCT create_struct)
	{
		if (CFrameWnd::OnCreate(create_struct) == -1)
		{
			return -1;
		}

		_font.CreatePointFont(95, _T("Segoe UI"));
		create_controls();
		layout_controls();
		start_logic();
		SetTimer(UI_TIMER_ID, UI_REFRESH_INTERVAL_MS, nullptr);
		refresh_ui();
		return 0;
	}

	afx_msg void OnDestroy()
	{
		KillTimer(UI_TIMER_ID);
		stop_logic();
		CFrameWnd::OnDestroy();
	}

	afx_msg void OnClose()
	{
		DestroyWindow();
	}

	afx_msg void OnSize(UINT type, int cx, int cy)
	{
		CFrameWnd::OnSize(type, cx, cy);
		if (::IsWindow(_tab_main.GetSafeHwnd()))
		{
			layout_controls();
		}
	}

	afx_msg void OnGetMinMaxInfo(MINMAXINFO* info)
	{
		if (info != nullptr)
		{
			info->ptMinTrackSize.x = 1380;
			info->ptMinTrackSize.y = 860;
		}
		CFrameWnd::OnGetMinMaxInfo(info);
	}

	afx_msg void OnTimer(UINT_PTR id)
	{
		if (id == UI_TIMER_ID)
		{
			refresh_ui();
		}
		CFrameWnd::OnTimer(id);
	}

	afx_msg void OnTabChanged(NMHDR* notify, LRESULT* result)
	{
		(void)notify;
		show_tab_page();
		layout_controls();
		*result = 0;
	}

	afx_msg void OnTopTabChanged(NMHDR* notify, LRESULT* result)
	{
		(void)notify;
		show_top_right_page();
		layout_controls();
		*result = 0;
	}

	afx_msg void OnFavoriteChanged()
	{
		const int index = _list_favorites.GetCurSel();
		if (index == LB_ERR)
		{
			return;
		}

		CString text;
		_list_favorites.GetText(index, text);
		const auto code = to_std_string(text);
		set_market_focus_code(code, true);
		set_order_contract_code(code);
	}

	afx_msg void OnContractChanged()
	{
		CString text;
		_combo_code.GetWindowText(text);
		const auto code = to_std_string(text);
		if (!code.empty())
		{
			set_order_contract_code(code);
		}
	}

	afx_msg void OnSideChanged()
	{
		update_close_today_visibility();
	}

	afx_msg void OnOrderSelectionChanged(NMHDR* notify, LRESULT* result)
	{
		auto* info = reinterpret_cast<LPNMLISTVIEW>(notify);
		if (info != nullptr && (info->uNewState & LVIS_SELECTED) != 0 && info->iItem >= 0)
		{
			const auto code = to_std_string(_list_orders.GetItemText(info->iItem, 1));
			if (!code.empty())
			{
				set_order_contract_code(code);
			}
		}
		*result = 0;
	}

	afx_msg void OnPositionSelectionChanged(NMHDR* notify, LRESULT* result)
	{
		auto* info = reinterpret_cast<LPNMLISTVIEW>(notify);
		if (info != nullptr && (info->uNewState & LVIS_SELECTED) != 0 && info->iItem >= 0)
		{
			const auto code = to_std_string(_list_positions.GetItemText(info->iItem, 0));
			if (!code.empty())
			{
				set_order_contract_code(code);
			}
		}
		*result = 0;
	}

	afx_msg void OnSendOrder()
	{
		submit_order();
	}

	afx_msg void OnCancelSelected()
	{
		cancel_selected_order();
	}

	afx_msg void OnCancelAll()
	{
		batch_cancel(false);
	}

	afx_msg void OnCloseAll()
	{
		close_all_positions();
	}

	afx_msg void OnPauseTrading()
	{
		toggle_pause();
	}

	afx_msg void OnSetLimits()
	{
		set_limits();
	}

	afx_msg void OnLogFilterChanged()
	{
		refresh_ui();
	}

	DECLARE_MESSAGE_MAP()

private:
	void create_controls()
	{
		const DWORD group_style = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
		const DWORD label_style = WS_CHILD | WS_VISIBLE;
		const DWORD combo_list_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL;
		const DWORD combo_edit_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | WS_VSCROLL;
		const DWORD edit_style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
		const DWORD button_style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;

		_group_quick.Create(_T("Quick Order"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_QUICK);
		_group_market.Create(_T("Market Depth"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_MARKET);
		_group_ticks.Create(_T("Tick Stream"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_TICKS);
		_group_risk.Create(_T("Risk"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_RISK);
		_group_monitor.Create(_T("Monitor"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_MONITOR);
		_group_favorites.Create(_T("Favorite Contracts"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_FAVORITES);
		_group_orders.Create(_T("Orders"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_ORDERS);
		_group_positions.Create(_T("Positions"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_POSITIONS);
		_group_logs.Create(_T("Activity Log"), group_style, CRect(0, 0, 0, 0), this, IDC_GROUP_LOGS);

		_label_contract.Create(_T("Contract"), label_style, CRect(0, 0, 0, 0), this, IDC_LABEL_CONTRACT);
		_label_side.Create(_T("Side"), label_style, CRect(0, 0, 0, 0), this, IDC_LABEL_SIDE);
		_label_volume.Create(_T("Volume"), label_style, CRect(0, 0, 0, 0), this, IDC_LABEL_VOLUME);
		_label_price.Create(_T("Price"), label_style, CRect(0, 0, 0, 0), this, IDC_LABEL_PRICE);
		_label_order_limit.Create(_T("Order Limit"), label_style, CRect(0, 0, 0, 0), this, IDC_LABEL_ORDER_LIMIT);
		_label_cancel_limit.Create(_T("Cancel Limit"), label_style, CRect(0, 0, 0, 0), this, IDC_LABEL_CANCEL_LIMIT);
		_risk_divider.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, CRect(0, 0, 0, 0), this, IDC_STATIC_RISK_DIVIDER);

		_combo_code.Create(combo_edit_style, CRect(0, 0, 0, 260), this, IDC_COMBO_CODE);
		_combo_side.Create(combo_list_style, CRect(0, 0, 0, 220), this, IDC_COMBO_SIDE);
		_combo_side.AddString(_T("Buy Open"));
		_combo_side.AddString(_T("Sell Open"));
		_combo_side.AddString(_T("Buy Close"));
		_combo_side.AddString(_T("Sell Close"));
		_combo_side.SetCurSel(0);

		_edit_volume.Create(edit_style, CRect(0, 0, 0, 0), this, IDC_EDIT_VOLUME);
		_edit_volume.SetWindowText(_T("1"));
		_edit_price.Create(edit_style, CRect(0, 0, 0, 0), this, IDC_EDIT_PRICE);
		_edit_price.SetWindowText(_T("0"));
		_list_favorites.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, CRect(0, 0, 0, 0), this, IDC_LIST_FAVORITES);
		for (const auto* code : kFavoriteContracts)
		{
			const CString item = to_cstring(code);
			_list_favorites.AddString(item);
			_combo_code.AddString(item);
		}
		_list_favorites.SetCurSel(0);
		_combo_code.SetCurSel(0);
		_chart_market.Create(nullptr, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(0, 0, 0, 0), this, IDC_CHART_MARKET);

		_list_ticks.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, CRect(0, 0, 0, 0), this, IDC_LIST_TICKS);
		_list_ticks.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		_list_ticks.InsertColumn(0, _T("Time"), LVCFMT_LEFT, 110);
		_list_ticks.InsertColumn(1, _T("Price"), LVCFMT_RIGHT, 90);
		_list_ticks.InsertColumn(2, _T("Delta"), LVCFMT_RIGHT, 80);
		_list_ticks.InsertColumn(3, _T("OI"), LVCFMT_RIGHT, 90);
		_list_ticks.InsertColumn(4, _T("Bid1"), LVCFMT_RIGHT, 90);
		_list_ticks.InsertColumn(5, _T("Ask1"), LVCFMT_RIGHT, 90);

		_check_close_today.Create(_T("Close Today"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, CRect(0, 0, 0, 0), this, IDC_CHECK_CLOSE_TODAY);
		_button_order.Create(_T("Insert Order"), button_style, CRect(0, 0, 0, 0), this, IDC_BUTTON_ORDER);
		_button_cancel.Create(_T("Cancel Selected"), button_style, CRect(0, 0, 0, 0), this, IDC_BUTTON_CANCEL);
		_button_cancel_all.Create(_T("Cancel All"), button_style, CRect(0, 0, 0, 0), this, IDC_BUTTON_BATCH_CANCEL_ALL);
		_button_close_all.Create(_T("Close All"), button_style, CRect(0, 0, 0, 0), this, IDC_BUTTON_CLOSE_ALL);
		_button_pause.Create(_T("Pause Trading"), button_style, CRect(0, 0, 0, 0), this, IDC_BUTTON_PAUSE);
		_button_set_limits.Create(_T("Apply Limits"), button_style, CRect(0, 0, 0, 0), this, IDC_BUTTON_SET_LIMITS);

		_edit_order_limit.Create(edit_style, CRect(0, 0, 0, 0), this, IDC_EDIT_ORDER_LIMIT);
		_edit_order_limit.SetWindowText(_T("100"));
		_edit_cancel_limit.Create(edit_style, CRect(0, 0, 0, 0), this, IDC_EDIT_CANCEL_LIMIT);
		_edit_cancel_limit.SetWindowText(_T("100"));

		_tab_top_right.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, CRect(0, 0, 0, 0), this, IDC_TAB_TOP_RIGHT);
		_tab_top_right.InsertItem(0, _T("Risk"));
		_tab_top_right.InsertItem(1, _T("Monitor"));
		_tab_top_right.SetCurSel(0);

		_tab_main.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, CRect(0, 0, 0, 0), this, IDC_TAB_MAIN);
		_tab_main.InsertItem(0, _T("Orders / Positions"));
		_tab_main.InsertItem(1, _T("Activity Log"));
		_tab_main.SetCurSel(0);

		_list_orders.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, CRect(0, 0, 0, 0), this, IDC_LIST_ORDERS);
		_list_orders.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		_list_orders.InsertColumn(0, _T("EstId"), LVCFMT_LEFT, 115);
		_list_orders.InsertColumn(1, _T("Contract"), LVCFMT_LEFT, 120);
		_list_orders.InsertColumn(2, _T("Offset"), LVCFMT_LEFT, 80);
		_list_orders.InsertColumn(3, _T("Side"), LVCFMT_LEFT, 80);
		_list_orders.InsertColumn(4, _T("Price"), LVCFMT_RIGHT, 90);
		_list_orders.InsertColumn(5, _T("Total"), LVCFMT_RIGHT, 70);
		_list_orders.InsertColumn(6, _T("Left"), LVCFMT_RIGHT, 70);
		_list_orders.InsertColumn(7, _T("Time"), LVCFMT_LEFT, 120);

		_list_positions.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, CRect(0, 0, 0, 0), this, IDC_LIST_POSITIONS);
		_list_positions.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		_list_positions.InsertColumn(0, _T("Contract"), LVCFMT_LEFT, 90);
		_list_positions.InsertColumn(1, _T("Cur L"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(2, _T("Cur S"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(3, _T("His L"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(4, _T("His S"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(5, _T("Fz L"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(6, _T("Fz S"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(7, _T("Pd L"), LVCFMT_RIGHT, 56);
		_list_positions.InsertColumn(8, _T("Pd S"), LVCFMT_RIGHT, 56);

		_edit_logs.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, CRect(0, 0, 0, 0), this, IDC_EDIT_LOGS);
		_combo_log_level.Create(combo_list_style, CRect(0, 0, 0, 200), this, IDC_COMBO_LOG_LEVEL);
		_combo_log_level.AddString(_T("All"));
		_combo_log_level.AddString(_T("Trace"));
		_combo_log_level.AddString(_T("Debug"));
		_combo_log_level.AddString(_T("Info"));
		_combo_log_level.AddString(_T("Warning"));
		_combo_log_level.AddString(_T("Error"));
		_combo_log_level.AddString(_T("Fatal"));
		_combo_log_level.SetCurSel(0);

		_summary_label.Create(_T("summary"), label_style, CRect(0, 0, 0, 0), this, IDC_STATIC_SUMMARY);
		_monitor_label.Create(_T("monitor"), label_style, CRect(0, 0, 0, 0), this, IDC_STATIC_MONITOR);
		_hint_label.Create(_T("Tip: price=0 uses market-style handling when supported."), label_style, CRect(0, 0, 0, 0), this, IDC_STATIC_HINT);
		_status_label.Create(_T("starting..."), label_style, CRect(0, 0, 0, 0), this, IDC_STATIC_STATUS);

		apply_font(_group_quick);
		apply_font(_group_market);
		apply_font(_group_ticks);
		apply_font(_group_risk);
		apply_font(_group_monitor);
		apply_font(_group_favorites);
		apply_font(_group_orders);
		apply_font(_group_positions);
		apply_font(_group_logs);
		apply_font(_label_contract);
		apply_font(_label_side);
		apply_font(_label_volume);
		apply_font(_label_price);
		apply_font(_label_order_limit);
		apply_font(_label_cancel_limit);
		apply_font(_combo_code);
		apply_font(_combo_side);
		apply_font(_list_favorites);
		apply_font(_list_ticks);
		apply_font(_edit_volume);
		apply_font(_edit_price);
		apply_font(_check_close_today);
		apply_font(_button_order);
		apply_font(_button_cancel);
		apply_font(_button_cancel_all);
		apply_font(_button_close_all);
		apply_font(_button_pause);
		apply_font(_button_set_limits);
		apply_font(_edit_order_limit);
		apply_font(_edit_cancel_limit);
		apply_font(_tab_top_right);
		apply_font(_tab_main);
		apply_font(_list_orders);
		apply_font(_list_positions);
		apply_font(_edit_logs);
		apply_font(_combo_log_level);
		apply_font(_summary_label);
		apply_font(_monitor_label);
		apply_font(_hint_label);
		apply_font(_status_label);
		_tab_main.ShowWindow(SW_HIDE);
		update_close_today_visibility();
		show_top_right_page();
		show_tab_page();
	}

	void apply_font(CWnd& wnd)
	{
		wnd.SetFont(&_font);
	}

	void layout_controls()
	{
		CRect client;
		GetClientRect(&client);

		const int margin = 18;
		const int gap = 14;
		const int button_height = 28;
		const int input_height = 24;
		const int label_height = 18;
		const int tab_height = 28;
		const int status_height = 22;
		const int total_width = std::max(680, client.Width());
		const bool compact_top = total_width < 1320;
		const int client_height = std::max(720, client.Height());
		const int content_width = total_width - margin * 2;
		const int usable_height = client_height - margin * 2 - status_height;
		const int min_bottom_height = 240;
		const int min_top_height = compact_top ? 520 : 340;
		int top_group_height = std::max(min_top_height, (usable_height * 58) / 100);
		if (top_group_height > usable_height - min_bottom_height - gap)
		{
			top_group_height = std::max(min_top_height, usable_height - min_bottom_height - gap);
		}
		if (top_group_height < min_top_height)
		{
			top_group_height = min_top_height;
		}
		int bottom_height = usable_height - top_group_height - gap;
		if (bottom_height < min_bottom_height)
		{
			bottom_height = min_bottom_height;
			top_group_height = std::max(min_top_height, usable_height - bottom_height - gap);
		}
		const int status_y = margin + usable_height;

		int current_y = margin;
		const int top_y = current_y;
		const int favorites_width = compact_top ? content_width : 182;
		const int side_width = compact_top ? content_width : std::max(294, content_width * 25 / 100);
		const int main_width = compact_top ? content_width : content_width - favorites_width - side_width - gap * 2;
		const int right_x = compact_top ? margin : margin + favorites_width + gap + main_width + gap;
		const int main_x = compact_top ? margin : margin + favorites_width + gap;
		const int favorites_x = margin;

		if (compact_top)
		{
			const int favorites_height = 114;
			const int quick_height = 184;
			const int side_page_height = 146;
			const int top_tabs_block = tab_height + 8 + side_page_height;
			const int market_area_height = std::max(170, top_group_height - favorites_height - quick_height - top_tabs_block - gap * 3);
			const int market_height = std::max(120, (market_area_height * 34) / 100);
			const int tick_height = std::max(150, market_area_height - market_height - gap);
			const int tabs_y = top_y + favorites_height + gap + quick_height + gap + market_height + gap + tick_height + gap;

			_group_favorites.MoveWindow(favorites_x, top_y, content_width, favorites_height);
			_list_favorites.MoveWindow(favorites_x + 12, top_y + 24, content_width - 24, favorites_height - 36);

			_group_quick.MoveWindow(margin, top_y + favorites_height + gap, content_width, quick_height);

			const int market_y = top_y + favorites_height + gap + quick_height + gap;
			_group_market.MoveWindow(margin, market_y, content_width, market_height);
			_chart_market.MoveWindow(margin + 12, market_y + 24, content_width - 24, market_height - 36);

			_group_ticks.MoveWindow(margin, market_y + market_height + gap, content_width, tick_height);
			_list_ticks.MoveWindow(margin + 12, market_y + market_height + gap + 24, content_width - 24, tick_height - 36);

			_tab_top_right.MoveWindow(margin, tabs_y, content_width, tab_height);
			_group_risk.MoveWindow(margin, tabs_y + tab_height + 8, content_width, side_page_height);
			_group_monitor.MoveWindow(margin, tabs_y + tab_height + 8, content_width, side_page_height);
			current_y = top_y + top_group_height + gap;
		}
		else
		{
			_group_favorites.MoveWindow(favorites_x, top_y, favorites_width, top_group_height);
			_list_favorites.MoveWindow(favorites_x + 12, top_y + 24, favorites_width - 24, top_group_height - 36);

			const int market_height = std::max(150, top_group_height * 35 / 100);
			const int tick_height = top_group_height - market_height - gap;
			_group_market.MoveWindow(main_x, top_y, main_width, market_height);
			_chart_market.MoveWindow(main_x + 12, top_y + 24, main_width - 24, market_height - 36);
			_group_ticks.MoveWindow(main_x, top_y + market_height + gap, main_width, tick_height);
			_list_ticks.MoveWindow(main_x + 12, top_y + market_height + gap + 24, main_width - 24, tick_height - 36);

			const int order_height = std::max(182, top_group_height * 44 / 100);
			_group_quick.MoveWindow(right_x, top_y, side_width, order_height);
			_tab_top_right.MoveWindow(right_x, top_y + order_height + gap, side_width, tab_height);
			_group_risk.MoveWindow(right_x, top_y + order_height + gap + tab_height + 8, side_width, top_group_height - order_height - gap - tab_height - 8);
			_group_monitor.MoveWindow(right_x, top_y + order_height + gap + tab_height + 8, side_width, top_group_height - order_height - gap - tab_height - 8);
			current_y = top_y + top_group_height + gap;
		}

		const int order_x = compact_top ? margin : right_x;
		const int order_y = compact_top ? (top_y + 120 + gap) : top_y;
		const int order_width = compact_top ? content_width : side_width;
		const int label_x = order_x + 18;
		const int label_width = 48;
		const int value_x = order_x + 82;
		const int side_combo_width = compact_top ? 128 : 122;
		const int side_row_y = order_y + 64;
		const int close_today_width = 92;
		const int close_today_x = value_x + side_combo_width + 10;
		const int volume_label_x = compact_top ? (order_x + 204) : (order_x + 202);
		const int volume_edit_x = compact_top ? (order_x + 262) : (order_x + 260);
		const int volume_edit_width = 72;
		const int price_gap = 10;
		const int price_width = std::max(92, volume_label_x - value_x - 12);
		_label_contract.MoveWindow(label_x, order_y + 34, label_width, label_height);
		_combo_code.MoveWindow(order_x + 82, order_y + 30, order_width - 100, 220);
		_label_side.MoveWindow(label_x, order_y + 68, label_width, label_height);
		_combo_side.MoveWindow(value_x, side_row_y, side_combo_width, 220);
		_check_close_today.MoveWindow(close_today_x, side_row_y + 1, close_today_width, input_height);
		_label_price.MoveWindow(label_x, order_y + 102, label_width, label_height);
		_edit_price.MoveWindow(value_x, order_y + 98, price_width, input_height);
		_label_volume.MoveWindow(volume_label_x, order_y + 102, 48, label_height);
		_edit_volume.MoveWindow(volume_edit_x, order_y + 98, volume_edit_width, input_height);
		_button_order.MoveWindow(order_x + 18, order_y + 130, order_width - 36, button_height);
		_hint_label.MoveWindow(order_x + 18, order_y + 164, order_width - 36, label_height);

		const int risk_x = compact_top ? margin : right_x;
		const int risk_page_y = compact_top
			? (top_y + top_group_height - 128)
			: (top_y + std::max(164, top_group_height * 48 / 100) + gap + tab_height + 8);
		const int risk_width = compact_top ? content_width : side_width;
		const int risk_height = compact_top ? 146 : top_group_height - std::max(164, top_group_height * 48 / 100) - gap - tab_height - 8;
		const int risk_inner_width = risk_width - 32;
		const int risk_label_width = 68;
		const int risk_edit_width = compact_top ? 92 : 86;
		const int apply_width = compact_top ? 112 : 102;
		const int action_gap = 14;
		const int left_col_x = risk_x + 22;
		const int left_value_x = left_col_x + risk_label_width + 4;
		const int apply_x = risk_x + risk_width - 22 - apply_width;
		const int panel_content_offset = -22;
		const int row1_y = risk_page_y + 42 + panel_content_offset;
		const int row2_y = risk_page_y + 76 + panel_content_offset;
		const int divider_y = risk_page_y + 116 + panel_content_offset;
		const int button_row_y = risk_page_y + 128 + panel_content_offset;
		_label_order_limit.MoveWindow(left_col_x, row1_y + 4, risk_label_width, label_height);
		_edit_order_limit.MoveWindow(left_value_x, row1_y, risk_edit_width, input_height);
		_label_cancel_limit.MoveWindow(left_col_x, row2_y + 4, risk_label_width, label_height);
		_edit_cancel_limit.MoveWindow(left_value_x, row2_y, risk_edit_width, input_height);
		_button_set_limits.MoveWindow(apply_x, row1_y - 1, apply_width, button_height * 2 + 8);
		_risk_divider.MoveWindow(risk_x + 22, divider_y, risk_inner_width - 12, 2);
		const int risk_action_total_width = risk_inner_width - 12;
		const int risk_action_width = (risk_action_total_width - action_gap) / 2;
		_button_pause.MoveWindow(risk_x + 22, button_row_y, risk_action_width, button_height);
		_button_close_all.MoveWindow(risk_x + 22 + risk_action_width + action_gap, button_row_y, risk_action_width, button_height);
		_summary_label.MoveWindow(risk_x + 22, risk_page_y + 46 + panel_content_offset, risk_inner_width - 12, 22);
		_monitor_label.MoveWindow(risk_x + 22, risk_page_y + 90 + panel_content_offset, risk_inner_width - 12, std::max(44, risk_height - 114 - panel_content_offset));

		const int orders_y = current_y;
		const int left_width = (content_width * 51) / 100;
		const int right_width = content_width - left_width - gap;
		const int left_x = margin;
		const int logs_x = margin + left_width + gap;
		const int top_half_height = std::max(150, (bottom_height * 57) / 100);
		const int bottom_half_height = bottom_height - top_half_height - gap;

		_group_orders.MoveWindow(left_x, orders_y, left_width, top_half_height);
		_list_orders.MoveWindow(left_x + 12, orders_y + 24, left_width - 24, top_half_height - 72);
		_button_cancel.MoveWindow(left_x + 12, orders_y + top_half_height - 40, 120, button_height);
		_button_cancel_all.MoveWindow(left_x + 12 + 120 + 12, orders_y + top_half_height - 40, 120, button_height);

		_group_positions.MoveWindow(left_x, orders_y + top_half_height + gap, left_width, bottom_half_height);
		_list_positions.MoveWindow(left_x + 12, orders_y + top_half_height + gap + 24, left_width - 24, bottom_half_height - 36);

		_group_logs.MoveWindow(logs_x, orders_y, right_width, bottom_height);
		_combo_log_level.MoveWindow(logs_x + right_width - 122, orders_y + 22, 100, 220);
		_edit_logs.MoveWindow(logs_x + 14, orders_y + 52, right_width - 28, bottom_height - 66);
		layout_list_columns();
		_status_label.MoveWindow(margin, status_y, content_width, status_height);
		show_top_right_page();
		show_tab_page();
	}

	void show_tab_page()
	{
		_group_orders.ShowWindow(SW_SHOW);
		_list_orders.ShowWindow(SW_SHOW);
		_button_cancel.ShowWindow(SW_SHOW);
		_button_cancel_all.ShowWindow(SW_SHOW);
		_group_positions.ShowWindow(SW_SHOW);
		_list_positions.ShowWindow(SW_SHOW);
		_group_logs.ShowWindow(SW_SHOW);
		_edit_logs.ShowWindow(SW_SHOW);
	}

	void show_top_right_page()
	{
		const int tab_index = _tab_top_right.GetCurSel();
		const bool show_risk = tab_index != 1;
		_group_risk.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_label_order_limit.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_edit_order_limit.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_label_cancel_limit.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_edit_cancel_limit.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_risk_divider.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_button_set_limits.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_button_close_all.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);

		_group_monitor.ShowWindow(show_risk ? SW_HIDE : SW_SHOW);
		_button_pause.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_summary_label.ShowWindow(show_risk ? SW_HIDE : SW_SHOW);
		_monitor_label.ShowWindow(show_risk ? SW_HIDE : SW_SHOW);
	}

	void start_logic()
	{
		if (_trading_thread.joinable())
		{
			_trading_thread.join();
		}
		_trading_ready = false;
		_trading_starting = true;
		_stop_requested = false;
		_trading_thread = std::thread([this]() {
			_logic->start_trading();
			if (!_stop_requested)
			{
				_trading_ready = true;
			}
			_trading_starting = false;
		});
	}

	void stop_logic()
	{
		_stop_requested = true;
		if (_logic)
		{
			_logic->stop_trading();
		}
		if (_trading_thread.joinable())
		{
			_trading_thread.join();
		}
	}

	void set_status(const std::string& text)
	{
		set_window_text(_status_label, text);
	}

	void refresh_instrument_combo(const gui_logic::snapshot& snapshot)
	{
		const std::string current_text = get_window_text(_combo_code);
		std::vector<std::string> codes;
		for (const auto* code : kFavoriteContracts)
		{
			codes.emplace_back(code);
		}
		for (const auto& pos : snapshot.positions)
		{
			if (!pos.code.empty())
			{
				codes.emplace_back(pos.code);
			}
		}
		std::sort(codes.begin(), codes.end());
		codes.erase(std::unique(codes.begin(), codes.end()), codes.end());

		if (codes != _cached_codes)
		{
			_cached_codes = codes;
			_combo_code.ResetContent();
			for (const auto& code : _cached_codes)
			{
				_combo_code.AddString(to_cstring(code));
			}
		}

		if (!_pending_market_focus_code.empty() && _pending_market_focus_code == snapshot.focused_code)
		{
			_pending_market_focus_code.clear();
		}
		const std::string effective_order_code = !_pending_order_contract_code.empty()
			? _pending_order_contract_code
			: current_text;
		if (!effective_order_code.empty())
		{
			set_window_text(_combo_code, effective_order_code);
		}
		else if (_combo_code.GetCount() > 0 && current_text.empty())
		{
			set_window_text(_combo_code, _cached_codes.front());
		}
		sync_favorite_selection();
	}

	void refresh_market_panel(const gui_logic::snapshot& snapshot)
	{
		_chart_market.set_data(snapshot.recent_ticks, snapshot.last_price, snapshot.average_price);
	}

	void sync_order_price_with_focus(const gui_logic::snapshot& snapshot)
	{
		if (snapshot.focused_code.empty())
		{
			return;
		}

		double reference_price = snapshot.last_price;
		if (reference_price <= 0.0)
		{
			for (const auto& row : snapshot.market_levels)
			{
				if (row.price > 0.0)
				{
					reference_price = row.price;
					break;
				}
			}
		}
		if (reference_price <= 0.0 && !snapshot.recent_ticks.empty())
		{
			reference_price = snapshot.recent_ticks.front().price;
		}
		if (reference_price <= 0.0)
		{
			return;
		}

		if (_last_price_sync_code != snapshot.focused_code &&
			get_window_text(_combo_code) == snapshot.focused_code)
		{
			_edit_price.SetWindowText(to_cstring(format_price(reference_price)));
			_last_price_sync_code = snapshot.focused_code;
		}
	}

	void refresh_tick_panel(const gui_logic::snapshot& snapshot)
	{
		std::ostringstream signature;
		for (const auto& tick : snapshot.recent_ticks)
		{
			signature << tick.time_text << "|" << tick.price << "|" << tick.delta_volume << ";";
		}
		if (signature.str() == _tick_list_signature)
		{
			return;
		}
		_tick_list_signature = signature.str();
		_list_ticks.SetRedraw(FALSE);
		_list_ticks.DeleteAllItems();
		for (size_t index = 0; index < snapshot.recent_ticks.size(); ++index)
		{
			const auto& tick = snapshot.recent_ticks[index];
			const int row = _list_ticks.InsertItem(static_cast<int>(index), to_cstring(tick.time_text));
			_list_ticks.SetItemText(row, 1, to_cstring(format_price(tick.price)));
			_list_ticks.SetItemText(row, 2, to_cstring(std::to_string(tick.delta_volume)));
			_list_ticks.SetItemText(row, 3, to_cstring(format_price(tick.open_interest)));
			_list_ticks.SetItemText(row, 4, to_cstring(format_price(tick.bid_price)));
			_list_ticks.SetItemText(row, 5, to_cstring(format_price(tick.ask_price)));
		}
		_list_ticks.SetRedraw(TRUE);
		_list_ticks.Invalidate(FALSE);
	}

	void refresh_order_list(const gui_logic::snapshot& snapshot)
	{
		std::ostringstream signature;
		for (const auto& order : snapshot.orders)
		{
			signature << order.estid << "|" << order.code << "|" << order.last_volume << ";";
		}
		if (signature.str() == _order_list_signature)
		{
			return;
		}
		_order_list_signature = signature.str();
		_list_orders.SetRedraw(FALSE);
		_list_orders.DeleteAllItems();
		for (size_t index = 0; index < snapshot.orders.size(); ++index)
		{
			const auto& order = snapshot.orders[index];
			const int row = _list_orders.InsertItem(static_cast<int>(index), to_cstring(std::to_string(order.estid)));
			_list_orders.SetItemText(row, 1, to_cstring(order.code));
			_list_orders.SetItemText(row, 2, to_cstring(order.offset));
			_list_orders.SetItemText(row, 3, to_cstring(order.direction));
			_list_orders.SetItemText(row, 4, to_cstring(format_price(order.price)));
			_list_orders.SetItemText(row, 5, to_cstring(std::to_string(order.total_volume)));
			_list_orders.SetItemText(row, 6, to_cstring(std::to_string(order.last_volume)));
			_list_orders.SetItemText(row, 7, to_cstring(std::to_string(order.create_time)));
		}
		_list_orders.SetRedraw(TRUE);
		_list_orders.Invalidate(FALSE);
	}

	void refresh_position_list(const gui_logic::snapshot& snapshot)
	{
		std::ostringstream signature;
		for (const auto& pos : snapshot.positions)
		{
			signature << pos.code << "|" << pos.current_long << "|" << pos.current_short << "|" << pos.history_long << "|" << pos.history_short << ";";
		}
		if (signature.str() == _position_list_signature)
		{
			return;
		}
		_position_list_signature = signature.str();
		_list_positions.SetRedraw(FALSE);
		_list_positions.DeleteAllItems();
		for (size_t index = 0; index < snapshot.positions.size(); ++index)
		{
			const auto& pos = snapshot.positions[index];
			const int row = _list_positions.InsertItem(static_cast<int>(index), to_cstring(pos.code));
			_list_positions.SetItemText(row, 1, to_cstring(std::to_string(pos.current_long)));
			_list_positions.SetItemText(row, 2, to_cstring(std::to_string(pos.current_short)));
			_list_positions.SetItemText(row, 3, to_cstring(std::to_string(pos.history_long)));
			_list_positions.SetItemText(row, 4, to_cstring(std::to_string(pos.history_short)));
			_list_positions.SetItemText(row, 5, to_cstring(std::to_string(pos.long_frozen)));
			_list_positions.SetItemText(row, 6, to_cstring(std::to_string(pos.short_frozen)));
			_list_positions.SetItemText(row, 7, to_cstring(std::to_string(pos.long_pending)));
			_list_positions.SetItemText(row, 8, to_cstring(std::to_string(pos.short_pending)));
		}
		_list_positions.SetRedraw(TRUE);
		_list_positions.Invalidate(FALSE);
	}

	void refresh_logs(const gui_logic::snapshot& snapshot)
	{
		refresh_framework_logs();
		std::ostringstream oss;
		for (const auto& line : snapshot.logs)
		{
			oss << line << "\r\n";
		}
		for (const auto& line : _framework_log_lines)
		{
			if (should_show_framework_log(line))
			{
				oss << line << "\r\n";
			}
		}
		_edit_logs.SetWindowText(to_cstring(oss.str()));
		_edit_logs.SetSel(-1, -1);
		_edit_logs.LineScroll(_edit_logs.GetLineCount());
	}

	void refresh_framework_logs()
	{
		if (_framework_log_path.empty() || !std::filesystem::exists(_framework_log_path))
		{
			_framework_log_path = resolve_framework_log_path();
			_framework_log_offset = 0;
			_framework_log_lines.clear();
		}
		if (_framework_log_path.empty() || !std::filesystem::exists(_framework_log_path))
		{
			return;
		}

		std::ifstream input(_framework_log_path, std::ios::binary);
		if (!input.is_open())
		{
			return;
		}
		input.seekg(0, std::ios::end);
		const auto size = input.tellg();
		if (size < 0)
		{
			return;
		}
		if (_framework_log_offset > static_cast<uintmax_t>(size))
		{
			_framework_log_offset = 0;
			_framework_log_lines.clear();
		}
		input.seekg(static_cast<std::streamoff>(_framework_log_offset), std::ios::beg);
		std::string line;
		while (std::getline(input, line))
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			if (!line.empty())
			{
				_framework_log_lines.emplace_back(line);
				append_framework_test_record(line);
			}
		}
		_framework_log_offset = static_cast<uintmax_t>(input.tellg());
		if (input.fail() && !input.bad())
		{
			input.clear();
			input.seekg(0, std::ios::end);
			_framework_log_offset = static_cast<uintmax_t>(input.tellg());
		}
		if (_framework_log_lines.size() > 400U)
		{
			_framework_log_lines.erase(_framework_log_lines.begin(),
				_framework_log_lines.begin() + static_cast<long long>(_framework_log_lines.size() - 400U));
		}
	}

	void append_framework_test_record(const std::string& line)
	{
		if (!_logic)
		{
			return;
		}

		if (line.find("OnRspUserLogin") != std::string::npos ||
			line.find("OnFrontConnected") != std::string::npos ||
			line.find("OnFrontDisconnected") != std::string::npos ||
			line.find("用户登录") != std::string::npos ||
			line.find("FrontID") != std::string::npos ||
			line.find("SessionID") != std::string::npos ||
			line.find("AppID") != std::string::npos)
		{
			_logic->append_test_record("system", line);
		}

		if (line.find("[报单统计]") != std::string::npos ||
			line.find("[撤单统计]") != std::string::npos ||
			line.find("报单数量") != std::string::npos ||
			line.find("撤单数量") != std::string::npos)
		{
			_logic->append_test_record("monitor", line);
		}

		if (line.find("ErrorID") != std::string::npos ||
			line.find("ErrorMsg") != std::string::npos ||
			line.find("[ERROR]") != std::string::npos ||
			line.find("错误") != std::string::npos)
		{
			_logic->append_test_record("error", line);
		}
	}

	std::filesystem::path resolve_framework_log_path() const
	{
		std::vector<std::filesystem::path> candidates = {
			std::filesystem::current_path() / "log",
			get_module_directory() / "log",
			get_module_directory().parent_path() / "log",
			std::filesystem::current_path() / "bin" / "log",
			get_module_directory() / "bin" / "log"
		};
		std::filesystem::path best_path;
		std::filesystem::file_time_type best_time{};
		for (const auto& dir : candidates)
		{
			if (!std::filesystem::exists(dir))
			{
				continue;
			}
			for (const auto& entry : std::filesystem::directory_iterator(dir))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}
				const auto filename = entry.path().filename().string();
				if (filename.rfind("lt_", 0) != 0 || entry.path().extension() != ".txt")
				{
					continue;
				}
				const auto write_time = entry.last_write_time();
				if (best_path.empty() || write_time > best_time)
				{
					best_time = write_time;
					best_path = entry.path();
				}
			}
		}
		return best_path;
	}

	bool should_show_framework_log(const std::string& line) const
	{
		const int filter = _combo_log_level.GetCurSel();
		if (filter <= 0)
		{
			return true;
		}
		static const std::array<const char*, 7> levels = {
			"",
			"[TRACE]",
			"[DEBUG]",
			"[INFO]",
			"[WARNING]",
			"[ERROR]",
			"[FATAL]"
		};
		return line.find(levels[filter]) != std::string::npos;
	}

	void refresh_monitor(const gui_logic::snapshot& snapshot)
	{
		std::ostringstream monitor;
		monitor << "Connection: " << snapshot.connection_status
			<< "\r\nOrders: " << snapshot.order_submit_count << "/" << snapshot.order_threshold
			<< (snapshot.order_threshold_alert ? " (limit reached)" : " (normal)")
			<< "\r\nCancels: " << snapshot.cancel_request_count << "/" << snapshot.cancel_threshold
			<< (snapshot.cancel_threshold_alert ? " (limit reached)" : " (normal)")
			<< "\r\nDuplicate warnings: " << snapshot.duplicate_warning_count
			<< "    Rejects: " << snapshot.reject_count;
		set_window_text(_monitor_label, monitor.str());
		_button_pause.SetWindowText(snapshot.trading_paused ? _T("Resume Trading") : _T("Pause Trading"));
	}

	void refresh_ui()
	{
		const auto snapshot = _logic->get_snapshot();
		refresh_instrument_combo(snapshot);
		refresh_market_panel(snapshot);
		sync_order_price_with_focus(snapshot);
		refresh_tick_panel(snapshot);
		refresh_order_list(snapshot);
		refresh_position_list(snapshot);
		refresh_logs(snapshot);
		refresh_monitor(snapshot);
		if (snapshot.error_event_id > _last_error_event_id && !snapshot.error_message.empty())
		{
			_last_reject_count = snapshot.reject_count;
			_last_error_event_id = snapshot.error_event_id;
			_last_error_popup_status = snapshot.error_message;
			AfxMessageBox(to_cstring(snapshot.error_message), MB_ICONERROR | MB_OK);
		}

		const std::string effective_focus = !_pending_market_focus_code.empty() ? _pending_market_focus_code : snapshot.focused_code;
		std::ostringstream summary;
		summary << "Focus " << (effective_focus.empty() ? "-" : effective_focus)
			<< "    Trading day " << snapshot.trading_day
			<< "    Time " << snapshot.last_time
			<< "    Mode " << (snapshot.trading_paused ? "Paused" : "Active")
			<< "    Contracts " << snapshot.instruments.size()
			<< "    Orders " << snapshot.orders.size()
			<< "    Positions " << snapshot.positions.size();
		set_window_text(_summary_label, summary.str());

		if (_trading_starting && !_trading_ready)
		{
			set_status("Status: logging in, please wait");
		}
		else
		{
			set_status("Status: " + snapshot.status);
		}
	}

	void send_command(const std::string& command, bool require_ready = true)
	{
		if (require_ready && !_trading_ready)
		{
			set_status("Status: trading service is still starting, please wait");
			AfxMessageBox(_T("Trading service is still starting, please wait."), MB_ICONINFORMATION | MB_OK);
			return;
		}
		_logic->post_command(command);
	}

	void submit_order()
	{
		try
		{
			const auto code = get_window_text(_combo_code);
			const auto volume_text = get_window_text(_edit_volume);
			const auto price_text = get_window_text(_edit_price);
			if (code.empty())
			{
				set_status("Status: please select or input a contract");
				AfxMessageBox(_T("Please select a contract."), MB_ICONWARNING | MB_OK);
				return;
			}
			if (volume_text.empty())
			{
				set_status("Status: please input volume");
				AfxMessageBox(_T("Please input volume."), MB_ICONWARNING | MB_OK);
				return;
			}

			const bool close_today = _check_close_today.GetCheck() == BST_CHECKED;
			const auto volume = std::stoul(volume_text);
			const auto price = price_text.empty() ? 0.0 : std::stod(price_text);
			const auto snapshot = _logic->get_snapshot();
			if (snapshot.order_submit_count >= snapshot.order_threshold)
			{
				set_status("Status: order limit reached");
				AfxMessageBox(_T("Order limit reached."), MB_ICONWARNING | MB_OK);
				return;
			}

			std::string side = "buy_open";
			switch (_combo_side.GetCurSel())
			{
			case 1:
				side = "sell_open";
				break;
			case 2:
				side = "buy_close";
				break;
			case 3:
				side = "sell_close";
				break;
			default:
				break;
			}

			std::ostringstream cmd;
			cmd << "action=order"
				<< "&code=" << code
				<< "&side=" << side
				<< "&volume=" << volume
				<< "&price=" << price
				<< "&flag=NOR"
				<< "&close_today=" << (close_today ? 1 : 0);
			send_command(cmd.str());
		}
		catch (const std::exception&)
		{
			set_status("Status: invalid order input");
			AfxMessageBox(_T("Invalid order input."), MB_ICONERROR | MB_OK);
		}
	}

	void cancel_selected_order()
	{
		POSITION pos = _list_orders.GetFirstSelectedItemPosition();
		if (pos == nullptr)
		{
			set_status("Status: please select an order to cancel");
			AfxMessageBox(_T("Please select an order to cancel."), MB_ICONINFORMATION | MB_OK);
			return;
		}

		const int row = _list_orders.GetNextSelectedItem(pos);
		const std::string estid = to_std_string(_list_orders.GetItemText(row, 0));
		if (estid.empty())
		{
			set_status("Status: selected order has no estid");
			return;
		}

		if (AfxMessageBox(_T("Do you want to cancel the selected order?"), MB_ICONQUESTION | MB_YESNO) != IDYES)
		{
			return;
		}

		const auto snapshot = _logic->get_snapshot();
		if (snapshot.cancel_request_count >= snapshot.cancel_threshold)
		{
			set_status("Status: cancel limit reached");
			AfxMessageBox(_T("Cancel limit reached."), MB_ICONWARNING | MB_OK);
			return;
		}

		send_command("action=cancel&estid=" + estid);
	}

	void batch_cancel(bool contract_only)
	{
		if (!contract_only && AfxMessageBox(_T("Do you want to cancel all working orders?"), MB_ICONQUESTION | MB_YESNO) != IDYES)
		{
			return;
		}
		std::ostringstream cmd;
		cmd << "action=batch_cancel&scope=" << (contract_only ? "contract" : "all");
		if (contract_only)
		{
			const auto code = get_window_text(_combo_code);
			if (code.empty())
			{
				set_status("Status: please select or input a contract");
				return;
			}
			cmd << "&code=" << code;
		}
		const auto snapshot = _logic->get_snapshot();
		if (snapshot.cancel_request_count >= snapshot.cancel_threshold)
		{
			set_status("Status: cancel limit reached");
			AfxMessageBox(_T("Cancel limit reached."), MB_ICONWARNING | MB_OK);
			return;
		}
		send_command(cmd.str());
	}

	void close_all_positions()
	{
		if (AfxMessageBox(_T("Do you want to close all positions?"), MB_ICONQUESTION | MB_YESNO) != IDYES)
		{
			return;
		}
		send_command("action=close_all");
	}

	void toggle_pause()
	{
		const auto snapshot = _logic->get_snapshot();
		std::ostringstream cmd;
		cmd << "action=pause&value=" << (snapshot.trading_paused ? 0 : 1);
		send_command(cmd.str());
	}

	void set_limits()
	{
		try
		{
			const auto order_limit = std::stoul(get_window_text(_edit_order_limit));
			const auto cancel_limit = std::stoul(get_window_text(_edit_cancel_limit));
			std::ostringstream cmd;
			cmd << "action=set_limits"
				<< "&order_threshold=" << order_limit
				<< "&cancel_threshold=" << cancel_limit;
			send_command(cmd.str());
		}
		catch (const std::exception&)
		{
			set_status("Status: invalid threshold input");
		}
	}

	static std::string format_price(double price)
	{
		std::ostringstream oss;
		oss.setf(std::ios::fixed);
		oss.precision(4);
		oss << price;
		return oss.str();
	}

	void layout_list_columns()
	{
		resize_list_columns(_list_ticks, { 18, 14, 12, 16, 20, 20 }, { 90, 72, 68, 80, 84, 84 });
		resize_list_columns(_list_orders, { 13, 18, 10, 10, 12, 9, 9, 19 }, { 88, 110, 70, 70, 78, 60, 60, 96 });
		resize_list_columns(_list_positions, { 18, 10, 10, 10, 10, 10, 10, 11, 11 }, { 88, 50, 50, 50, 50, 50, 50, 54, 54 });
	}

	void resize_list_columns(CListCtrl& list, std::initializer_list<int> weights, std::initializer_list<int> minimums)
	{
		if (!::IsWindow(list.GetSafeHwnd()))
		{
			return;
		}

		CRect rect;
		list.GetClientRect(&rect);
		int width = rect.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - 4;
		if (width <= 0)
		{
			return;
		}

		const int count = static_cast<int>(weights.size());
		if (count <= 0 || count != static_cast<int>(minimums.size()))
		{
			return;
		}

		const int total_weight = std::accumulate(weights.begin(), weights.end(), 0);
		std::vector<int> widths(count, 0);
		int used = 0;
		int index = 0;
		auto min_it = minimums.begin();
		for (int weight : weights)
		{
			const int min_width = *min_it++;
			int column_width = std::max(min_width, (width * weight) / std::max(1, total_weight));
			widths[index++] = column_width;
			used += column_width;
		}

		if (used != width && !widths.empty())
		{
			widths.back() = std::max(widths.back() + (width - used), 40);
		}

		for (int column = 0; column < count; ++column)
		{
			list.SetColumnWidth(column, widths[column]);
		}
	}

	void sync_favorite_selection()
	{
		const std::string current_focus = !_pending_market_focus_code.empty() ? _pending_market_focus_code : _logic->get_snapshot().focused_code;
		const CString current = to_cstring(current_focus);
		const int count = _list_favorites.GetCount();
		const int current_selection = _list_favorites.GetCurSel();
		int target_selection = LB_ERR;
		for (int index = 0; index < count; ++index)
		{
			CString text;
			_list_favorites.GetText(index, text);
			if (text == current)
			{
				target_selection = index;
				break;
			}
		}

		if (target_selection != LB_ERR)
		{
			const bool user_is_interacting = (GetFocus() == &_list_favorites) && _pending_market_focus_code.empty();
			if (!user_is_interacting && current_selection != target_selection)
			{
				_list_favorites.SetCurSel(target_selection);
			}
			return;
		}
		if (count > 0 && current.IsEmpty() && current_selection == LB_ERR)
		{
			_list_favorites.SetCurSel(0);
		}
	}

	void set_market_focus_code(const std::string& code, bool notify_logic)
	{
		if (code.empty())
		{
			return;
		}
		if (_pending_market_focus_code == code)
		{
			return;
		}
		_pending_market_focus_code = code;
		sync_favorite_selection();
		if (notify_logic)
		{
			send_command("action=focus&code=" + code, false);
		}
	}

	void set_order_contract_code(const std::string& code)
	{
		if (code.empty())
		{
			return;
		}
		_pending_order_contract_code = code;
		set_window_text(_combo_code, code);
		update_close_today_visibility();
	}

	void update_close_today_visibility()
	{
		const auto code = get_window_text(_combo_code);
		const int side_index = _combo_side.GetCurSel();
		const bool is_close_side = side_index == 2 || side_index == 3;
		const bool show = !code.empty() && is_close_side && lt::code_t(code.c_str()).is_distinct();
		_check_close_today.ShowWindow(show ? SW_SHOW : SW_HIDE);
		if (!show)
		{
			_check_close_today.SetCheck(BST_UNCHECKED);
		}
	}

private:
	CFont _font;
	CButton _group_quick;
	CButton _group_market;
	CButton _group_ticks;
	CButton _group_risk;
	CButton _group_monitor;
	CButton _group_favorites;
	CButton _group_orders;
	CButton _group_positions;
	CButton _group_logs;
	CStatic _label_contract;
	CStatic _label_side;
	CStatic _label_volume;
	CStatic _label_price;
	CStatic _label_order_limit;
	CStatic _label_cancel_limit;
	CStatic _risk_divider;
	CComboBox _combo_code;
	CComboBox _combo_side;
	CListBox _list_favorites;
	market_chart_ctrl _chart_market;
	CListCtrl _list_ticks;
	CEdit _edit_volume;
	CEdit _edit_price;
	CButton _check_close_today;
	CButton _button_order;
	CButton _button_cancel;
	CButton _button_cancel_all;
	CButton _button_close_all;
	CButton _button_pause;
	CButton _button_set_limits;
	CEdit _edit_order_limit;
	CEdit _edit_cancel_limit;
	CTabCtrl _tab_main;
	CTabCtrl _tab_top_right;
	CListCtrl _list_orders;
	CListCtrl _list_positions;
	CEdit _edit_logs;
	CComboBox _combo_log_level;
	CStatic _summary_label;
	CStatic _monitor_label;
	CStatic _hint_label;
	CStatic _status_label;
	std::shared_ptr<gui_logic> _logic;
	std::thread _trading_thread;
	std::vector<std::string> _cached_codes;
	std::string _pending_market_focus_code;
	std::string _pending_order_contract_code;
	std::string _last_price_sync_code;
	uint32_t _last_reject_count = 0;
	uint64_t _last_error_event_id = 0;
	std::string _last_error_popup_status;
	std::string _tick_list_signature;
	std::string _order_list_signature;
	std::string _position_list_signature;
	std::filesystem::path _framework_log_path;
	uintmax_t _framework_log_offset = 0;
	std::vector<std::string> _framework_log_lines;
	std::atomic<bool> _trading_ready { false };
	std::atomic<bool> _trading_starting { false };
	std::atomic<bool> _stop_requested { false };
};

BEGIN_MESSAGE_MAP(demo_gui_frame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_TIMER()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &demo_gui_frame::OnTabChanged)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_TOP_RIGHT, &demo_gui_frame::OnTopTabChanged)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ORDERS, &demo_gui_frame::OnOrderSelectionChanged)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_POSITIONS, &demo_gui_frame::OnPositionSelectionChanged)
	ON_LBN_SELCHANGE(IDC_LIST_FAVORITES, &demo_gui_frame::OnFavoriteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_CODE, &demo_gui_frame::OnContractChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO_CODE, &demo_gui_frame::OnContractChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_SIDE, &demo_gui_frame::OnSideChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_LOG_LEVEL, &demo_gui_frame::OnLogFilterChanged)
	ON_BN_CLICKED(IDC_BUTTON_ORDER, &demo_gui_frame::OnSendOrder)
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &demo_gui_frame::OnCancelSelected)
	ON_BN_CLICKED(IDC_BUTTON_BATCH_CANCEL_ALL, &demo_gui_frame::OnCancelAll)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE_ALL, &demo_gui_frame::OnCloseAll)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &demo_gui_frame::OnPauseTrading)
	ON_BN_CLICKED(IDC_BUTTON_SET_LIMITS, &demo_gui_frame::OnSetLimits)
END_MESSAGE_MAP()

class loading_popup : public CFrameWnd
{
public:
	BOOL CreatePopup()
	{
		const CString class_name = AfxRegisterWndClass(
			CS_HREDRAW | CS_VREDRAW,
			::LoadCursor(nullptr, IDC_WAIT),
			reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
			nullptr);
		return CFrameWnd::Create(class_name,
			_T("Loading"),
			WS_POPUP | WS_BORDER | WS_CAPTION,
			CRect(0, 0, 480, 170),
			nullptr);
	}

	void set_text(const CString& text)
	{
		_message = text;
		if (::IsWindow(GetSafeHwnd()))
		{
			Invalidate(FALSE);
		}
	}

protected:
	afx_msg void OnPaint()
	{
		CPaintDC dc(this);
		CRect rect;
		GetClientRect(&rect);
		dc.FillSolidRect(rect, RGB(250, 250, 250));
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(50, 50, 50));
		CFont font;
		font.CreatePointFont(92, _T("Segoe UI"));
		CFont* old_font = dc.SelectObject(&font);
		dc.DrawText(_T("Loading market data..."), CRect(rect.left + 20, rect.top + 26, rect.right - 20, rect.top + 58), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		dc.DrawText(_message, CRect(rect.left + 28, rect.top + 70, rect.right - 28, rect.bottom - 24), DT_CENTER | DT_WORDBREAK);
		dc.SelectObject(old_font);
	}

	virtual void PostNcDestroy() override
	{
		// This popup lives on the stack in InitInstance, so MFC must not delete it.
	}

	DECLARE_MESSAGE_MAP()

private:
	CString _message = _T("Please wait while trading service and the first market snapshot finish loading.");
};

BEGIN_MESSAGE_MAP(loading_popup, CFrameWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

class demo_gui_app : public CWinApp
{
public:
	virtual BOOL InitInstance() override
	{
		CWinApp::InitInstance();
		INITCOMMONCONTROLSEX controls = {};
		controls.dwSize = sizeof(controls);
		controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
		InitCommonControlsEx(&controls);

		auto* frame = new demo_gui_frame();
		if (!frame->Create(nullptr, _T("Lightning Futures GUI"), WS_OVERLAPPEDWINDOW, CRect(100, 100, 1600, 920)))
		{
			delete frame;
			return FALSE;
		}

		m_pMainWnd = frame;
		frame->ShowWindow(SW_HIDE);
		frame->UpdateWindow();

		loading_popup loading;
		if (loading.CreatePopup())
		{
			CRect popup_rect;
			loading.GetWindowRect(&popup_rect);
			const int screen_x = (::GetSystemMetrics(SM_CXSCREEN) - popup_rect.Width()) / 2;
			const int screen_y = (::GetSystemMetrics(SM_CYSCREEN) - popup_rect.Height()) / 2;
			loading.SetWindowPos(&CWnd::wndTopMost, screen_x, screen_y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			loading.UpdateWindow();
		}

		loading.set_text(_T("Please wait while trading service startup completes."));

		const auto start = std::chrono::steady_clock::now();
		while (!frame->is_service_started() && std::chrono::steady_clock::now() - start < std::chrono::seconds(30))
		{
			MSG msg = {};
			while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					return FALSE;
				}
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			::Sleep(50);
		}

		if (::IsWindow(loading.GetSafeHwnd()))
		{
			loading.DestroyWindow();
		}

		frame->ShowWindow(SW_SHOW);
		frame->UpdateWindow();
		return TRUE;
	}

	virtual int ExitInstance() override
	{
		return CWinApp::ExitInstance();
	}
};

demo_gui_app the_app;
