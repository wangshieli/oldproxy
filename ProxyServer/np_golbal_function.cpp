#include "np_golbal_header.h"

SOCKET g_6086socket = INVALID_SOCKET;
SOCKET g_5001socket = INVALID_SOCKET;
SOCKET g_HeartBeatsocket = INVALID_SOCKET;

char g_cCurrentIP[32] = {0};
CRITICAL_SECTION g_csCurrentIP;

char* g_pCurrentPort = NULL;
CRITICAL_SECTION g_csCurrentPort;

unsigned int g_n5001And6086ThreadNo = 0;
HANDLE g_hEventOf5001And6086Thread = NULL;

unsigned int g_n5001ThreadNo = 0;

unsigned int g_n6086ThreadNo = 0;

unsigned int g_nRedialThreadNo = 0;
HANDLE g_hEventOfRedialThread = NULL;
HANDLE g_hEventForRedialCompelet = NULL;

unsigned int g_nReportInfoThreadNo = 0;
HANDLE g_hEventOfReportThread = NULL;
HANDLE g_hEventForReportCompelet = NULL;

unsigned int g_nDownloadThreadNo = 0;
HANDLE g_hEventOfDownloadThread = NULL;
HANDLE g_hEventForDownloadCompelet = NULL;

HANDLE g_hMode2StartOk = NULL;

//HANDLE g_hEventFor5001Compelet = NULL;;

unsigned int g_nMessageThreadNo = 0;

BOOL bNeedDownUpgrade = FALSE;

BOOL g_bIsRedialing = FALSE;
BOOL g_bReportError = FALSE;

vector<SOCKET> g_vSocket;
CRITICAL_SECTION g_cs;

DWORD m_dwTimeOut = 1000*60;
DWORD m_dwRecvTimeOut = 1000 * 150;
DWORD m_dwTimeOut1 = 1000*10;

BOOL b5001Linking = FALSE;

BOOL bHeartBeatOk = TRUE;

unsigned int tHeartBeatThreadID = 0;

CRITICAL_SECTION cs_Host;

map<string, ULONG> db;

char* pHostId = NULL;

DWORD g_dwPerMainLinkWork = 0;

BOOL bModeOldExit = FALSE;
BOOL bModeOld6086Exit = FALSE;

BOOL bOldMode = FALSE;

char g_username[16];
char g_password[16];

char g_pwatchversion[8];