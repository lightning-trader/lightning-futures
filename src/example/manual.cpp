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

#include "manual.h"
#include <define.h>
#include <thread>
#include <windows.h>
#include "runtime_engine.h"
#include "replace_strategy.h"
#include <time_utils.hpp>
// 全局变量用于存储钩子句柄
HHOOK hKeyboardHook = NULL;
std::shared_ptr<lt::hft::runtime_engine> app = nullptr;
lt::hft::straid_t current_strategy = 0U;
// 键盘钩子回调函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        // 获取按键信息
        KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;

        // 检查按键按下事件
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            // 检查特定按键
            switch (pKeyInfo->vkCode)
            {
            case 0x26: // 上键
            case 0x57: // W键
                if (app)
                {
                    app->change_strategy(current_strategy, true, true, "ratio=1");
                }
                std::cout << "开多" << std::endl;
                break;
            case 0x28: // 下键
            case 0x53: // S键
                if (app)
                {
                    app->change_strategy(current_strategy, true, true, "ratio=-1");
                }
                std::cout << "开空" << std::endl;
                break;
            case 0x20: // 空格
                if (app)
                {
                    app->change_strategy(current_strategy, true, true, "ratio=0");
                }
                std::cout << "平仓" << std::endl;
                break;

            }
        }
        // 将消息传递给下一个钩子
        return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
    };
}
void start_maual(const char* account_config)
{
    app = std::make_shared<lt::hft::runtime_engine>(account_config);
    std::vector<std::shared_ptr<lt::hft::strategy>> strategys;
	current_strategy = 1U;
    strategys.emplace_back(std::make_shared<replace_strategy>(current_strategy, app.get(), "SHFE.rb2510", 10));
    app->set_trading_filter([](const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, uint32_t count, double_t price, lt::order_flag flag)->bool {
        auto now = app->get_last_time();
        auto last_order = app->last_order_time();
        if (now - last_order < 1000)
        {
            return false;
        }
        return true;
        });
    app->start_trading(strategys);
    // 安装键盘钩子
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);

    if (hKeyboardHook == NULL)
    {
        std::cerr << "获取键盘数据错误!" << std::endl;
        return;
    }
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    app->stop_trading();
    // 卸载键盘钩子
    UnhookWindowsHookEx(hKeyboardHook);
};


int main(int argc, char* argv[])
{
    start_maual("rt_simnow.ini");
    return 0;
}
