#include "np_redial_thread.h"
#include "np_tool_function.h"

//unsigned int _stdcall np_redial_thread_function(LPVOID pVoid)
//{
//	MSG msg;
//	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
//
//	::SetEvent(g_hEventOfRedialThread);
//
//	while (true)
//	{
//		if (GetMessage(&msg, 0, 0, 0))
//		{
//			switch (msg.message)
//			{
//			case WM_MESSAGE_REDIAL_START:
//				{
//					while (PeekMessage(&msg, NULL, WM_MESSAGE_REDIAL_START, WM_MESSAGE_REDIAL_START, PM_REMOVE))
//					{
//						;
//					}
//
//					char* pAdslIsExist = NULL;
//					char *pCmdResultInfo = NULL;
//					do
//					{
//						if (pCmdResultInfo != NULL)
//						{
//							free(pCmdResultInfo);
//							pCmdResultInfo = NULL;
//						}
//						if (!Execmd("rasdial adsl /disconnect", &pCmdResultInfo))
//							continue;
//						pAdslIsExist = strstr(pCmdResultInfo, "没有连接");
//						if (pAdslIsExist == NULL)
//							pAdslIsExist = strstr(pCmdResultInfo, "No connections");
//					}while(pAdslIsExist == NULL);
//
//					if (pCmdResultInfo != NULL)
//					{
//						free(pCmdResultInfo);
//						pCmdResultInfo = NULL;
//					}
//#if IS_TEST
//					myprintf("测试不真的拨号");
//#else
//					if (!Redial())
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//#endif
//					char newIP[32] = {0};
//#if IS_TEST
//					memcpy(newIP, REDAIL_IP, strlen(REDAIL_IP));
//#else
//					if (!GetAdslIp(newIP))
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//#endif
//					if (!CheckAdslIp(newIP))
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//
//					EnterCriticalSection(&g_csCurrentIP);
//					memcpy(g_cCurrentIP, newIP, strlen(newIP)+1);
//					LeaveCriticalSection(&g_csCurrentIP);
//					::SetEvent(g_hEventForRedialCompelet);
//				}
//				break;
//
//			case WM_MESSAGE_REDIAL_AGAIN:
//				{
//					while (PeekMessage(&msg, NULL, WM_MESSAGE_REDIAL_AGAIN, WM_MESSAGE_REDIAL_AGAIN, PM_REMOVE))
//					{
//						;
//					}
//					myprintf("come in to redial again");
//				//	char netling[] = "网络连接";
//
//					HWND hNetLingWnd = NULL;
//					do
//					{
//						hNetLingWnd = ::FindWindow(NULL, "网络连接");
//						if (hNetLingWnd != NULL)
//						{
//							::SendMessage(hNetLingWnd, WM_CLOSE, 0, 0);
//						}
//					}while(hNetLingWnd != NULL);// 这样处理会不会有问题
//
//					char oldIP[32] = {0};
//					if(!GetAdslIp(oldIP))
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_START, 0, 0);
//						break;
//					}
//					char* pAdslIsExist = NULL;
//					char *pCmdResultInfo = NULL;
//					do
//					{
//						if (pCmdResultInfo != NULL)
//						{
//							free(pCmdResultInfo);
//							pCmdResultInfo = NULL;
//						}
//						if (!Execmd("rasdial adsl /disconnect", &pCmdResultInfo))
//							continue;
//						pAdslIsExist = strstr(pCmdResultInfo, "没有连接");
//						if (pAdslIsExist == NULL)
//							pAdslIsExist = strstr(pCmdResultInfo, "No connections");
//					}while(pAdslIsExist == NULL);
//
//					if (pCmdResultInfo != NULL)
//					{
//						free(pCmdResultInfo);
//						pCmdResultInfo = NULL;
//					}
//
//					//char className[] = "连接 adsl";
//					HWND hWnd = ::FindWindow(NULL, "连接 adsl");
//					if (hWnd != NULL)
//					{
//						::SendMessage(hWnd, WM_CLOSE, 0, 0);
//					}
//
//					Sleep(1000 * 5);
//#if IS_TEST
//					myprintf("测试不进行真的拨号");
//#else
//					if (!Redial(msg.wParam, msg.lParam))
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//#endif
//
//					char newIP[32] = {0};
//
//#if IS_TEST
//					memcpy(newIP, REDAIL_IP, strlen(REDAIL_IP));
//#else
//					if (!GetAdslIp(newIP))
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//#endif
//
//#if IS_TEST
//					myprintf("测试不比较新老ip");	
//#else
//					if (0 == strcmp(newIP, oldIP))
//					{
//						cout << "Dial-up repeat, redial again" << endl;
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//#endif
//
//					if (!CheckAdslIp(newIP))
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//						break;
//					}
//
//					EnterCriticalSection(&g_csCurrentIP);
//					memcpy(g_cCurrentIP, newIP, strlen(newIP)+1);
//					LeaveCriticalSection(&g_csCurrentIP);
//					::SetEvent(g_hEventForRedialCompelet);
//				}
//				break;
//
//				case WM_MESSAGE_MODIFY_DNS:
//					{
//						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
//					}
//					break;
//
//			default:
//				break;
//			}
//		}
//	}
//	return 0;
//}

