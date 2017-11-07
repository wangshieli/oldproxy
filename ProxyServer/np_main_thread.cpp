#include "np_golbal_header.h"
#include "np_helper_function.h"
#include "np_5001And6086_thread.h"
#include "np_redial_thread.h"
#include "np_reportinformation_thread.h"
#include "np_download_thread.h"
#include "np_ontimer_thread.h"
#include "np_message_thread.h"
#include "np_heartbeat_timer.h"
#include "np_tool_function.h"
#include "np_2_work_thread.h"
#include "np_usernamea_password.h"
#include "np_ward.h"
#include "aes.h"
#include "np_DNS.h"

//#pragma data_seg("Proxy_Exe_Shared_Memory")
//volatile LONG g_lApplicationInstances = 0;
//#pragma data_seg()
//
//#pragma comment(linker, "/Section:Proxy_Exe_Shared_Memory,RWS") 

extern unsigned int _stdcall socketioclient(LPVOID);

extern unsigned int _stdcall ListCtx(LPVOID pVoid);

HANDLE hTheOneInstance = NULL;

BOOL MoveProxy()
{
	char CurrentPath[MAX_PATH] = {0};
	if (!GetModuleFileName(NULL, CurrentPath, MAX_PATH))
	{
		printf("获取proxy执行程序位置失败 error = %d\n", GetLastError());
		return FALSE;
	}

	printf("CurrentProxy_path : %s\n", CurrentPath);
	char FileNmae[_MAX_FNAME] = {0};
	_splitpath_s(CurrentPath, NULL, 0, NULL, 0, FileNmae, _MAX_FNAME, NULL, 0);

	char DeskPath[MAX_PATH] = {0};
	if (!SHGetSpecialFolderPath(NULL, DeskPath, CSIDL_DESKTOPDIRECTORY, 0))
	{
		printf("获取桌面路径失败 error = %d\n", GetLastError());
		return FALSE;
	}
	printf("DeskPath : %s\n", DeskPath);

	char DeskProxyPath[MAX_PATH] = {0};
	_makepath_s(DeskProxyPath, NULL, DeskPath, FileNmae, "exe");
	printf("DeskProxyPath : %s\n", DeskProxyPath);

	if (strcmp(DeskProxyPath, CurrentPath) == 0)
		return TRUE;

	if (!CopyFile(CurrentPath, DeskProxyPath, FALSE))
	{
		printf("复制文件失败 error = %d\n", GetLastError());
		return FALSE;
	}

	ShellExecute(0, "open", DeskProxyPath, NULL, NULL, SW_SHOWNORMAL);
	ExitProcess(0);

	return TRUE;
}

