#include "ServerFeature.h"
void AppendChatLog(const std::string& id,const std::string& msg) {
    EnterCriticalSection(&cs_log);
    chat_log.push_back({id,msg});
    while(chat_log.size()>HISTORY_MAX)chat_log.erase(chat_log.begin());
    LeaveCriticalSection(&cs_log);
}
void SendChatLogToClient(SOCKET client) {
    EnterCriticalSection(&cs_log);
    for(const auto& e:chat_log)QueueMessage(client,MSG_RESTORE,e.id,e.msg);
    LeaveCriticalSection(&cs_log);
}
void HandleDisconnect(SOCKET client,const char* id) {
    if(!id || !*id)return;
    std::string msg="["+std::string(id)+"]님 퇴장하셨습니다.";
    DisplayText("%s",msg.c_str());AppendChatLog("시스템",msg);
    auto packet=ChatPacket(MSG_CHAT,"시스템",msg);
    BroadcastExceptSender(client,packet.data(),static_cast<int>(packet.size()));
}
