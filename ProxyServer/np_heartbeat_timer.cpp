#include "np_heartbeat_timer.h"
#include "np_ontimer_function.h"
#include "np_helper_function.h"
#include "np_tool_function.h"

unsigned int _stdcall np_heartbeat_timer(LPVOID pVoid)
{
	bHeartBeatOk = FALSE;
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	SetTimer(NULL, 4, 1000, np_ontimer_heartbeat); // ¼ì²éÁªÍø×´Ì¬

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_TIMER_THREAD_QUIT)
		{
			break;
		}

		if (msg.message == WM_TIMER)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	KillTimer(NULL, 4);
	bHeartBeatOk = TRUE;
	return 0;
}