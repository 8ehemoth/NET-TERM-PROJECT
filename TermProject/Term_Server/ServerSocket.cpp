#include "ServerFeature.h"
extern bool debugMode;
namespace {
void DropClient(int i) {
    auto info=client_list[i];if(!info)return;
    std::string id=info->id;SOCKET s=info->sock;
    client_list[i]=nullptr;closesocket(s);delete info;
    HandleDisconnect(s,id.c_str());PostMessageA(hDlg,WM_UPDATE_CLIENTLIST,0,0);
}
bool DispatchFrames(SOCKETINFO* info) {
    size_t length=0;
    for(;;) {
        int state=ClientFrameSize(info->input,length);
        if(state<0)return false;
        if(state==0 || info->input.size()<length)return true;
        int type=0;std::memcpy(&type,info->input.data(),sizeof(type));
        if(debugMode)DisplayText("[DEBUG] type=%d bytes=%u",type,static_cast<unsigned>(length));
        const char* data=info->input.data();
        if(type==MSG_ENROLL) {
            EnrollMsg h;std::memcpy(&h,data,sizeof(h));std::string id(data+sizeof(h),h.idLen);
            if(id.find('\0')!=std::string::npos || !ValidId(id))return false;
            HandleEnroll(info->sock,h,id.c_str());
        } else if(type==MSG_CHAT) {
            ChatMsg h;std::memcpy(&h,data,sizeof(h));
            std::string id(data+sizeof(h),h.idLen),msg(data+sizeof(h)+h.idLen,h.msgLen);
            if(id!=info->id || info->id.empty() || msg.find('\0')!=std::string::npos)return false;
            HandleChatMessage(info->sock,h,info->id.c_str(),msg.c_str());
        } else {
            FileMsg h;std::memcpy(&h,data,sizeof(h));
            if(info->id.empty())return false;
            HandleFileReceive(info->sock,h,data+sizeof(h),data+sizeof(h)+h.nameLen);
        }
        info->input.erase(info->input.begin(),info->input.begin()+length);
        if(info->closing)return true;
    }
}
}
void IoLoop() {
    SOCKET listener=socket(AF_INET,SOCK_STREAM,0);
    if(listener==INVALID_SOCKET){DisplayText("[오류] 채팅 소켓 생성 실패");return;}
    sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons(SERVERPORT);
    u_long on=1;
    if(bind(listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR ||
       listen(listener,MAX_CLIENTS)==SOCKET_ERROR || ioctlsocket(listener,FIONBIO,&on)==SOCKET_ERROR) {
        DisplayText("[오류] 채팅 서버 시작 실패: %d",WSAGetLastError());closesocket(listener);return;
    }
    DisplayText("채팅 서버 시작: 포트 %d",SERVERPORT);
    while(serverRunning.load()) {
        fd_set rf,wf;FD_ZERO(&rf);FD_ZERO(&wf);FD_SET(listener,&rf);
        // Only the I/O thread adds/removes connections. UI access is protected by the same mutex.
        {
            std::lock_guard<std::recursive_mutex> guard(clientsMutex);
            for(auto info:client_list)if(info){if(!info->closing)FD_SET(info->sock,&rf);if(!info->output.empty())FD_SET(info->sock,&wf);}
        }
        timeval tv{0,100000};int result=select(0,&rf,&wf,nullptr,&tv);
        if(result==SOCKET_ERROR){DisplayText("[오류] 채팅 select 실패");break;}
        std::lock_guard<std::recursive_mutex> guard(clientsMutex);
        if(FD_ISSET(listener,&rf)) {
            SOCKET c=accept(listener,nullptr,nullptr);
            if(c!=INVALID_SOCKET) {
                int slot=-1;for(int i=0;i<MAX_CLIENTS;++i)if(!client_list[i]){slot=i;break;}
                if(slot<0 || ioctlsocket(c,FIONBIO,&on)==SOCKET_ERROR)closesocket(c);
                else {auto info=new SOCKETINFO;info->sock=c;info->lastProgress=GetTickCount64();client_list[slot]=info;}
            }
        }
        for(int i=0;i<MAX_CLIENTS;++i) {
            auto info=client_list[i];if(!info)continue;
            ULONGLONG now=GetTickCount64();
            if(info->kick_pending && !info->closing) {
                QueueMessage(info->sock,MSG_KICK,"서버","서버에서 강퇴되었습니다.");
                info->closing=true;info->closeDeadline=now+2000;
            }
            if((info->closing && (info->output.empty() || now>=info->closeDeadline)) ||
               (!info->input.empty() && now-info->lastProgress>30000)) {DropClient(i);continue;}
            if(!info->closing && FD_ISSET(info->sock,&rf)) {
                char chunk[65536];int n=recv(info->sock,chunk,sizeof(chunk),0);
                if(n==0 || (n<0 && WSAGetLastError()!=WSAEWOULDBLOCK)){DropClient(i);continue;}
                if(n>0) {
                    info->lastProgress=now;
                    if(info->input.size()+static_cast<size_t>(n)>static_cast<size_t>(FILE_SIZE_MAX)+FILE_NAME_MAX+sizeof(FileMsg)+sizeof(chunk)){DropClient(i);continue;}
                    info->input.insert(info->input.end(),chunk,chunk+n);
                    if(!DispatchFrames(info)){DropClient(i);continue;}
                }
            }
            if(FD_ISSET(info->sock,&wf) && !info->output.empty()) {
                auto& packet=info->output.front();size_t left=packet.size()-info->sendOffset;
                int n=send(info->sock,packet.data()+info->sendOffset,static_cast<int>(left),0);
                if(n>0){info->sendOffset+=n;info->queuedBytes-=n;
                    if(info->sendOffset==packet.size()){info->output.pop_front();info->sendOffset=0;}}
                else if(n==0 || WSAGetLastError()!=WSAEWOULDBLOCK){DropClient(i);continue;}
            }
        }
    }
    closesocket(listener);
    std::lock_guard<std::recursive_mutex> guard(clientsMutex);
    for(auto& info:client_list)if(info){closesocket(info->sock);delete info;info=nullptr;}
}
