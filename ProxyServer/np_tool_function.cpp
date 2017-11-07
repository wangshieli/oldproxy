#include "np_tool_function.h"
// #include <io.h>

void myprintf (const char *lpFormat, ... ) {

	char* pLog = new char[512];
	ZeroMemory(pLog, 512);

	va_list arglist ;
	va_start(arglist, lpFormat);
	wvsprintf( pLog, lpFormat, arglist );
	va_end(arglist);

	PostThreadMessage(g_nMessageThreadNo, WM_MESSAGE_LOG, (WPARAM)pLog, NULL);
}

void clean_string(char *str)
{
    char *start = str - 1;
    char *end = str;
    char *p = str;
    while(*p)
    {
        switch(*p)
        {
        case ' ':
        case '\r':
        case '\n':
            {
                if(start + 1==p)
                    start = p;
            }
            break;
        default:
            break;
        }
        ++p;
    }
    //现在来到了字符串的尾部 反向向前
    --p;
    ++start;
    if(*start == 0)
    {
        //已经到字符串的末尾了
        *str = 0 ;
        return;
    }
    end = p + 1;
    while(p > start)
    {
        switch(*p)
        {
        case ' ':
        case '\r':
        case '\n':
            {
                if(end - 1 == p)
                    end = p;
            }
            break;
        default:
            break;
        }
        --p;
    }
    memmove(str,start,end-start);
    *(str + (int)end - (int)start) = 0;
}


BOOL GetHostId(char** _pHostId)
{
	if (*_pHostId != NULL)
	{
		return TRUE;
	}
	FILE* fHostId = fopen(CLIENT_ID_PATH, "r");
	if (NULL == fHostId)
	{
		myprintf("fopen C:\\client_id.txt failed wiht error code : %d", WSAGetLastError());
		return FALSE;
	}

	fseek(fHostId, 0, SEEK_END);
	int FileLen = ftell(fHostId);
	fseek(fHostId, 0, SEEK_SET);

	char* p = NULL;
	p = new char[FileLen + 1];// ftell返回的字节数不包含'\0'
	memset(p, 0x00, FileLen + 1);

	int nReadBytes = fread(p, FileLen + 1, 1, fHostId);

	//for (int i = 0; i < strlen(p); i++)
	//{
	//	if (p[i] == '\n' || p[i] == '\r')
	//		p[i] = '\0';
	//}
	clean_string(p);

	fclose(fHostId);

	*_pHostId = p; 

	return TRUE;
}

BOOL SendData(SOCKET TargetScoket, char* pSendBuf)
{
	int iRet = 0;
	int nSendBufLen = strlen(pSendBuf);

	int sendlen = 0;
	while (sendlen < nSendBufLen)
	{
		iRet = send(TargetScoket, pSendBuf + sendlen, nSendBufLen - sendlen, 0);
		if (iRet == 0 || iRet == SOCKET_ERROR)
		{
			myprintf("send host_id to server time out wiht error code : %d", WSAGetLastError());
			return FALSE;
		}
		sendlen += iRet;
	}
	return TRUE;
}

int SendData1(SOCKET s, const char* buf, int bufSize)
{
	int pos = 0;
	while (pos < bufSize)
	{
		int ret = send(s, buf+pos,bufSize-pos,0);
		if (ret > 0) {
			pos += ret;
		} else {
			return ret;
		}
	}
	return pos;
}

BOOL AnalyzePortFromRetInfo(char* pRetInfo, char** _pNewPort)
{
	char* pBegin = strstr(pRetInfo, "post=");
	if (NULL == pBegin)
	{
		myprintf("AnalyzePortFromRetInfo pBegin failed");
		return FALSE;
	}
	char* pEnd = strstr(pBegin, "IAMNEWPORTEND");
	if (NULL == pEnd)
	{
		myprintf("AnalyzePortFromRetInfo pEnd failed");
		return FALSE;
	}

	int portlen = pEnd - pBegin - strlen("post=");
	int nByteSize = MY_ALIGN(portlen + 1, 8);
	char* pNewPort = new char[nByteSize];
	memset(pNewPort, 0x00, nByteSize);
	memcpy(pNewPort, pBegin + strlen("post="), portlen);
	
	*_pNewPort = pNewPort;

	return TRUE;
}

BOOL RecvRequest(SOCKET TargetSocket, char** _pRecvInfo)
{
	int nRet = 0;
	int RecvLen = 0;
	unsigned long WaitBytes = 0;
	char* pRecvBuf = (char*)malloc(1024);
	do
	{
		nRet = ioctlsocket(TargetSocket, FIONREAD, &WaitBytes);
		if (SOCKET_ERROR == nRet)
		{
			myprintf("RecvRequest ioctlsocket failed with error code : %d", WSAGetLastError());
			free(pRecvBuf);
			pRecvBuf = NULL;
			return FALSE;
		}

	//	WaitBytes = 4;
		if (RecvLen + WaitBytes > _msize(pRecvBuf))
		{
			int RelAllocByteSize = MY_ALIGN((RecvLen+WaitBytes+1), 8);
			pRecvBuf = (char*)realloc(pRecvBuf, RelAllocByteSize);
		}

		nRet = recv(TargetSocket, pRecvBuf + RecvLen, 512, 0);
		if (nRet > 0)
		{
			RecvLen += nRet;
			pRecvBuf[RecvLen] = '\0';
		}
		else
		{
			myprintf("RecvRequest failed with error code : %d", WSAGetLastError());
			free(pRecvBuf);
			pRecvBuf = NULL;
			return FALSE;
		}

		if (RecvLen >= 4)
		{
			if (strstr(pRecvBuf, "\r\n\r\n"))
				break;
		}
	} while (true);
	pRecvBuf[RecvLen] = '\0';
//	cout << "6086 接收完成" << endl;

	*_pRecvInfo = pRecvBuf;

	return TRUE;
}

