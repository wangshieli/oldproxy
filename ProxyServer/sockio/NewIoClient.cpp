#include "..\np_golbal_header.h"
#include <functional>
#include <mutex>
#include <condition_variable>
//#include <process.h>

#include "../cJSON.h"
#include "sio_client.h"

#include "../np_helper_function.h"
#include "../np_tool_function.h"



using namespace std;
using namespace sio;

#define BUFSIZE 512

#define MSG_DISPOSE						WM_USER + 51
#define MSG_RESSTART_COMPUTER			WM_USER + 52
#define MSG_RESSTART_PROXY				WM_USER + 53
#define MSG_SEND_2_SERVER				WM_USER + 54
#define MSG_DNS_CHANGE					WM_USER + 55
#define MSG_CHROME_REINSTALL			WM_USER + 56
#define MSG_EXTENSION_REINSTALL			WM_USER + 57

unsigned int _stdcall socketioclient(LPVOID);

mutex _lock;
condition_variable_any _cond;
bool connect_finish = false;

class connection_listener
{
	sio::client &handler;
public:
	connection_listener(sio::client &h) :handler(h)
	{

	}
	//~connection_listener();

public:
	void on_connected()
	{
		_lock.lock();
		_cond.notify_all();
		connect_finish = true;
		_lock.unlock();
	}

	void on_close(client::close_reason const &reason)
	{
		cout << "sio closed" << endl;
//		exit(0);
	}

	void on_fail()
	{
		cout << "sio failed" << endl;
//		exit(0);
	}
};

struct Json_Data
{
public:
	Json_Data()
	{
		pAction = NULL;
	//	pData = NULL;
		jData = NULL;
		pOld = NULL;
	}

	~Json_Data()
	{
		pAction = NULL;
	//	pData = NULL;
		jData = NULL;
		pOld = NULL;
	}
public:
	char* pAction;
	//char* pData;
	cJSON* jData;
	char* pOld;
};

//BOOL IsNull(const char* pItem)
//{
//	if (NULL == pItem || strcmp(pItem, "") == 0)
//		return TRUE;
//
//	return FALSE;
//}
//
//BOOL CheckJsonData(Json_Data& jd)
//{
//	if (IsNull(jd.pAction))
//		return FALSE;
//
//	return TRUE;
//}

cJSON* ParseJsonData(const char* _pInfo, Json_Data& jd)
{
	cJSON* root = NULL;
	root = cJSON_Parse(_pInfo);
	if (NULL == root)
		return NULL;

	cJSON* action = cJSON_GetObjectItem(root, "action");
	cJSON* data = cJSON_GetObjectItem(root, "data");

	if (NULL == action)
	{
		cJSON_Delete(root);
		return NULL;
	}

	jd.pAction = action->valuestring;
	if (NULL != data)
		jd.jData = data;
	jd.pOld = (char*)_pInfo;

	return root;
}

char* PackCallBackJsonData(Json_Data& jd, const char* pValue, BOOL bError = FALSE)
{
	cJSON* root = NULL;
	root = cJSON_CreateObject();
	if (NULL == root)
		return NULL;

	char CallBackAction[64];
	sprintf_s(CallBackAction, "%s__callback", jd.pAction);
	cJSON_AddStringToObject(root, "action", CallBackAction);

	cJSON* subroot = NULL;
	subroot = cJSON_CreateObject();
	if (NULL == subroot)
	{
		cJSON_Delete(root);
		return NULL;
	}

	cJSON_AddStringToObject(subroot, "value", pValue);
	if (bError)
		cJSON_AddStringToObject(subroot, "oldData", jd.pOld);

	cJSON_AddItemToObject(root, "data", subroot);
	
	char* pJsonData = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return pJsonData;
}

BOOL MySystemShutdown()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return(FALSE);

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
		&tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
		(PTOKEN_PRIVILEGES)NULL, 0);

	if (GetLastError() != ERROR_SUCCESS)
		return FALSE;

	//if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,
	//	SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
	//	SHTDN_REASON_MINOR_UPGRADE |
	//	SHTDN_REASON_FLAG_PLANNED))
	//	return FALSE;

	if (!ExitWindowsEx(EWX_REBOOT, 0))
		return FALSE;

	return TRUE;
}

