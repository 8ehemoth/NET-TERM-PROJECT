// ChatMsg.h
#pragma once

#define MSG_CHAT     0x01
#define MSG_RESTORE  0x02
#define MSG_ENROLL   0x03

#define MSG_CALC     0x08
#define MSG_KICK 99


struct EnrollMsg {
    int type;
    int idLen;
};

struct ChatMsg {
    int type;
    int idLen;
    int msgLen;
};



struct CalcMsg {
    int type;
    int exprLen;
};


// Server replies, including MSG_KICK and MSG_ERROR, use ChatMsg.
#define MSG_ERROR 0x05