BOOL Execmd(char* cmd, char** result)
{
	int readlen = 0;
	int readtotal = 0;
	char buffer[128];
	char* pUserData = (char*)malloc(1024);
	ZeroMemory(pUserData, 1024);
	FILE *pipe = _popen(cmd, "r");
	if (!pipe)
	{
		myprintf("Execmd failed with error code : %d", WSAGetLastError());
		free(pUserData);
		pUserData = NULL;
		return FALSE;
	}

	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe))
		{
			readlen = strlen(buffer)+1;
			readtotal += readlen;
			if (readtotal > _msize(pUserData))
			{
				int RelAllocByteSize = MY_ALIGN((readtotal+1), 8);
				pUserData = (char*)realloc(pUserData, RelAllocByteSize);
			}
			strcat(pUserData, buffer);
		}
	}

	_pclose(pipe);
	*result = pUserData;

	return TRUE;
}

BOOL is_internal_ip(const char* ip)
{
	unsigned long lip = inet_addr(ip);
	unsigned long net_a = inet_addr("10.255.255.255") >> 24;
	unsigned long net_b = inet_addr("172.31.255.255") >> 20;
	unsigned long net_c = inet_addr("192.168.255.255") >> 16;

	return (lip >> 24 == net_a) || (lip >> 20 == net_b) || (lip >> 16 == net_c);
}

BOOL is_reserved_ip(const char *ip)
{
    static const char *address_blocks[] = {
        "0.0.0.0/8",
        "10.0.0.0/8",
        "100.64.0.0/10",
        "127.0.0.0/8",
        "169.254.0.0/16",
        "172.16.0.0/12",
        "192.0.0.0/24",
        "192.0.2.0/24",
        "192.88.99.0/24",
        "192.168.0.0/16",
        "198.18.0.0/15",
        "198.51.100.0/24",
        "203.0.113.0/24",
        "224.0.0.0/4",
        "240.0.0.0/4",
        "255.255.255.255/32"
    };

    static const int blk_nums = sizeof(address_blocks) / sizeof(*address_blocks);
    static char buf[40];

    for (int i = 0; i < blk_nums; ++i) {
        strncpy(buf, address_blocks[i], sizeof(buf));
        char *pos = strchr(buf, '/');
		if (pos == NULL)
		{
			return FALSE;
		}
        *pos = '\0';
		ULONG val = htonl(inet_addr(buf)); // big endian
        int msk = atoi(pos + 1);
        int offset = 32 - msk;

        ULONG tar = htonl(inet_addr(ip));

        if ((val >> offset) == (tar >> offset))
			return TRUE;
    }

	return FALSE;
}

unsigned int _stdcall np_openadsl_thread(LPVOID pVoid)
{
	system("rasphone -d adsl");
	return 0;
}

BOOL _stdcall enumprocess(HWND hwnd, LPARAM lParam)
{
	char ctlName[256];
	char* p = "rasautou.exe";
	::SendMessage(hwnd, WM_GETTEXT, sizeof(ctlName)/sizeof(char),(LPARAM)ctlName);
	_strlwr_s(ctlName, strlen(ctlName)+1);
	if (strstr(ctlName, p) != NULL)
	{
		HWND hButton = ::FindWindowEx(hwnd, NULL, "Button", "确定");
		if (hButton == NULL)
		{
			myprintf("确定按钮未找到");
		}
		::SendMessage(hButton, BM_CLICK, 0, 0);
		*(int*)lParam = 1;
		return FALSE;
	}
	return TRUE;
}

BOOL UpdateDns(HWND hAdsl, const char* dns1, const char* dns2);