BOOL RestartProxy()
{
	/*BOOL bFind = FALSE;
	HKEY hKey;
	char proxy_path[MAX_PATH];
	DWORD path_len = MAX_PATH;

	if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Proxy",
		0, KEY_ALL_ACCESS, &hKey))
		return FALSE;

	if (ERROR_SUCCESS != RegQueryValueEx(hKey, "proxy_path", NULL, NULL,
		(LPBYTE)proxy_path, &path_len))
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	char proxy_name[_MAX_FNAME];
	char proxy_suf[_MAX_FNAME];
	_splitpath_s(proxy_path, NULL, 0, NULL, 0, proxy_name, _MAX_FNAME, proxy_suf, _MAX_FNAME);
	char proxy_full_name[_MAX_FNAME];
	sprintf_s(proxy_full_name, "%s%s", proxy_name, proxy_suf);

	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (NULL == hProcessSnap)
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	PROCESSENTRY32 pe32;
	ZeroMemory(&pe32, sizeof(pe32));
	pe32.dwSize = sizeof(pe32);

	BOOL bMode = ::Process32First(hProcessSnap, &pe32);
	while (bMode)
	{
		if (0 == strcmp(proxy_full_name, pe32.szExeFile))
		{
			HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			TerminateProcess(ProcessHandle, 0);
		}
		bMode = ::Process32Next(hProcessSnap, &pe32);
	}
	RegCloseKey(hKey);
	CloseHandle(hProcessSnap);

	Sleep(1000 * 3);
	ShellExecute(0, "open", proxy_path, NULL, NULL, SW_SHOWNORMAL);*/

	HKEY hKey;
	char proxy_path[MAX_PATH];
	DWORD path_len = MAX_PATH;

	if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Proxy", 
		0, KEY_ALL_ACCESS, &hKey))
		return FALSE;

	if (ERROR_SUCCESS != RegQueryValueEx(hKey, "proxy_path", NULL, NULL, 
		(LPBYTE)proxy_path, &path_len))
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	RegCloseKey(hKey);

	if (NULL != hTheOneInstance)
	{
		CloseHandle(hTheOneInstance);
	}
	ShellExecute(0, "open", proxy_path, NULL, NULL, SW_SHOWNORMAL);
//	exit(0);
	ExitProcess(0);
	return TRUE;
}

void DisposeMap2String(std::map<std::string, message::ptr>& mp, char* buf)
{
	sprintf(buf + strlen(buf), "%s", "{");
	std::map<std::string, message::ptr>::iterator it = mp.begin();
	for (; it != mp.end(); it++)
	{
		if (it->second->get_flag() == message::flag::flag_string)
		{
			if (it == mp.begin())
				sprintf(buf + strlen(buf), "\"%s\":\"%s\"", it->first.c_str(), it->second->get_string().c_str());
			else
				sprintf(buf + strlen(buf), ",\"%s\":\"%s\"", it->first.c_str(), it->second->get_string().c_str());
		}
		else if (it->second->get_flag() == message::flag::flag_object)
		{
			if (it == mp.begin())
				sprintf(buf + strlen(buf), "\"%s\":", it->first.c_str());
			else
				sprintf(buf + strlen(buf), ",\"%s\":", it->first.c_str());
			DisposeMap2String(it->second->get_map(), buf);
		}
	}

	sprintf(buf + strlen(buf), "%s", "}");
}

unsigned int nThreadSC = 0;

