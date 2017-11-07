#include "np_message_thread.h"

unsigned int _stdcall np_message_thread_function(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	while (true)
	{
		if (GetMessage(&msg, 0, 0, 0))
		{
			switch (msg.message)
			{
			case WM_MESSAGE_COUT_MESSAGE:
				{
					char* pInfo = (char*)msg.wParam;
					SYSTEMTIME sys;
					GetLocalTime(&sys);
					printf("%d-%d-%d %02d:%02d:%02d:%s\n", sys.wYear, 
						sys.wMonth, 
						sys.wDay, 
						sys.wHour, 
						sys.wMinute, 
						sys.wSecond,
						pInfo);
					if (pInfo != NULL)
					{
						delete pInfo;
						pInfo = NULL;
					}
				}
				break;

			case WM_MESSAGE_LOG:
				{
					char* pLog = (char*)msg.wParam;
					SYSTEMTIME sys;
					GetLocalTime(&sys);
					printf("%d-%d-%d %02d:%02d:%02d:%s\n", sys.wYear, 
						sys.wMonth, 
						sys.wDay, 
						sys.wHour, 
						sys.wMinute, 
						sys.wSecond,
						pLog);
					delete pLog;
					pLog = NULL;
				}
				break;
			default:
				break;
			}
		}
	}

	return 0;
}