BOOL ExcCmd(const char* cmd, char** out)
{
	FILE* pipe = _popen(cmd, "r");
	if (!pipe)
		return FALSE;

	int nTotal = 0;
	int len = 0;
	char buffer[128];
	char* pData = (char*)malloc(512);
	ZeroMemory(pData, 512);
	int nReallocSize = 512;

	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe))
		{
			len = strlen(buffer) + 1;
			nTotal += len;
			if (nTotal > nReallocSize)
			{
				nReallocSize = MY_ALIGN((nTotal + 1), 8);
				pData = (char*)realloc(pData, nReallocSize);
			}
			strcat_s(pData, nReallocSize, buffer);
		}
	}

	_pclose(pipe);
	*out = pData;
	return TRUE;
}

BOOL CloseRasphone()
{
	char* pAdslExit = NULL;
	char* pCmdInfo = NULL;

	do
	{
		if (NULL != pCmdInfo)
		{
			free(pCmdInfo);
			pCmdInfo = NULL;
		}

		if (!ExcCmd("rasdial adsl /disconnect", &pCmdInfo))
			continue;
		pAdslExit = strstr(pCmdInfo, "没有连接");
		if (NULL == pAdslExit)
			pAdslExit = strstr(pCmdInfo, "No connections");
	} while (NULL == pAdslExit);

	if (NULL != pCmdInfo)
	{
		free(pCmdInfo);
		pCmdInfo = NULL;
	}

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		printf_s("创建系统进程映射失败 error = %d\n", GetLastError());
		return FALSE;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);

	BOOL bFind = Process32First(hProcessSnap, &pe32);
	while (bFind)
	{
		if (0 == strcmp(pe32.szExeFile, "rasphone.exe"))
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
		}
		bFind = Process32Next(hProcessSnap, &pe32);
	}

	CloseHandle(hProcessSnap);
	return TRUE;
}

BOOL GetAdslInfo(char* ip)
{
	PIP_ADAPTER_INFO pAdslAdapterInfo_ = NULL;
	PIP_ADAPTER_INFO pAdslAdapterInfo = (PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));

	unsigned long nInfoSize = 0;
	ULONG uLong = GetAdaptersInfo(pAdslAdapterInfo, &nInfoSize);
	if (ERROR_BUFFER_OVERFLOW == uLong)
	{
		free(pAdslAdapterInfo);
		pAdslAdapterInfo = (PIP_ADAPTER_INFO)malloc(nInfoSize);
		uLong = GetAdaptersInfo(pAdslAdapterInfo, &nInfoSize);
	}

	if (ERROR_SUCCESS == uLong)
	{
		pAdslAdapterInfo_ = pAdslAdapterInfo;
		while (pAdslAdapterInfo)
		{
			if (pAdslAdapterInfo->Type == MIB_IF_TYPE_PPP)
			{
				IP_ADDR_STRING* pAdslIpAddrString = &(pAdslAdapterInfo->IpAddressList);
				memcpy_s(ip, 16, pAdslIpAddrString->IpAddress.String, 16);
				if (pAdslAdapterInfo_)
				{
					free(pAdslAdapterInfo_);
					pAdslAdapterInfo_ = NULL;
				}
				return TRUE;
			}
			pAdslAdapterInfo = pAdslAdapterInfo->Next;
		}
	}

	if (pAdslAdapterInfo_)
	{
		free(pAdslAdapterInfo_);
		pAdslAdapterInfo_ = NULL;
	}

	return FALSE;
}