void bind_events(socket::ptr &socket)
{
	socket->on("name", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		cout << "监听到的事件:" << name << endl;
		cout << "接收到的数据:" << data->get_string() << endl;
		cout << "事件 " << name << " 处理完成" << endl;
		_lock.unlock();
	}));

	socket->on("data", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		cout << "监听到的事件:" << name << endl;

		cout <<  (data->get_map()).size() << endl;;

		char* pJsonData = (char*)malloc(BUFSIZE);
		memset(pJsonData, 0x00, BUFSIZE);
		DisposeMap2String(data->get_map(), pJsonData);
		printf("%s\n", pJsonData);
		Json_Data jd;
		cJSON* root = ParseJsonData(pJsonData, jd);
		if (NULL == root)
		{
			cout << "接收到的数据：" << pJsonData << " 格式错误" << endl;
			jd.pAction = "error";
			jd.pOld = pJsonData;
			char* pBackJson = PackCallBackJsonData(jd, "order error", TRUE);
			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);

			free(pJsonData);
			_lock.unlock();
			return;
		}
		//if (!CheckJsonData(jd))
		//{
		//	cout << "接收到的数据：" << pJsonData << " 中存在非法数据" << endl;
		//	goto error;
		//}

		if (strcmp(jd.pAction, "host_restart") == 0)
		{
			char* pBackJson = PackCallBackJsonData(jd, "host_restart ok");
			if (NULL == pBackJson)
			{
				cout << "组装 restart 命令 返回数据时失败" << endl;
				goto error;
			}

			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
			PostThreadMessage(nThreadSC, MSG_RESSTART_COMPUTER, NULL, NULL);
		}
		else if (strcmp(jd.pAction, "proxy_restart") == 0)
		{
			char* pBackJson = PackCallBackJsonData(jd, "proxy_restart ok");
			if (NULL == pBackJson)
			{
				cout << "组装 resproxy 命令 返回数据时失败" << endl;
				goto error;
			}
			// 发送数据给服务器
			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
			PostThreadMessage(nThreadSC, MSG_RESSTART_PROXY, NULL, NULL);
		}else if (strcmp(jd.pAction, "dns_update") == 0)
		{
			// netsh interface ip set dns "adsl" static 192.168.2.1
			char* pBackJson = NULL;
			if (NULL == jd.jData)	
			{
				pBackJson = PackCallBackJsonData(jd, "dns = null");
				PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
				cout << "hello" << endl;
				goto error;
			}
			
			cJSON* jDns = cJSON_GetObjectItem(jd.jData, "dns");
			if (NULL == jDns)
			{
				pBackJson = PackCallBackJsonData(jd, "dns = null");
				PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
				cout << "hello" << endl;
				goto error;
			}

			pBackJson = PackCallBackJsonData(jd, "dns_updata ok");
			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);

			char * pData = new char[MAX_PATH];
			ZeroMemory(pData, MAX_PATH);
			char* pDataSrc = jDns->valuestring;
			memcpy(pData, pDataSrc, strlen(pDataSrc));
			PostThreadMessage(nThreadSC, MSG_DNS_CHANGE, (WPARAM)pData, NULL);

			/*char* pData = cJSON_GetObjectItem(jd.jData, "dns")->valuestring;
			char* p1 = strtok(pData, ",");
			if (NULL != p1)
			{
				char cmdc[MAX_PATH] = {0};
				sprintf_s(cmdc, "netsh interface ip set dns \"adsl\" static %s", p1);
				char* pInfo = NULL;
				Execmd(cmdc, &pInfo);
				printf("%s\n", pInfo);
				free(pInfo);
			}
			p1 = strtok(NULL, ",");
			if (NULL != p1)
			{
				char cmdc[MAX_PATH] = {0};
				sprintf_s(cmdc, "netsh interface ip add dns \"adsl\" %s ", p1);
				char* pInfo = NULL;
				Execmd(cmdc, &pInfo);
				printf("%s\n", pInfo);
				free(pInfo);
			}*/
		}else if(strcmp(jd.pAction, "chrome_reinstall") == 0)
		{
			char* pBackJson = PackCallBackJsonData(jd, "chrome_reinstall ok");
			if (NULL == pBackJson)
			{
				cout << "组装 chrome_reinstall 命令 返回数据时失败" << endl;
				goto error;
			}
			// 发送数据给服务器
			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
			PostThreadMessage(nThreadSC, MSG_CHROME_REINSTALL, NULL, NULL);
		}else if (strcmp(jd.pAction, "extension_reinstall") == 0)
		{
			char* pBackJson = PackCallBackJsonData(jd, "extension_reinstall ok");
			if (NULL == pBackJson)
			{
				cout << "组装 extension_reinstall 命令 返回数据时失败" << endl;
				goto error;
			}
			// 发送数据给服务器
			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
			PostThreadMessage(nThreadSC, MSG_EXTENSION_REINSTALL, NULL, NULL);
		}
		else
		{
			char* pBackJson = PackCallBackJsonData(jd, "Invalid Command");
			PostThreadMessage(nThreadSC, MSG_SEND_2_SERVER, (WPARAM)pBackJson, NULL);
		}
