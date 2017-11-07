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
		printf("��ȡproxyִ�г���λ��ʧ�� error = %d\n", GetLastError());
		return FALSE;
	}

	printf("CurrentProxy_path : %s\n", CurrentPath);
	char FileNmae[_MAX_FNAME] = {0};
	_splitpath_s(CurrentPath, NULL, 0, NULL, 0, FileNmae, _MAX_FNAME, NULL, 0);

	char DeskPath[MAX_PATH] = {0};
	if (!SHGetSpecialFolderPath(NULL, DeskPath, CSIDL_DESKTOPDIRECTORY, 0))
	{
		printf("��ȡ����·��ʧ�� error = %d\n", GetLastError());
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
		printf("�����ļ�ʧ�� error = %d\n", GetLastError());
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
		printf("�Ѿ��������еĴ���ʵ��, �벻Ҫ�ظ���������\n");
		Sleep(1000 * 10);
		return 0;
	}

	hTheOneInstance = ::CreateEvent(NULL, FALSE, FALSE, NP_THE_ONE_INSTANCE);
	if (NULL == hTheOneInstance)
	{
		printf("����ʧ��%d\n", GetLastError());
		return 0;
	}

	cout << "��ǰ�汾�ţ�" << CURRENT_VERSION << endl;
	if (!InitSock2())
	{
		return -1;
	}

	GetHostId(&pHostId);
	if (NULL == pHostId || strcmp(pHostId, "") == 0)
	{
		printf("��ȡclient_id.txt����ȷ���Ƿ����ô��ļ�\n");
		Sleep(1000*20);
		return -1;
	}

	//_beginthreadex(NULL, 0, socketioclient, NULL, 0, &nThreadSC);
	//getchar();

	set_starting_items();
	if (!reg_proxy_path())
		printf("����ע��ʧ��\n");

	CheckPWatch();

	InitializeCriticalSection(&g_csCurrentIP);
	InitializeCriticalSection(&g_csCurrentPort);
	InitializeCriticalSection(&cs_Host);
	db.clear();
	// ��ʼ����صĹ�������
	InitializeCriticalSection(&g_cs);
	g_vSocket.clear();// ��������socket��Դ

	g_hEventForDownloadCompelet = ::CreateEvent(NULL, FALSE, FALSE, NULL);// �������֪ͨ
	g_hEventOfDownloadThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// �����߳�����֪ͨ
	g_hEventForReportCompelet = ::CreateEvent(NULL, FALSE, FALSE, NULL);// �ϱ����֪ͨ
	g_hEventOfReportThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// �ϱ��߳�����֪ͨ
	g_hEventOf5001And6086Thread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// ���߳�����֪ͨ
	g_hEventForRedialCompelet = ::CreateEvent(NULL, FALSE, FALSE, NULL);// �������֪ͨ
	g_hEventOfRedialThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);// �����߳�����֪ͨ
	hEventOfWorkerExit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventOf6086Exit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventOfNetCheckExit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventOfTimerStart = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE hMessageThread = (HANDLE)_beginthreadex(NULL, 0, np_message_thread_function, NULL, 0, &g_nMessageThreadNo);
	if (NULL == hMessageThread)
	{
		cout << "��־�߳�����ʧ��" << endl;
		return -1;
	}
	Sleep(1000);

	HANDLE hDownloadThread = (HANDLE)_beginthreadex(NULL, 0, np_downlaod_thread_function, NULL, 0, &g_nDownloadThreadNo);
	if (NULL == hDownloadThread)
	{
		cout << "�����߳�����ʧ��" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOfDownloadThread, INFINITE);

	HANDLE hReportInfoThread = (HANDLE)_beginthreadex(NULL, 0, np_report_thread_function, NULL, 0, &g_nReportInfoThreadNo);
	if (NULL == hReportInfoThread)
	{
		cout << "�ϱ��߳�����ʧ��" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOfReportThread, INFINITE);

	HANDLE h5001And6086Thread = (HANDLE)_beginthreadex(NULL, 0, np_5001And6086_thread_function, NULL, 0, &g_n5001And6086ThreadNo);
	if (NULL == h5001And6086Thread)
	{
		cout << "���߳�����ʧ��" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOf5001And6086Thread, INFINITE);

	HANDLE hRedialThread = (HANDLE)_beginthreadex(NULL, 0, np_redial_thread_function, NULL, 0, &g_nRedialThreadNo);
	if (NULL == hRedialThread)
	{
		cout << "�����߳�����ʧ��" << endl;
		return -1;
	}
	::WaitForSingleObject(g_hEventOfRedialThread, INFINITE);
	PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_START, 0, 0);// ��ʼ����

	WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);// �������
	Sleep(1000*3);

	if (!GetSocketIoServerIp())
		url_b = SOCKET_IO_URL;
	else
		reg_get_server_ip(url_b);
	url_a = SOCKET_IO_URL;

	AddModRiskFileType();
	AddWhiteList();
	if (!InstallChrome())
		cout << "��װchrome����չʧ��" << endl;

	PostThreadMessage(g_nDownloadThreadNo, WM_MESSAGE_DOWNLOAD_START, 0, 0);// ��ʼ�������
	cout << "�ز����" << endl;
	WaitForSingleObject(g_hEventForDownloadCompelet, INFINITE);// ����������
	if (!isOnWorking())
	{
		char pwatch_path[MAX_PATH];
		sprintf_s(pwatch_path, "%s\\pwatch-v%s.exe", WARD_FOLDER, g_pwatchversion);
		ShellExecute(0, "open", pwatch_path, NULL, NULL, SW_SHOWNORMAL);
	}
	
	GetHostInfo_start();
//	AddWhiteList();
	PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);// ��ʼ�������߳�
	cout << "����������" << endl;

	// ������ʱ��
	HANDLE hOnTimerThread = (HANDLE)_beginthreadex(NULL, 0, np_ontimer_thread_function, NULL, 0, NULL);
	if (NULL == hOnTimerThread)
	{
		cout << "��ʱ�ϱ���汾����߳�����ʧ��" << endl;
		return -1;
	}

	HANDLE hListCtx = (HANDLE)_beginthreadex(NULL, 0, ListCtx, NULL, 0, NULL);
	if (NULL != hListCtx)
		CloseHandle(hListCtx);

	HANDLE hSocketIoClient = (HANDLE)_beginthreadex(NULL, 0, socketioclient, NULL, 0, &nThreadSC);
	WaitForSingleObject(hSocketIoClient, INFINITE);
	RestartProxy();

	// �رճ���ʱ��Դ���ͷ���
	return 0;
}