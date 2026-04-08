#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/optional.h>
#include <stdexcept>
#include <sstream>
#include <bridge.h>

namespace nb = nanobind;
using namespace lt;
using namespace lt::cta;

// Helper function to convert string to code_t
lt::code_t string_to_code(const std::string& code_str) {
    if (code_str.empty()) {
        throw std::invalid_argument("Code string cannot be empty");
    }
    return lt::code_t(code_str);
}

// Helper function to convert offset_type string to enum
lt::offset_type string_to_offset_type(const std::string& offset_str) {
    if (offset_str == "OPEN") return lt::offset_type::OT_OPEN;
    if (offset_str == "CLOSE") return lt::offset_type::OT_CLOSE;
    if (offset_str == "CLSTD") return lt::offset_type::OT_CLSTD;
    throw std::invalid_argument("Unknown offset_type: " + offset_str + ". Valid values: OPEN, CLOSE, CLSTD");
}

// Helper function to convert direction_type string to enum
lt::direction_type string_to_direction_type(const std::string& direction_str) {
    if (direction_str == "LONG") return lt::direction_type::DT_LONG;
    if (direction_str == "SHORT") return lt::direction_type::DT_SHORT;
    throw std::invalid_argument("Unknown direction_type: " + direction_str + ". Valid values: LONG, SHORT");
}

// Helper function to get string representation of offset_type
std::string offset_type_to_string(lt::offset_type offset) {
    switch (offset) {
        case lt::offset_type::OT_OPEN: return "OPEN";
        case lt::offset_type::OT_CLOSE: return "CLOSE";
        case lt::offset_type::OT_CLSTD: return "CLSTD";
        default: return "UNKNOWN";
    }
}

// Helper function to get string representation of direction_type
std::string direction_type_to_string(lt::direction_type direction) {
    switch (direction) {
        case lt::direction_type::DT_LONG: return "LONG";
        case lt::direction_type::DT_SHORT: return "SHORT";
        default: return "UNKNOWN";
    }
}

