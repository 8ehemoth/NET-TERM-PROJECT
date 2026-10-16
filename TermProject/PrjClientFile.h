// PrjClientFile.h
#pragma once

#define MSG_FILE 0x04

// 파일 전송 헤더
struct FileMsg {
    int type;     // MSG_FILE
    int nameLen;
    int fileLen;
    int isText;   // 텍스트=1, 바이너리=0
};
