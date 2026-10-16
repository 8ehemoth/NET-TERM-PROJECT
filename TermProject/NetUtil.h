#pragma once
#include "Common.h"
// One deadline covers the whole transfer. A partial timeout requires closing the connection.
inline bool TransferExact(SOCKET s, char* data, int length, bool sending, int timeoutMs,
                          const std::atomic<bool>* running = nullptr) {
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    int done=0;
    while (done < length) {
        if ((running && !running->load()) || GetTickCount64() >= deadline) return false;
        int n = sending ? send(s,data+done,length-done,0) : recv(s,data+done,length-done,0);
        if (n>0) { done+=n; continue; }
        if (n==0 || WSAGetLastError()!=WSAEWOULDBLOCK) return false;
        fd_set f; FD_ZERO(&f); FD_SET(s,&f); timeval tv{0,100000};
        int ready = select(0,sending?nullptr:&f,sending?&f:nullptr,nullptr,&tv);
        if (ready==SOCKET_ERROR) return false;
    }
    return true;
}
inline bool SendExact(SOCKET s,const void* p,int n,int ms,const std::atomic<bool>* running=nullptr) {
    return TransferExact(s,const_cast<char*>(static_cast<const char*>(p)),n,true,ms,running);
}
inline bool RecvExact(SOCKET s,void* p,int n,int ms,const std::atomic<bool>* running=nullptr) {
    return TransferExact(s,static_cast<char*>(p),n,false,ms,running);
}
inline SOCKET ConnectTcp(const char* ip,int port,int timeoutMs,const std::atomic<bool>* running=nullptr) {
    SOCKET s=socket(AF_INET,SOCK_STREAM,0); if(s==INVALID_SOCKET) return s;
    u_long on=1;
    if(ioctlsocket(s,FIONBIO,&on)==SOCKET_ERROR){closesocket(s);return INVALID_SOCKET;}
    sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(static_cast<u_short>(port));
    if(inet_pton(AF_INET,ip,&addr.sin_addr)!=1){closesocket(s);return INVALID_SOCKET;}
    int result=connect(s,reinterpret_cast<sockaddr*>(&addr),sizeof(addr));
    if(result==0)return s;
    if(WSAGetLastError()!=WSAEWOULDBLOCK){closesocket(s);return INVALID_SOCKET;}
    ULONGLONG deadline=GetTickCount64()+timeoutMs;
    while(GetTickCount64()<deadline && (!running || running->load())) {
        fd_set w,e;FD_ZERO(&w);FD_ZERO(&e);FD_SET(s,&w);FD_SET(s,&e);timeval tv{0,100000};
        int n=select(0,nullptr,&w,&e,&tv);
        if(n==SOCKET_ERROR)break;
        if(n>0){int err=0,len=sizeof(err);if(getsockopt(s,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&err),&len)==0 && err==0 && FD_ISSET(s,&w))return s;break;}
    }
    closesocket(s);return INVALID_SOCKET;
}
