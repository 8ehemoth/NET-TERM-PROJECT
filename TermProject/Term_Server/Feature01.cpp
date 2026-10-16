#include "ServerFeature.h"
int HandleEnroll(SOCKET client,const EnrollMsg&,const char* newId) {
    SOCKETINFO* target=nullptr;
    for(auto info:client_list) {
        if(info && info->sock==client)target=info;
        if(info && info->sock!=client && info->id==newId) {
            QueueMessage(client,MSG_ERROR,"서버","이미 사용 중인 ID입니다.");return -1;
        }
    }
    if(!target || !ValidId(newId)) {
        QueueMessage(client,MSG_ERROR,"서버","올바른 ID를 입력하세요.");return -1;
    }
    std::string old=target->id;target->id=newId;
    if(old.empty())SendChatLogToClient(client);
    // Registration acknowledgment is sent before the public system announcement.
    QueueMessage(client,MSG_ENROLL,"서버",target->id);
    std::string msg=old.empty()?"["+target->id+"]님 입장하셨습니다.":"["+old+"]님이 ["+target->id+"]로 변경되었습니다.";
    DisplayText("[시스템] %s",msg.c_str());AppendChatLog("시스템",msg);
    auto packet=ChatPacket(MSG_CHAT,"시스템",msg);
    BroadcastAll(client,packet.data(),static_cast<int>(packet.size()));
    PostMessageA(hDlg,WM_UPDATE_CLIENTLIST,0,0);return 0;
}
