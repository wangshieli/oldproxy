#include "base64.h"
#include <io.h>
#include <Psapi.h>

BOOL isWardFolderNoExit()
{
	return _access(WARD_FOLDER, 0) == -1;
}

BOOL reg_self_starting(char* exe_path)
{
	HKEY hKey;
	char path[MAX_PATH];
	DWORD len = MAX_PATH;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0, KEY_ALL_ACCESS, &hKey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, "ward_path", NULL, NULL, (LPBYTE)path, &len))
		{
			if (strcmp(path, exe_path) != 0)
			{
				if (ERROR_SUCCESS != RegDeleteValue(hKey, "ward_path"))
				{
					printf("删除注册表失败\n");
					RegCloseKey(hKey);
					return FALSE;
				}

				if (ERROR_SUCCESS != RegSetValueEx(hKey, "ward_path", 0, REG_SZ, (const unsigned char*)exe_path, MAX_PATH))
				{
					printf("更新注册表失败\n");
					RegCloseKey(hKey);
					return FALSE;
				}
			}
		}else // ERROR_FILE_NOT_FOUND 
		{
			if (ERROR_SUCCESS != RegSetValueEx(hKey, "ward_path", 0, REG_SZ, (const unsigned char*)exe_path, MAX_PATH))
			{
				printf("更新注册表失败\n");
				RegCloseKey(hKey);
				return FALSE;
			}
		}
	}else
	{
		printf("打开注册表失败\n");
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

void Get_Ward_Exe_Name(vector<string>& exename)
{
	char checkFile[MAX_PATH];
	_makepath(checkFile, NULL, WARD_FOLDER, "*", "exe");
	_finddata_t fileInfo;
	long handle = _findfirst(checkFile, &fileInfo);
	if (-1 != handle)
	{
		do
		{
			exename.push_back(fileInfo.name);
		} while (0 == _findnext(handle, &fileInfo));
	}
	_findclose(handle);
}

BOOL closeAllWardExe(vector<string>& exename)
{
	if (exename.empty())
		return TRUE;

	PROCESSENTRY32 pe32;
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe32.dwSize = sizeof(pe32);

	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		printf("创建进程映射失败\n");
		return FALSE;
	}

	BOOL bMode = ::Process32First(hProcessSnap, &pe32);

	int nNumOfName = exename.size();
	while (bMode)
	{
		for (int i = 0; i < nNumOfName; i++)
		{
			if (0 == strcmp(exename[i].c_str(), pe32.szExeFile))
			{
				HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
				TerminateProcess(ProcessHandle, 0);
			}
		}
		
		bMode = ::Process32Next(hProcessSnap, &pe32);
	}
	CloseHandle(hProcessSnap);

	return TRUE;
}

BOOL isOnWorking()
{
	PROCESSENTRY32 pe32;
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe32.dwSize = sizeof(pe32);

	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		printf("创建进程映射失败\n");
		return FALSE;
	}

	BOOL bMode = ::Process32First(hProcessSnap, &pe32);

	char pwatchname[_MAX_FNAME];
	sprintf_s(pwatchname, "pwatch-v%s.exe", g_pwatchversion);
	while (bMode)
	{
		if (0 == strcmp(pwatchname, pe32.szExeFile))
		{
			CloseHandle(hProcessSnap);
			return TRUE;
		}	
		bMode = ::Process32Next(hProcessSnap, &pe32);
	}
	CloseHandle(hProcessSnap);

	return FALSE;
}

BOOL isPWatchFloderIsExist()
{
	return _access(WARD_FOLDER, 0) != -1;
}

