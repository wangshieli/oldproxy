#include "np_5001And6086_thread.h"
#include "np_5001_thread.h"
#include "np_6086_thread.h"
#include "np_tool_function.h"
#include "TraceLog.h"
#include "np_2_work_thread.h"
#include "np_usernamea_password.h"

unsigned int _stdcall np_5001And6086_thread_function(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	::SetEvent(g_hEventOf5001And6086Thread);

	while (true)
	{
		if (GetMessage(&msg, 0, 0, 0))
		{
			switch (msg.message)
			{
			case WM_MESSAGE_5001_START:
				{
//#if USE_BASIC
					GetUsernameAndPassword();
//#endif
					HANDLE h5001Thread = NULL;
					if (!bOldMode)
						h5001Thread = (HANDLE)_beginthreadex(NULL, 0, np_5001_thread_function, NULL, 0, &g_n5001ThreadNo);
					else
						h5001Thread = (HANDLE)_beginthreadex(NULL, 0, np_2_worker_function, NULL, 0, &g_n5001ThreadNo);
					if (NULL == h5001Thread)
						break;
					CloseHandle(h5001Thread);
				}
				break;

			case WM_MESSAGE_6086_START:
				{
					HANDLE h6086Thread = NULL;
					if (!bOldMode)
						h6086Thread = (HANDLE)_beginthreadex(NULL, 0, np_6086_thread_function, (void*)msg.wParam, 0, &g_n6086ThreadNo);
					else
						h6086Thread = (HANDLE)_beginthreadex(NULL, 0, np_2_6086_function, NULL, 0, &g_n6086ThreadNo);
					if (NULL == h6086Thread)
						break;
					CloseHandle(h6086Thread);
				}
				break;

			default:
				break;
			}
		}
	}

	return 0;
}