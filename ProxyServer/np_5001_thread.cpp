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
		myprintf("���������ipʧ��");
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
				alive_in.keepalivetime = 1000*60*2; // ��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ��
				alive_in.keepaliveinterval = 1000*5;// ����KeepAlive̽����ʱ����
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

				if (!SendHostIdToServer(g_5001socket, pHostId)) // ��server���ʹ����host_id
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
	// �������һֱ����server�������Ŀͻ��˶˿���Ϣ�ˣ�Ȼ������µ��̣߳�ע��������ݳ���Ҫʮ�ֵľ�ȷ
	// ��Ϊ����ԭ�������������ڻ������Ļ��ۣ����ܾ��кü����ͻ�����Ϣ��ѹ
	while (1)
	{
		char* pRetInfo = NULL;
		try
		{
			if (!RecvNewPortInfoFromServer(g_5001socket, &pRetInfo))// �������ݳ��Ƚ�������
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
		if (!AnalyzePortFromRetInfo(pRetInfo, &pNewPort))// �������µĶ˿ں�
		{
			myprintf("�����˿ں�ʧ��");
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

		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, np_worker_thread_function, pNewPort, 0, NULL);// ���������߳�
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