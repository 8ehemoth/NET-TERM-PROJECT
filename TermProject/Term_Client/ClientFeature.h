// ClientFeature.h
// ─────────────────────────────────────────────
// 클라이언트 공용 함수/전역 변수 extern 선언부
// ─────────────────────────────────────────────

#pragma once
#include "../Common.h"
#include "../ChatMsg.h"

#define WM_ENABLE_SEND_BUTTON  (WM_APP + 1)
extern HWND hTabDlg[3];
extern HWND hEdit2;
extern char buf[];
extern char user_id[];
extern bool enrolled;
extern HWND hDlg;

// 출력
void DisplayText(const char* fmt, ...);

// 파일 탭
extern HWND hEditFilePath, hEditFileLog, hProgress;
void DisplayFileLog(HWND hLogEdit, const char* fmt, ...);
void ClientFeature_OnFileSelect(HWND hPathEdit);
void ClientFeature_OnFileSend(SOCKET sock, HWND hPathEdit, HWND hProgress, HWND hLogEdit);

// 계산기 탭
extern HWND hEditCalcInput, hEditCalcResult;
void ClientFeature_OnCalcSend(HWND hInputEdit, HWND hResultEdit);

// 서버 수신 메시지 처리
void Feature_OnReceive(int type, const char* id, const char* msg);
void Feature_RegisterID(const char* id);
void Feature_SendChat(const char* msg);