BOOL Redial(WPARAM wParam, LPARAM lParam)
{
	HANDLE hOpenAdslThread = (HANDLE)_beginthreadex(NULL, 0, np_openadsl_thread, NULL, 0, NULL);
	if (NULL == hOpenAdslThread)
	{
		myprintf("_beginthreadex np_openadsl_thread failed with error code : %d", GetLastError());
		return FALSE;
	}
	Sleep(1000 * 4);// 等待线程中的操作完成

	char pErrorWnd[] = "连接到 adsl 时出错";
	HWND hErrorWnd = ::FindWindow(NULL, pErrorWnd);
	if (hErrorWnd != NULL)
	{
		::SendMessage(hErrorWnd, WM_CLOSE, 0, 0);
		Sleep(1000);
		TerminateThread(hOpenAdslThread, 0);
		CloseHandle(hOpenAdslThread);
		return FALSE;
	}

	char className[] = "连接 adsl";
	HWND hWnd = ::FindWindow(NULL, className);
	if (hWnd == NULL)
	{
		myprintf("对话框不存在");
		TerminateThread(hOpenAdslThread, 0);
		CloseHandle(hOpenAdslThread);
		return FALSE;
	}

	TerminateThread(hOpenAdslThread, 0);
	CloseHandle(hOpenAdslThread);


	int* pUpdateDns = (int*)lParam;
	if (NULL != pUpdateDns)
	{
		char* pData = (char*)wParam;
		char* p1 = strtok(pData, ",");
		char* p2 = strtok(NULL, ",");

		UpdateDns(hWnd, p1, p2);

		delete pUpdateDns;
		delete pData;
	}


	HWND hButton = ::FindWindowEx(hWnd, NULL, "Button", "连接(&C)");
	if (hButton == NULL)
	{
		myprintf("连接(&C)按钮未找到");
		return FALSE;
	}
	::SendMessage(hButton, BM_CLICK, 0, 0);

	Sleep(3000);

	DWORD nLinkingTime = 0;
	HWND hLinking;
	char pLinking[] = "正在连接 adsl...";
	while ((hLinking = ::FindWindow(NULL, pLinking)) != NULL)
	{
		char pErrorWnd1[] = "连接到 adsl 时出错";
		HWND hErrorWnd1 = ::FindWindow(NULL, pErrorWnd1);
		if (hErrorWnd1 != NULL)
		{
			::SendMessage(hErrorWnd1, WM_CLOSE, 0, 0);
			Sleep(1000);
			return FALSE;
		}
		Sleep(1000);
		if (nLinkingTime++ > 90)
		{
			myprintf("adsl连接时间超过90s重新拨号");
			::SendMessage(hLinking, WM_CLOSE, 0, 0);
			Sleep(1000);
			return FALSE;;
		}
	}

	int nFlag = 0;
	::EnumChildWindows(NULL, enumprocess, (LPARAM)&nFlag);
	if (nFlag != 0)
	{
		myprintf("拨号过程中rasautou.exe崩溃");
		return FALSE;
	}

	return TRUE;
}

BOOL GetAdslIp(char* pIP)
{
	char* pIpConfigInfo = NULL;
	if (!Execmd("ipconfig", &pIpConfigInfo))
	{
		memcpy(pIP, "0.0.0.0", strlen("0.0.0.0"));
		return FALSE;
	}

	char* pFindAdsl = strstr(pIpConfigInfo, "adsl:");
	if (pFindAdsl == NULL)
	{
		myprintf("adsl adapter not find");
		if (pIpConfigInfo != NULL)
		{
			free(pIpConfigInfo);
			pIpConfigInfo = NULL;
		}
		memcpy(pIP, "0.0.0.0", strlen("0.0.0.0"));
		return FALSE;
	}

	char *pFindIp = strstr(pFindAdsl, "IP");
	if (pFindIp == NULL)
	{
		myprintf("adsl ip not find");
		if (pIpConfigInfo == NULL)
		{
			free(pIpConfigInfo);
			pIpConfigInfo = NULL;
		}
		memcpy(pIP, "0.0.0.0", strlen("0.0.0.0"));
		return FALSE;
	}

	char* pFindMH = strstr(pFindIp, ":");
	if (pFindMH == NULL)
	{
		myprintf("adsl ip info is not intact");
		if (pIpConfigInfo == NULL)
		{
			free(pIpConfigInfo);
			pIpConfigInfo = NULL;
		}
		memcpy(pIP, "0.0.0.0", strlen("0.0.0.0"));
		return FALSE;
	}

	char* pFindEndNull = strstr(pFindMH, "\n");
	int nIPLen = pFindEndNull - pFindMH - 2;
	memcpy(pIP, pFindMH+2, nIPLen);

	if (NULL != pIpConfigInfo)
	{
		free(pIpConfigInfo);
		pIpConfigInfo = NULL;
	}

	//for (int i = 0; i < strlen(pIP); i++)
	//{
	//	if (pIP[i] == '\r' || pIP[i] == '\n')
	//		pIP[i] = '\0';
	//}
	clean_string(pIP);

	return TRUE;
}

BOOL CheckAdslIp(const char* pIP)
{
	if (strcmp(pIP, "") ==0 || strcmp(pIP, "0.0.0.0") == 0)
	{
		myprintf("redial failed ,redial again");
		return FALSE;
	}

	if (/*is_internal_ip(pIP) || */is_reserved_ip(pIP))
	{
		bOldMode = FALSE;
	}else
		bOldMode = TRUE;
	
	return TRUE;
}

int ConnectServer(SOCKET& s, char* recvBuf, int len)
{
	char strHost[512] = {0};
	char strPort[8] = {0};
	int port = 80;
	char* sp = (char*)(memchr(recvBuf+8, ' ', len - 8));
	if (!sp)
		return -1;

	char* pt = (char*)(memchr(recvBuf+8, ':', sp-recvBuf - 8));
	if (pt)
	{
		int l = sp - pt - 1;
		if (l >= 8)
			return -1;
		memcpy(strPort, pt + 1, l);
		port = atoi(strPort);
		memcpy(strHost, recvBuf + 8, pt- recvBuf - 8);
	}else
		memcpy(strHost, recvBuf+8, sp-recvBuf - 8);

	return InitHost(&s, strHost, port) - 1;
}

