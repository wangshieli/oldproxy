#pragma once

#include "np_golbal_header.h"
#include "TraceLog.h"

BOOL InitSock2();

int ConnectToServer(SOCKET& conSocket, char* ServerIP, unsigned short ServerPort);

BOOL SendHostIdToServer(SOCKET TargetSocket, char* _pHostId);

BOOL RecvHostIdReturnFromServer(SOCKET TargetSocket, char* pRetInfo);

BOOL Send6086HostIdToServer(SOCKET TargetSocket, char* _pHostId);

BOOL Send6086RedialOKToServer(SOCKET TargetSocket, char* pinfo, char* _pHostId);

BOOL RecvNewPortInfoFromServer(SOCKET TargetSocket, char** pRetInfo);

BOOL ProxyThread(RECVPARAM* lpParameter);

BOOL ProxyThread01(RECVPARAM* lpParameter);

BOOL SendWebRequest(RECVPARAM *lpParameter, char *SendBuf, char *RecvBuf, int DataLen);

BOOL SendHostInfo2API(pCurlWriteFunction _pCurlWriteFunction);

BOOL CheckVersionAndDownFile(char* pszItem, int nMode);

BOOL GetDownFile(char* pUrl, char* pFileName);

BOOL new_download();

BOOL GetHostInfo();

BOOL GetHostInfo_start();// «ø∆»÷¢

BOOL InstallChrome();


BOOL Curl_DownloadFile(const char* url, const char* downPath);

BOOL DeleteDirectoryByFullName(const char* pFilename);