error:
		free(pJsonData);
		cJSON_Delete(root);
		pJsonData = NULL;
		//
		cout << "事件 " << name << " 处理完成" << endl;
		_lock.unlock();
	}));
}

BOOL UpdateDns(char* dns1);

// SIO_TLS
std::string url_b = "";
std::string url_a = "";
unsigned int _stdcall socketioclient(LPVOID)
{
	sio::client h;
	socket::ptr current_socket;
	connection_listener l(h);
	char name[32] = {0};
	sprintf(name, "%s:proxy", pHostId);
	h.sethost(name);

	std::string url;

//	h.set_open_listener(std::bind(&connection_listener::on_connected, &l));
//	h.set_close_listener(std::bind(&connection_listener::on_close, &l, std::placeholders::_1));
//	h.set_fail_listener(std::bind(&connection_listener::on_fail, &l));
//	h.connect(SOCKET_IO_URL);
////	h.connect("https://123.206.21.202/");
//	_lock.lock();
//	if (!connect_finish)
//	{
//		_cond.wait(_lock);
//	}
//	_lock.unlock();
//	current_socket = h.get_ptr();
//
//	bind_events(current_socket);

	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	PostThreadMessage(nThreadSC, WM_MESSAGE_SOCKIO, NULL, NULL);

	while (GetMessage(&msg, 0, 0, 0))
	{
		switch (msg.message)
		{
		case WM_MESSAGE_SOCKIO:
			{
				if (NULL != current_socket)
				{
					current_socket->off_all();
					current_socket->off_error();
					current_socket->close();
					current_socket.reset();
					h.sync_close();
					h.clear_con_listeners();
				}

				url = url_a;
				url_a = url_b;
				url_b = url;

				h.set_open_listener(std::bind(&connection_listener::on_connected, &l));
				h.set_close_listener(std::bind(&connection_listener::on_close, &l, std::placeholders::_1));
				h.set_fail_listener(std::bind(&connection_listener::on_fail, &l));
				//h.connect("http://127.0.0.1:3000");
				//h.connect(SOCKET_IO_URL);
				h.connect(url);
				_lock.lock();
				if (!connect_finish)
				{
					_cond.wait(_lock);
				}
				_lock.unlock();
				current_socket = h.get_ptr();

				bind_events(current_socket);
			}
			break;

		case MSG_SEND_2_SERVER:
		{
			char* pJsonData = (char*)msg.wParam;
			if (h.opened())
			{
				string info(pJsonData);
				current_socket->emit("data", info);
			}
			free(pJsonData);
			pJsonData = NULL;
		}
		break;

		case MSG_RESSTART_COMPUTER:
		{
			MySystemShutdown();
		}
		break;

		case MSG_RESSTART_PROXY:
		{
			RestartProxy();
		}
		break;

		case MSG_DNS_CHANGE:
			{
				printf("开始设置dns\n");
				char* pData = (char*)msg.wParam;
			//	UpdateDns(pData);
				if (g_bIsRedialing)
				{
					delete pData;
					break;
				}

				g_bIsRedialing = TRUE;					
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
				//int *pFlag = new int(1);
				//PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, (WPARAM)pData, (LPARAM)pFlag);
				PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_MODIFY_DNS, (WPARAM)pData, 0);
				WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
				PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
				/*char* p1 = strtok(pData, ",");
				if (NULL != p1)
				{
					char cmdc[MAX_PATH] = {0};
					sprintf_s(cmdc, "netsh interface ip set dns \"adsl\" static %s", p1);
					char* pInfo = NULL;
					Execmd(cmdc, &pInfo);
					printf("%s\n", pInfo);
					free(pInfo);
				}
				p1 = strtok(NULL, ",");
				if (NULL != p1)
				{
					char cmdc[MAX_PATH] = {0};
					sprintf_s(cmdc, "netsh interface ip add dns \"adsl\" %s ", p1);
					char* pInfo = NULL;
					Execmd(cmdc, &pInfo);
					printf("%s\n", pInfo);
					free(pInfo);
				}*/
				printf("dns设置完成\n");

				// delete pData;
			}
			break;

		case MSG_CHROME_REINSTALL:
			{
				CloseChromeExe_ctx();
				Sleep(1000 * 2);
				DeleteDirectoryByFullName("\"C:\\Program Files\\chrome\\chrome-bin\"");
				Sleep(1000 * 5);
				RestartProxy();
			}
			break;

		case MSG_EXTENSION_REINSTALL:
			{
				CloseChromeExe_ctx();
				Sleep(1000 * 2);
				DeleteDirectoryByFullName("\"C:\\Program Files\\chrome\\userdata\"");
				Sleep(1000 * 5);
				RestartProxy();
			}
			break;
		default:
			break;
		}
	}
	return 0;
}

