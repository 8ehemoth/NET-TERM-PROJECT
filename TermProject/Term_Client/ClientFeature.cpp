#include "ClientSocket.h"
#include "../NetUtil.h"
#include "../UiDispatch.h"
HWND hEditCalcInput=nullptr,hEditCalcResult=nullptr;
extern SOCKET sock;
extern char user_id[ID_MAX+1];
extern bool enrolled;
namespace {
void AppendLog(HWND edit,const std::string& text) {
    if(!edit)return;
    int len=GetWindowTextLengthA(edit);
    SendMessageA(edit,EM_SETSEL,len,len);SendMessageA(edit,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageA(edit,EM_SCROLL,SB_BOTTOM,0);
}
std::string FormatLog(const char* fmt,va_list ap){char buf[4096]{};vsnprintf(buf,sizeof(buf),fmt,ap);return buf;}
}
void DisplayText(const char* fmt,...) {
    va_list ap;va_start(ap,fmt);auto text=FormatLog(fmt,ap);va_end(ap);
    PostUi(hDlg,[text]{AppendLog(hEdit2,text);});
}
void DisplayFileLog(HWND edit,const char* fmt,...) {
    va_list ap;va_start(ap,fmt);auto text=FormatLog(fmt,ap);va_end(ap);
    PostUi(hDlg,[edit,text]{AppendLog(edit,text);});
}
void ClientFeature_OnFileSelect(HWND edit) {
    OPENFILENAMEA ofn{};ofn.lStructSize=sizeof(ofn);char path[MAX_PATH]{};
    ofn.hwndOwner=hDlg;ofn.lpstrFilter="All Files\0*.*\0";ofn.lpstrFile=path;ofn.nMaxFile=MAX_PATH;
    ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    if(GetOpenFileNameA(&ofn))SetWindowTextA(edit,path);
}
void ClientFeature_OnFileSend(SOCKET socket,HWND pathEdit,HWND progress,HWND logEdit) {
    if(!connected.load() || !enrolled || registrationPending){DisplayFileLog(logEdit,"[오류] 먼저 연결 후 ID를 등록하세요.\r\n");return;}
    if(fileSending){DisplayFileLog(logEdit,"[안내] 파일 전송 진행 중\r\n");return;}
    char path[MAX_PATH]{};GetWindowTextA(pathEdit,path,MAX_PATH);
    if(!*path){DisplayFileLog(logEdit,"[오류] 파일을 선택하세요.\r\n");return;}
    std::string filePath=path;fileSending=true;
    clientWorkers.emplace_back([socket,filePath,progress,logEdit]{
        bool sentOk=false;
        std::unique_lock<std::mutex> lock(sendMutex);
        FILE* fp=fopen(filePath.c_str(),"rb");
        if(!fp)DisplayFileLog(logEdit,"[오류] 파일 열기 실패\r\n");
        else {
            const size_t slash=filePath.find_last_of("\\/");
            std::string name=filePath.substr(slash==std::string::npos?0:slash+1);
            long length=-1;
            if(fseek(fp,0,SEEK_END)==0){length=ftell(fp);if(fseek(fp,0,SEEK_SET)!=0)length=-1;}
            if(!SafeFileName(name) || length<0 || length>FILE_SIZE_MAX)DisplayFileLog(logEdit,"[오류] 파일명 또는 크기 오류 (최대 16 MiB)\r\n");
            else {
                FileMsg h{MSG_FILE,static_cast<int>(name.size()),static_cast<int>(length),0};
                bool ok=SendExact(socket,&h,sizeof(h),10000,&clientRunning) && SendExact(socket,name.data(),h.nameLen,10000,&clientRunning);
                char chunk[4096];int sent=0;
                while(ok && sent<h.fileLen) {
                    int want=std::min(static_cast<int>(sizeof(chunk)),h.fileLen-sent);
                    size_t n=fread(chunk,1,want,fp);
                    if(n!=static_cast<size_t>(want)){ok=false;break;}
                    ok=SendExact(socket,chunk,want,10000,&clientRunning);
                    if(ok){sent+=want;int percent=static_cast<int>(static_cast<long long>(sent)*100/h.fileLen);
                        PostUi(hDlg,[progress,percent]{SendMessageA(progress,PBM_SETPOS,percent,0);});}
                }
                sentOk=ok;
                if(ok){PostUi(hDlg,[progress]{SendMessageA(progress,PBM_SETPOS,100,0);});DisplayFileLog(logEdit,"[송신 완료] %s — 서버 저장 응답 대기\r\n",name.c_str());}
                else {DisplayFileLog(logEdit,"[오류] 전송 실패. 연결을 종료합니다.\r\n");FailConnection();}
            }
            fclose(fp);
        }
        // A successful upload remains pending until the server's save result arrives.
        if(!sentOk)PostUi(hDlg,[]{fileSending=false;});
    });
}
void ClientFeature_OnCalcSend(HWND input,HWND output) {
    char text[256]{};GetWindowTextA(input,text,sizeof(text));SetWindowTextA(output,text);
}
void Feature_OnReceive(int type,const char* id,const char* msg) {
    switch(type) {
    case MSG_ENROLL:
        if(!ValidId(msg))break;
        strncpy_s(user_id,ID_MAX+1,msg,_TRUNCATE);enrolled=true;registrationPending=false;
        DisplayText("[시스템] ID 등록 완료: %s\r\n",user_id);break;
    case MSG_CHAT:case MSG_RESTORE:DisplayText("[%s] %s\r\n",id,msg);break;
    case MSG_FILE:
        fileSending=false;DisplayFileLog(hEditFileLog,"[서버 저장 완료] %s\r\n",msg);break;
    case MSG_ERROR:
        registrationPending=false;fileSending=false;
        DisplayText("[서버 오류] %s\r\n",msg);DisplayFileLog(hEditFileLog,"[서버 오류] %s\r\n",msg);break;
    }
}
