#include "ClientSocket.h"
#include "../NetUtil.h"
std::atomic<bool> clientRunning{true},connected{false};
std::mutex sendMutex;
std::vector<std::thread> clientWorkers;
bool registrationPending=false,fileSending=false,calcPending=false;
namespace { bool winsockReady=false; }
bool SockInit(){if(winsockReady)return true;WSADATA w{};winsockReady=WSAStartup(MAKEWORD(2,2),&w)==0;return winsockReady;}
void SockCleanup(){
    clientRunning=false;
    if(sock!=INVALID_SOCKET)shutdown(sock,SD_BOTH);
    for(auto& worker:clientWorkers)if(worker.joinable())worker.join();
    clientWorkers.clear();
    if(sock!=INVALID_SOCKET){closesocket(sock);sock=INVALID_SOCKET;}
    connected=false;
    if(winsockReady){WSACleanup();winsockReady=false;}
}
bool SockConnect(const char* ip,int port,int timeout_ms){
    sock=ConnectTcp(ip,port,timeout_ms,&clientRunning);connected=(sock!=INVALID_SOCKET);return connected.load();
}
void FailConnection(){connected=false;if(sock!=INVALID_SOCKET)shutdown(sock,SD_BOTH);}
int SendEnrollMessage(const char* id){
    std::unique_lock<std::mutex> lock(sendMutex,std::try_to_lock);
    if(!lock.owns_lock() || !connected.load())return -1;
    EnrollMsg h{MSG_ENROLL,static_cast<int>(strlen(id))};
    std::vector<char> packet(sizeof(h)+h.idLen);memcpy(packet.data(),&h,sizeof(h));memcpy(packet.data()+sizeof(h),id,h.idLen);
    if(!SendExact(sock,packet.data(),static_cast<int>(packet.size()),2000,&clientRunning)){FailConnection();return -1;}return 1;
}
int SendChatMessage(const char* id,const char* text){
    std::unique_lock<std::mutex> lock(sendMutex,std::try_to_lock);
    if(!lock.owns_lock() || !connected.load())return -1;
    auto packet=ChatPacket(MSG_CHAT,id,text);
    if(!SendExact(sock,packet.data(),static_cast<int>(packet.size()),2000,&clientRunning)){FailConnection();return -1;}return 1;
}