ULONG QueryIPByHostName(const char * hostName)
{
	EnterCriticalSection(&cs_Host);
		ULONG ret = 0;
		map<string,ULONG>::iterator it = db.find(hostName);
		if (it == db.end() )
		{
			HOSTENT *hostent=gethostbyname(hostName);
			if (hostent)
			{
				in_addr inad = *( (in_addr*) *hostent->h_addr_list);
				ret = inad.s_addr;
				db[hostName] = ret;
			}
		} else {
			ret = it->second;
		}
	LeaveCriticalSection(&cs_Host);
	return ret;
}

BOOL InitHost(SOCKET* ServerSocket, char* HostName, int Port)
{
	struct sockaddr_in Server;
	Server.sin_family = AF_INET;
	Server.sin_port = htons(Port);

	Server.sin_addr.s_addr = QueryIPByHostName(HostName);

	*ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == *ServerSocket)
	{
		myprintf("InitHost ServerSocket failed with error code : %d", WSAGetLastError());
		return FALSE;
	}

	//cout << inet_ntoa(Server.sin_addr) << endl;;
	//cout << Port << endl;
	if (connect(*ServerSocket, (const SOCKADDR *)&Server,sizeof(Server)) == SOCKET_ERROR)
	{
		myprintf("InitHost connect failed with error code : %d", WSAGetLastError());
		closesocket(*ServerSocket);
		return FALSE;
	}

	return TRUE;
}

int PreResponse(RECVPARAM* svc)
{
	const char response[] = "HTTP/1.0 200 Connection established\r\n"
					"Proxy-agent: HTTP Proxy Lite /0.2\r\n\r\n";   // 这里是代理程序的名称"

	int ret = SendData1(svc->ClientSocket,response,sizeof(response)-1);
	if (ret <= 0){ return -2;}
	return 0;
}

DWORD WINAPI ExcThread(LPVOID lpp)
{
	SOCKET s1 = ((RECVPARAM*)lpp)->ClientSocket;
	SOCKET s2 = ((RECVPARAM*)lpp)->ServerSocket;
	char buf[MAXSIZE];
	while(1)
	{
		int ret = recv(s1,buf,MAXSIZE,0);
		if (ret <=0 ) { return ret;}
		ret = SendData1(s2,buf,ret);
		if (ret <=0 ) {return ret;}
	}
	return 0;
}

int	ExchangeData(RECVPARAM* svc)
{					
	RECVPARAM th1 = *svc;
	RECVPARAM th2 = {svc->ServerSocket,svc->ClientSocket};
	DWORD tid;
	
	HANDLE h1 = CreateThread(0,0,ExcThread,&th1,0,&tid);
	if (!h1) 
		return -1;
	HANDLE h2 = CreateThread(0,0,ExcThread,&th2,0,&tid);
	if (!h2)
	{	
		TerminateThread(h1,0x1); 
		CloseHandle(h1);
		return -1;
	}
	HANDLE hds[2] = {h1,h2};
	WaitForMultipleObjects(2,hds,0,INFINITE);

	shutdown(svc->ServerSocket,2);	// SD_BOTH
	shutdown(svc->ClientSocket,2);	// SD_BOTH

	WaitForSingleObject(h2,INFINITE);
	CloseHandle(h2);
	WaitForSingleObject(h1,INFINITE);
	CloseHandle(h1);

	return 0;
}

BOOL SendPostDataAndRecvBackInfo(char* pApiUrl, char* pPostData, pCurlWriteFunction _pCurlWriteFunction, void* pBackInfo)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist* http_header = NULL;
	curl = curl_easy_init();
	if (!curl)
	{
		myprintf("SendPostDataAndRecvBackInfo curl_easy_init failed");
		return FALSE;
	}

	curl_easy_setopt(curl, CURLOPT_URL, pApiUrl);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pPostData);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _pCurlWriteFunction);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, pBackInfo);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);// 超时单位s
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);


	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		switch(res)
		{
			case CURLE_UNSUPPORTED_PROTOCOL:
				fprintf(stderr,"不支持的协议,由URL的头部指定\n");
			case CURLE_COULDNT_CONNECT:
				fprintf(stderr,"不能连接到remote主机或者代理\n");
			case CURLE_HTTP_RETURNED_ERROR:
				fprintf(stderr,"http返回错误\n");
			case CURLE_READ_ERROR:
				fprintf(stderr,"读本地文件错误\n");
			default:
				fprintf(stderr,"返回值:%d\n",res);
		}
		curl_easy_cleanup(curl);

		return FALSE;
	}

	curl_easy_cleanup(curl);

	return TRUE;
}

BOOL SendPostDataAndRecvBackInfo1(char* pApiUrl, char* pPostData, pCurlWriteFunction _pCurlWriteFunction, void* pBackInfo)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist* http_header = NULL;
	curl = curl_easy_init();
	if (!curl)
	{
		myprintf("SendPostDataAndRecvBackInfo curl_easy_init failed");
		return FALSE;
	}

	curl_easy_setopt(curl, CURLOPT_URL, pPostData);
	//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pApiUrl);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _pCurlWriteFunction);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, pBackInfo);
	curl_easy_setopt(curl, CURLOPT_POST, 0);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