// Custom exception class for trading errors
class TradingError : public std::exception {
private:
    std::string message_;
public:
    explicit TradingError(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
};


NB_MODULE(lightures, m) {
    m.doc() = "Python API for trading system - A comprehensive trading platform interface";

    // Register custom exception
    nb::exception<TradingError>(m, "TradingError");
    //nb::register_exception_translator("TradingError",translate_exception);

    nb::class_<lt::code_t>(m, "Code", "Represents a trading instrument code")
        .def(nb::init<const std::string&>(), nb::arg("code_str"), "Initialize Code with string representation")
        .def("to_string", &lt::code_t::to_string, "Get string representation of the code")
        .def("__str__", [](const lt::code_t& code) { return code.to_string(); }, "String representation for printing")
        .def("__repr__", [](const lt::code_t& code) { return "<Code '" + code.to_string() + "'>"; }, "Developer-friendly representation")
        .def("__eq__", [](const lt::code_t& a, const lt::code_t& b) { return a == b; }, "Equality comparison")
        .def("__hash__", [](const lt::code_t& code) { 
            std::hash<std::string> hasher;
            return hasher(code.to_string()); 
        }, "Hash function for use in sets and dictionaries");

    nb::enum_<lt::offset_type>(m, "OffsetType", "Order offset type enumeration")
        .value("OPEN", lt::offset_type::OT_OPEN, "Open position")
        .value("CLOSE", lt::offset_type::OT_CLOSE, "Close position")
        .value("CLSTD", lt::offset_type::OT_CLSTD, "Close today position")
        .export_values();

    nb::enum_<lt::direction_type>(m, "DirectionType", "Order direction type enumeration")
        .value("LONG", lt::direction_type::DT_LONG, "Long position")
        .value("SHORT", lt::direction_type::DT_SHORT, "Short position")
        .export_values();

    nb::enum_<lt::error_type>(m, "ErrorType", "Error type enumeration")
        .value("PLACE_ORDER", lt::error_type::ET_PLACE_ORDER, "Order placement error")
        .value("CANCEL_ORDER", lt::error_type::ET_CANCEL_ORDER, "Order cancellation error")
        .value("OTHER_ERROR", lt::error_type::ET_OTHER_ERROR, "Other error")
        .export_values();

    nb::class_<position_cell>(m, "PositionCell", "Represents position information for a specific direction")
        .def_ro("position", &position_cell::position, "Total position quantity")
        .def_ro("frozen", &position_cell::frozen, "Frozen position quantity")
        .def("__repr__", [](const position_cell& cell) {
            return "<PositionCell position=" + std::to_string(cell.position)
                + " frozen=" + std::to_string(cell.frozen) + ">";
        }, "Developer-friendly representation")
        .def("__str__", [](const position_cell& cell) {
            return "Position: " + std::to_string(cell.position) + 
                   ", Frozen: " + std::to_string(cell.frozen);
        }, "Human-readable string representation");

    // Bind order_info struct
    nb::class_<order_info>(m, "OrderInfo", "Represents order information")
        .def_ro("estid", &order_info::estid, "Order ID")
        .def_ro("code", &order_info::code, "Contract code")
        .def_ro("total_volume", &order_info::total_volume, "Total order volume")
        .def_ro("last_volume", &order_info::last_volume, "Last traded volume")
        .def_ro("create_time", &order_info::create_time, "Order creation time")
        .def_ro("offset", &order_info::offset, "Order offset type")
        .def_ro("direction", &order_info::direction, "Order direction")
        .def_ro("price", &order_info::price, "Order price")
        .def("__repr__", [](const order_info& order) {
            std::ostringstream oss;
            oss << "<OrderInfo estid=" << order.estid 
                << " code='" << order.code.to_string() 
                << "' direction=" << direction_type_to_string(order.direction)
                << " offset=" << offset_type_to_string(order.offset)
                << " price=" << order.price 
                << " volume=" << order.total_volume << ">";
            return oss.str();
        }, "Developer-friendly representation")
        .def("__str__", [](const order_info& order) {
            std::ostringstream oss;
            oss << "Order " << order.estid 
                << ": " << order.code.to_string()
                << " " << direction_type_to_string(order.direction)
                << " " << offset_type_to_string(order.offset)
                << " @ " << order.price 
                << " (" << order.total_volume << " lots)";
            return oss.str();
        }, "Human-readable string representation");

    // Bind position_info struct
    nb::class_<position_info>(m, "PositionInfo", "Represents complete position information for a contract")
        .def_ro("code", &position_info::code, "Contract code")
        .def_ro("long_pending", &position_info::long_pending, "Pending long orders")
        .def_ro("short_pending", &position_info::short_pending, "Pending short orders")
        .def_ro("current_long", &position_info::current_long, "Current long position")
        .def_ro("current_short", &position_info::current_short, "Current short position")
        .def_ro("history_long", &position_info::history_long, "Historical long position")
        .def_ro("history_short", &position_info::history_short, "Historical short position")
        .def("get_total", &position_info::get_total, "Get total position (long + short)")
        .def("get_real", &position_info::get_real, "Get real position (current + history)")
        .def("get_long_position", &position_info::get_long_position, "Get total long position")
        .def("get_short_position", &position_info::get_short_position, "Get total short position")
        .def("__repr__", [](const position_info& pos) {
            std::ostringstream oss;
            oss << "<PositionInfo code='" << pos.code.to_string() 
                << "' long=" << pos.get_long_position()
                << " short=" << pos.get_short_position() << ">";
            return oss.str();
        }, "Developer-friendly representation")
        .def("__str__", [](const position_info& pos) {
            std::ostringstream oss;
            oss << "Position " << pos.code.to_string() 
                << ": Long=" << pos.get_long_position()
                << " Short=" << pos.get_short_position();
            return oss.str();
        }, "Human-readable string representation");

    // Bind tick_info struct
    nb::class_<lt::tick_info>(m, "TickInfo", "Represents market tick data")
        .def_ro("code", &lt::tick_info::code, "Contract code")
        .def_ro("time", &lt::tick_info::time, "Timestamp")
        .def_ro("price", &lt::tick_info::price, "Last traded price")
        .def_ro("volume", &lt::tick_info::volume, "Volume")
        .def_ro("open_interest", &lt::tick_info::open_interest, "Open interest")
        .def_ro("average_price", &lt::tick_info::average_price, "Average price")
        .def_ro("trading_day", &lt::tick_info::trading_day, "Trading day")
        .def("__repr__", [](const lt::tick_info& tick) {
            std::ostringstream oss;
            oss << "<TickInfo code='" << tick.code.to_string() 
                << "' price=" << tick.price 
                << " time=" << tick.time << ">";
            return oss.str();
        }, "Developer-friendly representation")
        .def("__str__", [](const lt::tick_info& tick) {
            std::ostringstream oss;
            oss << "Tick " << tick.code.to_string() 
                << ": " << tick.price 
                << " @ " << tick.time;
            return oss.str();
        }, "Human-readable string representation");

    // Bind bar_info struct
    nb::class_<lt::bar_info>(m, "BarInfo", "Represents market bar/kline data")
        .def_ro("code", &lt::bar_info::code, "Contract code")
        .def_ro("time", &lt::bar_info::time, "Timestamp")
        .def_ro("period", &lt::bar_info::period, "Bar period in seconds")
        .def_ro("open", &lt::bar_info::open, "Open price")
        .def_ro("close", &lt::bar_info::close, "Close price")
        .def_ro("high", &lt::bar_info::high, "High price")
        .def_ro("low", &lt::bar_info::low, "Low price")
        .def_ro("volume", &lt::bar_info::volume, "Volume")
        .def("__repr__", [](const lt::bar_info& bar) {
            std::ostringstream oss;
            oss << "<BarInfo code='" << bar.code.to_string() 
                << "' period=" << bar.period
                << " ohlc=(" << bar.open << "," << bar.high << "," << bar.low << "," << bar.close << ")>";
            return oss.str();
        }, "Developer-friendly representation")
        .def("__str__", [](const lt::bar_info& bar) {
            std::ostringstream oss;
            oss << "Bar " << bar.code.to_string() 
                << " (" << bar.period << "s): " 
                << bar.open << "/" << bar.high << "/" << bar.low << "/" << bar.close;
            return oss.str();
        }, "Human-readable string representation");

    // Bind order_statistic struct
    nb::class_<order_statistic>(m, "OrderStatistic", "Represents order statistics for a contract")
        .def_ro("place_order_amount", &order_statistic::place_order_amount, "Total placed orders")
        .def_ro("entrust_amount", &order_statistic::entrust_amount, "Total entrusted orders")
        .def_ro("trade_amount", &order_statistic::trade_amount, "Total traded orders")
        .def_ro("cancel_amount", &order_statistic::cancel_amount, "Total cancelled orders")
        .def_ro("error_amount", &order_statistic::error_amount, "Total error orders")
        .def("__repr__", [](const order_statistic& stat) {
            std::ostringstream oss;
            oss << "<OrderStatistic placed=" << stat.place_order_amount
                << " entrusted=" << stat.entrust_amount
                << " traded=" << stat.trade_amount
                << " cancelled=" << stat.cancel_amount
                << " errors=" << stat.error_amount << ">";
            return oss.str();
        }, "Developer-friendly representation")
        .def("__str__", [](const order_statistic& stat) {
            std::ostringstream oss;
            oss << "OrderStats: Placed=" << stat.place_order_amount
                << ", Entrusted=" << stat.entrust_amount
                << ", Traded=" << stat.trade_amount
                << ", Cancelled=" << stat.cancel_amount
                << ", Errors=" << stat.error_amount;
            return oss.str();
        }, "Human-readable string representation");

    // Bind bridge class
    nb::class_<bridge>(m, "bridge", "Main trading bridge class")
        .def(nb::init<const std::string&>(), nb::arg("json_config"), 
             "Initialize bridge with JSON configuration string\n\n"
             "Args:\n"
             "    json_config (str): JSON string containing configuration\n"
             "        Required fields: account_config, section_config\n"
             "        Optional fields: launch_setting, ltds_setting, contract_settings")
        .def("polling", &bridge::polling, "The trading loop and connect to market/trader")
        .def("set_entrust_callback", &bridge::set_entrust_callback, 
             nb::arg("callback"), "Set callback for order entrust events\n\n"
             "Callback signature: callback(order_info) -> None")
        .def("set_deal_callback", &bridge::set_deal_callback, 
             nb::arg("callback"), "Set callback for deal events\n\n"
             "Callback signature: callback(estid, deal_volume) -> None")
        .def("set_trade_callback", &bridge::set_trade_callback, 
             nb::arg("callback"), "Set callback for trade events\n\n"
             "Callback signature: callback(estid, code, offset, direction, price, volume) -> None")
        .def("set_cancel_callback", &bridge::set_cancel_callback, 
             nb::arg("callback"), "Set callback for cancel events\n\n"
             "Callback signature: callback(estid, code, offset, direction, price, cancel_volume, total_volume) -> None")
        .def("set_error_callback", &bridge::set_error_callback, 
             nb::arg("callback"), "Set callback for error events\n\n"
             "Callback signature: callback(error_type, estid, error_code) -> None")
        .def("set_tick_callback", &bridge::set_tick_callback, 
             nb::arg("callback"), "Set callback for tick events\n\n"
             "Callback signature: callback(tick_info) -> None")
        .def("set_bar_callback", &bridge::set_bar_callback, 
             nb::arg("callback"), "Set callback for bar events\n\n"
             "Callback signature: callback(bar_info) -> None")
        .def("get_trading_day", &bridge::get_trading_day, "Get current trading day as integer (YYYYMMDD)")
        .def("is_trading", nb::overload_cast<const code_t&>(&bridge::is_trading), 
             nb::arg("code"), "Check if a symbol is currently in trading time")
        .def("is_trading", [](bridge& self, const std::string& code) {
            return self.is_trading(string_to_code(code));
        }, nb::arg("code"), "Check if a symbol is currently in trading time (string version)")
        .def("get_last_tick", nb::overload_cast<const code_t&>(&bridge::get_last_tick), 
             nb::arg("code"), "Get last tick for a symbol")
        .def("get_last_tick", [](bridge& self, const std::string& code) {
            return self.get_last_tick(string_to_code(code));
        }, nb::arg("code"), "Get last tick for a symbol (string version)")
        .def("get_order_statistic", nb::overload_cast<const code_t&>(&bridge::get_order_statistic), 
             nb::arg("code"), "Get order statistic for a symbol")
        .def("get_order_statistic", [](bridge& self, const std::string& code) {
            return self.get_order_statistic(string_to_code(code));
        }, nb::arg("code"), "Get order statistic for a symbol (string version)")
        .def("get_all_contract", &bridge::get_all_contract, "Get all configured contracts as list of Code objects")
        .def("get_kline", nb::overload_cast<const code_t&, uint32_t>(&bridge::get_kline), 
             nb::arg("code"), nb::arg("period"), "Get kline data for a symbol and period")
        .def("get_kline", [](bridge& self, const std::string& code, uint32_t period) {
            return self.get_kline(string_to_code(code), period);
        }, nb::arg("code"), nb::arg("period"), "Get kline data for a symbol and period (string version)")
        .def("get_ticks", nb::overload_cast<const code_t&, uint32_t>(&bridge::get_ticks), 
             nb::arg("code"), nb::arg("length"), "Get tick data for a symbol")
        .def("get_ticks", [](bridge& self, const std::string& code, uint32_t length) {
            return self.get_ticks(string_to_code(code), length);
        }, nb::arg("code"), nb::arg("length"), "Get tick data for a symbol (string version)")
        .def("get_order", &bridge::get_order, 
             nb::arg("estid"), "Get a specific order by estid")
        .def("get_all_orders", &bridge::get_all_orders, "Get all current orders as list of OrderInfo objects")
        .def("get_position", nb::overload_cast<const code_t&>(&bridge::get_position), 
             nb::arg("code"), "Get a specific position by contract code")
        .def("get_position", [](bridge& self, const std::string& code) {
            return self.get_position(string_to_code(code));
        }, nb::arg("code"), "Get a specific position by contract code (string version)")
        .def("get_all_positions", &bridge::get_all_positions, "Get all current positions as list of PositionInfo objects")
        .def("place_order", nb::overload_cast<const code_t&, offset_type, direction_type, uint32_t, double_t>(&bridge::place_order), 
             nb::arg("code"), nb::arg("offset"), nb::arg("direction"), nb::arg("count"), nb::arg("price"),
             "Place an order\n\n"
             "Returns:\n"
             "    estid_t: Order ID, or INVALID_ESTID if failed")
        .def("place_order", [](bridge& self, const std::string& code, offset_type offset, direction_type direction, uint32_t count, double_t price) {
            return self.place_order(string_to_code(code), offset, direction, count, price);
        }, nb::arg("code"), nb::arg("offset"), nb::arg("direction"), nb::arg("count"), nb::arg("price"),
           "Place an order (string version)\n\n"
           "Returns:\n"
           "    estid_t: Order ID, or INVALID_ESTID if failed")
        .def("cancel_order", &bridge::cancel_order, 
             nb::arg("estid"), "Cancel an order\n\n"
             "Returns:\n"
             "    bool: True if cancellation request was sent successfully");

    // Helper functions for easier Python usage
    m.def("string_to_code", &string_to_code, nb::arg("code_str"), "Convert string to Code object");
    m.def("string_to_offset_type", &string_to_offset_type, nb::arg("offset_str"), "Convert string to OffsetType enum");
    m.def("string_to_direction_type", &string_to_direction_type, nb::arg("direction_str"), "Convert string to DirectionType enum");
    m.def("offset_type_to_string", &offset_type_to_string, nb::arg("offset"), "Convert OffsetType enum to string");
    m.def("direction_type_to_string", &direction_type_to_string, nb::arg("direction"), "Convert DirectionType enum to string");

    // Constants
    m.attr("INVALID_ESTID") = INVALID_ESTID;

    // Module-level convenience functions
    m.def("is_valid_estid", [](uint64_t estid) {
        return estid != INVALID_ESTID;
    }, nb::arg("estid"), "Check if an estid is valid (not equal to INVALID_ESTID)");
}
