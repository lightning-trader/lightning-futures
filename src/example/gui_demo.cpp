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
#include <filesystem>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "runtime.hpp"
#include "gui_bridge_strategy.h"

namespace
{
	constexpr UINT_PTR UI_TIMER_ID = 2001U;
	constexpr UINT UI_REFRESH_INTERVAL_MS = 33U;
	constexpr lt::hft::straid_t MANUAL_UI_STRATEGY_ID = 9527U;
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
}

class market_chart_ctrl : public CWnd
{
public:
	void set_data(const std::vector<gui_bridge_strategy::tick_row>& ticks, double last_price, double average_price)
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
	std::vector<gui_bridge_strategy::tick_row> _ticks;
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
			_app = std::make_shared<lt::hft::runtime>(runtime_config.string().c_str(), control_config.string().c_str(), section_config.string().c_str());
			_bridge = std::make_shared<gui_bridge_strategy>(MANUAL_UI_STRATEGY_ID, _app.get());
			_app->regist_strategy({ _bridge });
		}
		catch (const std::exception& ex)
		{
			AfxMessageBox(to_cstring(ex.what()), MB_ICONERROR | MB_OK);
			throw;
		}
	}

	virtual ~demo_gui_frame()
	{
		stop_runtime();
	}

	bool is_service_started() const
	{
		return _trading_ready && !_trading_starting;
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
		start_runtime();
		SetTimer(UI_TIMER_ID, UI_REFRESH_INTERVAL_MS, nullptr);
		refresh_ui();
		return 0;
	}

	afx_msg void OnDestroy()
	{
		KillTimer(UI_TIMER_ID);
		stop_runtime();
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

		_combo_code.Create(combo_list_style, CRect(0, 0, 0, 260), this, IDC_COMBO_CODE);
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
		const int action_button_width = (risk_inner_width - action_gap * 2) / 3;
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
		_button_cancel_all.MoveWindow(risk_x + 22, button_row_y, action_button_width, button_height);
		_button_pause.MoveWindow(risk_x + 22 + action_button_width + action_gap, button_row_y, action_button_width, button_height);
		_button_close_all.MoveWindow(apply_x + apply_width - action_button_width, button_row_y, action_button_width, button_height);
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
		_button_cancel_all.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_button_close_all.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);

		_group_monitor.ShowWindow(show_risk ? SW_HIDE : SW_SHOW);
		_button_pause.ShowWindow(show_risk ? SW_SHOW : SW_HIDE);
		_summary_label.ShowWindow(show_risk ? SW_HIDE : SW_SHOW);
		_monitor_label.ShowWindow(show_risk ? SW_HIDE : SW_SHOW);
	}

	void start_runtime()
	{
		_trading_ready = false;
		_trading_starting = true;
		_stop_requested = false;
		_trading_thread = std::thread([this]() {
			_app->start_trading();
			if (!_stop_requested)
			{
				_trading_ready = true;
			}
			_trading_starting = false;
		});
	}

	void stop_runtime()
	{
		_stop_requested = true;
		if (_trading_ready)
		{
			_app->stop_trading();
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

	void refresh_instrument_combo(const gui_bridge_strategy::snapshot& snapshot)
	{
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
			: get_window_text(_combo_code);
		if (!effective_order_code.empty())
		{
			_combo_code.SelectString(-1, to_cstring(effective_order_code));
		}
		else if (_combo_code.GetCount() > 0 && _combo_code.GetCurSel() == CB_ERR)
		{
			_combo_code.SetCurSel(0);
		}
		sync_favorite_selection();
	}

	void refresh_market_panel(const gui_bridge_strategy::snapshot& snapshot)
	{
		_chart_market.set_data(snapshot.recent_ticks, snapshot.last_price, snapshot.average_price);
	}

	void sync_order_price_with_focus(const gui_bridge_strategy::snapshot& snapshot)
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

	void refresh_tick_panel(const gui_bridge_strategy::snapshot& snapshot)
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

	void refresh_order_list(const gui_bridge_strategy::snapshot& snapshot)
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

	void refresh_position_list(const gui_bridge_strategy::snapshot& snapshot)
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

	void refresh_logs(const gui_bridge_strategy::snapshot& snapshot)
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

	void refresh_monitor(const gui_bridge_strategy::snapshot& snapshot)
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
		const auto snapshot = _bridge->get_snapshot();
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
		_app->change_strategy(MANUAL_UI_STRATEGY_ID, command);
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
			const auto snapshot = _bridge->get_snapshot();
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

		const auto snapshot = _bridge->get_snapshot();
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
		const auto snapshot = _bridge->get_snapshot();
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
		const auto snapshot = _bridge->get_snapshot();
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
		const std::string current_focus = !_pending_market_focus_code.empty() ? _pending_market_focus_code : _bridge->get_snapshot().focused_code;
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

	void set_market_focus_code(const std::string& code, bool notify_runtime)
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
		if (notify_runtime)
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
		_combo_code.SelectString(-1, to_cstring(code));
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
	std::shared_ptr<lt::hft::runtime> _app;
	std::shared_ptr<gui_bridge_strategy> _bridge;
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
