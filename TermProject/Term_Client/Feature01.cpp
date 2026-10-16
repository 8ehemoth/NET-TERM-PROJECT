#include "ClientSocket.h"
extern char buf[MSG_MAX+1],user_id[ID_MAX+1];
extern bool enrolled;
void Feature_RegisterID(const char* id) {
    if(!connected.load()){DisplayText("[오류] 서버에 먼저 연결하세요.\r\n");return;}
    if(!ValidId(id)){DisplayText("[오류] ID는 공백만으로 구성할 수 없으며 최대 50바이트입니다.\r\n");return;}
    if(registrationPending || fileSending){DisplayText("[안내] 진행 중인 요청이 끝난 뒤 ID를 설정하세요.\r\n");return;}
    if(SendEnrollMessage(id)<=0){DisplayText("[오류] ID 등록 요청 전송 실패\r\n");return;}
    registrationPending=true;
    DisplayText("[시스템] ID 등록 승인 대기 중\r\n");
}
void Feature_SendChat(const char* text) {
    if(!connected.load() || !enrolled){DisplayText("[오류] 연결 후 ID를 등록하세요.\r\n");return;}
    if(registrationPending || fileSending){DisplayText("[안내] ID 변경 또는 파일 전송 중에는 채팅을 보낼 수 없습니다.\r\n");return;}
    if(!*text)return;
    if(strlen(text)>MSG_MAX){DisplayText("[오류] 메시지가 너무 깁니다.\r\n");return;}
    if(SendChatMessage(user_id,text)<=0){DisplayText("[오류] 채팅 전송 실패\r\n");return;}
    DisplayText("[%s] %s\r\n",user_id,text);
}
