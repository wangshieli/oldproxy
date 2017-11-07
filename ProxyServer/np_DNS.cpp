#include "np_DNS.h"

#define HOST_URL "pservers.disi.se"

vector<char*> vip;

BOOL GetServerIpFromHost()
{
	nCountOfArray = 0;
	if (NULL != pArrayIndex)
	{
		delete pArrayIndex;
		pArrayIndex = NULL;
	}

	for (int i = 0; i < vip.size(); i++)
	{
		if (NULL == vip[i])
			continue;
		delete vip[i];
		vip[i] = NULL;
	}
	vip.clear();

	HOSTENT *hostent = gethostbyname(HOST_URL);
	if (NULL == hostent)
	{
		printf("解析服务器域名失败:%d\n", WSAGetLastError());
		return FALSE;
	}

	for (int i = 0; hostent->h_addr_list[i]; i++)
	{
		in_addr inad = *( (in_addr*) hostent->h_addr_list[i]);
		char *pip = new char[16];
		memcpy(pip, inet_ntoa(inad), strlen(inet_ntoa(inad))+1);
		vip.push_back(pip);
	}

	nCountOfArray = vip.size();
	if (nCountOfArray == 0)
	{
		printf("没有解析出任何ip数据\n");
		return FALSE;
	}

	pArrayIndex = new int[nCountOfArray];
	if (NULL == pArrayIndex)
	{
		printf("ip索引分配失败\n");
		return FALSE;
	}

	return TRUE;
}

BOOL reg_save_serverip(const char* server_ip);

BOOL GetSocketIoServerIp()
{
	HOSTENT *hostent = gethostbyname(SOCKET_IO_NAME);
	if (NULL == hostent)
	{
		printf("GetSocketIoServerIp解析服务器域名失败:%d\n", WSAGetLastError());
		return FALSE;
	}

	char pip[16] = {0};
	char ServerIp[MAX_PATH] = {0};
	in_addr inad = *( (in_addr*) hostent->h_addr_list[0]);
	memcpy(pip, inet_ntoa(inad), strlen(inet_ntoa(inad))+1);
	sprintf_s(ServerIp, MAX_PATH, "https://%s:443/", pip);

	return reg_save_serverip(ServerIp);

	return TRUE;
}


BOOL reg_save_serverip(const char* server_ip)
{
	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	DWORD len = MAX_PATH;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "software", 0, KEY_WRITE|KEY_READ, &hSoftKey) == ERROR_SUCCESS)
	{
		DWORD dw;
		if (RegCreateKeyEx(hSoftKey, "Proxy", 0, REG_NONE, REG_OPTION_NON_VOLATILE,
			KEY_WRITE|KEY_READ, NULL, &hAppKey, &dw) != ERROR_SUCCESS)
		{
			printf("创建Proxy失败%d\n", GetLastError());
			RegCloseKey(hSoftKey);
			return FALSE;
		}
		if (dw == REG_CREATED_NEW_KEY)
		{
			if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "server_ip", 0, REG_SZ, (const unsigned char*)server_ip, len))
			{
				printf("新server_ip设置失败%d\n", GetLastError());
				RegCloseKey(hAppKey);
				RegCloseKey(hSoftKey);
				return FALSE;
			}
		}else if(dw == REG_OPENED_EXISTING_KEY)
		{
			char old_server_ip[MAX_PATH];
			DWORD plen = MAX_PATH;
			if (ERROR_SUCCESS == RegQueryValueEx(hAppKey, "server_ip", NULL, NULL, (LPBYTE)old_server_ip, &plen))
			{
				if (strcmp(old_server_ip, server_ip) != 0)
				{
					if (ERROR_SUCCESS != RegDeleteValue(hAppKey, "server_ip"))
					{
						printf("删除注册表失败%d\n", GetLastError());
						RegCloseKey(hAppKey);
						RegCloseKey(hSoftKey);
						return FALSE;
					}

					if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "server_ip", 0, REG_SZ, (const unsigned char*)server_ip, len))
					{
						printf("更新注册表失败%d\n", GetLastError());
						RegCloseKey(hAppKey);
						RegCloseKey(hSoftKey);
						return FALSE;
					}
				}
			}else
			{
				if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "server_ip", 0, REG_SZ, (const unsigned char*)server_ip, len))
				{
					printf("更新注册表失败%d\n", GetLastError());
					RegCloseKey(hAppKey);
					RegCloseKey(hSoftKey);
					return FALSE;
				}
			}
		}
	}else
	{
		printf("打开software失败%d\n", GetLastError());
		return FALSE;
	}
	RegCloseKey(hAppKey);
	RegCloseKey(hSoftKey);
	return TRUE;
}


BOOL reg_get_server_ip(string& str)
{
	HKEY hKey;
	char server_ip[MAX_PATH] = {0};
	DWORD ip_len = MAX_PATH;

	if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Proxy", 
		0, KEY_ALL_ACCESS, &hKey))
		return FALSE;

	if (ERROR_SUCCESS != RegQueryValueEx(hKey, "server_ip", NULL, NULL, 
		(LPBYTE)server_ip, &ip_len))
	{
		RegCloseKey(hKey);
		return FALSE;
	}

	RegCloseKey(hKey);

	str = server_ip;
	return TRUE;
}