//	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);// 超时单位s
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		switch(res)
		{
			case CURLE_UNSUPPORTED_PROTOCOL:
				fprintf(stderr,"不支持的协议,由URL的头部指定\n");
			case CURLE_COULDNT_CONNECT:
				fprintf(stderr,"不能连接到remote主机或者代理\n");
			case CURLE_HTTP_RETURNED_ERROR:
				fprintf(stderr,"http返回错误\n");
			case CURLE_READ_ERROR:
				fprintf(stderr,"读本地文件错误\n");
			default:
				fprintf(stderr,"返回值:%d\n",res);
		}
		curl_easy_cleanup(curl);

		return FALSE;
	}

	curl_easy_cleanup(curl);

	return TRUE;
}

size_t curl_recvbackinfo_function(void* buffer,size_t size,size_t nmemb,void *pRecvInfo)
{
	CURLRECVDATA* pCurlRecvData = (CURLRECVDATA*)pRecvInfo;
	size_t recvlen = MY_ALIGN((pCurlRecvData->nRecvedLen+(size*nmemb) +1), 8);
	if (_msize(pCurlRecvData->pRecvData) < recvlen)
		pCurlRecvData->pRecvData = (char*)realloc(pCurlRecvData->pRecvData, recvlen);
	if (pCurlRecvData->pRecvData)
	{
		memcpy(pCurlRecvData->pRecvData + pCurlRecvData->nRecvedLen, buffer, size* nmemb);
		pCurlRecvData->nRecvedLen += nmemb * size;
		memset(pCurlRecvData->pRecvData + pCurlRecvData->nRecvedLen, 0x00, 1);
	}
	return size * nmemb;
}

void spliteVersion(char *p , int* pout)
{
	char *p1 = strstr(p, ".");
	if (p1 != NULL)
	{
		int n = p1 - p;
		char buf[8] = {0};
		memcpy(buf, p, n);
		pout[0] = atoi(buf);
		pout += 1;
		p = p1+1;
		spliteVersion(p, pout);
	}else
	{
		pout[0] = atoi(p);
	}
}

int compV(int* l, int* s, int* times)
{
	*times -= 1;
	if (l[0] > s[0])
	{
		return 0;
	}
	if (l[0] < s[0])
	{
		return -1;
	}
	if (l[0] == s[0])
	{
		if (*times > 0)
		{		
			s += 1;
			l += 1;
			return compV(l, s, times);
		}else
		{
			return 0;
		}		
	}
	return 0;
}

BOOL CompareVersion(char* pl, char* ps)
{
	int* plint = new int[3];
	int* psint = new int[3];
	spliteVersion(pl, plint);
	spliteVersion(ps, psint);
	int times = 3;
	if(compV(plint, psint, &times) == 0)
	{
		delete[] plint;
		delete[] psint;
		return TRUE;
	}
	else
	{
		delete[] plint;
		delete[] psint;
		return FALSE;
	}
	return TRUE;
}

// 只是用来处理简单的key:value对,不能解析数组类型的json串
BOOL GetJsonValue(char* pSrc, char* pTemp, char* pOutValue)
{
	char TempBuf[256];
	sprintf(TempBuf, "\"%s\":", pTemp);
	int nLenTemp = strlen(TempBuf);
	char* pKey = strstr(pSrc, TempBuf);
	if (pKey == NULL)
	{
	//	PostThreadMessage(g_nShowMessageThreadID, SHOWLOGSTATIC, (WPARAM)"解析版本信息失败", 0);
		return FALSE;
	}
	pKey += nLenTemp;
	char *pSp = ",";
	if (pKey[0] == '"')
	{
		pKey += 1;
		pSp = "\",";
	}
	char* pItem = strstr(pKey, pSp);
	if (pItem == NULL)
	{
		pSp = "\"}";
		pItem = strstr(pKey, pSp);
		if (pItem == NULL)
		{
		//	PostThreadMessage(g_nShowMessageThreadID, SHOWLOGSTATIC, (WPARAM)"找不到数据", 0);
			return FALSE;
		}
	}
	int nValueLen = pItem - pKey;
	memcpy(pOutValue, pKey, nValueLen);
	pOutValue[nValueLen] = '\0';

	return TRUE;
}

void UrlFormating(char *purl, char* pt, char *pu)
{
	char *p = strstr(purl, pt);
	if (p != NULL)
	{
		int n = p - purl;
		memcpy(pu, purl, n);
		pu += n;
		purl += n+1;
		UrlFormating(purl, pt, pu);
	}else
	{
		strcat(pu, purl);
	}
}

BOOL needdownload(char* psPath, char* pmd5)
{
	FILE* pFileEx = NULL;
	fopen_s(&pFileEx, psPath, "rb");
	if (pFileEx == NULL)
	{
		//文件不存在重新下载
		return FALSE;
	}else
	{
		//文件存在
		fclose(pFileEx);
		string md5value = MD5(ifstream(psPath, ios::binary)).toString();
		if (strcmp(md5value.c_str(), pmd5) == 0)
		{
			//不需要重新下载
			return TRUE;
		}
		// 重新下载
		DeleteFile(psPath);

		return FALSE;
	}

	return TRUE;
}

