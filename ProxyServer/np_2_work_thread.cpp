#include "np_2_work_thread.h"
#include "np_tool_function.h"
#include "np_helper_function.h"
#include "np_usernamea_password.h"

SOCKET sListenSock = INVALID_SOCKET;
SOCKET s6086Sock  = INVALID_SOCKET;

HANDLE hEventOfWorkerExit = NULL;
HANDLE hEventOf6086Exit = NULL;
HANDLE hEventOfNetCheckExit = NULL;
HANDLE hEventOfTimerStart = NULL;

unsigned int TimerId = 0;

unsigned int _stdcall np_2_worker_function(LPVOID pVoid)
{
	sListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (sListenSock == INVALID_SOCKET)
	{
		goto REDAIL;
	}
	struct sockaddr_in sAddr;
	do
	{
		sAddr.sin_family = AF_INET;
		sAddr.sin_port = htons(GetPort());
		sAddr.sin_addr.S_un.S_addr = INADDR_ANY;

		if ((bind(sListenSock, (LPSOCKADDR)&sAddr, sizeof(sAddr)) == SOCKET_ERROR)
			|| (listen(sListenSock, SOMAXCONN) == SOCKET_ERROR))
		{
			if (WSAGetLastError() == WSAEADDRINUSE)
				continue;

			goto REDAIL;
		}
		break;
	}while (true);

	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_6086_START, NULL, NULL);
	bModeOldExit = FALSE;
	SOCKET sAccept = INVALID_SOCKET;
	while (!bModeOldExit)
	{
		sAccept = accept(sListenSock, NULL, NULL);
		if (INVALID_SOCKET == sAccept)
			continue;
		RECVPARAM* pParameter = new RECVPARAM();
		pParameter->ServerSocket = INVALID_SOCKET;
		pParameter->ClientSocket = sAccept;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, np_proxy_function, pParameter, 0, NULL);
		CloseHandle(hThread);
	}
	SetEvent(hEventOfWorkerExit);
	return 0;

REDAIL:
	bModeOldExit = TRUE;
	if (INVALID_SOCKET != sListenSock)
	{
		closesocket(sListenSock);
		sListenSock = INVALID_SOCKET;
	}
	
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
	return 0;
}

unsigned int _stdcall np_proxy_function(LPVOID pVoid)
{
	RECVPARAM* pParameter = (RECVPARAM*)pVoid;
	char* RecvBuf = (char*)malloc(MAXSIZE);
	memset(RecvBuf, 0x00, MAXSIZE);
	char* SendBuf = NULL;
	__try
	{
		int retval = RecvRequest01(pParameter->ClientSocket, &RecvBuf, MAXSIZE);
		if (SOCKET_ERROR == retval || 0 == retval)
			__leave;
#if USE_BASIC
		printf("接收到的请求数据：%s\n", RecvBuf);
		if (!VerifyBasic(RecvBuf))
		{
			Return401(pParameter->ClientSocket);
			__leave;
		}
#endif
		if (strncmp("CONNECT ", RecvBuf, 8) == 0)
		{
			myprintf("Connect Request connection");
			if (ConnectServer(pParameter->ServerSocket, RecvBuf, retval) < 0)
				__leave;
			if (PreResponse(pParameter) < 0)
				__leave;
			ExchangeData(pParameter);
		}
		else
		{
			size_t len = _msize(RecvBuf);
			SendBuf = (char*)malloc(len);
			memset(SendBuf, 0x00, len);
			SendWebRequest(pParameter, SendBuf, RecvBuf, retval);
		}
	}
	__finally
	{
		free(RecvBuf);
		RecvBuf = NULL;
		if (SendBuf != NULL)
		{
			free(SendBuf);
			SendBuf = NULL;
		}
		if (pParameter->ClientSocket != INVALID_SOCKET)
		{
			closesocket(pParameter->ClientSocket);
			pParameter->ClientSocket = INVALID_SOCKET;
		}
		if (pParameter->ServerSocket != INVALID_SOCKET)
		{
			closesocket(pParameter->ServerSocket);
			pParameter->ServerSocket = INVALID_SOCKET;
		}
		delete pParameter;
		pParameter = NULL;
	}
	return 0;
}

