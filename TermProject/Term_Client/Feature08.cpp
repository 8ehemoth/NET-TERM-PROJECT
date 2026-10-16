// Feature08.cpp
// ─────────────────────────────────────────────
// 계산기 탭/스레드/입력 처리 등
// - 서버 연결, 수식 송신, 결과 수신, 입력 엔터키 처리
// ─────────────────────────────────────────────

#include "../Common.h"
#include "../ChatMsg.h"
#include "ClientFeature.h"
#include "ClientSocket.h"
#include "../NetUtil.h"
#include "../UiDispatch.h"


// extern 전역
extern HWND hEditCalcInput, hEditCalcResult;
extern char server_ip[32];

// ── 계산기 Enter 키 처리용 서브클래스 프로시저 ────────────────
LRESULT CALLBACK CalcEditSubclassProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRef*/)
{
    switch (msg)
    {
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;

    case WM_KEYDOWN:
        if (wp == VK_RETURN) {
            HWND parent = GetParent(hWnd);
            // '=' 버튼 눌러주기
            SendMessage(parent,
                WM_COMMAND,
                MAKELONG(IDC_BUTTON_CALC_SEND, BN_CLICKED),
                (LPARAM)GetDlgItem(parent, IDC_BUTTON_CALC_SEND));
            // 포커스 유지
            SetFocus(hEditCalcInput);
            return 0;
        }
        break;

    case WM_CHAR:
        if (wp == VK_RETURN) return 0;
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, CalcEditSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, msg, wp, lp);
}

// ── 계산기 요청 스레드 ────────────────────────────────
void StartCalculation() {
    if(calcPending)return;
    if(!SockInit()){SetWindowTextA(hEditCalcResult,"[Winsock 초기화 실패]");return;}
    char expr[256]{};GetWindowTextA(hEditCalcInput,expr,sizeof(expr));
    if(!*expr){SetWindowTextA(hEditCalcResult,"[수식을 입력하세요]");return;}
    std::string input=expr,ip=server_ip;calcPending=true;
    clientWorkers.emplace_back([input,ip]{
        std::string result="[서버 연결 실패]";
        SOCKET cs=ConnectTcp(ip.c_str(),9001,2000,&clientRunning);
        if(cs!=INVALID_SOCKET) {
            CalcMsg h{MSG_CALC,static_cast<int>(input.size())};
            int length=0;
            if(SendExact(cs,&h,sizeof(h),10000,&clientRunning) && SendExact(cs,input.data(),h.exprLen,10000,&clientRunning) &&
               RecvExact(cs,&length,sizeof(length),10000,&clientRunning) && length>0 && length<=SERVER_TEXT_MAX) {
                result.assign(length,'\0');
                if(!RecvExact(cs,&result[0],length,10000,&clientRunning))result="[서버 응답 오류]";
            } else result="[서버 응답 오류]";
            closesocket(cs);
        }
        PostUi(::hDlg,[result]{SetWindowTextA(hEditCalcResult,result.c_str());calcPending=false;});
    });
}

// ── 계산기 탭 콜백 ─────────────────────────────────────────
INT_PTR CALLBACK CalcTabProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        hEditCalcInput = GetDlgItem(dlg, IDC_EDIT_CALC_INPUT);
        hEditCalcResult = GetDlgItem(dlg, IDC_EDIT_CALC_RESULT);
        SendMessageA(hEditCalcInput,EM_SETLIMITTEXT,CALC_EXPR_MAX,0);
        SetWindowSubclass(hEditCalcInput, CalcEditSubclassProc, 1, 0);
        return TRUE;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        // 숫자
        if (id >= IDC_BUTTON_CALC_0 && id <= IDC_BUTTON_CALC_9) {
            char buf[256]; GetWindowTextA(hEditCalcInput, buf, 256);
            int len = lstrlenA(buf);
            if(len >= 255)return TRUE;
            buf[len] = char('0' + (id - IDC_BUTTON_CALC_0));
            buf[len + 1] = '\0';
            SetWindowTextA(hEditCalcInput, buf);
            return TRUE;
        }
        // 연산자
        if (id >= IDC_BUTTON_CALC_ADD && id <= IDC_BUTTON_CALC_DIV) {
            char op = (id == IDC_BUTTON_CALC_ADD ? '+' :
                id == IDC_BUTTON_CALC_SUB ? '-' :
                id == IDC_BUTTON_CALC_MUL ? '*' : '/');
            char buf[256]; GetWindowTextA(hEditCalcInput, buf, 256);
            int len = lstrlenA(buf);
            if (len > 0 && len < 255) {
                buf[len] = op;
                buf[len + 1] = '\0';
                SetWindowTextA(hEditCalcInput, buf);
            }
            return TRUE;
        }
        // 소수점
        if (id == IDC_BUTTON_CALC_DOT) {
            char buf[256]; GetWindowTextA(hEditCalcInput, buf, 256);
            int len = lstrlenA(buf);
            if (len < 255) {
                buf[len] = '.';
                buf[len + 1] = '\0';
                SetWindowTextA(hEditCalcInput, buf);
            }
            return TRUE;
        }
        // 한 글자 지우기
        if (id == IDC_BUTTON_CALC_BS) {
            char buf[256]; GetWindowTextA(hEditCalcInput, buf, 256);
            int len = lstrlenA(buf);
            if (len > 0) {
                buf[len - 1] = '\0';
                SetWindowTextA(hEditCalcInput, buf);
            }
            return TRUE;
        }
        // 전체 지우기
        if (id == IDC_BUTTON_CALC_AC) {
            SetWindowTextA(hEditCalcInput, "");
            SetWindowTextA(hEditCalcResult, "");
            return TRUE;
        }
        // = (계산 요청) → 스레드에서 처리!
        if (id == IDC_BUTTON_CALC_SEND) {
            StartCalculation();
            return TRUE;
        }
    } break;
    }
    return FALSE;
}
