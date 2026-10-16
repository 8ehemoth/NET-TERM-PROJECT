// Feature03.cpp
// ─────────────────────────────────────────────
// 채팅탭 콜백(ChatTabProc), 에디트 서브클래스(엔터처리), 수신 스레드(RecvThread)
// 서버에서 들어오는 메시지(채팅, 복원, 강퇴 등) 처리
// ─────────────────────────────────────────────

#include "../Common.h"
#include "../ChatMsg.h"
#include "../PrjClientFile.h"
#include "ClientSocket.h"
#include "ClientFeature.h"
#include "resource.h"
#include "../NetUtil.h"
#include "../UiDispatch.h"

extern SOCKET sock;
extern HWND   hDlg;
extern HWND   hEdit1, hEdit2, hEdit3;
extern HWND   hTabDlg[3];
extern bool   enrolled;
extern char   buf[MSG_MAX + 1];
extern char   user_id[ID_MAX + 1];

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRef*/)
{
    switch (msg)
    {
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
        if (wp == VK_RETURN) {
            if (hWnd == hEdit3) {
                char tmp[ID_MAX + 1] = {};
                GetWindowTextA(hEdit3, tmp, _countof(tmp));
                Feature_RegisterID(tmp);
                MessageBeep(MB_OK);
                SetFocus(hEdit1);
            }
            else if (hWnd == hEdit1) {
                HWND p = GetParent(hWnd);
                PostMessage(p,
                    WM_COMMAND,
                    MAKELONG(IDOK, BN_CLICKED),
                    (LPARAM)GetDlgItem(p, IDOK));
            }
            return 0;
        }
        break;
    case WM_CHAR:
        if (wp == VK_RETURN) return 0;
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, EditSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hWnd, msg, wp, lp);
}

// ── 채팅 탭 콜백 ───────────────────────────
INT_PTR CALLBACK ChatTabProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        hEdit1 = GetDlgItem(dlg, IDC_EDIT1);
        hEdit2 = GetDlgItem(dlg, IDC_EDIT2);
        hEdit3 = GetDlgItem(dlg, IDC_EDIT3);

        SendMessageA(hEdit1, EM_SETLIMITTEXT, MSG_MAX, 0);
        SendMessageA(hEdit3, EM_SETLIMITTEXT, ID_MAX, 0);

        SetWindowSubclass(hEdit1, EditSubclassProc, 1, 0);
        SetWindowSubclass(hEdit3, EditSubclassProc, 1, 0);
        return TRUE;

    case WM_COMMAND:
    {
        int id = LOWORD(wp);
        int code = HIWORD(wp);

        if (id == IDC_BUTTON1 && code == BN_CLICKED) {
            char tmp[ID_MAX + 1] = {};
            GetWindowTextA(hEdit3, tmp, _countof(tmp));
            Feature_RegisterID(tmp);
            return TRUE;
        }
        else if (id == IDOK && code == BN_CLICKED) {
            char msgbuf[MSG_MAX + 1] = {};
            GetWindowTextA(hEdit1, msgbuf, _countof(msgbuf));
            Feature_SendChat(msgbuf);
            SetWindowTextA(hEdit1, "");
            SetFocus(hEdit1);
            return TRUE;
        }
        else if ((id == IDC_BUTTON_EMOJI1 || id == IDC_BUTTON_EMOJI2 || id == IDC_BUTTON_EMOJI3) && code == BN_CLICKED) {
            const char* emoji = nullptr;
            switch (id) {
            case IDC_BUTTON_EMOJI1: emoji = "( >ㅅ< )"; break;
            case IDC_BUTTON_EMOJI2: emoji = "(∩^o^)⊃━☆*";   break;
            case IDC_BUTTON_EMOJI3: emoji = "( ˘▽˘)っ♨"; break;
            }
            if (emoji) {
                Feature_SendChat(emoji);
                SetFocus(hEdit1);
            }
            return TRUE;
        }
    }
    break;
    case WM_ENABLE_SEND_BUTTON:
        EnableWindow(GetDlgItem(dlg, IDOK), TRUE);
        return TRUE;
    }
    return FALSE;
}

// ── 서버 메시지 수신 스레드 ────────────────────────────────
unsigned __stdcall RecvThread(void*) {
    std::string reason="서버와의 연결이 종료되었습니다.";
    while(clientRunning.load()) {
        fd_set rf;FD_ZERO(&rf);FD_SET(sock,&rf);timeval tv{0,100000};
        int ready=select(0,&rf,nullptr,nullptr,&tv);
        if(ready==0)continue;
        if(ready==SOCKET_ERROR)break;
        ChatMsg h{};
        if(!RecvExact(sock,&h,sizeof(h),10000,&clientRunning))break;
        if(h.idLen<0 || h.idLen>PROTOCOL_ID_MAX || h.msgLen<0 || h.msgLen>SERVER_TEXT_MAX) {reason="잘못된 서버 메시지입니다.";break;}
        if(h.type!=MSG_CHAT && h.type!=MSG_RESTORE && h.type!=MSG_ENROLL && h.type!=MSG_FILE && h.type!=MSG_ERROR && h.type!=MSG_KICK){reason="알 수 없는 메시지 타입입니다.";break;}
        std::string id(h.idLen,'\0'),msg(h.msgLen,'\0');
        if((h.idLen && !RecvExact(sock,&id[0],h.idLen,10000,&clientRunning)) ||
           (h.msgLen && !RecvExact(sock,&msg[0],h.msgLen,10000,&clientRunning)))break;
        if(id.find('\0')!=std::string::npos || msg.find('\0')!=std::string::npos)break;
        if(h.type==MSG_KICK){reason=msg;break;}
        PostUi(hDlg,[h,id,msg]{Feature_OnReceive(h.type,id.c_str(),msg.c_str());});
    }
    connected=false;
    if(clientRunning.load())PostUi(hDlg,[reason]{
        enrolled=false;registrationPending=false;
        DisplayText("[시스템] %s\r\n",reason.c_str());
        MessageBoxA(hDlg,reason.c_str(),"연결 종료",MB_OK|MB_ICONINFORMATION);
        SendMessageA(hDlg,WM_COMMAND,IDCANCEL,0);
    });
    return 0;
}
