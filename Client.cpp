// GUITCPClient.cpp - Enroll 메시지 + Chat 메시지 전송 (고정+가변 길이 구조 포함)
#include "..\..\Common.h"
#include "resource.h"
#include "..\ChatMsg.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void DisplayText(const char* fmt, ...);
void DisplayError(const char* msg);
DWORD WINAPI ClientMain(LPVOID arg);

SOCKET sock;
char buf[BUFSIZE + 1];
HANDLE hReadEvent, hWriteEvent;
HWND hSendButton;
HWND hEdit1, hEdit2, hEdit3;
char user_id[50] = "";
bool enrolled = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    CloseHandle(hReadEvent);
    CloseHandle(hWriteEvent);
    WSACleanup();
    return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
        hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
        hEdit3 = GetDlgItem(hDlg, IDC_EDIT3);
        hSendButton = GetDlgItem(hDlg, IDOK);
        SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON1: // ID 설정 버튼
            GetDlgItemTextA(hDlg, IDC_EDIT3, user_id, sizeof(user_id));
            if (strlen(user_id) > 0) enrolled = false; // ✅ ID가 바뀌면 다시 등록
            return TRUE;

        case IDOK:
            EnableWindow(hSendButton, FALSE);
            WaitForSingleObject(hReadEvent, INFINITE);
            GetDlgItemTextA(hDlg, IDC_EDIT1, buf, BUFSIZE + 1);
            SetEvent(hWriteEvent);
            SetFocus(hEdit1);
            SendMessage(hEdit1, EM_SETSEL, 0, -1);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            closesocket(sock);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

void DisplayText(const char* fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char cbuf[BUFSIZE * 2];
    vsprintf(cbuf, fmt, arg);
    va_end(arg);

    int nLength = GetWindowTextLength(hEdit2);
    SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
    SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}

void DisplayError(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&lpMsgBuf, 0, NULL);
    DisplayText("[%s] %s\r\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

DWORD WINAPI ClientMain(LPVOID)
{
    int retval;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    while (1) {
        WaitForSingleObject(hWriteEvent, INFINITE);

        if (!enrolled && strlen(user_id) > 0) {
            EnrollMsg emsg;
            emsg.type = MSG_ENROLL;
            emsg.idLen = (int)strlen(user_id);
            send(sock, (char*)&emsg, sizeof(emsg), 0);
            send(sock, user_id, emsg.idLen, 0);
            DisplayText("[시스템] Enroll 전송 완료\r\n");
            enrolled = true;
            EnableWindow(hSendButton, TRUE);
            SetEvent(hReadEvent);
            continue;
        }

        if (strlen(buf) == 0) {
            EnableWindow(hSendButton, TRUE);
            SetEvent(hReadEvent);
            continue;
        }

        ChatMsg cmsg;
        cmsg.type = MSG_CHAT;
        cmsg.idLen = (int)strlen(user_id);
        cmsg.msgLen = (int)strlen(buf);
        send(sock, (char*)&cmsg, sizeof(cmsg), 0);
        send(sock, user_id, cmsg.idLen, 0);
        send(sock, buf, cmsg.msgLen, 0);

        DisplayText("[%s] %s\r\n", user_id, buf);
        EnableWindow(hSendButton, TRUE);
        SetEvent(hReadEvent);
    }
    return 0;
}
