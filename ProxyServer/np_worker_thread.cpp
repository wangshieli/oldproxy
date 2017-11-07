#include "np_worker_thread.h"
#include "np_helper_function.h"
#include "np_tool_function.h"

unsigned int _stdcall np_worker_thread_function(LPVOID pVoid)
{
	int nNewPort = atoi((char*)pVoid);
	delete (char*)pVoid;

	SOCKET newPortSocket = INVALID_SOCKET;

	int testTimes = 0;
	int ret = 0;
	do
	{
		if (testTimes > 5)
			return 0;
		ret = ConnectToServer(newPortSocket, vip[pArrayIndex[nIndex]], nNewPort);
		if (ret == 0)
			break;
		else if (ret == -1)
		{
			LSCloseSocket(newPortSocket);
			return 0;
		}
		testTimes++;
	} while (ret == 1);

	setsockopt(newPortSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&m_dwTimeOut, sizeof(DWORD));
	setsockopt(newPortSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&m_dwTimeOut, sizeof(DWORD));

	EnterCriticalSection(&g_cs);
	g_vSocket.push_back(newPortSocket);
	LeaveCriticalSection(&g_cs);

	RECVPARAM* lpParameter = new RECVPARAM;
	lpParameter->ClientSocket = newPortSocket;
	lpParameter->ServerSocket = INVALID_SOCKET;
	if (!ProxyThread01(lpParameter))
	{
		newPortSocket = INVALID_SOCKET;
		delete lpParameter;
		lpParameter = NULL;
		return 0;
	}

	newPortSocket = INVALID_SOCKET;
	delete lpParameter;
	lpParameter = NULL;
	return 0;
}