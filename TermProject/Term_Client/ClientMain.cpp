// ClientMain.cpp
// ─────────────────────────────────────────────
// [전체 클라이언트 앱 메인]
// - WinMain: 메인 함수
// - DlgProc: 메인 대화상자 콜백, 탭 구조/이벤트/연결/스레드
// - 전역 변수, 탭 컨트롤, 서버 IP 관리 등
// ─────────────────────────────────────────────

#include "../Common.h"
#include "../ChatMsg.h"
#include "../PrjClientFile.h"
#include "ClientFeature.h"
#include "ClientSocket.h"
#include "resource.h"
#include "../UiDispatch.h"

#pragma comment(lib, "comctl32.lib")

extern HWND hEditCalcInput;
extern HWND hEditCalcResult;
extern INT_PTR CALLBACK ChatTabProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK FileTabProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR CALLBACK CalcTabProc(HWND, UINT, WPARAM, LPARAM);
extern unsigned __stdcall RecvThread(void*);

HINSTANCE g_hInst = NULL;
SOCKET    sock = INVALID_SOCKET;
char      buf[MSG_MAX + 1] = {};
char      user_id[ID_MAX + 1] = {};
bool      enrolled = false;

HWND hDlg = NULL;
HWND hTab = NULL;
HWND hTabDlg[3] = { NULL, NULL, NULL };

HWND hEdit1 = NULL;
HWND hEdit2 = NULL;
HWND hEdit3 = NULL;
HWND hEditFilePath = NULL;
HWND hEditFileLog = NULL;
HWND hProgress = NULL;

char server_ip[32] = "127.0.0.1"; // 서버 IP 입력(기본값)

INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    g_hInst = hInst;
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_TAB_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&ic);

    DialogBoxParamA(
        hInst,
        MAKEINTRESOURCEA(IDD_DIALOG1),
        NULL,
        DlgProc,
        0);

    SockCleanup();
    return 0;
}

// 메인 대화상자 프로시저(탭, 연결 버튼 등 처리)
INT_PTR CALLBACK DlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        hDlg = dlg;
        hTab = GetDlgItem(dlg, IDC_TAB_MAIN);

        SetWindowTextA(GetDlgItem(dlg, IDC_EDIT_SERVERIP), server_ip); // 기본값

        // 탭 생성
        TCITEM ti = {};
        ti.mask = TCIF_TEXT;
        ti.pszText = const_cast<LPSTR>("채팅");
        TabCtrl_InsertItem(hTab, 0, &ti);
        ti.pszText = const_cast<LPSTR>("파일 전송");
        TabCtrl_InsertItem(hTab, 1, &ti);
        ti.pszText = const_cast<LPSTR>("계산기");
        TabCtrl_InsertItem(hTab, 2, &ti);

        // 탭별 자식 대화상자 생성
        hTabDlg[0] = CreateDialogParamA(g_hInst, MAKEINTRESOURCEA(IDD_CHATTAB), dlg, ChatTabProc, 0);
        hTabDlg[1] = CreateDialogParamA(g_hInst, MAKEINTRESOURCEA(IDD_FILETAB), dlg, FileTabProc, 0);
        hTabDlg[2] = CreateDialogParamA(g_hInst, MAKEINTRESOURCEA(IDD_CALCTAB), dlg, CalcTabProc, 0);

        RECT rcTab;
        GetClientRect(hTab, &rcTab);
        TabCtrl_AdjustRect(hTab, FALSE, &rcTab);
        POINT pt = { rcTab.left, rcTab.top };
        MapWindowPoints(hTab, dlg, &pt, 1);
        int left = pt.x, top = pt.y, width = rcTab.right - rcTab.left, height = rcTab.bottom - rcTab.top;
        for (int i = 0; i < 3; i++) {
            SetWindowPos(hTabDlg[i], HWND_TOP, left, top, width, height, SWP_NOZORDER);
        }
        ShowWindow(hTabDlg[0], SW_SHOW);
        ShowWindow(hTabDlg[1], SW_HIDE);
        ShowWindow(hTabDlg[2], SW_HIDE);
        return TRUE;
    }

    case WM_UI_ACTION:
        RunUiAction(lp);return TRUE;
    case WM_CLOSE:
        SendMessageA(dlg,WM_COMMAND,IDCANCEL,0);return TRUE;
    case WM_NOTIFY:
        if (((LPNMHDR)lp)->idFrom == IDC_TAB_MAIN &&
            ((LPNMHDR)lp)->code == TCN_SELCHANGE)
        {
            int sel = TabCtrl_GetCurSel(hTab);
            for (int i = 0; i < 3; i++)
                ShowWindow(hTabDlg[i], (i == sel) ? SW_SHOW : SW_HIDE);
            return TRUE;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wp) == IDCANCEL) {
            SockCleanup();
            DiscardUiActions(dlg);
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        // 서버 연결 버튼
        if (LOWORD(wp) == IDC_BUTTON_CONNECT) {
            if(connected.load()) {MessageBoxA(dlg,"이미 연결되어 있습니다.","안내",MB_OK);break;}
            GetWindowTextA(GetDlgItem(dlg, IDC_EDIT_SERVERIP), server_ip, sizeof(server_ip));
            if (!SockInit()) {
                MessageBoxA(dlg, "WinSock 초기화 실패", "오류", MB_ICONERROR);
                break;
            }
            if (!SockConnect(server_ip, SERVERPORT)) {
                MessageBoxA(dlg, "서버 연결 실패", "오류", MB_ICONERROR);
                break;
            }
            EnableWindow(GetDlgItem(dlg,IDC_BUTTON_CONNECT),FALSE);
            clientWorkers.emplace_back([]{RecvThread(nullptr);});
            MessageBoxA(dlg, "서버 연결 성공", "알림", MB_OK);
            break;
        }
        break;
    }
    return FALSE;
}
