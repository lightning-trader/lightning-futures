#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "strategy.hpp"

class gui_bridge_strategy : public lt::hft::strategy, public lt::tick_receiver
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
	gui_bridge_strategy(lt::hft::straid_t id, lt::hft::syringe* syringe);

	snapshot get_snapshot() const;

private:
	virtual void on_init(lt::subscriber& suber) override;

	virtual void on_destroy(lt::unsubscriber& unsuber) override;

	virtual void on_update() override;

	virtual void on_change(const lt::params& p) override;

	virtual void on_entrust(const lt::order_info& order) override;

	virtual void on_deal(lt::estid_t estid, uint32_t deal_volume) override;

	virtual void on_trade(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t volume) override;

	virtual void on_cancel(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t cancel_volume, uint32_t total_volume) override;

	virtual void on_error(lt::error_type type, lt::estid_t estid, const lt::error_code error) override;

	virtual void on_tick(const lt::tick_info& tick) override;

private:
	void update_focus(const lt::code_t& code);

	void refresh_snapshot();

	void set_status(const std::string& status);

	void append_log(const std::string& category, const std::string& message);

	void open_log_file();

	void update_connection_status();

	void update_threshold_flags();

	static std::string to_string(lt::offset_type offset);

	static std::string to_string(lt::direction_type direction);

	static std::string to_string(lt::error_type type);

	static std::string to_string(lt::error_code error);

	static lt::order_flag parse_order_flag(const std::string& flag_text);

private:
	mutable std::mutex _snapshot_mutex;
	snapshot _snapshot;
	std::unordered_map<std::string, lt::daytm_t> _recent_orders;
	std::unordered_map<std::string, uint64_t> _last_tick_keys;
	std::unordered_map<std::string, uint32_t> _last_tick_volumes;
	std::unordered_map<std::string, std::vector<tick_row>> _tick_history;
	std::unordered_set<std::string> _tick_seen_codes;
	std::chrono::steady_clock::time_point _start_time;
	bool _no_tick_warning_emitted;
	std::ofstream _log_file;
	std::filesystem::path _log_path;
	std::vector<lt::code_t> _favorites;
};
