#pragma once
#include "Common.h"
constexpr UINT WM_UI_ACTION = WM_APP + 50;
inline void PostUi(HWND target, std::function<void()> action) {
    auto work = new std::function<void()>(std::move(action));
    if (!PostMessageA(target, WM_UI_ACTION, 0, reinterpret_cast<LPARAM>(work))) delete work;
}
inline void RunUiAction(LPARAM value) {
    std::unique_ptr<std::function<void()>> work(reinterpret_cast<std::function<void()>*>(value));
    (*work)();
}
inline void DiscardUiActions(HWND target) {
    MSG msg;
    while (PeekMessageA(&msg,target,WM_UI_ACTION,WM_UI_ACTION,PM_REMOVE))
        delete reinterpret_cast<std::function<void()>*>(msg.lParam);
}
