#include "np_download_thread.h"
#include "np_helper_function.h"

unsigned int _stdcall np_downlaod_thread_function(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	::SetEvent(g_hEventOfDownloadThread);

	while (true)
	{
		if (GetMessage(&msg, 0, 0, 0))
		{
			switch (msg.message)
			{
			case WM_MESSAGE_DOWNLOAD_START:
				{
					if (!new_download())
					{
						::SetEvent(g_hEventForDownloadCompelet);
						break;
					}
					/*if (!CheckVersionAndDownFile("client.exe", 1))
					{
						::SetEvent(g_hEventForDownloadCompelet);
						break;
					}
					if (bNeedDownUpgrade)
					{
						if (!CheckVersionAndDownFile("upgrade.exe", 2))
						{
							::SetEvent(g_hEventForDownloadCompelet);
							break;
						}
					}*/
					::SetEvent(g_hEventForDownloadCompelet);
				}
				break;
			default:
				break;
			}
		}
	}

	return 0;
}