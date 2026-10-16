#include "ServerFeature.h"
void HandleFileReceive(SOCKET client,const FileMsg& fm,const char* rawName,const char* data) {
    std::string name(rawName,fm.nameLen);
    if(!SafeFileName(name)) {QueueMessage(client,MSG_ERROR,"서버","허용되지 않는 파일명입니다.");return;}
    if(!CreateDirectoryA("received_files",nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) {
        QueueMessage(client,MSG_ERROR,"서버","저장 폴더 생성 실패");return;
    }
    // CREATE_NEW prevents accidental replacement of an existing file.
    std::string path="received_files\\"+name;
    HANDLE file=CreateFileA(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) {QueueMessage(client,MSG_ERROR,"서버","파일 저장 실패: 같은 이름의 파일 또는 경로/권한 문제");return;}
    bool ok=true;DWORD done=0;
    while(done<static_cast<DWORD>(fm.fileLen)) {
        DWORD n=0;
        if(!WriteFile(file,data+done,static_cast<DWORD>(fm.fileLen)-done,&n,nullptr) || n==0){ok=false;break;}
        done+=n;
    }
    if(ok && !FlushFileBuffers(file))ok=false;
    if(!CloseHandle(file))ok=false;
    if(!ok){DeleteFileA(path.c_str());QueueMessage(client,MSG_ERROR,"서버","파일 저장 중 오류가 발생했습니다.");return;}
    DisplayText("[파일] %s (%d bytes) 저장 완료",name.c_str(),fm.fileLen);
    QueueMessage(client,MSG_FILE,"서버",name);
}
