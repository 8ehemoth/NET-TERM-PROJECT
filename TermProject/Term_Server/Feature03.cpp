#include "ServerFeature.h"
void QueuePacket(SOCKET client,const std::vector<char>& packet) {
    for(auto info:client_list) if(info && info->sock==client && !info->closing) {
        if(packet.size()>SEND_QUEUE_MAX-info->queuedBytes) {
            info->closing=true; info->output.clear();info->queuedBytes=0;info->sendOffset=0;return;
        }
        info->output.push_back(packet);info->queuedBytes+=packet.size();return;
    }
}
void QueueMessage(SOCKET c,int type,const std::string& id,const std::string& text) {
    QueuePacket(c,ChatPacket(type,id,text));
}
void BroadcastAll(SOCKET,const char* data,int len) {
    if(len<=0)return;
    std::vector<char> packet(data,data+len);
    for(auto info:client_list) if(info && !info->id.empty())QueuePacket(info->sock,packet);
}
void BroadcastExceptSender(SOCKET sender,const char* data,int len) {
    if(len<=0)return;
    std::vector<char> packet(data,data+len);
    for(auto info:client_list) if(info && info->sock!=sender && !info->id.empty())QueuePacket(info->sock,packet);
}
void HandleChatMessage(SOCKET client,const ChatMsg&,const char* id,const char* msg) {
    DisplayText("[%s] %s",id,msg);AppendChatLog(id,msg);
    auto packet=ChatPacket(MSG_CHAT,id,msg);
    BroadcastExceptSender(client,packet.data(),static_cast<int>(packet.size()));
}