BOOL FindRasphone()
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		printf_s("创建系统进程映射失败 error = %d\n", GetLastError());
		return FALSE;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);

	BOOL bFind = Process32First(hProcessSnap, &pe32);
	while (bFind)
	{
		if (0 == strcmp(pe32.szExeFile, "rasphone.exe"))
		{
			CloseHandle(hProcessSnap);
			return TRUE;
		}
		bFind = Process32Next(hProcessSnap, &pe32);
	}

	CloseHandle(hProcessSnap);
	return FALSE;
}

BOOL DoAdsl()
{
	while (true)
	{
		CloseRasphone();
		WinExec("rasphone -d adsl", SW_NORMAL);

		Sleep(1000 * 4);
		HWND hAdsl = NULL;
		hAdsl = FindWindow(NULL, "连接 adsl");
		if (NULL == hAdsl)
			continue;

		HWND hLinkButton = NULL;
		hLinkButton = FindWindowEx(hAdsl, NULL, "Button", "连接(&C)");
		if (NULL == hLinkButton)
			continue;
		SendMessage(hLinkButton, BM_CLICK, 0, 0);

		BOOL bContinue = FALSE;
		DWORD dwWaitTime = 0;
		HWND hLinking = NULL;
		do
		{
			Sleep(1000 * 2);
			dwWaitTime += 2;
			hLinking = FindWindow(NULL, "正在连接 adsl...");
			if (NULL == hLinking)
			{
				if (FindRasphone())
				{
					bContinue = TRUE;
					break;
				}
			}
			else
			{
				HWND hErrorWnd = NULL;
				hErrorWnd = FindWindow(NULL, "连接到 adsl 时出错");
				if (NULL != hErrorWnd)
				{
					bContinue = TRUE;
					break;
				}
			}
		} while (NULL != hLinking && dwWaitTime < 90);

		if (bContinue)
			continue;

		return TRUE;
	}

	return TRUE;
}

int IpStr2Hex(const char* Ip)
{
	int a, b, c, d;
	sscanf_s(Ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	
	a <<= 24;
	b <<= 16;
	c <<= 8;
	a = a + b + c + d;

	return a;
}

void SelectSysListControl(HWND hwnd)
{
	int count, i;
	char item[512] = { 0 }, subitem[512] = { 0 };

	LVITEM lvi, *_lvi;
	char *_item, *_subitem;
	DWORD pid;
	HANDLE process;

	count = (int)SendMessage(hwnd, LVM_GETITEMCOUNT, 0, 0);

	GetWindowThreadProcessId(hwnd, &pid);
	process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ |
		PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);

	_lvi = (LVITEM*)VirtualAllocEx(process, NULL, sizeof(LVITEM),
		MEM_COMMIT, PAGE_READWRITE);
	_item = (char*)VirtualAllocEx(process, NULL, 512, MEM_COMMIT,
		PAGE_READWRITE);
	_subitem = (char*)VirtualAllocEx(process, NULL, 512, MEM_COMMIT,
		PAGE_READWRITE);

	lvi.cchTextMax = 512;

	int nTcpIpIndex = 0;
	for (i = 0; i<count; i++) {
		lvi.iSubItem = 0;
		lvi.pszText = _item;
		WriteProcessMemory(process, _lvi, &lvi, sizeof(LVITEM), NULL);
		SendMessage(hwnd, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)_lvi);

		if (strcmp(item, "Internet 协议 (TCP/IP)") == 0)
		{
			nTcpIpIndex = i;
			break;
		}
	}

	lvi.mask = LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	WriteProcessMemory(process, _lvi, &lvi, sizeof(LVITEM), NULL);
	SendMessage(hwnd, LVM_SETITEMSTATE, (WPARAM)nTcpIpIndex, (LPARAM)_lvi);

	VirtualFreeEx(process, _lvi, 0, MEM_RELEASE);
	VirtualFreeEx(process, _item, 0, MEM_RELEASE);
	VirtualFreeEx(process, _subitem, 0, MEM_RELEASE);
}

