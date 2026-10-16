// ServerMain.cpp
// ──────────────────────────────────────────────────────────
// 메인 진입점 및 대화상자 프로시저(DlgProc), 로그 출력 함수 정의
// - 프로그램 전체의 전역 변수 및 GUI 핸들 선언
// - 서버 스레드(IoLoop, CalcLoop) 시작
// - 디버그 모드 /d 옵션 지원
// - 클라이언트 리스트/로그 컨트롤 초기화, 강퇴 버튼/선택 기능
// ──────────────────────────────────────────────────────────

#define WIN32_LEAN_AND_MEAN
#include "../Common.h"
#include "../ChatMsg.h"
#include "ServerFeature.h"
#include "../PrjClientFile.h"
#pragma comment(lib, "comctl32.lib")
#include "resource.h"
#include "../UiDispatch.h"

// ─── 전역 변수 선언 ───
std::vector<ChatLog> chat_log; // 채팅 로그(복원용)
CRITICAL_SECTION     cs_log;   // 채팅 로그 임계영역
SOCKETINFO* client_list[MAX_CLIENTS] = {}; // 클라이언트 목록
HWND hDlg = NULL;           // 메인 다이얼로그 핸들
HWND hListLog = NULL;       // 로그창(채팅 메시지)
HWND hListClients = NULL;   // 접속자 리스트 박스
HWND hEditKickID = NULL;    // 강퇴 ID 입력창
std::recursive_mutex clientsMutex;
std::atomic<bool> serverRunning{false};
std::thread ioWorker,calcWorker;
bool winsockReady=false;
bool debugMode = false;     // 디버그 모드 플래그

#ifndef WM_UPDATE_CLIENTLIST
#define WM_UPDATE_CLIENTLIST (WM_APP + 100)
#endif

// 외부 선언
extern void UpdateClientList();
extern void KickSelectedClient();
extern void OnClientListSelect();

extern void IoLoop();
extern void CalcLoop();

// ────────────── 로그 출력 ──────────────
void DisplayText(const char* fmt, ...)
{
    char buf[BUFSIZE * 2];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf) - 3, fmt, args);
    va_end(args);

    if (n < 0) n = 0;
    if (n > (int)sizeof(buf) - 3) n = sizeof(buf) - 3;
    buf[n] = '\0';

    // 개행 보정
    std::string fixed;
    for (int i = 0; buf[i]; ++i) {
        if (buf[i] == '\r') {
            if (buf[i + 1] != '\n') fixed += "\r\n";
            else fixed += '\r';
        }
        else if (buf[i] == '\n') {
            if (i == 0 || buf[i - 1] != '\r') fixed += "\r\n";
            else fixed += '\n';
        }
        else fixed += buf[i];
    }
    if (fixed.size() < 2 || fixed.substr(fixed.size() - 2) != "\r\n")
        fixed += "\r\n";

    PostUi(hDlg,[fixed] {
        SendMessageA(hListLog, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessageA(hListLog, EM_REPLACESEL, FALSE, (LPARAM)fixed.c_str());
        SendMessageA(hListLog, EM_SCROLL, SB_BOTTOM, 0);
    });
}

// ────────────── 메인 대화상자 프로시저 ──────────────
INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        hDlg=dlg;
        hListLog = GetDlgItem(dlg, IDC_LIST_LOG);
        SendMessageA(hListLog, EM_LIMITTEXT, 0, 0);
        hListClients = GetDlgItem(dlg, IDC_LIST_CLIENTS);
        hEditKickID = GetDlgItem(dlg, IDC_EDIT_KICKID);

        {
            WSADATA w;
            if(WSAStartup(MAKEWORD(2,2),&w)!=0) {
                MessageBoxA(dlg,"Winsock 초기화 실패","오류",MB_ICONERROR);
                EndDialog(dlg,0);return TRUE;
            }
            winsockReady=true;
        }
        UpdateClientList();

        serverRunning=true;
        ioWorker=std::thread(IoLoop);
        calcWorker=std::thread(CalcLoop);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            serverRunning=false;
            if(ioWorker.joinable())ioWorker.join();
            if(calcWorker.joinable())calcWorker.join();
            DiscardUiActions(dlg);
            EndDialog(dlg, 0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_LIST_CLIENTS && HIWORD(wParam) == LBN_SELCHANGE)
            OnClientListSelect();
        if (LOWORD(wParam) == IDC_BUTTON_KICK)
            KickSelectedClient();
        break;

    case WM_CLOSE:
        SendMessageA(dlg,WM_COMMAND,IDCANCEL,0);return TRUE;
    case WM_UI_ACTION:
        RunUiAction(lParam);return TRUE;
    case WM_UPDATE_CLIENTLIST:
        UpdateClientList();
        return TRUE;
    }
    return FALSE;
}

// ────────────── WinMain ──────────────
#pragma warning(push)
#pragma warning(disable:28251)
int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE /*hPrev*/,
    LPSTR     /*lpCmdLine*/,
    int       /*nCmdShow*/
)
{
    for (int i = 1; i < __argc; ++i)
        if (_stricmp(__argv[i], "/d") == 0)
            debugMode = true;
    if (debugMode) {
        MessageBoxA(NULL, "서버를 디버깅 모드로 실행합니다.", "Debug Mode", MB_OK | MB_ICONINFORMATION);
    }
    INITCOMMONCONTROLSEX ic{ sizeof(ic), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&ic);

    InitializeCriticalSection(&cs_log);
    int ret = (int)DialogBoxParamA(
        hInst,
        MAKEINTRESOURCEA(IDD_DIALOG_SERVER),
        NULL,
        DlgProc,
        0
    );
    DeleteCriticalSection(&cs_log);
    if(winsockReady)WSACleanup();
    return ret;
}
#pragma warning(pop)
