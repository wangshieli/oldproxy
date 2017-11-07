#include "np_reportinformation_thread.h"
#include "np_helper_function.h"
#include "np_tool_function.h"

unsigned int _stdcall np_report_thread_function(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	::SetEvent(g_hEventOfReportThread);

	while (true)
	{
		if (GetMessage(&msg, 0, 0, 0))
		{
			switch (msg.message)
			{
			case WM_MESSAGE_REPORT_START:
				{
					g_bReportError = FALSE;
					int nTimesOfReport = 0;
					BOOL bOk = FALSE;
					BOOL bRt = FALSE;
					int *pFlag = (int*)msg.wParam;
					if (NULL != pFlag)
					{
						delete pFlag;
						pFlag = NULL;
						bRt = TRUE;
					}
					
					do
					{
						if (!SendHostInfo2API(curl_recvbackinfo_function))
						{
							if (bRt)
							{
								if (g_bIsRedialing)
								{
									bOk = TRUE;
									break;
								}
							}
						}else
						{
							bOk = TRUE;
							break;
						}
					} while (nTimesOfReport++ < 3);
					if (bOk)
					{
						if (!bRt)
							SetEvent(g_hEventForReportCompelet);
						break;
					}
					else
					{
						g_bIsRedialing = TRUE;
						if (!bRt)
						{
							g_bReportError = TRUE;
							SetEvent(g_hEventForReportCompelet);
						}					
						if (!bOldMode)
						{
							if (INVALID_SOCKET != g_HeartBeatsocket)
								LSCloseSocket(g_HeartBeatsocket);
							if (INVALID_SOCKET != g_5001socket)
								LSCloseSocket(g_5001socket);
							if (INVALID_SOCKET != g_6086socket)
								LSCloseSocket(g_6086socket);
							ClearThreadResource();
						}else
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
								WaitForSingleObject(hEventOf6086Exit, INFINITE);
							}
							if (TimerId != 0)
							{
								PostThreadMessage(TimerId, WM_TIMER_THREAD_QUIT, 0, 0);
								TimerId = 0;
								WaitForSingleObject(hEventOfNetCheckExit, INFINITE);
							}
						}
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
						PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
						break;
					}
				}
				break;
			default:
				break;
			}
		}
	}

	return 0;
}