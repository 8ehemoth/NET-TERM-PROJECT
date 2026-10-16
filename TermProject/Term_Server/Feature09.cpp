// Feature09.cpp
// ─────────────────────────────────────────────
// 디버깅 모드일 때 로그 출력 함수
// ─────────────────────────────────────────────

#include "../Common.h"
extern bool debugMode;
extern void DisplayText(const char* fmt, ...);

void DebugLog(const char* fmt, ...)
{
    if (!debugMode) return;

    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    DisplayText("[DEBUG] %s", buf);
}
