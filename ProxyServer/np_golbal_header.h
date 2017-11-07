#pragma once

#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <process.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#include <vector>
#include <curl\curl.h>
#include "md5.h"
#include <map>
#include <time.h>
#include <ShlObj.h>

#include <IPHlpApi.h>
#include <CommCtrl.h>
#include <TlHelp32.h>



using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "IPHlpApi.lib")

#define IS_TEST					0
#define USE_BASIC				0

#define CURRENT_VERSION			"0.1.37"

#define CHROME_LNK	"\\chrome.lnk"

#if IS_TEST
#define CLIENT_ID_PATH	"C:\\client_id.txt"
#define REDAIL_IP		"192.168.248.128"
#define WARD_FOLDER		"C:\\MyWard"
#else
#define CLIENT_ID_PATH	"C:\\client_id.txt"
#define WARD_FOLDER		"C:\\MyWard"
#endif

#define NP_THE_ONE_INSTANCE "Global\\np_The_One_Instance_Event"

#define WM_MESSAGE_5001_START	WM_USER + 1
#define WM_MESSAGE_6086_START	WM_USER + 2

#define WM_MESSAGE_REDIAL_START	WM_USER + 3
#define WM_MESSAGE_REDIAL_AGAIN	WM_USER + 4
#define WM_MESSAGE_MODIFY_DNS	WM_USER + 12

#define WM_MESSAGE_REPORT_START	WM_USER + 5

#define WM_MESSAGE_DOWNLOAD_START	WM_USER + 6

#define WM_MESSAGE_COUT_MESSAGE		WM_USER + 7
#define WM_MESSAGE_LOG				WM_USER + 9

#define WM_TIMER_THREAD_QUIT		WM_USER + 8
#define WM_TIMER_THREAD_EXIT		WM_USER + 10
extern unsigned int tHeartBeatThreadID;

#define WM_MESSAGE_SOCKIO WM_USER + 11

#define SPEED_DATA 1024

#define MAXSIZE 1024*8
#define HEADLEN 7// http://

#define DEFAULTPORT 80

#define MY_ALIGN(size, boundary) (((size) + ((boundary) - 1 )) & ~((boundary) - 1))

#define P_WATCH_EXIT_EVETN "Global\\p_watch_Exit_Event"
#define P_WATCH_EXIT_COMPLETE "Global\\p_watch_ExitComplete_Event"

#define POSTHOSTINFOURL		"https://wwwphpapi-0.disi.se/v1/host/save_proxy_info"
#define POSTHOSTINFOURL2 "https://cnapi.disi.se/hashes/proxies/%s" // %s = host_id
#define GETHOSTINFO "https://wwwphpapi-0.disi.se/v1/host/%s/get"
//#define POSTHOSTINFOURL		"http://192.168.2.21:3000/v1/host/save_proxy_info"
#define POSTURL				"https://wwwphpapi-0.disi.se/v1/extension/version"
//#define POSTURL				"https://www.disi.se/index.php/Admin/extensionApi/version"
#define UPGRADEEXE			"https://wwwphpapi-0.disi.se/v1/extension/version?app_secret=F$~((kb~AjO*xgn~&appid=app-upgrade"
#define PROXYEXE			"https://wwwphpapi-0.disi.se/v1/extension/version?app_secret=F$~((kb~AjO*xgn~&appid=app-proxy&host_id=%s"
#define NEW_EXE_INFO		"https://wwwphpapi-0.disi.se/v1/extension/versions?app_secret=F$~((kb~AjO*xgn~&codes=app-proxy,proxy-monitor,app-upgrade&host_id=%s"
#define NEW_EXE_INFO_LOCAL	"http://192.168.2.127:8070/v1/extension/versions?app_secret=F$~((kb~AjO*xgn~&codes=app-proxy,app-pwatch,app-upgrade&host_id=%s"
#define CTX_INFO "https://wwwphpapi-0.disi.se/v1/extension/versions?app_secret=F$~((kb~AjO*xgn~&codes=edffiillgkekafkdjahahdjhjffllgjg,npogbneglgfmgafpepecdconkgapppkd&host_id=%s"

#define SOCKET_IO_URL "https://host-notification.disi.se/"
#define SOCKET_IO_NAME "host-notification.disi.se"

extern SOCKET g_6086socket;
extern SOCKET g_5001socket;
extern SOCKET g_HeartBeatsocket;

extern SOCKET sListenSock;
extern SOCKET s6086Sock;

extern char g_cServerip[32];
extern unsigned short g_nServerport;

extern char g_cCurrentIP[32];
extern CRITICAL_SECTION g_csCurrentIP;

extern char* g_pCurrentPort;
extern CRITICAL_SECTION g_csCurrentPort;

extern unsigned int g_n5001And6086ThreadNo;
extern HANDLE g_hEventOf5001And6086Thread;

extern unsigned int g_n5001ThreadNo;
extern unsigned int g_n6086ThreadNo;

extern unsigned int g_nRedialThreadNo;
extern HANDLE g_hEventOfRedialThread;
extern HANDLE g_hEventForRedialCompelet;

extern unsigned int g_nReportInfoThreadNo;
extern HANDLE g_hEventOfReportThread;
extern HANDLE g_hEventForReportCompelet;

extern unsigned int g_nDownloadThreadNo;
extern HANDLE g_hEventOfDownloadThread;
extern HANDLE g_hEventForDownloadCompelet;

extern HANDLE g_hMode2StartOk;

//extern HANDLE g_hEventFor5001Compelet;

extern unsigned int g_nMessageThreadNo;

extern BOOL bNeedDownUpgrade;

extern BOOL g_bIsRedialing;
extern BOOL g_bReportError;

extern vector<SOCKET> g_vSocket;
extern CRITICAL_SECTION g_cs;

extern DWORD m_dwTimeOut;
extern DWORD m_dwRecvTimeOut;
extern DWORD m_dwTimeOut1;

extern BOOL b5001Linking;

extern BOOL bHeartBeatOk;

struct RECVPARAM
{
	SOCKET ClientSocket;
	SOCKET ServerSocket;
};

struct CURLRECVDATA
{
public:
	CURLRECVDATA()
	{
		clean();
	}

	~CURLRECVDATA()
	{
		clean();
	}

protected:
	void clean()
	{
		nRecvedLen = 0;
		pRecvData = NULL;
	}

public:
	size_t nRecvedLen;
	char*  pRecvData;
};

typedef size_t(*pCurlWriteFunction)(void*, size_t, size_t, void*);

extern CRITICAL_SECTION cs_Host;

extern map<string, ULONG> db;

extern char* pHostId;

extern DWORD g_dwPerMainLinkWork;

extern int nIndex;

extern BOOL bModeOldExit;
extern BOOL bModeOld6086Exit;

extern BOOL bOldMode;

extern char g_username[];
extern char g_password[];

extern vector<char*> vip;
extern int *pArrayIndex;
extern int nCountOfArray;

extern HANDLE hEventOfWorkerExit;
extern HANDLE hEventOf6086Exit;
extern HANDLE hEventOfNetCheckExit;
extern HANDLE hEventOfTimerStart;

extern unsigned int TimerId;

extern char g_pwatchversion[];

extern HANDLE hTheOneInstance;

extern unsigned int nThreadSC;

extern std::string url_b;
extern std::string url_a;