//void main()
//{
//	_beginthreadex(NULL, 0, socketioclient, NULL, 0, &nThreadSC);
//	
//	getchar();
//}

char Lname[MAX_PATH];

BOOL FindPPPoEAdapter(HKEY rootKey, const TCHAR* path)
{
	HKEY hKey;
	if (RegOpenKeyEx(rootKey, path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	TCHAR    achKey[MAX_PATH];   // buffer for subkey name    
	DWORD    cbName;                   // size of name string     
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name     
	DWORD    cchClassName = MAX_PATH;  // size of class string     
	DWORD    cSubKeys = 0;               // number of subkeys     
	DWORD    cbMaxSubKey;              // longest subkey size     
	DWORD    cchMaxClass;              // longest class string     
	DWORD    cValues;              // number of values for key     
	DWORD    cchMaxValue;          // longest value name     
	DWORD    cbMaxValueData;       // longest value data     
	DWORD    cbSecurityDescriptor; // size of security descriptor     
	FILETIME ftLastWriteTime;      // last write time     

	DWORD i, retCode;

	// Get the class name and the value count.     
	retCode = RegQueryInfoKey(
		hKey,                    // key handle     
		achClass,                // buffer for class name     
		&cchClassName,           // size of class string     
		NULL,                    // reserved     
		&cSubKeys,               // number of subkeys     
		&cbMaxSubKey,            // longest subkey size     
		&cchMaxClass,            // longest class string     
		&cValues,                // number of values for this key     
		&cchMaxValue,            // longest value name     
		&cbMaxValueData,         // longest value data     
		&cbSecurityDescriptor,   // security descriptor     
		&ftLastWriteTime);       // last write time     

								 // Enumerate the subkeys, until RegEnumKeyEx fails.    
	if (cSubKeys)
	{ 
		for (i = 0; i<cSubKeys; i++)
		{
			cbName = MAX_PATH;
			retCode = RegEnumKeyEx(hKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				char RegPath[MAX_PATH];
				sprintf_s(RegPath, "%s\\%s", path, achKey);
				HKEY hSubKey;
				if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegPath,
					0, KEY_ALL_ACCESS, &hSubKey))
				{
					char _ip[MAX_PATH];
					DWORD len = MAX_PATH;
					if (ERROR_SUCCESS == RegQueryValueEx(hSubKey, "DhcpIPAddress", NULL, NULL, (LPBYTE)_ip, &len))
					{
						if (strcmp(_ip, g_cCurrentIP) == 0)
						{
							sprintf_s(Lname, MAX_PATH, "%s", achKey);
							RegCloseKey(hSubKey);
							RegCloseKey(hKey);
							return TRUE;
						}
					}

					RegCloseKey(hSubKey);
				}
				else
				{
					printf("打开注册表 %s 失败\n", RegPath);
				}
			}
		}
	}

	RegCloseKey(hKey);
	return FALSE;
}

