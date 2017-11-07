#include "np_ontimer_thread.h"
#include "np_ontimer_function.h"

unsigned int _stdcall np_ontimer_thread_function(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	
	SetTimer(NULL, 4, 1000*60*5, np_ontimer_report); // 定时上报
	SetTimer(NULL, 2, 1000*60*10, np_ontimer_checkversion); // 检查版本号
//	SetTimer(NULL, 3, 1000*30, np_ontimer_checklinking); // 检查联网状态
	SetTimer(NULL, 3, 1000*60*60*6, np_ontimer_anal_ip); // 的定时保存sockioserver ip

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (msg.message == WM_TIMER)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	KillTimer(NULL, 4);
	KillTimer(NULL, 2);
	KillTimer(NULL, 3);

	return 0;
}