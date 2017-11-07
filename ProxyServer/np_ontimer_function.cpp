#include "np_helper_function.h"
#include "np_tool_function.h"
#include "np_DNS.h"

void _stdcall np_ontimer_anal_ip(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	if (g_bIsRedialing)
		return;
	
	if (!GetSocketIoServerIp())
		url_b = SOCKET_IO_URL;
	else
		reg_get_server_ip(url_b);
	url_a = SOCKET_IO_URL;
}

void _stdcall np_ontimer_report(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	if (g_bIsRedialing)
		return;
	
	int *pFlag = new int(1);
	PostThreadMessage(g_nReportInfoThreadNo, WM_MESSAGE_REPORT_START, (WPARAM)pFlag, 0);// wparam 参数应该是端口参数
}

void _stdcall np_ontimer_checkversion(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	if (g_bIsRedialing)
		return;
	GetHostInfo();
	PostThreadMessage(g_nDownloadThreadNo, WM_MESSAGE_DOWNLOAD_START, 0, 0);
}

void _stdcall np_ontimer_checklinking(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	if (g_bIsRedialing)
		return;

	if(b5001Linking)
		goto redial;
	
	int nByteSent=send(g_5001socket,"",0,0);
	if (SOCKET_ERROR == nByteSent) 
	{
		if(g_5001socket != INVALID_SOCKET)
		{
			LSCloseSocket(g_5001socket);
			return;
		}else
		{
			if (g_6086socket != INVALID_SOCKET)
			{
				LSCloseSocket(g_6086socket);
			}
			goto redial;
		}
	}

	nByteSent = send(g_6086socket,"",0,0);
	if (SOCKET_ERROR == nByteSent) 
	{
		if(g_5001socket != INVALID_SOCKET)
		{
			LSCloseSocket(g_5001socket);
			return;
		}else
		{
			if (g_6086socket != INVALID_SOCKET)
			{
				LSCloseSocket(g_6086socket);
			}
			goto redial;
		}
		
	}

	return;

redial:
	ClearThreadResource();
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
	return;
}

void _stdcall np_ontimer_heartbeat(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime)
{
	if (g_bIsRedialing)
	{
		bHeartBeatOk = TRUE;
		PostThreadMessage(tHeartBeatThreadID, WM_TIMER_THREAD_QUIT, NULL, NULL);
		return;
	}

	int ret = 0;
	if (INVALID_SOCKET == g_HeartBeatsocket)
	{
		int testTimes = 0;
		do
		{
			if (testTimes > 5)
				goto REDIAL;
			ret = ConnectToServer(g_HeartBeatsocket, vip[pArrayIndex[nIndex]], 5005);
			if (ret == 0)
			{
				if (!SendHostIdToServer(g_HeartBeatsocket, pHostId)) // 向server发送代理的host_id
				{
					goto REDIAL;;
				}

				setsockopt(g_HeartBeatsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&m_dwTimeOut1, sizeof(DWORD));
				setsockopt(g_HeartBeatsocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&m_dwTimeOut1, sizeof(DWORD));

				break;
			}
			else if (ret == -1)
			{
				goto REDIAL;
			}
			testTimes++;
		} while (ret == 1);
	}

	if (!SendHostIdToServer(g_HeartBeatsocket, pHostId)) // 向server发送代理的host_id
	{
		goto REDIAL;;
	}

	return;

REDIAL:
	if (g_bIsRedialing)
	{
		//if (INVALID_SOCKET != g_HeartBeatsocket)
		//	LSCloseSocket(g_HeartBeatsocket);
		bHeartBeatOk = TRUE;
		PostThreadMessage(tHeartBeatThreadID, WM_TIMER_THREAD_QUIT, NULL, NULL);
		return;
	}

	g_bIsRedialing = TRUE;
	if (INVALID_SOCKET != g_HeartBeatsocket)
		LSCloseSocket(g_HeartBeatsocket);
	if (INVALID_SOCKET != g_5001socket)
		LSCloseSocket(g_5001socket);
	if (INVALID_SOCKET != g_6086socket)
		LSCloseSocket(g_6086socket);
	ClearThreadResource();
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);

	PostThreadMessage(tHeartBeatThreadID, WM_TIMER_THREAD_QUIT, NULL, NULL);
	bHeartBeatOk = TRUE;
}