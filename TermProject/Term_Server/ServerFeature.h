#pragma once
#include "../Common.h"
#include "../ProtocolCore.h"
constexpr int MAX_CLIENTS=10;
constexpr int HISTORY_MAX=20;
constexpr size_t SEND_QUEUE_MAX=4*1024*1024;
struct ChatLog { std::string id,msg; };
struct SOCKETINFO {
    SOCKET sock=INVALID_SOCKET;
    std::vector<char> input;
    std::deque<std::vector<char>> output;
    size_t sendOffset=0,queuedBytes=0;
    std::string id;
    bool kick_pending=false,closing=false;
    ULONGLONG closeDeadline=0,lastProgress=0;
};
extern std::vector<ChatLog> chat_log;
extern CRITICAL_SECTION cs_log;
extern SOCKETINFO* client_list[MAX_CLIENTS];
extern HWND hListLog,hDlg;
extern std::recursive_mutex clientsMutex;
extern std::atomic<bool> serverRunning;
void DisplayText(const char* fmt,...);
void IoLoop();
void CalcLoop();
void AppendChatLog(const std::string& id,const std::string& msg);
void SendChatLogToClient(SOCKET client);
int HandleEnroll(SOCKET client,const EnrollMsg& em,const char* id);
void HandleChatMessage(SOCKET client,const ChatMsg& cm,const char* id,const char* msg);
void HandleDisconnect(SOCKET client,const char* id);
void HandleFileReceive(SOCKET client,const FileMsg& fm,const char* name,const char* data);
void QueuePacket(SOCKET client,const std::vector<char>& packet);
void QueueMessage(SOCKET client,int type,const std::string& id,const std::string& text);
void BroadcastAll(SOCKET sender,const char* buf,int len);
void BroadcastExceptSender(SOCKET sender,const char* buf,int len);
void UpdateClientList();
void KickSelectedClient();
void OnClientListSelect();
#define WM_UPDATE_CLIENTLIST (WM_USER + 123)
