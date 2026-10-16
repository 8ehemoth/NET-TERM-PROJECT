// Feature05.cpp
// ─────────────────────────────────────────────
// 파일탭 대화상자 콜백(FileTabProc), 파일 선택/전송 연동
// ─────────────────────────────────────────────

#include "../Common.h"
#include "../PrjClientFile.h"
#include "ClientFeature.h"
#include "ClientSocket.h"
#include "resource.h"

extern SOCKET sock;
extern HWND   hEditFilePath, hEditFileLog, hProgress;

INT_PTR CALLBACK FileTabProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        hEditFilePath = GetDlgItem(dlg, IDC_EDIT_FILEPATH);
        hEditFileLog = GetDlgItem(dlg, IDC_EDIT_FILELOG);
        hProgress = GetDlgItem(dlg, IDC_PROGRESS1);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_BUTTON_FILE_SELECT) {
            ClientFeature_OnFileSelect(hEditFilePath);
            return TRUE;
        }
        if (LOWORD(wp) == IDC_BUTTON_FILE_SEND) {
            ClientFeature_OnFileSend(sock, hEditFilePath, hProgress, hEditFileLog);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