size_t write_flie(void* buffer,size_t size,size_t nmemb,void *stream)
{
	FILE *f = (FILE*)stream;
	fwrite(buffer, 1, nmemb*size, f);
	return size*nmemb;
}

BOOL GetFileFullPath(char* filePath)
{
	if (!::GetModuleFileName(NULL, filePath, MAX_PATH))
	{
		return FALSE;
	}

	return TRUE;
}

void ClearThreadResource()
{
	vector<SOCKET>::iterator it;
	EnterCriticalSection(&g_cs);
	for (it = g_vSocket.begin(); it != g_vSocket.end(); it++)
	{
		if (*it != INVALID_SOCKET)
		{
			closesocket(*it);
			*it = INVALID_SOCKET;
		}
	}
	g_vSocket.clear();
	LeaveCriticalSection(&g_cs);

	//vector<HANDLE>::iterator ith;
	//EnterCriticalSection(&g_hcs);
	//for (ith = g_vThreadHandle.begin(); ith != g_vThreadHandle.end(); ith++)
	//{
	//	if (*ith != NULL)
	//	{
	//		CloseHandle(*ith);
	//		*ith = NULL;
	//	}
	//}
	//g_vThreadHandle.clear();
	//LeaveCriticalSection(&g_hcs);
}

int RecvRequest01(SOCKET s, char** buf, int bufSize)
{
	int len = 0;
	int ret = 0;
	char* p = *buf;
	do
	{
		ret = recv(s, p + len, bufSize - len, 0);
		if (ret > 0)
			len += ret;
		else
			return ret;

		if (strstr(p, "\r\n\r\n") != NULL)
			break;
	} while (len < bufSize);

	p[len] = '\0';

	return len;
}

int RecvRequest02(SOCKET s, char** buf, int bufSize)
{
	int len = 0;
	int ret = 0;
	char* p = *buf;
	unsigned long waitbytes = 0;
	do
	{
		ret = recv(s, p + len, bufSize - len, 0);
		if (ret > 0)
			len += ret;
		else
			return ret;

		if (ret < waitbytes)
			continue;

		ret = ioctlsocket(s, FIONREAD, &waitbytes);
		if (ret == SOCKET_ERROR)
			return ret;

		if (waitbytes > 0)
		{
			size_t buflen = MY_ALIGN(len + waitbytes, 8);
			if (bufSize <= buflen)
			{
				p = (char*)realloc(p,buflen+8);
				*buf = p;
				bufSize = buflen;
			}		
		}
	} while (waitbytes > 0);

	p[len] = '\0';

	return len;
}

BOOL reg_proxy_path()
{
	char filepath[MAX_PATH] = {0};
	HMODULE GetModH = GetModuleHandle(NULL);

	GetModuleFileName(GetModH, filepath, MAX_PATH);
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
			if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "proxy_path", 0, REG_SZ, (const unsigned char*)filepath, len))
			{
				printf("新proxy_path设置失败%d\n", GetLastError());
				RegCloseKey(hAppKey);
				RegCloseKey(hSoftKey);
				return FALSE;
			}
		}else if(dw == REG_OPENED_EXISTING_KEY)
		{
			char proxy_path[MAX_PATH];
			DWORD plen = MAX_PATH;
			if (ERROR_SUCCESS == RegQueryValueEx(hAppKey, "proxy_path", NULL, NULL, (LPBYTE)proxy_path, &plen))
			{
				if (strcmp(proxy_path, filepath) != 0)
				{
					if (ERROR_SUCCESS != RegDeleteValue(hAppKey, "proxy_path"))
					{
						printf("删除注册表失败%d\n", GetLastError());
						RegCloseKey(hAppKey);
						RegCloseKey(hSoftKey);
						return FALSE;
					}

					if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "proxy_path", 0, REG_SZ, (const unsigned char*)filepath, len))
					{
						printf("更新注册表失败%d\n", GetLastError());
						RegCloseKey(hAppKey);
						RegCloseKey(hSoftKey);
						return FALSE;
					}
				}
			}else
			{
				if (ERROR_SUCCESS != RegSetValueEx(hAppKey, "proxy_path", 0, REG_SZ, (const unsigned char*)filepath, len))
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

void set_starting_items()
{
//	BOOL bRegedit = FALSE;
//	char filepath[MAX_PATH];
//	HMODULE GetModH = GetModuleHandle(NULL);

//	GetModuleFileName(GetModH, filepath, MAX_PATH);

	HKEY hKey;
	char path[MAX_PATH];
	DWORD len = MAX_PATH;
	long l = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		0, KEY_ALL_ACCESS, &hKey);
	if (ERROR_SUCCESS == l)
	{ 
		if (RegQueryValueEx(hKey, "proxy_path", NULL, NULL, (LPBYTE)path,
			&len) == ERROR_SUCCESS)
		{
			//if (strcmp(path, filepath) == 0)
			//	bRegedit = TRUE;
			//else
			//{
				if (RegDeleteValue(hKey,"proxy_path") == ERROR_SUCCESS)
					myprintf("注册表清除完成");
		//	}
		}
	}else
	{
		myprintf("注册表打开失败");
		return;
	}

	//if (!bRegedit)
	//{
	//	RegSetValueEx(hKey, "proxy_path", 0, REG_SZ, (const unsigned char*)filepath, MAX_PATH);
	//	myprintf("注册开机启动完成");
	//}
	
	RegCloseKey(hKey);
}

