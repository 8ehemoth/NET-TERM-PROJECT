#include "ServerFeature.h"
extern bool debugMode;
namespace {
struct CalcClient {SOCKET sock;std::vector<char> input,output;size_t sent=0;ULONGLONG since;};
}
void CalcLoop() {
    SOCKET listener=socket(AF_INET,SOCK_STREAM,0);if(listener==INVALID_SOCKET)return;
    sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons(9001);
    u_long on=1;
    if(bind(listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR || listen(listener,10)==SOCKET_ERROR || ioctlsocket(listener,FIONBIO,&on)==SOCKET_ERROR){DisplayText("[오류] 계산기 서버 시작 실패");closesocket(listener);return;}
    DisplayText("계산기 서버 시작: 포트 9001");std::vector<CalcClient> clients;
    while(serverRunning.load()) {
        fd_set rf,wf;FD_ZERO(&rf);FD_ZERO(&wf);FD_SET(listener,&rf);
        for(auto& c:clients){if(c.output.empty())FD_SET(c.sock,&rf);else FD_SET(c.sock,&wf);}
        timeval tv{0,100000};if(select(0,&rf,&wf,nullptr,&tv)==SOCKET_ERROR)break;
        if(FD_ISSET(listener,&rf)) {
            SOCKET s=accept(listener,nullptr,nullptr);
            if(s!=INVALID_SOCKET){if(clients.size()>=10 || ioctlsocket(s,FIONBIO,&on)==SOCKET_ERROR)closesocket(s);else clients.push_back({s,{},{},0,GetTickCount64()});}
        }
        for(size_t i=0;i<clients.size();) {
            auto& c=clients[i];bool drop=GetTickCount64()-c.since>10000;
            if(!drop && c.output.empty() && FD_ISSET(c.sock,&rf)) {
                char chunk[512];int n=recv(c.sock,chunk,sizeof(chunk),0);
                if(n==0 || (n<0 && WSAGetLastError()!=WSAEWOULDBLOCK))drop=true;
                if(n>0){c.input.insert(c.input.end(),chunk,chunk+n);
                    if(c.input.size()>sizeof(CalcMsg)+CALC_EXPR_MAX)drop=true;
                    if(!drop && c.input.size()>=sizeof(CalcMsg)) {
                        CalcMsg h;std::memcpy(&h,c.input.data(),sizeof(h));
                        if(h.type!=MSG_CALC || h.exprLen<=0 || h.exprLen>CALC_EXPR_MAX)drop=true;
                        else if(c.input.size()>=sizeof(h)+static_cast<size_t>(h.exprLen)) {
                            std::string expr(c.input.data()+sizeof(h),h.exprLen),out=EvaluateExpression(expr);
                            if(debugMode)DisplayText("[DEBUG][Calc] %s -> %s",expr.c_str(),out.c_str());
                            int len=static_cast<int>(out.size());c.output.resize(sizeof(len)+out.size());
                            std::memcpy(c.output.data(),&len,sizeof(len));std::memcpy(c.output.data()+sizeof(len),out.data(),out.size());
                        }
                    }
                }
            }
            if(!drop && !c.output.empty() && FD_ISSET(c.sock,&wf)) {
                int n=send(c.sock,c.output.data()+c.sent,static_cast<int>(c.output.size()-c.sent),0);
                if(n>0){c.sent+=n;if(c.sent==c.output.size())drop=true;}
                else if(n==0 || WSAGetLastError()!=WSAEWOULDBLOCK)drop=true;
            }
            if(drop){closesocket(c.sock);clients.erase(clients.begin()+i);}else ++i;
        }
    }
    for(auto& c:clients)closesocket(c.sock);
    closesocket(listener);
}
