#include "..\..\Common.h"
#include "..\ChatMsg.h"
#include "resource.h"

#define SERVERPORT 9000
#define BUFSIZE    512
#define ID_MAX     50
#define MSG_MAX    512

INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void DisplayText(const char* fmt, ...);
void DisplayError(const char* msg);
DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClient(LPVOID arg);

HINSTANCE hInst;
HWND hEdit;
CRITICAL_SECTION cs;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    hInst = hInstance;
    InitializeCriticalSection(&cs);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    DeleteCriticalSection(&cs);
    WSACleanup();
    return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        hEdit = GetDlgItem(hDlg, IDC_EDIT1);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_EXIT:
            EndDialog(hDlg, ID_BTN_EXIT);
            return TRUE;
        }
        break;
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

    EnterCriticalSection(&cs);
    int nLength = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, nLength, nLength);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
    LeaveCriticalSection(&cs);
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

DWORD WINAPI ServerMain(LPVOID)
{
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    sockaddr_in serveraddr = {};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    if (bind(listen_sock, (sockaddr*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("bind()");
    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
        err_quit("listen()");

    while (1) {
        sockaddr_in clientaddr;
        int addrlen = sizeof(clientaddr);
        SOCKET client_sock = accept(listen_sock, (sockaddr*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            DisplayError("accept()");
            continue;
        }
        CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, NULL);
    }
    closesocket(listen_sock);
    return 0;
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;

    while (1) {
        int type;
        retval = recv(client_sock, (char*)&type, sizeof(int), MSG_PEEK);
        if (retval <= 0) break;

        if (type == MSG_ENROLL) {
            EnrollMsg emsg;
            retval = recv(client_sock, (char*)&emsg, sizeof(emsg), MSG_WAITALL);
            if (retval <= 0) break;

            char idbuf[ID_MAX] = {};
            retval = recv(client_sock, idbuf, emsg.idLen, MSG_WAITALL);
            if (retval <= 0) break;

            idbuf[emsg.idLen] = '\0';
            DisplayText("[등록] ID: %s\r\n", idbuf);
        }
        else if (type == MSG_CHAT) {
            ChatMsg cmsg;
            retval = recv(client_sock, (char*)&cmsg, sizeof(cmsg), MSG_WAITALL);
            if (retval <= 0) break;

            char id[ID_MAX], msg[MSG_MAX];
            retval = recv(client_sock, id, cmsg.idLen, MSG_WAITALL);
            if (retval <= 0) break;
            retval = recv(client_sock, msg, cmsg.msgLen, MSG_WAITALL);
            if (retval <= 0) break;

            id[cmsg.idLen] = '\0';
            msg[cmsg.msgLen] = '\0';

            DisplayText("[%s] %s\r\n", id, msg);

            send(client_sock, (char*)&cmsg, sizeof(cmsg), 0);
            send(client_sock, id, cmsg.idLen, 0);
            send(client_sock, msg, cmsg.msgLen, 0);
        }
        else {
            break;
        }
    }

    closesocket(client_sock);
    return 0;
}