void LSCloseSocket(SOCKET& s)
{
	//LINGER  lingerStruct;
	//lingerStruct.l_onoff = 1;
	//lingerStruct.l_linger = 0;
	//setsockopt(s, SOL_SOCKET, SO_LINGER,
	//	(char *)&lingerStruct, sizeof(lingerStruct) );
	closesocket(s);
	s = INVALID_SOCKET;
}

DWORD dwRandIndex  = 0;
void GetRandIndex(int arrayIndex[], int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		arrayIndex[i] = -1;
	}

	for (int i = 0; i < nCount;)
	{
		try
		{
			time_t t;
			srand((unsigned) (time(&t)+dwRandIndex++));
			int a=rand()%(nCount);
			for (int j = 0; j < i; j++)
			{
				if (arrayIndex[j] == a)
					throw new int(0);
			}
			arrayIndex[i] = a;
			i++;
		}catch(int *err)
		{
			delete err;
			continue;
		}
	}
}

int Random(int m, int n)
{
	time_t t;
	srand((unsigned) (time(&t)+dwRandIndex++));
	int pos, dis;
	if(m == n)
	{
		return m;
	}
	else if(m > n)
	{
		pos = n;
		dis = m - n + 1;
		return rand() % dis + pos;
	}
	else
	{
		pos = m;
		dis = n - m + 1;
		return rand() % dis + pos;
	}
}

void getguid(char *buf)
{
	GUID guid;
	CoCreateGuid(&guid);
	sprintf_s(buf, 128, "%08X%04X%04x%02X%02X%02X%02X%02X%02X%02X%02X"
		,guid.Data1
		, guid.Data2
		, guid.Data3
		, guid.Data4[0], guid.Data4[1]   
	, guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5]
	, guid.Data4[6], guid.Data4[7]
	);
}

int GetPort()
{
	int nNeed = 0;
#if IS_TEST
	nNeed = 5001;
#else
	char szBuf[256] = {0};
	getguid(szBuf);
	
	char szAllDigit[256] = {0};
	int nDigitCount = 0;
	for (size_t i = 0; i < strlen(szBuf); i++)
	{
		if (isdigit(szBuf[i]))
		{
			if (nDigitCount == 0 && szBuf[i] == '0')
				continue;
			szAllDigit[nDigitCount++] = szBuf[i];
		}
		else
			continue;
	}

	while (nDigitCount < 6)
	{
		szAllDigit[nDigitCount++] = '3';
	}

	char szNeedBuf[32] = {0};
	memcpy(szNeedBuf, szAllDigit, 5);
	nNeed = atoi(szNeedBuf);
	if (nNeed > 63000)
		nNeed /= 2;
#endif
	return nNeed;
}

int GetSpeedSize(char* info)
{
	char abuf[4] = {0};
	int n = 0;
	for (size_t i = 0; i < strlen(info); i++)
	{
		if (!isalpha(info[i]))
			abuf[n++] = info[i];
		else
			break;
	}

	return atoi(abuf);
}

BOOL CheckWindowsUP(const char* _username, const char* _password)
{
	HANDLE hUser;
	if (LogonUser(_username, ".", _password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hUser))
	{
		CloseHandle(hUser);
		return TRUE;
	}else
	{
		CloseHandle(hUser);
		return FALSE;
	}
	CloseHandle(hUser);
	return TRUE;
}