int main()
{
	//_beginthreadex(NULL, 0, socketioclient, NULL, 0, &nThreadSC);
	//getchar();
	//Sleep(1000 * 3);
	//MoveProxy();
	hTheOneInstance = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, NP_THE_ONE_INSTANCE);
	if (NULL != hTheOneInstance)
	{
		printf("已经有在运行的代理实例, 请不要重复开启代理\n");
		Sleep(1000 * 10);
		return 0;
	}

	hTheOneInstance = ::CreateEvent(NULL, FALSE, FALSE, NP_THE_ONE_INSTANCE);
	if (NULL == hTheOneInstance)
	{
		printf("启动失败%d\n", GetLastError());
		return 0;
	}

	cout << "当前版本号：" << CURRENT_VERSION << endl;
	if (!InitSock2())
	{
		return -1;
	}

	GetHostId(&pHostId);
	if (NULL == pHostId || strcmp(pHostId, "") == 0)
	{
		printf("读取client_id.txt错误，确定是否配置此文件\n");
		Sleep(1000*20);
		return -1;
	}

	//_beginthreadex(NULL, 0, socketioclient, NULL, 0, &nThreadSC);
	//getchar();

	set_starting_items();
	if (!reg_proxy_path())
		printf("代理注册失败\n");

	CheckPWatch();

	InitializeCriticalSection(&g_csCurrentIP);
	InitializeCriticalSection(&g_csCurrentPort);
	InitializeCriticalSection(&cs_Host);
	db.clear();
	// 初始化相关的管理容器
	InitializeCriticalSection(&g_cs);
	g_vSocket.clear();// 用来保存socket资源

	g_hEventForDownloadCompelet = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 下载完成通知
	g_hEventOfDownloadThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 下载线程启动通知
	g_hEventForReportCompelet = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 上报完成通知
	g_hEventOfReportThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 上报线程启动通知
	g_hEventOf5001And6086Thread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 主线程启动通知
	g_hEventForRedialCompelet = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 拨号完成通知
	g_hEventOfRedialThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// 拨号线程启动通知
	hEventOfWorkerExit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventOf6086Exit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventOfNetCheckExit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventOfTimerStart = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE hMessageThread = (HANDLE)_beginthreadex(NULL, 0, np_message_thread_function, NULL, 0, &g_nMessageThreadNo);
	if (NULL == hMessageThread)
	{
		cout << "日志线程启动失败" << endl;
		return -1;
	}
	Sleep(1000);

	HANDLE hDownloadThread = (HANDLE)_beginthreadex(NULL, 0, np_downlaod_thread_function, NULL, 0, &g_nDownloadThreadNo);
	if (NULL == hDownloadThread)
	{
		cout << "下载线程启动失败" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOfDownloadThread, INFINITE);

	HANDLE hReportInfoThread = (HANDLE)_beginthreadex(NULL, 0, np_report_thread_function, NULL, 0, &g_nReportInfoThreadNo);
	if (NULL == hReportInfoThread)
	{
		cout << "上报线程启动失败" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOfReportThread, INFINITE);

	HANDLE h5001And6086Thread = (HANDLE)_beginthreadex(NULL, 0, np_5001And6086_thread_function, NULL, 0, &g_n5001And6086ThreadNo);
	if (NULL == h5001And6086Thread)
	{
		cout << "主线程启动失败" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOf5001And6086Thread, INFINITE);

	HANDLE hRedialThread = (HANDLE)_beginthreadex(NULL, 0, np_redial_thread_function, NULL, 0, &g_nRedialThreadNo);
	if (NULL == hRedialThread)
	{
		cout << "拨号线程启动失败" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOfRedialThread, INFINITE);
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_START, 0, 0);// 开始拨号

	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);// 拨号完成
	Sleep(1000*3);

	if (!GetSocketIoServerIp())
		url_b = SOCKET_IO_URL;
	else
		reg_get_server_ip(url_b);
	url_a = SOCKET_IO_URL;

	AddModRiskFileType();
	AddWhiteList();
	if (!InstallChrome())
		cout << "安装chrome与扩展失败" << endl;

	PostThreadMessage(g_nDownloadThreadNo, WM_MESSAGE_DOWNLOAD_START, 0, 0);// 开始检测下载
	cout << "重拨完成" << endl;
	WaitForSingleObject(g_hEventForDownloadCompelet, INFINITE);// 检测下载完成
	if (!isOnWorking())
	{
		char pwatch_path[MAX_PATH];
		sprintf_s(pwatch_path, "%s\\pwatch-v%s.exe", WARD_FOLDER, g_pwatchversion);
		ShellExecute(0, "open", pwatch_path, NULL, NULL, SW_SHOWNORMAL);
	}
	
	GetHostInfo_start();
//	AddWhiteList();
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);// 开始主工作线程
	cout << "检查下载完成" << endl;

	// 启动定时器
	HANDLE hOnTimerThread = (HANDLE)_beginthreadex(NULL, 0, np_ontimer_thread_function, NULL, 0, NULL);
	if (NULL == hOnTimerThread)
	{
		cout << "定时上报与版本检查线程启动失败" << endl;
		return -1;
	}

	HANDLE hListCtx = (HANDLE)_beginthreadex(NULL, 0, ListCtx, NULL, 0, NULL);
	if (NULL != hListCtx)
		CloseHandle(hListCtx);

	HANDLE hSocketIoClient = (HANDLE)_beginthreadex(NULL, 0, socketioclient, NULL, 0, &nThreadSC);
	WaitForSingleObject(hSocketIoClient, INFINITE);
	RestartProxy();

	// 关闭程序时资源就释放了
	return 0;
}