BOOL dodd(char* psrc, char* dns)
{
	char url[MAX_PATH];
	sprintf_s(url, psrc, Lname);
	printf("%s\n", url);
	HKEY hKey;
	char cNameServer[MAX_PATH];
	DWORD len = MAX_PATH;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, url,
		0, KEY_ALL_ACCESS, &hKey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, "NameServer", NULL, NULL, (LPBYTE)cNameServer, &len))
		{
			if (strcmp(cNameServer, dns) != 0)
			{
				if (ERROR_SUCCESS != RegDeleteValue(hKey, "NameServer"))
				{
					printf("删除注册表失败\n");
					RegCloseKey(hKey);
					return FALSE;
				}

				if (ERROR_SUCCESS != RegSetValueEx(hKey, "NameServer", 0, REG_SZ, (const unsigned char*)dns, strlen(dns) + 1))
				{
					printf("更新注册表失败\n");
					RegCloseKey(hKey);
					return FALSE;
				}
			}
		}
		else // ERROR_FILE_NOT_FOUND 
		{
			//if (ERROR_SUCCESS != RegSetValueEx(hKey, "ward_path", 0, REG_SZ, (const unsigned char*)exe_path, MAX_PATH))
			//{
			//	printf("更新注册表失败\n");
			//	RegCloseKey(hKey);
			//	return FALSE;
			//}
		}
	}
	else
	{
		printf("打开注册表失败\n");
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}


BOOL dodd0(char* psrc, char* dns)
{
	char url[MAX_PATH];
	sprintf_s(url, psrc, Lname);
	printf("%s\n", url);

	HKEY hAppKey = NULL;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, url, 0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_WRITE|KEY_READ, NULL, &hAppKey, &dw) != ERROR_SUCCESS)
	{
		printf("创建Proxy失败%d\n", GetLastError());
		return FALSE;
	}
	if (dw == REG_CREATED_NEW_KEY)
	{
		if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "NameServer", 0, REG_SZ, (const unsigned char*)dns, strlen(dns) + 1))
		{
			printf("更新注册表失败\n");
			RegCloseKey(hAppKey);
			return FALSE;
		}
	}else if(dw == REG_OPENED_EXISTING_KEY)
	{
		char cNameServer[MAX_PATH];
		DWORD len = MAX_PATH;
		if (ERROR_SUCCESS == RegQueryValueEx(hAppKey, "NameServer", NULL, NULL, (LPBYTE)cNameServer, &len))
		{
			if (strcmp(cNameServer, dns) != 0)
			{
				if (ERROR_SUCCESS != RegDeleteValue(hAppKey, "NameServer"))
				{
					printf("删除注册表失败\n");
					RegCloseKey(hAppKey);
					return FALSE;
				}

				if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "NameServer", 0, REG_SZ, (const unsigned char*)dns, strlen(dns) + 1))
				{
					printf("更新注册表失败\n");
					RegCloseKey(hAppKey);
					return FALSE;
				}
			}
		}
		else // ERROR_FILE_NOT_FOUND 
		{
			//if (ERROR_SUCCESS != RegSetValueEx(hKey, "ward_path", 0, REG_SZ, (const unsigned char*)exe_path, MAX_PATH))
			//{
			//	printf("更新注册表失败\n");
			//	RegCloseKey(hKey);
			//	return FALSE;
			//}
		}
	}

	RegCloseKey(hAppKey);
	return TRUE;
}

BOOL UpdateDns(char* dns1)
{
	if (!FindPPPoEAdapter(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\Interfaces"))
		return FALSE;
	printf("%s\n", Lname);

	dodd("SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\Interfaces\\%s", dns1);
	dodd("SYSTEM\\ControlSet002\\Services\\Tcpip\\Parameters\\Interfaces\\%s", dns1);
	dodd0("SYSTEM\\ControlSet001\\Services\\%s\\Parameters\\Tcpip", dns1);
	dodd0("SYSTEM\\ControlSet002\\Services\\%s\\Parameters\\Tcpip", dns1);

	return TRUE;
}