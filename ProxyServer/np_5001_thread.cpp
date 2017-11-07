#include "np_5001_thread.h"
#include "np_helper_function.h"
#include "np_tool_function.h"
#include "np_worker_thread.h"
#include "np_DNS.h"

int nIndex = 0;
int nCountOfArray = 0;
int *pArrayIndex = NULL;

unsigned int _stdcall np_5001_thread_function(LPVOID pVoid)
{
	if (!GetServerIpFromHost())
	{
		myprintf("解析服务端ip失败");
		g_bIsRedialing = FALSE;
		goto REDIAL;
	}
	GetRandIndex(pArrayIndex, nCountOfArray);
	int ret = 0;

	for (int i = 0; i < nCountOfArray;)
	{
		try
		{
			ret = ConnectToServer(g_5001socket, vip[pArrayIndex[i]], 5001);
			if (ret == 0)
			{
				throw new int(1);
			}
			else /*if (ret == -1)*/
			{
				throw new int(0);
			}
		}catch(int *err)
		{
			if (*err == 0)
			{
				delete err;
				if (++i == nCountOfArray)
				{
					g_bIsRedialing = FALSE;
					goto REDIAL;
				}
				continue;
			}else if (*err == 1)
			{
				delete err;

				/******************************/
				//setsockopt(g_5001socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&m_dwRecvTimeOut, sizeof(DWORD));
				//setsockopt(g_5001socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&m_dwTimeOut, sizeof(DWORD));

				BOOL bKeepAlive = TRUE;
				int nRet = ::setsockopt(g_5001socket, 
					SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));
				if (SOCKET_ERROR == nRet)
				{
					if (++i == nCountOfArray)
					{
						g_bIsRedialing = FALSE;
						goto REDIAL;
					}
					continue;
				}

				struct tcp_keepalive alive_in = {0};
				struct tcp_keepalive alive_out = {0};
				alive_in.keepalivetime = 1000*60*2; // 开始首次KeepAlive探测前的TCP空闭时间
				alive_in.keepaliveinterval = 1000*5;// 两次KeepAlive探测间的时间间隔
				alive_in.onoff = TRUE;
				unsigned long ulBytesReturn = 0;
				nRet = WSAIoctl(g_5001socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
					&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
				if (SOCKET_ERROR == nRet)
				{
					if (++i == nCountOfArray)
					{
						g_bIsRedialing = FALSE;
						goto REDIAL;
					}
					continue;
				}

				if (!SendHostIdToServer(g_5001socket, pHostId)) // 向server发送代理的host_id
				{
					if (++i == nCountOfArray)
					{
						g_bIsRedialing = FALSE;
						goto REDIAL;
					}
					continue;
				}

				//	setsockopt(g_5001socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&m_dwRecvTimeOut, sizeof(DWORD));
				char* pretruninfo = NULL;
				try
				{
					if (!RecvNewPortInfoFromServer(g_5001socket, &pretruninfo))
					{
						if (++i == nCountOfArray)
						{
							g_bIsRedialing = FALSE;
							goto REDIAL;
						}
						continue;
					}
				}catch(int* pErr)
				{
					delete pErr;
					if (++i == nCountOfArray)
					{
						g_bIsRedialing = FALSE;
						goto REDIAL;
					}
					continue;
				}

				if (strstr(pretruninfo, "-Proxy5001IsRead") == NULL)
				{
					if (pretruninfo != NULL)
					{
						delete pretruninfo;
						pretruninfo = NULL;
					}
					myprintf("recv from server for hostid info failed");
					if (++i == nCountOfArray)
					{
						g_bIsRedialing = FALSE;
						goto REDIAL;
					}
					continue;
				}
				else
				{
					myprintf("%s", pretruninfo);
					char *pNewPort = NULL;
					if (!AnalyzePortFromRetInfo(pretruninfo, &pNewPort))
					{
						if (pretruninfo != NULL)
						{
							delete pretruninfo;
							pretruninfo = NULL;
						}
						if (++i == nCountOfArray)
						{
							g_bIsRedialing = FALSE;
							goto REDIAL;
						}
						continue;
					}
					if (pretruninfo != NULL)
					{
						delete pretruninfo;
						pretruninfo = NULL;
					}
					if (strncmp("0000", pNewPort, 4) == 0)
					{
						if (++i == nCountOfArray)
						{
							g_bIsRedialing = FALSE;
							goto REDIAL;
						}
						continue;
					}

					nIndex = i;
					PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_6086_START, (WPARAM)pNewPort, NULL);
					break;
				}
				/******************************/
			}	
		}
	}

	//DWORD m_notimeout = 0;
	//setsockopt(g_5001socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&m_notimeout, sizeof(DWORD));
	//setsockopt(g_5001socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&m_notimeout, sizeof(DWORD));
	// 下面就是一直接收server发送来的客户端端口信息了，然后分配新的线程，注意接收数据长度要十分的精确
	// 因为网络原因可能造成数据在缓冲区的积累，可能就有好几个客户端信息积压
	while (1)
	{
		char* pRetInfo = NULL;
		try
		{
			if (!RecvNewPortInfoFromServer(g_5001socket, &pRetInfo))// 按照数据长度接收数据
			{
				//goto REDIAL;
				return 0;
			}
		}catch(int* pErr)
		{
			delete pErr;
			goto REDIAL;
		}

	//	myprintf("%s", pRetInfo);
		char *pNewPort = NULL;
		if (!AnalyzePortFromRetInfo(pRetInfo, &pNewPort))// 解析出新的端口号
		{
			myprintf("解析端口号失败");
			if (pRetInfo != NULL)
			{
				delete pRetInfo;
				pRetInfo = NULL;
			}
			goto REDIAL;
		}

		if (pRetInfo != NULL)
		{
			delete pRetInfo;
			pRetInfo = NULL;
		}

		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, np_worker_thread_function, pNewPort, 0, NULL);// 启动工作线程
		CloseHandle(hWorkerThread);
		hWorkerThread = NULL;
	}

	return 0;

REDIAL:
	if (g_bIsRedialing)
		return 0;

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
	return 0;
}