HWND FindWindowByName(const char* szText)
{
	int nWaitTime = 0;
	HWND hwnd = NULL;
	do
	{
		Sleep(1000 * 2);
		nWaitTime += 2;
		hwnd = FindWindow(NULL, szText);
	} while (NULL == hwnd && nWaitTime < 30);

	return hwnd;
}

BOOL SwitchSysTabCtrl(HWND hwnd)
{
	HWND hTabCtrl = FindWindowEx(hwnd, NULL, "SysTabControl32", NULL);
	if (NULL == hTabCtrl)
		return FALSE;

	int nTabCtrlCount = TabCtrl_GetItemCount(hTabCtrl);
	if (0 == nTabCtrlCount)
		return FALSE;;

	for (int i = 0; i < nTabCtrlCount; i++)
		TabCtrl_SetCurFocus(hTabCtrl, i);

	return TRUE;
}

BOOL ModifyDnsServer(const char* dns1, const char* dns2)
{
	while (true)
	{
		CloseRasphone();
		WinExec("rasphone -d adsl", SW_NORMAL);

		Sleep(1000 * 4);
		HWND hAdsl = NULL;
		hAdsl = FindWindow(NULL, "连接 adsl");
		if (NULL == hAdsl)
			continue;

		HWND hProButton = FindWindowEx(hAdsl, NULL, "Button", "属性(&O)");
		if (NULL == hProButton)
			goto error;

		PostMessage(hProButton, BM_CLICK, 0, 0);

		HWND hAdsl_pro = NULL;
		hAdsl_pro = FindWindowByName("adsl 属性");
		if (NULL == hAdsl_pro)
			goto error;

		if (!SwitchSysTabCtrl(hAdsl_pro))
			goto error;

		HWND hNet = FindWindowEx(hAdsl_pro, NULL, "#32770", "网络");
		if (NULL == hNet)
			goto error;

		HWND hListCtrl = FindWindowEx(hNet, NULL, "SysListView32", NULL);
		if (NULL == hListCtrl)
			goto error;

		SelectSysListControl(hListCtrl);

		HWND hSetButton = FindWindowEx(hNet, NULL, "Button", "属性(&R)");
		if (NULL == hSetButton)
			goto error;
		PostMessage(hSetButton, BM_CLICK, 0, 0);

		HWND hTcpIp = NULL;
		hTcpIp = FindWindowByName("Internet 协议 (TCP/IP) 属性");
		if (NULL == hTcpIp)
			goto error;

		if (!SwitchSysTabCtrl(hTcpIp))
			goto error;

		HWND hRoutineCard = FindWindowEx(hTcpIp, NULL, "#32770", "常规");
		if (NULL == hRoutineCard)
			goto error;

		HWND hDnsButton = FindWindowEx(hRoutineCard, NULL, "Button", "使用下面的 DNS 服务器地址(&E):");
		if (NULL == hDnsButton)
			goto error;

		SendMessage(hDnsButton, BM_CLICK, 0, 0);
		Sleep(1000 * 1);

		HWND ip = NULL;
		if (NULL != dns1)
		{
			ip = FindWindowEx(hRoutineCard, NULL, "static", "首选 DNS 服务器(&P):");
			if (NULL == ip)
				goto error;
			ip = FindWindowEx(hRoutineCard, ip, "SysIPAddress32", NULL);
			if (NULL == ip)
				goto error;
			SendMessage(ip, IPM_SETADDRESS, 0, (LPARAM)IpStr2Hex(dns1));
			Sleep(1000);
		}

		if (NULL != dns2)
		{
			ip = FindWindowEx(hRoutineCard, ip, "static", "备用 DNS 服务器(&A):");
			if (NULL == ip)
				goto error;
			ip = FindWindowEx(hRoutineCard, ip, "SysIPAddress32", NULL);
			if (NULL == ip)
				goto error;
			SendMessage(ip, IPM_SETADDRESS, 0, (LPARAM)IpStr2Hex(dns2));
			Sleep(1000);
		}

		HWND hSureOnTcpIp = FindWindowEx(hTcpIp, NULL, "Button", "确定");
		if (NULL == hSureOnTcpIp)
			goto error;
		SendMessage(hSureOnTcpIp, BM_CLICK, 0, 0);
		while (NULL != hTcpIp)
		{
			Sleep(1000);
			hTcpIp = FindWindow(NULL, "Internet 协议 (TCP/IP) 属性");
		}

		HWND hSureOnAdslPro = FindWindowEx(hAdsl_pro, NULL, "Button", "确定");
		if (NULL == hSureOnAdslPro)
			goto error;
		SendMessage(hSureOnAdslPro, BM_CLICK, 0, 0);
		while (NULL != hAdsl_pro)
		{
			Sleep(1000);
			hAdsl_pro = FindWindow(NULL, "adsl 属性");
		}

		printf_s("修改dns成功\n");

		return TRUE;
	}

error:
	printf("修改dns失败\n");
	return FALSE;
}