DWORD PerTimer = 0;
int CALLBACK AcceptCondition(LPWSABUF lpCallerId, LPWSABUF, 
							 LPQOS, LPQOS ,
							 LPWSABUF, LPWSABUF, 
							 GROUP FAR*, DWORD_PTR dwCallbackData
							 )
{
	if (g_bIsRedialing)
		return CF_REJECT;

	DWORD NowTimer = GetTickCount();
	if ((NowTimer - PerTimer) < 15000)
		return CF_REJECT;

	PerTimer = GetTickCount();
	return CF_ACCEPT;
}

DWORD nTimeOut = 5*1000;
HANDLE hTimer = NULL;

unsigned int _stdcall np_2_6086_function(LPVOID pVoid)
{
	s6086Sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == s6086Sock)
	{
		goto REDAIL;
	}
	struct sockaddr_in sAddr;
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(6086);
	sAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if ((bind(s6086Sock, (LPSOCKADDR)&sAddr, sizeof(sAddr)) == SOCKET_ERROR) ||
		(listen(s6086Sock, 1) == SOCKET_ERROR))
	{
		if (WSAGetLastError() == WSAEADDRINUSE)
		{
			myprintf("6086端口已经被使用，检查是否多用户登录远程，然后重启代理或电脑");
			return 0;
		}
		goto REDAIL;
	}

	sockaddr_in lsnAddr;
	int lsnAddrLen = sizeof(sockaddr_in);
	if (0 != getsockname(sListenSock, (sockaddr*)&lsnAddr, &lsnAddrLen))
	{
		goto REDAIL;
	}

	char* pnewport = new char[8];
	ZeroMemory(pnewport, 8);
	_itoa_s((int)ntohs(lsnAddr.sin_port), pnewport,  8, 10);

	EnterCriticalSection(&g_csCurrentPort);
	if (g_pCurrentPort != NULL)
	{
		delete g_pCurrentPort;
		g_pCurrentPort = NULL;
	}
	g_pCurrentPort = pnewport;
	LeaveCriticalSection(&g_csCurrentPort);
	
	PostThreadMessage(g_nReportInfoThreadNo, WM_MESSAGE_REPORT_START, NULL, 0);

	WaitForSingleObject(g_hEventForReportCompelet, INFINITE);
	if (g_bReportError)
	{
		SetEvent(hEventOf6086Exit);
		return 0;
	}

	g_bIsRedialing = FALSE;
	if (NULL != hTimer)
	{
		CloseHandle(hTimer);
		hTimer = NULL;
	}
	
	hTimer = (HANDLE)_beginthreadex(NULL, 0, timer_thread, NULL, 0, &TimerId);
	if (hTimer != NULL)
	{
		WaitForSingleObject(hEventOfTimerStart, INFINITE);
		//CloseHandle(hTimer);
	}
	else
		TimerId = 0;

	bModeOld6086Exit = FALSE;

	SOCKET sAccept = INVALID_SOCKET;
	while (!bModeOld6086Exit)
	{
		struct sockaddr_in addr;
		int len = sizeof(addr);
		sAccept = WSAAccept(s6086Sock, (sockaddr*)&addr, &len, AcceptCondition, NULL);
		if (INVALID_SOCKET == sAccept)
			continue;
		setsockopt(sAccept, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeOut, sizeof(DWORD));
		setsockopt(sAccept, SOL_SOCKET, SO_SNDTIMEO, (const char*)&nTimeOut, sizeof(DWORD));

		char* rcvBuf = (char*)malloc(1024);
		memset(rcvBuf, 0x00, 1024);

		__try
		{
			int retval = RecvRequest02(sAccept, &rcvBuf, 1024);
			if (SOCKET_ERROR == retval || 0 == retval)
				__leave;

			if (strncmp("POST ", rcvBuf, 5) == 0)
			{
				if (strncmp("/change", rcvBuf + 5, 7) != 0)
				{
					if (strncmp("/chmode", rcvBuf + 5, 7) != 0)
					{
						__leave;
					}
					else
					{
						bModeOldExit = TRUE;
						if (INVALID_SOCKET != sListenSock)
						{
							closesocket(sListenSock);
							sListenSock = INVALID_SOCKET;
							WaitForSingleObject(hEventOfWorkerExit, INFINITE);
						}
						
						bModeOld6086Exit = TRUE;
						if (INVALID_SOCKET != s6086Sock)
						{
							closesocket(s6086Sock);
							s6086Sock = INVALID_SOCKET;
						}

						if (TimerId != 0)
						{
							PostThreadMessage(TimerId, WM_TIMER_THREAD_QUIT, 0, 0);
							TimerId = 0;
							WaitForSingleObject(hEventOfNetCheckExit, INFINITE);						
						}

						if (!g_bIsRedialing)
						{
							g_bIsRedialing = TRUE;
							bOldMode = FALSE;
							PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
						}
						send(sAccept, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
						//SetEvent(hEventOf6086Exit);
						return 0;
					}		
					__leave;
				}
			}else
			{
				myprintf("6086重拨请求格式错误");
				__leave;
			}

			myprintf("6086重拨请求:%s", rcvBuf);
			send(sAccept, "ok", 2, 0);

			if (!g_bIsRedialing)
			{
				g_bIsRedialing = TRUE;
				goto REDAIL;
			}
			//SetEvent(hEventOf6086Exit);
			return 0;
		}
		__finally
		{
			free(rcvBuf);
			rcvBuf = NULL;
			closesocket(sAccept);
			sAccept = INVALID_SOCKET;
		}
	}
	SetEvent(hEventOf6086Exit);
	return 0;

REDAIL:
	bModeOldExit = TRUE;
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n\r\n\r\n开始关闭5005 6086socket\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	if (INVALID_SOCKET != sListenSock)
	{
		closesocket(sListenSock);
		sListenSock = INVALID_SOCKET;
		WaitForSingleObject(hEventOfWorkerExit, INFINITE);
	}
	bModeOld6086Exit = TRUE;
	if (INVALID_SOCKET != s6086Sock)
	{
		closesocket(s6086Sock);
		s6086Sock = INVALID_SOCKET;
	}

	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n\r\n\r\n关闭5005 6086socket完成\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n\r\n\r\n开始TimerId线程退出\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	if (TimerId != 0)
	{
		PostThreadMessage(TimerId, WM_TIMER_THREAD_QUIT, 0, 0);
		if (WaitForSingleObject(hEventOfNetCheckExit, 1000 * 10) == WAIT_TIMEOUT)
		{
			KillTimer(NULL, 1);
			TerminateThread(hTimer, 0);
		}
		CloseHandle(hTimer);
		hTimer = NULL;
		TimerId = 0;
	}
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n\r\n\r\nTimerId线程退出完成\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
	return 0;
}

