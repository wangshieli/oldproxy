#include "np_6086_thread.h"
#include "np_helper_function.h"
#include "np_tool_function.h"
#include "np_heartbeat_timer.h"

unsigned int _stdcall np_6086_thread_function(LPVOID pVoid)
{
	char* pnewport = (char*)pVoid;
	int ret1 = 0;
	int testTimes = 0;
	do
	{
		if (testTimes > 5)
		{
			g_bIsRedialing = FALSE;
			goto REDIAL;
		}
		ret1 = ConnectToServer(g_6086socket, vip[pArrayIndex[nIndex]], 6086);
		if (ret1 == 0)
		{
			break;
		}
		else if (ret1 == -1)
		{
			g_bIsRedialing = FALSE;
			goto REDIAL;
		}
		testTimes++;
	} while (ret1 == 1);

	BOOL bKeepAlive = TRUE;
	ret1 = ::setsockopt(g_6086socket, 
		SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));
	if (SOCKET_ERROR == ret1)
	{
		g_bIsRedialing = FALSE;
		goto REDIAL;
	}

	// http://www.cnblogs.com/BeginGame/archive/2011/09/24/2189750.html
	struct tcp_keepalive alive_in;
	struct tcp_keepalive alive_out;
	ZeroMemory(&alive_in, sizeof(alive_in));
	ZeroMemory(&alive_out, sizeof(alive_out));
	alive_in.keepalivetime = 20*1000; // 开始首次KeepAlive探测前的TCP空闭时间
	alive_in.keepaliveinterval = 1000*5;// 两次KeepAlive探测间的时间间隔
	alive_in.onoff = TRUE;
	unsigned long ulBytesReturn = 0;
	ret1 = WSAIoctl(g_6086socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	if (SOCKET_ERROR == ret1)
	{
		g_bIsRedialing = FALSE;
		goto REDIAL;
	}

	setsockopt(g_6086socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&m_dwTimeOut, sizeof(DWORD));
	if (!Send6086HostIdToServer(g_6086socket, pHostId))
	{
		g_bIsRedialing = FALSE;
		goto REDIAL;
	}

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
		return 0;

	g_bIsRedialing = FALSE;

	while (!bHeartBeatOk)
	{
		Sleep(1000);
	}

	HANDLE hOnTimerThread1 = (HANDLE)_beginthreadex(NULL, 0, np_heartbeat_timer, NULL, 0, &tHeartBeatThreadID);
	if (NULL == hOnTimerThread1)
	{
		goto REDIAL;
	}
	CloseHandle(hOnTimerThread1);

	char* pRecv6086Info = (char*)malloc(512);
	unsigned long WaitBytes = 0;
	int recved = 0;
	int nRet = 0;
	int ret = 0;
	do{
		if (_msize(pRecv6086Info) < (WaitBytes + recved))
		{
			int NeedLen = MY_ALIGN((WaitBytes + recved + 1), 8);
			pRecv6086Info = (char*)realloc(pRecv6086Info, NeedLen);
		}
		ret = recv(g_6086socket,pRecv6086Info + recved,512,0);
		if (ret > 0) {
			recved += ret;
		} else {
			//	cout << "6086socket接收过程中发生错误:" << WSAGetLastError() << endl;
			if (NULL != pRecv6086Info)
			{
				free(pRecv6086Info);
				pRecv6086Info = NULL;
			}
			if (10053 == WSAGetLastError() || 10038 == WSAGetLastError())
			{	
				return 0;
			}
			else
			{
				goto REDIAL;
			}
		}
		nRet = ioctlsocket(g_6086socket, FIONREAD, &WaitBytes);
		if (SOCKET_ERROR == nRet)
		{
			if (NULL != pRecv6086Info)
			{
				free(pRecv6086Info);
				pRecv6086Info = NULL;
			}
			if (10053 == WSAGetLastError() || 10038 == WSAGetLastError())
				return 0;
			else
				goto REDIAL;
		}
	}while (WaitBytes > 0);
	pRecv6086Info[recved] = '\0';
	myprintf("6086请求:%s", pRecv6086Info);

	if (strncmp("POST ", pRecv6086Info, 5) == 0)
	{
		if (strncmp("/change", pRecv6086Info + 5, 7) != 0)
		{
			if (strncmp("/chmode", pRecv6086Info + 5, 7) == 0)
			{
				if (!g_bIsRedialing)
				{
					g_bIsRedialing = TRUE;
					if (INVALID_SOCKET != g_HeartBeatsocket)
						LSCloseSocket(g_HeartBeatsocket);
					if (g_5001socket != INVALID_SOCKET)
						LSCloseSocket(g_5001socket);
					if (g_6086socket != INVALID_SOCKET)
						LSCloseSocket(g_6086socket);

					ClearThreadResource();
					Sleep(1000);
					bOldMode = TRUE;
					PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
				}
				if (NULL != pRecv6086Info)
				{
					free(pRecv6086Info);
					pRecv6086Info = NULL;
				}
				return 0;
			}		
		}
	}else
	{
		myprintf("6086重拨请求格式错误");
	}
	
	if (NULL != pRecv6086Info)
	{
		free(pRecv6086Info);
		pRecv6086Info = NULL;
	}


REDIAL:
	if (g_bIsRedialing)
		return 0;

	g_bIsRedialing = TRUE;
	if (INVALID_SOCKET != g_HeartBeatsocket)
		LSCloseSocket(g_HeartBeatsocket);

	if (g_5001socket != INVALID_SOCKET)
	{
		LSCloseSocket(g_5001socket);
	}
	if (g_6086socket != INVALID_SOCKET)
	{
		LSCloseSocket(g_6086socket);
	}

	ClearThreadResource();
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);

	return 0;
}