unsigned int _stdcall np_redial_thread_function(LPVOID pVoid)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	::SetEvent(g_hEventOfRedialThread);

	while (true)
	{
		if (GetMessage(&msg, 0, 0, 0))
		{
			switch (msg.message)
			{
			case WM_MESSAGE_REDIAL_START:
				{
					while (PeekMessage(&msg, NULL, WM_MESSAGE_REDIAL_START, WM_MESSAGE_REDIAL_START, PM_REMOVE))
					{
						;
					}
#if IS_TEST
					myprintf("测试不真的拨号");
#else
					DoAdsl();
#endif
					

					char newIP[16] = {0};
#if IS_TEST
					memcpy(newIP, REDAIL_IP, strlen(REDAIL_IP));
#else
					if (!GetAdslInfo(newIP))
					{
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						break;
					}
#endif

					if (!CheckAdslIp(newIP))
					{
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						break;
					}

					EnterCriticalSection(&g_csCurrentIP);
					memcpy(g_cCurrentIP, newIP, strlen(newIP)+1);
					LeaveCriticalSection(&g_csCurrentIP);
					::SetEvent(g_hEventForRedialCompelet);
				}
				break;

			case WM_MESSAGE_REDIAL_AGAIN:
				{
					printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n\r\n\r\n进入拨号线程\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					while (PeekMessage(&msg, NULL, WM_MESSAGE_REDIAL_AGAIN, WM_MESSAGE_REDIAL_AGAIN, PM_REMOVE))
					{
						;
					}
					printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n\r\n\r\n\r\n拨号线程PM_REMOVE完成\r\n\r\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					myprintf("come in to redial again");
				//	char netling[] = "网络连接";

					char oldIP[16] = {0};
					if(!GetAdslInfo(oldIP))
						memcpy(oldIP, "0.0.0.0", strlen("0.0.0.0") + 1);

#if IS_TEST
					myprintf("测试不真的拨号");
#else
					DoAdsl();
#endif

					char newIP[16] = {0};
#if IS_TEST
					memcpy(newIP, REDAIL_IP, strlen(REDAIL_IP));
#else
					if (!GetAdslInfo(newIP))
					{
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						break;
					}
#endif

#if IS_TEST
					myprintf("测试不比较新老ip");	
#else
					if (0 == strcmp(newIP, oldIP))
					{
						cout << "Dial-up repeat, redial again" << endl;
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						break;
					}
#endif

					if (!CheckAdslIp(newIP))
					{
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
						break;
					}

					EnterCriticalSection(&g_csCurrentIP);
					memcpy(g_cCurrentIP, newIP, strlen(newIP)+1);
					LeaveCriticalSection(&g_csCurrentIP);
					::SetEvent(g_hEventForRedialCompelet);
				}
				break;

				case WM_MESSAGE_MODIFY_DNS:
					{
						char* pData = (char*)msg.wParam;
						char* pdns1 = strtok(pData, ",");
						char* pdns2 = strtok(NULL, ",");
						ModifyDnsServer(pdns1, pdns2);
						delete pData;
						PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
					}
					break;

			default:
				break;
			}
		}
	}
	return 0;
}