void _stdcall OnTimerCheckLinking(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	if (g_bIsRedialing)
	{
		return;
	}
	int n = 3;

	while (n-- > 0)
	{
		//char result[1024*5] = {0};
		char* result = NULL;
		Execmd("ping www.baidu.com -n 1", &result);

		__try
		{
			if (strstr(result, "已接收 = 1") != NULL || strstr(result, "Received = 1") != NULL)
			{
				break;
			}else
			{
				if (!g_bIsRedialing)
				{		
					if (n == 0)
					{
						g_bIsRedialing = TRUE;
						bModeOldExit = TRUE;
						if (INVALID_SOCKET != sListenSock)
						{
							closesocket(sListenSock);
							sListenSock = INVALID_SOCKET;
							WaitForSingleObject(hEventOfWorkerExit, INFINITE);
						}
						bModeOld6086Exit = TRUE;
						if (INVALID_SOCKET != s6086Sock)
						{
							closesocket(s6086Sock);
							s6086Sock = INVALID_SOCKET;
							WaitForSingleObject(hEventOf6086Exit, INFINITE);
						}	

						if (TimerId != 0)
						{
							PostThreadMessage(TimerId, WM_TIMER_THREAD_EXIT, 0, 0);
						//	WaitForSingleObject(hEventOfNetCheckExit, INFINITE);
							TimerId = 0;
							return;
						}
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
						PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
					}
				}else
					break;
			}
		}
		__finally
		{
			if (NULL != result)
			{
				free(result);
				result = NULL;
			}
		}
	}
}

unsigned int _stdcall timer_thread(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(hEventOfTimerStart);
	SetTimer(NULL, 1, 1000*60, OnTimerCheckLinking); // 检查联网状态

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_TIMER_THREAD_QUIT)
		{
			KillTimer(NULL, 1);
			SetEvent(hEventOfNetCheckExit);
			break;
		}

		if (msg.message == WM_TIMER_THREAD_EXIT)
		{
			KillTimer(NULL, 1);
			//SetEvent(hEventOfNetCheckExit);
			PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
			WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
			PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
			break;
		}

		if (msg.message == WM_TIMER)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	return 0;
}