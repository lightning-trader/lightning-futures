#include "gui_bridge_strategy.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

#include <log_define.hpp>

namespace
{
	const std::array<const char*, 3> kDefaultFavorites = {
		"SHFE.ag2606",
		"SHFE.fu2609",
		"CZCE.MA505"
	};
	constexpr uint32_t kDefaultMaxSingleOrderVolume = 100U;
	constexpr uint32_t kAgOpenMaxSingleOrderVolume = 800U;

	std::string format_time_text(lt::daytm_t time)
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

	void pad_display_ticks(std::vector<gui_bridge_strategy::tick_row>& ticks, size_t target_count)
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

gui_bridge_strategy::gui_bridge_strategy(lt::hft::straid_t id, lt::hft::syringe* syringe)
	: strategy(id, syringe)
	, _start_time(std::chrono::steady_clock::now())
	, _no_tick_warning_emitted(false)
{
	for (const auto* code : kDefaultFavorites)
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
	open_log_file();
	append_log("system", "demo-gui bridge initialized");
}

gui_bridge_strategy::snapshot gui_bridge_strategy::get_snapshot() const
{
	const_cast<gui_bridge_strategy*>(this)->refresh_snapshot();
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	return _snapshot;
}

void gui_bridge_strategy::append_test_record(const std::string& category, const std::string& message)
{
	if (!_test_log_file.is_open() || !should_write_test_record(category))
	{
		return;
	}
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_test_log_file << "[external-" << category << "] " << message << std::endl;
}

void gui_bridge_strategy::on_init(lt::subscriber& suber)
{
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
	set_status("trading service started");
	append_log("system", "trading service started");
	append_log("market", "subscribed favorites: " + oss.str());
	update_connection_status();
	refresh_snapshot();
}

void gui_bridge_strategy::on_destroy(lt::unsubscriber& unsuber)
{
	for (const auto& code : _favorites)
	{
		unsuber.unregist_tick_receiver(code, this);
	}
}

void gui_bridge_strategy::on_update()
{
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
	refresh_snapshot();
}

void gui_bridge_strategy::on_change(const lt::params& p)
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
			const auto flag = parse_order_flag(p.get<std::string>("flag"));
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
			if (get_instrument(code).code == lt::default_code)
			{
				throw std::invalid_argument("contract not found");
			}
			const auto& instrument = get_instrument(code);
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
			const auto now = get_last_time();
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
			if (side == "buy_open")
			{
				estid = buy_open(code, volume, price, flag);
			}
			else if (side == "sell_open")
			{
				estid = sell_open(code, volume, price, flag);
			}
			else if (side == "buy_close")
			{
				estid = buy_close(code, volume, price, close_today, flag);
			}
			else if (side == "sell_close")
			{
				estid = sell_close(code, volume, price, close_today, flag);
			}
			else
			{
				throw std::invalid_argument("unsupported side");
			}

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
			cancel_order(estid);
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
			uint32_t cancel_count = 0U;
			std::vector<lt::estid_t> estids;
			for (const auto& it : get_orders())
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
				cancel_order(estid);
				cancel_count++;
			}
			{
				std::lock_guard<std::mutex> lock(_snapshot_mutex);
				_snapshot.cancel_request_count += cancel_count;
			}
			update_threshold_flags();
			std::ostringstream oss;
			oss << "batch cancel requested count=" << cancel_count;
			if (scope == "contract")
			{
				oss << " contract=" << code.to_string();
			}
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
			for (const auto& it : get_positions())
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
			for (const auto& it : get_positions())
			{
				const auto& code = it.first;
				const auto& pos = it.second;
				const auto& tick = get_last_tick(code);
				const double buy_price = tick.sell_price() > 0.0 ? tick.sell_price() : tick.price;
				const double sell_price = tick.buy_price() > 0.0 ? tick.buy_price() : tick.price;

				const auto history_short = pos.history_short.usable();
				const auto current_short = pos.current_short.usable();
				const auto history_long = pos.history_long.usable();
				const auto current_long = pos.current_long.usable();

				if (history_short > 0U)
				{
					if (buy_close(code, history_short, buy_price) != INVALID_ESTID)
					{
						close_count++;
					}
				}
				if (current_short > 0U)
				{
					if (buy_close(code, current_short, buy_price, true) != INVALID_ESTID)
					{
						close_count++;
					}
				}
				if (history_long > 0U)
				{
					if (sell_close(code, history_long, sell_price) != INVALID_ESTID)
					{
						close_count++;
					}
				}
				if (current_long > 0U)
				{
					if (sell_close(code, current_long, sell_price, true) != INVALID_ESTID)
					{
						close_count++;
					}
				}
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

void gui_bridge_strategy::on_entrust(const lt::order_info& order)
{
	std::ostringstream oss;
	oss << "entrusted " << order.code.to_string() << " estid=" << order.estid;
	set_status(oss.str());
	append_log("trade", oss.str());
	refresh_snapshot();
}

void gui_bridge_strategy::on_deal(lt::estid_t estid, uint32_t deal_volume)
{
	std::ostringstream oss;
	oss << "dealt estid=" << estid << " volume=" << deal_volume;
	set_status(oss.str());
	append_log("trade", oss.str());
	refresh_snapshot();
}

void gui_bridge_strategy::on_trade(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t volume)
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

void gui_bridge_strategy::on_cancel(lt::estid_t estid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double price, uint32_t cancel_volume, uint32_t total_volume)
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

void gui_bridge_strategy::on_error(lt::error_type type, lt::estid_t estid, const lt::error_code error)
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

void gui_bridge_strategy::on_tick(const lt::tick_info& tick)
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
				<< " time=" << format_time_text(tick.time);
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
			row.time_text = format_time_text(tick.time);
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

void gui_bridge_strategy::refresh_snapshot()
{
	snapshot new_snapshot;
	{
		std::lock_guard<std::mutex> lock(_snapshot_mutex);
		new_snapshot = _snapshot;
	}

	new_snapshot.trading_day = get_trading_day();
	new_snapshot.last_time = get_last_time();
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

	for (const auto& it : get_orders())
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
	std::sort(new_snapshot.orders.begin(), new_snapshot.orders.end(), [](const order_row& lhs, const order_row& rhs) {
		return lhs.estid > rhs.estid;
		});

	for (const auto& it : get_positions())
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
	std::sort(new_snapshot.positions.begin(), new_snapshot.positions.end(), [](const position_row& lhs, const position_row& rhs) {
		return lhs.code < rhs.code;
		});

	if (!new_snapshot.focused_code.empty())
	{
		const lt::code_t focused_code(new_snapshot.focused_code.c_str());
		const auto& market = get_market_info(focused_code);
		const auto& tick = get_last_tick(focused_code);
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
		if (history_it != _tick_history.end())
		{
			new_snapshot.recent_ticks = history_it->second;
		}
		else
		{
			new_snapshot.recent_ticks.clear();
		}

		if (!tick.invalid() && new_snapshot.recent_ticks.empty())
		{
			tick_row row;
			row.time_text = format_time_text(tick.time);
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

void gui_bridge_strategy::update_focus(const lt::code_t& code)
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	if (_snapshot.focused_code != code.to_string())
	{
		_snapshot.focused_code = code.to_string();
		auto history_it = _tick_history.find(_snapshot.focused_code);
		_snapshot.recent_ticks = history_it == _tick_history.end() ? std::vector<tick_row>{} : history_it->second;
	}
}

void gui_bridge_strategy::set_status(const std::string& status)
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.status = status;
}

void gui_bridge_strategy::publish_error(const std::string& message)
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.error_message = message;
	++_snapshot.error_event_id;
}

void gui_bridge_strategy::append_log(const std::string& category, const std::string& message)
{
	const std::string line = format_time_text(get_last_time()) + " [" + category + "] " + message;
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

void gui_bridge_strategy::open_log_file()
{
	try
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t current = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm = {};
		localtime_s(&local_tm, &current);

		std::ostringstream name;
		name << "demo-gui-"
			<< std::put_time(&local_tm, "%Y%m%d")
			<< ".log";
		_log_path = std::filesystem::current_path() / "logs" / name.str();

		std::ostringstream test_name;
		test_name << "demo-gui-test-record-"
			<< std::put_time(&local_tm, "%Y%m%d")
			<< ".log";
		_test_log_path = std::filesystem::current_path() / "logs" / test_name.str();

		std::filesystem::create_directories(_log_path.parent_path());
		_log_file.open(_log_path, std::ios::app);
		_test_log_file.open(_test_log_path, std::ios::app);
	}
	catch (...)
	{
	}
}

void gui_bridge_strategy::update_connection_status()
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	if (_snapshot.trading_day != 0U)
	{
		_snapshot.connection_status = "connected";
	}
	else if (!_snapshot.instruments.empty())
	{
		_snapshot.connection_status = "initializing";
	}
	else
	{
		_snapshot.connection_status = "starting";
	}
}

void gui_bridge_strategy::update_threshold_flags()
{
	std::lock_guard<std::mutex> lock(_snapshot_mutex);
	_snapshot.order_threshold_alert = _snapshot.order_submit_count >= _snapshot.order_threshold;
	_snapshot.cancel_threshold_alert = _snapshot.cancel_request_count >= _snapshot.cancel_threshold;
}

std::string gui_bridge_strategy::to_string(lt::offset_type offset)
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

std::string gui_bridge_strategy::to_string(lt::direction_type direction)
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

std::string gui_bridge_strategy::to_string(lt::error_type type)
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

std::string gui_bridge_strategy::to_string(lt::error_code error)
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

lt::order_flag gui_bridge_strategy::parse_order_flag(const std::string& flag_text)
{
	if (flag_text == "FAK")
	{
		return lt::order_flag::OF_FAK;
	}
	if (flag_text == "FOK")
	{
		return lt::order_flag::OF_FOK;
	}
	return lt::order_flag::OF_NOR;
}