int Hex16(const TCHAR* ip)
{
	int a, b, c, d;
	sscanf_s(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	a <<= 24;
	b <<= 16;
	c <<= 8;
	a = a + b + c + d;

	return a;
}

BOOL UpdateDns(HWND hAdsl, const char* dns1 = "114.114.114.114", const char* dns2 = "223.5.5.5");

void SysListControl_(HWND hwnd)
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

		//lvi.iSubItem = 1;
		//lvi.pszText = _subitem;
		//WriteProcessMemory(process, _lvi, &lvi, sizeof(LVITEM), NULL);
		//SendMessage(hwnd, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)_lvi);

		//ReadProcessMemory(process, _item, item, 512, NULL);
		//ReadProcessMemory(process, _subitem, subitem, 512, NULL);

		//lvi.mask = LVIF_STATE;
		//lvi.iSubItem = 0;
		//lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
		//lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
		//WriteProcessMemory(process, _lvi, &lvi, sizeof(LVITEM), NULL);
		//SendMessage(hwnd, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)_lvi);

		//printf("%s - %s\n", item, subitem);
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

BOOL UpdateDns(HWND hAdsl, const char* dns1, const char* dns2)
{
	HWND hProButton = FindWindowEx(hAdsl, NULL, "Button", "属性(&O)");
	if (NULL == hProButton)
	{
		printf("获取 adsl 登陆界面 \"属性\" 按钮失败\n");
		goto error;
	}

	PostMessage(hProButton, BM_CLICK, 0, 0);
	int nFindTime = 0;
	HWND hAdsl_Pro = NULL;
	while (NULL == hAdsl_Pro && nFindTime < 30)
	{
		hAdsl_Pro = FindWindow(NULL, "adsl 属性");
		Sleep(1000 * 2);
		nFindTime += 2;
	}

	if (NULL == hAdsl_Pro)
	{
		printf("打开 \"adsl 属性\" 失败\n");
		goto error;
	}

	HWND hTabCtrl = FindWindowEx(hAdsl_Pro, NULL, "SysTabControl32", NULL);
	if (NULL == hTabCtrl)
	{
		printf("查找 \"SysTabControl32控件\" 失败\n");
		goto error;
	}

	int nTabControlCount = 0;
	nTabControlCount = TabCtrl_GetItemCount(hTabCtrl);
	if (0 == nTabControlCount)
	{
		printf("\"SysTabControl32控件\"中 行数 为 0\n");
		goto error;
	}

	for (int i = 0; i < nTabControlCount; i++)
	{
		TabCtrl_SetCurFocus(hTabCtrl, i);
	}

	HWND hNet = FindWindowEx(hAdsl_Pro, NULL, "#32770", "网络");
	if (NULL == hNet)
	{
		printf("查找 \"网络\" 选卡 失败\n");
		goto error;
	}

	HWND hListControl = FindWindowEx(hNet, NULL, "SysListView32", NULL);
	if (NULL == hListControl)
	{
		printf("查找 \"SysListView32\" 失败\n");
		goto error;
	}

	SysListControl_(hListControl);

	HWND hButtonPro = FindWindowEx(hNet, NULL, "Button", "属性(&R)");
	if (NULL == hButtonPro)
	{
		printf("查找 \"网络\" 选卡 中 \"属性\" 按钮失败\n");
		goto error;
	}
	PostMessage(hButtonPro, BM_CLICK, 0, 0);

	nFindTime = 0;
	HWND hTcpIp = NULL;
	while (NULL == hTcpIp && nFindTime < 30)
	{
		hTcpIp = FindWindow(NULL, "Internet 协议 (TCP/IP) 属性");
		Sleep(1000 * 2);
		nFindTime += 2;
	}

	if (NULL == hTcpIp)
	{
		printf("打开 \"Internet 协议 (TCP/IP) 属性\" 失败\n");
		goto error;
	}

	HWND hTabCtrl01 = FindWindowEx(hTcpIp, NULL, "SysTabControl32", NULL);
	if (NULL == hTabCtrl01)
	{
		printf("查找 \"SysTabControl32控件\" 失败\n");
		goto error;
	}

	nTabControlCount = 0;
	nTabControlCount = TabCtrl_GetItemCount(hTabCtrl01);
	if (0 == nTabControlCount)
	{
		printf("\"SysTabControl32控件\"中 行数 为 0\n");
		goto error;
	}

	for (int i = 0; i < nTabControlCount; i++)
	{
		TabCtrl_SetCurFocus(hTabCtrl01, i);
	}

	HWND hGren = FindWindowEx(hTcpIp, NULL, "#32770", "常规");
	if (NULL == hGren)
	{
		printf("查找 \"常规\" 选卡失败\n");
		goto error;
	}

	HWND hDns1 = FindWindowEx(hGren, NULL, "Button", "使用下面的 DNS 服务器地址(&E):");
	if (NULL == hDns1)
	{
		printf("查找 \"使用下面的 DNS 服务器地址(&E):\" 按钮 失败\n");
		goto error;
	}

	SendMessage(hDns1, BM_CLICK, 0, 0);
	Sleep(1000 * 1);

	HWND ip = NULL;
	if (NULL != dns1)
	{
		ip = FindWindowEx(hGren, NULL, "static", "首选 DNS 服务器(&P):");
		ip = FindWindowEx(hGren, ip, "SysIPAddress32", NULL);
		SendMessage(ip, IPM_SETADDRESS, 0, (LPARAM)Hex16(dns1));
		Sleep(1000);
	}

	if (NULL != dns2)
	{
		ip = FindWindowEx(hGren, ip, "static", "备用 DNS 服务器(&A):");
		ip = FindWindowEx(hGren, ip, "SysIPAddress32", NULL);
		SendMessage(ip, IPM_SETADDRESS, 0, (LPARAM)Hex16(dns2));
		Sleep(1000);
	}

	HWND hOk = FindWindowEx(hTcpIp, NULL, "Button", "确定");
	if (NULL == hOk)
	{
		printf("在 \"Internet 协议 (TCP/IP) 属性\" 界面 没有找到 \"确定\"按钮\n");
		goto error;
	}
	SendMessage(hOk, BM_CLICK, 0, 0);

	while (NULL != hTcpIp)
	{
		hTcpIp = FindWindow(NULL, "Internet 协议 (TCP/IP) 属性");
		Sleep(1000);
	}

	HWND hOKadsl = FindWindowEx(hAdsl_Pro, NULL, "Button", "确定");
	if (NULL == hOKadsl)
	{
		printf("在 \"adsl 属性\" 界面 没有找到 \"确定\"按钮\n");
		goto error;
	}
	SendMessage(hOKadsl, BM_CLICK, 0, 0);
	while (NULL != hAdsl_Pro)
	{
		hAdsl_Pro = FindWindow(NULL, "adsl 属性");
		Sleep(1000);
	}

	printf("设置dns成功\n");
	return TRUE;

error:
	printf("设置dns失败\n");
	return FALSE;
}