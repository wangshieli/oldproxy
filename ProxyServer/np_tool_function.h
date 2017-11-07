#pragma once
#include "np_golbal_header.h"

void myprintf (const char *lpFormat, ... );

void clean_string(char *str);

BOOL GetHostId(char** _pHostId);

BOOL SendData(SOCKET TargetScoket, char* pSendBuf);

int SendData1(SOCKET s, const char* buf, int bufSize);

BOOL AnalyzePortFromRetInfo(char* pRetInfo, char** _pNewPort);

BOOL RecvRequest(SOCKET TargetSocket, char** _pRecvInfo);

BOOL Execmd(char* cmd, char** result);

BOOL is_internal_ip(const char* ip);

BOOL is_reserved_ip(const char* ip);

BOOL Redial(WPARAM wParam = NULL, LPARAM lParam = NULL);

BOOL GetAdslIp(char* pIP);

BOOL CheckAdslIp(const char* pIP);

int ConnectServer(SOCKET& s, char* recvBuf, int len);

BOOL InitHost(SOCKET* ServerSocket, char* HostName, int Port);

int PreResponse(RECVPARAM* svc);

int	ExchangeData(RECVPARAM* svc);

BOOL SendPostDataAndRecvBackInfo(char* pApiUrl, char* pPostData, pCurlWriteFunction _pCurlWriteFunction, void* pBackInfo);

BOOL SendPostDataAndRecvBackInfo1(char* pApiUrl, char* pPostData, pCurlWriteFunction _pCurlWriteFunction, void* pBackInfo);

size_t curl_recvbackinfo_function(void* buffer,size_t size,size_t nmemb,void *pRecvInfo);

BOOL CompareVersion(char* pl, char* ps);

BOOL GetJsonValue(char* pSrc, char* pTemp, char* pOutValue);

void UrlFormating(char *purl, char* pt, char *pu);

BOOL needdownload(char* psPath, char* pmd5);

size_t write_flie(void* buffer,size_t size,size_t nmemb,void *stream);

BOOL GetFileFullPath(char* filePath);

void ClearThreadResource();

int RecvRequest01(SOCKET s, char** buf, int bufSize);
int RecvRequest02(SOCKET s, char** buf, int bufSize);


BOOL reg_proxy_path();
void set_starting_items();

void LSCloseSocket(SOCKET& s);

void GetRandIndex(int arrayIndex[], int nCount);

int Random(int m, int n);

void getguid(char *buf);

int GetPort();

int GetSpeedSize(char* info);

BOOL CheckWindowsUP(const char* _username, const char* _password);

void AddWhiteList();

void AddModRiskFileType();

BOOL CloseChromeExe_ctx();

BOOL RestartProxy();