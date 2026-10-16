#pragma once
#include "../Common.h"
#include "../ProtocolCore.h"
#include "resource.h"
#include "ClientFeature.h"
extern SOCKET sock;
extern std::atomic<bool> clientRunning;
extern std::atomic<bool> connected;
extern std::mutex sendMutex;
extern std::vector<std::thread> clientWorkers;
extern bool registrationPending;
extern bool fileSending;
extern bool calcPending;
bool SockInit();
void SockCleanup();
bool SockConnect(const char* ip,int port,int timeout_ms=2000);
int SendEnrollMessage(const char* id);
int SendChatMessage(const char* id,const char* text);
void FailConnection();