BOOL GetPWatchVersion(char* pVersion)
{
	HKEY hKey;
	char pwatchversion[8];
	DWORD len = 8;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\PWatch",
		0, KEY_ALL_ACCESS, &hKey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(hKey, "pwatch_version", NULL, NULL, (LPBYTE)pwatchversion, &len))
		{
			sprintf_s(pVersion, 8, "%s", pwatchversion);
			printf("pwatch_version:%s\n", pVersion);
		}else // ERROR_FILE_NOT_FOUND 
		{
			printf("注册表中没有pwatch_version字段\n");
			RegCloseKey(hKey);
			return FALSE;
		}
	}else
	{
		printf("打开注册表失败\n");
		return FALSE;
	}
	RegCloseKey(hKey);
	return TRUE;
}

BOOL CheckPWatch()
{
	if(!isPWatchFloderIsExist())
	{
		CreateDirectory(WARD_FOLDER, NULL);
		sprintf_s(g_pwatchversion, 8, "%s", "0.0.0");
	}else
	{
		vector<string> exename;
		exename.clear();
		Get_Ward_Exe_Name(exename);
		if (exename.size() != 1 || !GetPWatchVersion(g_pwatchversion))
		{
			HANDLE hExitEvnet = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, P_WATCH_EXIT_EVETN);
			if (NULL != hExitEvnet)
			{
				HANDLE hExitCompleteEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, P_WATCH_EXIT_COMPLETE);
				if (NULL != hExitCompleteEvent)
				{
					SetEvent(hExitEvnet);
					WaitForSingleObject(hExitCompleteEvent, INFINITE);
					printf("pwatch退出完成--------------------------------\n");
					CloseHandle(hExitCompleteEvent);
				}
				CloseHandle(hExitEvnet);
			}

			for (int i = 0; i < exename.size(); i++)
			{
				char paths[_MAX_PATH];
				_makepath(paths, NULL, WARD_FOLDER, exename[i].c_str(), NULL);
				DeleteFile(paths);
			}
			sprintf_s(g_pwatchversion, 8, "%s", "0.0.0");

			Sleep(1000);
		}
	}
	
	return TRUE;
}

void closepwatch()
{
	if(!isPWatchFloderIsExist())
	{
		CreateDirectory(WARD_FOLDER, NULL);
	}else
	{
		vector<string> exename;
		exename.clear();
		Get_Ward_Exe_Name(exename);
		HANDLE hExitEvnet = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, P_WATCH_EXIT_EVETN);
		if (NULL != hExitEvnet)
		{
			HANDLE hExitCompleteEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, P_WATCH_EXIT_COMPLETE);
			if (NULL != hExitCompleteEvent)
			{
				SetEvent(hExitEvnet);
				WaitForSingleObject(hExitCompleteEvent, INFINITE);
				printf("pwatch退出完成--------------------------------\n");
				CloseHandle(hExitCompleteEvent);
			}
			CloseHandle(hExitEvnet);
		}
		for (int i = 0; i < exename.size(); i++)
		{
			char paths[_MAX_PATH];
			_makepath(paths, NULL, WARD_FOLDER, exename[i].c_str(), NULL);
			DeleteFile(paths);
		}
		Sleep(1000);
	}
}

void closepwatch01()
{
	if(!isPWatchFloderIsExist())
	{
		CreateDirectory(WARD_FOLDER, NULL);
	}else
	{
		/*vector<string> exename;
		exename.clear();
		Get_Ward_Exe_Name(exename);*/
		HANDLE hExitEvnet = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, P_WATCH_EXIT_EVETN);
		if (NULL != hExitEvnet)
		{
			HANDLE hExitCompleteEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, P_WATCH_EXIT_COMPLETE);
			if (NULL != hExitCompleteEvent)
			{
				SetEvent(hExitEvnet);
				WaitForSingleObject(hExitCompleteEvent, INFINITE);
				printf("pwatch退出完成--------------------------------\n");
				CloseHandle(hExitCompleteEvent);
			}
			CloseHandle(hExitEvnet);
		}
		//for (int i = 0; i < exename.size(); i++)
		//{
		//	char paths[_MAX_PATH];
		//	_makepath(paths, NULL, WARD_FOLDER, exename[i].c_str(), NULL);
		//	DeleteFile(paths);
		//}
		Sleep(1000);
	}
}