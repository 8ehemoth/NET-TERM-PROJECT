// Common.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define _WINSOCKAPI_
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define ID_MAX      50
#define MSG_MAX     512
#define SERVERPORT  9000
#define BUFSIZE     512

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <memory>
#include <functional>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

inline void err_quit(const char* msg) {
    LPVOID buf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf, 0, NULL);
    MessageBoxA(NULL, (LPCSTR)buf, msg, MB_ICONERROR);
    LocalFree(buf);
    exit(1);
}
inline void err_display(const char* msg) {
    LPVOID buf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&buf, 0, NULL);
    printf("[%s] %s\n", msg, (char*)buf);
    LocalFree(buf);
}
