#include "../Common.h"
#include "ServerFeature.h"
extern HWND hListClients,hEditKickID;
void UpdateClientList() {
    SendMessageA(hListClients,LB_RESETCONTENT,0,0);
    std::lock_guard<std::recursive_mutex> guard(clientsMutex);
    for(auto info:client_list)if(info && !info->id.empty())
        SendMessageA(hListClients,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(info->id.c_str()));
}
void KickSelectedClient() {
    char id[ID_MAX+1]{};GetWindowTextA(hEditKickID,id,sizeof(id));
    bool found=false;
    {
        std::lock_guard<std::recursive_mutex> guard(clientsMutex);
        for(auto info:client_list)if(info && info->id==id && !info->id.empty()) {
            info->kick_pending=true;found=true;break;
        }
    }
    if(!found)MessageBoxA(hDlg,"접속 중인 ID를 선택하거나 입력하세요.","안내",MB_OK);
}
void OnClientListSelect() {
    int sel=static_cast<int>(SendMessageA(hListClients,LB_GETCURSEL,0,0));
    if(sel==LB_ERR)return;
    char id[ID_MAX+1]{};
    SendMessageA(hListClients,LB_GETTEXT,sel,reinterpret_cast<LPARAM>(id));
    SetWindowTextA(hEditKickID,id);
}
