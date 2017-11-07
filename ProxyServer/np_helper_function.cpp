#include "np_helper_function.h"
#include "np_tool_function.h"
#include "np_2_work_thread.h"
#include "np_usernamea_password.h"
#include "np_ward.h"
#include "cJSON.h"
#include "aes.h"
#include "np_schtasks.h"

BOOL InitSock2()
{
	int nRet = 0;
	WORD wVersionReuested = MAKEWORD(2, 2);
	WSADATA wsaData;

	nRet = WSAStartup(wVersionReuested, &wsaData);
	if ( 0 != nRet )
	{
		cout << "WSAStartup failed with error code : " << WSAGetLastError() << endl;
		return FALSE;
	}

	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		cout << "The Version OF WINSOCK is not matching" << endl;
		WSACleanup();
		return FALSE;
	}

	return TRUE;
}

int ConnectToServer(SOCKET& sock5001, char* ServerIP, unsigned short ServerPort)
{
	sock5001 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sock5001)
	{
		myprintf("Init Main Socket failed with error code : %d", WSAGetLastError());
		return -1;
	}

	int nRet = 0;
	int ulArgp = 1;
	nRet = ioctlsocket(sock5001, FIONBIO, (unsigned long*)&ulArgp);
	if (SOCKET_ERROR == nRet)
	{
		myprintf("ioctlsocket(1) failed with error code : %d", WSAGetLastError());
		return -1;
	}

	struct sockaddr_in sockaddr5001;
	ZeroMemory(&sockaddr5001, sizeof(sockaddr5001));
	sockaddr5001.sin_family = AF_INET;
	sockaddr5001.sin_addr.S_un.S_addr = inet_addr(ServerIP);
	sockaddr5001.sin_port = ntohs(ServerPort);

	nRet = connect(sock5001, (const sockaddr*)&sockaddr5001, sizeof(sockaddr5001));
	if (SOCKET_ERROR == nRet)
	{
		struct timeval tm;
		tm.tv_sec = 15;
		tm.tv_usec = 0;

		fd_set set;
		FD_ZERO(&set);
		FD_SET(sock5001, &set);
		if (select(sock5001+1, NULL, &set, NULL, &tm) <= 0)
		{
			myprintf("select time out");
			return 1;
		}else
		{
			int error = -1;
			int errorlen = sizeof(int);
			getsockopt(sock5001, SOL_SOCKET, SO_ERROR, (char*)&error, &errorlen);
			if (0 != error)
			{
				myprintf("Main Socket connect failed wiht error code : %d", WSAGetLastError());
				return -1;
			}
		}
	}

	ulArgp = 0;
	nRet = ioctlsocket(sock5001, FIONBIO, (unsigned long*)&ulArgp);
	if (SOCKET_ERROR == nRet)
	{
		myprintf("ioctlsocket(0) failed with error code : %d", WSAGetLastError());
		return -1;
	}

	return 0;
}

BOOL SendHostIdToServer(SOCKET TargetSocket, char* _pHostId)
{
	char* pTemp = "IAMPROXYBEGINhost_id=%sIAMPROXYEND";
	char* pSendBuf = new char[256];
	sprintf(pSendBuf, pTemp, _pHostId);

	if( !SendData(TargetSocket, pSendBuf))
	{
		delete pSendBuf;
		return FALSE;
	}
	
	if (pSendBuf != NULL)
	{
		delete pSendBuf;
		pSendBuf = NULL;
	}else
	{
		myprintf("SendHostIdToServer memory error");
		return FALSE;
	}

	return TRUE;
}

BOOL RecvHostIdReturnFromServer(SOCKET TargetSocket, char* pRetInfo)
{
	int nRet = 0;
	int RecvLen = strlen("Proxy5001IsRead");
	int len = 0;
	while (len < RecvLen)
	{
		nRet = recv(TargetSocket, pRetInfo + len, RecvLen - len, 0);
		if (SOCKET_ERROR == nRet)
		{
			myprintf("RecvHostIdReturnFromServer failed with error code : %d", WSAGetLastError());
			return FALSE;
		}
		len += nRet;
	}
	
	return TRUE;
}

BOOL Send6086HostIdToServer(SOCKET TargetSocket, char* _pHostId)
{
	char* pTemp = "IAMPROXY6086BEGINhost_id=%sIAMPROXY6086END";
	char* pSendBuf = new char[256];
	sprintf(pSendBuf, pTemp, _pHostId);

	if( !SendData(TargetSocket, pSendBuf))
		return FALSE;

	if (pSendBuf != NULL)
	{
		delete pSendBuf;
		pSendBuf = NULL;
	}else
	{
		myprintf("Send6086HostIdToServer memory error");
		return FALSE;
	}

	return TRUE;
}

BOOL Send6086RedialOKToServer(SOCKET TargetSocket, char* pinfo, char* _pHostId)
{
	char* pTemp = "IAMPROXY6086BEGINinfo=%shost_id=%sIAMPROXY6086END";
	char* pSendBuf = new char[256];
	sprintf(pSendBuf, pTemp, pinfo, _pHostId);

	if( !SendData(TargetSocket, pSendBuf))
		return FALSE;

	if (pSendBuf != NULL)
	{
		delete pSendBuf;
		pSendBuf = NULL;
	}else
	{
		myprintf("Send6086HostIdToServer memory error");
		return FALSE;
	}

	return TRUE;
}

BOOL RecvNewPortInfoFromServer(SOCKET TargetSocket, char** pRetInfo)
{
	int nRet = 0;
	unsigned long WaitBytes = 0;
	char* pRecvBuf = new char[128];
	memset(pRecvBuf, 0x00, 128);

	WaitBytes = 2;
	int RecvLen  = 0;
	nRet = recv(TargetSocket, pRecvBuf, WaitBytes, 0);
	if (nRet != SOCKET_ERROR && nRet != 0) // recv返回0 说明网络连接中断了
	{
		RecvLen += nRet;
		while (RecvLen < WaitBytes)
		{
			nRet = recv(TargetSocket, pRecvBuf + RecvLen, WaitBytes - RecvLen, 0);
			if (SOCKET_ERROR == nRet || nRet == 0)
			{
				delete pRecvBuf;
				myprintf("RecvNewPortInfoFromServer recv failed with error code : %d", WSAGetLastError());
				if (10054 == WSAGetLastError() || 10060 == WSAGetLastError())
					throw new int(10);
				return FALSE;
			}
			RecvLen += nRet;
		}

		int nMsgLen = pRecvBuf[0] * 256 + pRecvBuf[1];
		if (nMsgLen <= 0 || nMsgLen > 256)
		{
			delete pRecvBuf;
			throw new int(10);
		}
		if (nMsgLen > 128)
		{
			delete pRecvBuf;
			int nByteSize = MY_ALIGN(nMsgLen + 1, 8);// +1是为了保证以'\0'结尾
			pRecvBuf = new char[nByteSize];
			memset(pRecvBuf, 0x00, nByteSize);
		}
		RecvLen = 0;
		while (RecvLen < nMsgLen)
		{
			nRet = recv(TargetSocket, pRecvBuf + RecvLen, nMsgLen - RecvLen, 0);
			if (SOCKET_ERROR == nRet || nRet == 0)
			{
				myprintf("RecvNewPortInfoFromServer recv failed with error code : %d", WSAGetLastError());
				delete pRecvBuf;
				pRecvBuf = 0;
				if (10054 == WSAGetLastError() || 10060 == WSAGetLastError())
					throw new int(10);
				return FALSE;
			}
			RecvLen += nRet;
		}
		*pRetInfo = pRecvBuf;
	}else
	{
		delete pRecvBuf;
		myprintf("recv message len failed with error code : %d", WSAGetLastError());
		if (10054 == WSAGetLastError() || 10060 == WSAGetLastError() || 0 == WSAGetLastError())
			throw new int(10);
		return FALSE;
	}

	return TRUE;
}

BOOL ProxyThread(RECVPARAM* lpParameter)
{
//	cout << "开始处理数据" << endl;
	char* pRecvBuf = NULL;
	char SendBuf[MAXSIZE] = {0};
	if (!RecvRequest(lpParameter->ClientSocket, &pRecvBuf))
	{
		closesocket(lpParameter->ClientSocket);
		return FALSE;
	}
//	cout <<"接收到的数据:" <<pRecvBuf  << endl;
	int recvlen = strlen(pRecvBuf);
	if (strncmp("CONNECT ", pRecvBuf, 8) == 0)// """"""!!!!!!!!!!!!!!!
	{
		if (ConnectServer(lpParameter->ServerSocket, pRecvBuf, recvlen) < 0)// 说明连接目标服务器失败，直接关闭此次连接
		{
			goto error;
		}
		if (PreResponse(lpParameter) < 0)
		{
			goto error;
		}
		if (ExchangeData(lpParameter) != 0)
		{
			goto error;
		}
	}else
	{
		if (!SendWebRequest(lpParameter, SendBuf, pRecvBuf, recvlen))
		{
			goto error;
		}
	}
//	cout << "ok1111" << endl;

error:
	if(lpParameter->ClientSocket != INVALID_SOCKET)
		shutdown(lpParameter->ClientSocket, 2);
//	lpParameter->ServerSocket = INVALID_SOCKET;
	lpParameter->ClientSocket = INVALID_SOCKET;
error1:
	if (lpParameter->ServerSocket != INVALID_SOCKET)
		shutdown(lpParameter->ServerSocket, 2);
//	lpParameter->ClientSocket = INVALID_SOCKET;
	lpParameter->ServerSocket = INVALID_SOCKET;
	if (pRecvBuf != NULL)
	{
		free(pRecvBuf);
		pRecvBuf = NULL;
	}
	return FALSE;

}

BOOL ProxyThread01(RECVPARAM* lpParameter)
{
	char* RecvBuf = (char*)malloc(MAXSIZE);
	memset(RecvBuf, 0x00, MAXSIZE);
	char* SendBuf = NULL;

	int retval = RecvRequest01(lpParameter->ClientSocket, &RecvBuf, MAXSIZE);
	if (retval == SOCKET_ERROR || retval == 0) {goto error2;}
#if USE_BASIC
	if (!VerifyBasic(RecvBuf))
	{
		Return401(lpParameter->ClientSocket);
		goto error2;
	}
#endif
	if ( strncmp("CONNECT ",RecvBuf,8) == 0) // CONNECT 代理
	{
		myprintf("Connect Request connection");
		if (ConnectServer( ((RECVPARAM*)lpParameter)->ServerSocket, RecvBuf, retval) < 0 ) {goto error2;}
		if (PreResponse((RECVPARAM*)lpParameter )< 0 ) {goto error;}
		ExchangeData((RECVPARAM*)lpParameter);	// 连接已经建立，只需交换数据
	}
	else	// 直接转发
	{
		size_t len = _msize(RecvBuf);
		SendBuf = (char*)malloc(len);
		memset(SendBuf, 0x00, len);
		SendWebRequest((RECVPARAM*)lpParameter, SendBuf, RecvBuf, retval);
	}

	free(RecvBuf);
	RecvBuf = NULL;
	if (NULL != SendBuf)
	{
		free(SendBuf);
		SendBuf = NULL;
	}
	closesocket(lpParameter->ServerSocket);
	closesocket(lpParameter->ClientSocket);
	return TRUE;
	

error:
	closesocket(lpParameter->ServerSocket);
error2:
	closesocket(lpParameter->ClientSocket);
	free(RecvBuf);
	RecvBuf = NULL;
	if (NULL != SendBuf)
	{
		free(SendBuf);
		SendBuf = NULL;
	}
	//释放
	return FALSE;

}

BOOL SendWebRequest(RECVPARAM *lpParameter, char *SendBuf, char *RecvBuf, int DataLen)
{
	char HostName[512] = {0};
	int Port = DEFAULTPORT;

	if (strncmp("POST ", RecvBuf, 5) == 0)
	{
		if (strncmp("/test_up_spd", RecvBuf + 5, 12) == 0)
		{
			int nRecv = 0;
			char tRcv[MAXSIZE];
			char* pLength = strstr(RecvBuf, "Content-Length: ");
			if (NULL == pLength)
			{
				pLength = strstr(RecvBuf, "content-length: ");
				if (NULL == pLength)
				{
					pLength = strstr(RecvBuf, "CONTENT-LENGTH: ");
					if(NULL == pLength)
						return FALSE;
				}
			}
			int nTempLength = strlen("content-length: ");
			char* pEnd = strstr(pLength, "\r\n");
			char* pStart = strstr(RecvBuf, "\r\n\r\n");
			if (NULL == pEnd || NULL == pStart)
			{
				myprintf("测速请求格式错误");
				return FALSE;
			}
			char cTotal[32] = {0};
			memcpy(cTotal, pLength + nTempLength, pEnd - pLength - nTempLength);
			int nCleintTotal = atoi(cTotal);
			myprintf("客户端发送的所有数据%d", nCleintTotal);
			int nHasRecvData = DataLen - (pStart - RecvBuf) - strlen("\r\n\r\n");
			if (nHasRecvData == nCleintTotal)
			{
				send(lpParameter->ClientSocket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
				myprintf("上传接收完毕，接收字节数:%d", nHasRecvData);
				return TRUE;
			}
			myprintf("已经接收的数据%d", nHasRecvData);
			int nTotal = nCleintTotal - nHasRecvData;
			myprintf("还应该接收的数据数量:%d", nTotal);
			int nCount = 0;
		//	myprintf("请求内容%s", RecvBuf);
			do
			{
				nRecv = recv(lpParameter->ClientSocket, tRcv, MAXSIZE, 0);
				if (SOCKET_ERROR == nRecv || 0 == nRecv)
				{
					myprintf("测速失败:%d", WSAGetLastError());
					return FALSE;
				}
				nCount += nRecv;
				if ((nTotal -= nRecv) <= 0)
				{
					myprintf("接收完毕");
					break;
				}
				//myprintf("接收的字节数%d", nRecv);
			} while (nRecv != SOCKET_ERROR && nRecv != 0 );
			send(lpParameter->ClientSocket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
			myprintf("上传接收完毕，接收字节数:%d", nCount+nHasRecvData);
			
			return TRUE;
		}		
	}else if (strncmp("GET ", RecvBuf, 4) == 0)
	{
		if (strncmp("/test_down_spd.", RecvBuf + 4, 15) == 0)
		{
			int n = GetSpeedSize(RecvBuf+19);

			char buf[SPEED_DATA];
			memset(buf, '0', SPEED_DATA);

			int nCount = n * SPEED_DATA * 1024;
			int nHasSend = 0;
			int nPerSend = 0;
			int nNeedSend = 0;

			while (nHasSend < nCount)
			{
				nNeedSend = (nCount - nHasSend)/1024 ? 1024 : (nCount - nHasSend);
				nPerSend = send(lpParameter->ClientSocket, buf, nNeedSend, 0);
				if (SOCKET_ERROR == nPerSend || 0 == nPerSend)
					return FALSE;
				nHasSend += nPerSend;
			}
			return TRUE;
		}
	}

	char * line = strstr(RecvBuf,"\r\n");	// 一定有 因为request head 已经判定了\r\n
	
	char * UrlBegin = strchr(RecvBuf,' ');
	if (!UrlBegin) {return FALSE;}
	int nMsgLen = line - RecvBuf;
	char* purlpath = new char[nMsgLen + 4];
	memcpy(purlpath, RecvBuf, nMsgLen);
	purlpath[nMsgLen] = '\0';
	PostThreadMessage(g_nMessageThreadNo, WM_MESSAGE_COUT_MESSAGE, (LPARAM)purlpath, NULL);

	char * PathBegin = strchr(UrlBegin+1+HEADLEN,'/');
	if (!PathBegin) {return FALSE;}

	
	char * PortBegin = (char*)(memchr(UrlBegin+1+HEADLEN,':',PathBegin-UrlBegin-HEADLEN) );
	char * HostEnd = PathBegin;
	if (PortBegin)	// 有端口
	{
		HostEnd = PortBegin;
		char BufPort[64] = {0};
		memcpy(BufPort,PortBegin+1,PathBegin-PortBegin-1);
		Port = atoi(BufPort);
	}
	memcpy(HostName,UrlBegin+1+HEADLEN, HostEnd-UrlBegin-1-HEADLEN);

	char lineBuf[0x1000] = {0};
	int leng = line-RecvBuf;
	if (leng < sizeof(lineBuf) ) 
	{
		memcpy(lineBuf,RecvBuf,leng);
	} else {
		const static int lenc = 50;
		memcpy(lineBuf,RecvBuf,lenc);
		strcpy(lineBuf+lenc," ... ");
		memcpy(lineBuf+lenc+5,line-16,16);
	}

	if(!InitHost(&(lpParameter->ServerSocket),HostName,Port)) return FALSE;

	memcpy(SendBuf,RecvBuf, UrlBegin-RecvBuf+1 ); // 填写method
	memcpy(SendBuf+(UrlBegin-RecvBuf)+1,PathBegin,RecvBuf+DataLen-PathBegin);	// 填写剩余内容
	
	char * HTTPTag = strstr(SendBuf+(UrlBegin-RecvBuf)+1," HTTP/1.1\r\n");
	if (HTTPTag) { HTTPTag[8] = '0'; }	// 强制使用http/1.0 防止Keep-Alive

	size_t TotalLen = UrlBegin+1+DataLen-PathBegin;

	//cout<<"转发到web的是："<<endl<<SendBuf<<endl;

	if( SendData1(lpParameter->ServerSocket,SendBuf, TotalLen) <= 0)
	{ 
		myprintf("SendData1 failed with error code : %d", WSAGetLastError());
		return FALSE; 
	}

	if (ExchangeData( lpParameter ) != 0)
	{
		myprintf("ExchangeData failed with error code : %d", WSAGetLastError());
		return FALSE;
	}

	return TRUE;
}

#define TOTAL_PATH "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"

#define ADMIN_ITEM "AutoAdminLogon"
#define USER_ITEM "DefaultUserName"
#define PASSWORD_ITEM "Defaultpassword"

HKEY hWinlogonKey;

void DeleteValue()
{
	RegDeleteValue(hWinlogonKey, ADMIN_ITEM);
	RegDeleteValue(hWinlogonKey, USER_ITEM);
	RegDeleteValue(hWinlogonKey, PASSWORD_ITEM);
}

BOOL SetValue(const char* key, const char* value)
{
	char ovalue[MAX_PATH];
	DWORD plen = MAX_PATH;
	if (ERROR_SUCCESS == RegQueryValueEx(hWinlogonKey, key, NULL, NULL, (LPBYTE)ovalue, &plen))
	{
		if (strcmp(value, ovalue) != 0)
		{
			printf("%s\n", ovalue);
			if (ERROR_SUCCESS != RegDeleteValue(hWinlogonKey, key))
			{
				printf("删除注册表失败%d\n", GetLastError());
				return FALSE;
			}

			if (ERROR_SUCCESS != RegSetValueEx(hWinlogonKey, key, 0, REG_SZ, (const unsigned char*)value, strlen(value)+1))
			{
				printf("更新注册表失败%d\n", GetLastError());
				return FALSE;
			}
		}
	}else
	{
		if (ERROR_SUCCESS != RegSetValueEx(hWinlogonKey, key, 0, REG_SZ, (const unsigned char*)value, strlen(value)+1))
		{
			printf("更新注册表失败%d\n", GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}

BOOL Winlogon(const char* _username, const char* _password)
{
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TOTAL_PATH, 0, KEY_ALL_ACCESS, &hWinlogonKey))
	{
		if (!SetValue(ADMIN_ITEM, "1"))
		{
			DeleteValue();
			RegCloseKey(hWinlogonKey);
			return FALSE;
		}
		if (!SetValue(USER_ITEM, _username))
		{
			DeleteValue();
			RegCloseKey(hWinlogonKey);
			return FALSE;
		}
		if (!SetValue(PASSWORD_ITEM, _password))
		{
			DeleteValue();
			RegCloseKey(hWinlogonKey);
			return FALSE;
		}
	}else
		return FALSE;
	return TRUE;
}

BOOL GetHostInfo()
{
	char urlbuf[256] = {0};
	sprintf(urlbuf, GETHOSTINFO, pHostId);

	char PostDataNew[256] = {0};
	char* pTempNew = "app_secret=F$~((kb~AjO*xgn~&host_id=%s";
	sprintf(PostDataNew, pTempNew, pHostId);

	CURLRECVDATA* pCurlRecvData = new CURLRECVDATA;
	pCurlRecvData->pRecvData = (char*)malloc(512);
	if (!SendPostDataAndRecvBackInfo(urlbuf, PostDataNew, curl_recvbackinfo_function, pCurlRecvData))
	{
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return FALSE;
	}

//	printf("%s\n", pCurlRecvData->pRecvData);

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
		goto error;
	int nSuccess = cjSuccess->valueint;
	if (0 == nSuccess)
	{
		goto error;
	}
	cJSON* pArray = cJSON_GetObjectItem(root, "data");
	char* pusername = cJSON_GetObjectItem(pArray, "username")->valuestring;
	char* bpassword = cJSON_GetObjectItem(pArray, "password")->valuestring;
	if (NULL == pusername || strcmp(pusername, "") == 0 || NULL == bpassword || strcmp(bpassword, "") == 0)
	{
		printf("获取的主机信息中，用户名或密码为空\n");
		cJSON_Delete(root);
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return TRUE;
	}

	char *ppassword = NULL;
	GetPassWord(bpassword, &ppassword);
	if (NULL != ppassword)
	{
		//printf("%s\n", ppassword);
		///////////////////////////
		if (!CheckWindowsUP(pusername, ppassword))
		{
			printf("需要修改密码\n");
			char* pInfo = NULL;
			char cCmd[256];
			sprintf_s(cCmd, "net user %s %s", pusername, ppassword);
			Execmd(cCmd, &pInfo);
			printf("%s\n", pInfo);
			free(pInfo);

			RunClear(pusername, ppassword);

			if (!Winlogon(pusername, ppassword))
				printf("注册表修改失败\n");
		}

		free(ppassword);
	}
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

BOOL GetHostInfo_start()
{
	char urlbuf[256] = {0};
	sprintf(urlbuf, GETHOSTINFO, pHostId);

	char PostDataNew[256] = {0};
	char* pTempNew = "app_secret=F$~((kb~AjO*xgn~&host_id=%s";
	sprintf(PostDataNew, pTempNew, pHostId);

	CURLRECVDATA* pCurlRecvData = new CURLRECVDATA;
	pCurlRecvData->pRecvData = (char*)malloc(512);
	if (!SendPostDataAndRecvBackInfo(urlbuf, PostDataNew, curl_recvbackinfo_function, pCurlRecvData))
	{
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return FALSE;
	}

//	printf("%s\n", pCurlRecvData->pRecvData);

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
		goto error;
	int nSuccess = cjSuccess->valueint;
	if (0 == nSuccess)
	{
		goto error;
	}
	cJSON* pArray = cJSON_GetObjectItem(root, "data");
	char* pusername = cJSON_GetObjectItem(pArray, "username")->valuestring;
	char* bpassword = cJSON_GetObjectItem(pArray, "password")->valuestring;
	if (NULL == pusername || strcmp(pusername, "") == 0 || NULL == bpassword || strcmp(bpassword, "") == 0)
	{
		printf("获取的主机信息中，用户名或密码为空\n");
		cJSON_Delete(root);
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return TRUE;
	}
	char *ppassword = NULL;
	GetPassWord(bpassword, &ppassword);
	if (NULL != ppassword)
	{
//		printf("%s\n", ppassword);
		///////////////////////////
		if (!CheckWindowsUP(pusername, ppassword))
		{
			printf("需要修改密码\n");
			char* pInfo = NULL;
			char cCmd[256];
			sprintf_s(cCmd, "net user %s %s", pusername, ppassword);
			Execmd(cCmd, &pInfo);
			printf("%s\n", pInfo);
			free(pInfo);
		}

		if (!Winlogon(pusername, ppassword))
			printf("注册表修改失败\n");

		RunClear(pusername, ppassword);

		free(ppassword);
	}

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

BOOL SendHostInfo2API(pCurlWriteFunction _pCurlWriteFunction)
{
	char urlbuf[256] = {0};
	sprintf(urlbuf, POSTHOSTINFOURL2, pHostId);

	char PostDataNew[256] = {0};
	if (!bOldMode)
	{
		char* pTempNew = "value={\"host_id\":\"%s\",\"ip\":null,\"sip\":\"%s\",\"port\":%s,\"u\":\"%s\",\"p\":\"%s\"}";
		EnterCriticalSection(&g_csCurrentPort);
		sprintf(PostDataNew, pTempNew, pHostId, vip[pArrayIndex[nIndex]], g_pCurrentPort, g_username, g_password);
		LeaveCriticalSection(&g_csCurrentPort);
	}else
	{
		char proxy_ip[32] = {0};
#if IS_TEST
		memcpy(proxy_ip, REDAIL_IP, strlen(REDAIL_IP));
#else
		if (!GetAdslIp(proxy_ip))
			return FALSE;
#endif
		char* pTempNew = "value={\"host_id\":\"%s\",\"ip\":\"%s\",\"sip\":null,\"port\":%s,\"u\":\"%s\",\"p\":\"%s\"}";
		EnterCriticalSection(&g_csCurrentPort);
		sprintf(PostDataNew, pTempNew, pHostId, proxy_ip, g_pCurrentPort, g_username, g_password);
		LeaveCriticalSection(&g_csCurrentPort);
	}

	CURLRECVDATA* pCurlRecvData = new CURLRECVDATA;
	pCurlRecvData->pRecvData = (char*)malloc(512);
	if (!SendPostDataAndRecvBackInfo(urlbuf, PostDataNew, curl_recvbackinfo_function, pCurlRecvData))
	{
		free(pCurlRecvData->pRecvData);
		delete pCurlRecvData;
		pCurlRecvData = NULL;
		return FALSE;
	}

	myprintf("%s", pCurlRecvData->pRecvData);

	free(pCurlRecvData->pRecvData);
	delete pCurlRecvData;
	pCurlRecvData = NULL;

	return TRUE;
}

struct app_info
{
	char* pCode;
	char* pVersion;
	char* pMd5;
	char* pDUrl;
};

BOOL new_download()
{
	char GetVersionInfoCMD[256];
#if IS_TEST
	sprintf(GetVersionInfoCMD, NEW_EXE_INFO_LOCAL, "bj008");
#else
	sprintf(GetVersionInfoCMD, NEW_EXE_INFO, pHostId);
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

	app_info arrAppInfo[3];
	ZeroMemory(arrAppInfo, sizeof(arrAppInfo));
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
	int iCount = cJSON_GetArraySize(pArray);
	for (int i = 0; i < iCount; i++)
	{
		//cJSON *pItem = cJSON_GetArrayItem(pArray, i);
		cJSON *pItem = NULL;
		switch (i)
		{
		case 0:
			{
				pItem = cJSON_GetObjectItem(pArray, "proxy-monitor");
			}
			break;

		case 1:
			{
				pItem = cJSON_GetObjectItem(pArray, "app-proxy");
			}
			break;

		case 2:
			{
				pItem = cJSON_GetObjectItem(pArray, "app-upgrade");
			}
			break;
		default:
			break;
		}
		if (NULL == pItem)
		{
			printf("获取到的版本信息格式错误\n");
			cJSON_Delete(root);
			free(pCurlRecvData->pRecvData);
			delete pCurlRecvData;
			pCurlRecvData = NULL;
			return FALSE;
		}
		arrAppInfo[i].pCode = cJSON_GetObjectItem(pItem, "code")->valuestring;
		arrAppInfo[i].pVersion = cJSON_GetObjectItem(pItem, "version")->valuestring;
		arrAppInfo[i].pMd5 = cJSON_GetObjectItem(pItem, "ext_md5")->valuestring;
		arrAppInfo[i].pDUrl = cJSON_GetObjectItem(pItem, "update_url")->valuestring;
	}

	for (int i = 0; i < 2; i++)
	{
		switch (i)
		{
		case 0:
			{
				if (CompareVersion(g_pwatchversion, arrAppInfo[i].pVersion))
				{
					break;
				}else
				{
					closepwatch();
					sprintf_s(g_pwatchversion, 8, "%s", arrAppInfo[i].pVersion);
					char pwatch_path[MAX_PATH];
					sprintf_s(pwatch_path, "%s\\pwatch-v%s.exe", WARD_FOLDER, arrAppInfo[i].pVersion);
					char FormatingDownLoadUrl[256] = {0};
					UrlFormating(arrAppInfo[i].pDUrl, "\\", FormatingDownLoadUrl);
					myprintf("下载pwatch...");
					do
					{
						if (!GetDownFile(FormatingDownLoadUrl, pwatch_path))
						{
							cJSON_Delete(root);
							free(pCurlRecvData->pRecvData);
							delete pCurlRecvData;
							pCurlRecvData = NULL;
							myprintf("下载失败");
							return FALSE;
						}
					} while (!needdownload(pwatch_path, arrAppInfo[i].pMd5));
					myprintf("下载完成");
					do
					{
						Sleep(1000);
						ShellExecute(0, "open", pwatch_path, NULL, NULL, SW_SHOWNORMAL);
						Sleep(1000);
					}while(!isOnWorking());
				}
			}
			break;

		case 1:
			{
				char LocalVersion[32] = CURRENT_VERSION;
				if (CompareVersion(LocalVersion, arrAppInfo[i].pVersion))
				{
					break;
				}else
				{
					int nStep = 1;
					char szPath[MAX_PATH];
					char TempPath[MAX_PATH];
					memset(TempPath, 0x00, MAX_PATH);
					int TempLen = GetTempPath(MAX_PATH, TempPath);

					do
					{
						if (1 == nStep)
						{							
							_makepath(szPath, NULL, TempPath, "proxy.exe", NULL);
							myprintf("下载proxy...");
						}
						else
						{
							_makepath(szPath, NULL, TempPath, "upgrade.exe", NULL);
							myprintf("下载upgrade...");
						}

						char FormatingDownLoadUrl[256] = {0};
						UrlFormating(arrAppInfo[nStep].pDUrl, "\\", FormatingDownLoadUrl);

						while (!needdownload(szPath, arrAppInfo[nStep].pMd5))
						{
							if (!GetDownFile(FormatingDownLoadUrl, szPath))
							{
								cJSON_Delete(root);
								free(pCurlRecvData->pRecvData);
								delete pCurlRecvData;
								pCurlRecvData = NULL;
								myprintf("下载失败");
								return FALSE;
							}
						}
						myprintf("下载完成");

						if (2 == nStep)
						{
							closepwatch01();

							char LocalPath[MAX_PATH] = {0};
							GetFileFullPath(LocalPath);

							char ExeParam[1024] = {0};
							char *pSrc = "%d \"%s\" \"%s\"";
							DWORD pid = GetCurrentProcessId();
							char proxy_source_path[MAX_PATH];
							_makepath(proxy_source_path, NULL, TempPath, "proxy.exe", NULL);
							sprintf(ExeParam, pSrc, pid, proxy_source_path, LocalPath);

							ShellExecute(0, "open", szPath, ExeParam, NULL, SW_SHOWNORMAL);
						}
					}while(nStep++ < 2);
				}
			}
			break;
		default:
			break;
		}
	}

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

char ClientPath[MAX_PATH] = {0}; // 用来保存下载的代理程序的路径
// nMode :1 下载client.exe  2 下载upgrade.exe
BOOL CheckVersionAndDownFile(char* pszItem, int nMode)
{
	char szPath[MAX_PATH];
	char TempPath[MAX_PATH];
	memset(TempPath, 0x00, MAX_PATH);
	int TempLen = GetTempPath(MAX_PATH, TempPath);

	char LocalVersion[32] = CURRENT_VERSION;
	char GetVersionInfoCMD[256];

	//char *pHostId = NULL;
	//if (!GetHostId(&pHostId))
	//	return FALSE;
	sprintf(GetVersionInfoCMD, PROXYEXE, pHostId);
	//if (NULL != pHostId)
	//{
	//	delete pHostId;
	//	pHostId = NULL;
	//}
	CURLRECVDATA* pCurlRecvData = new CURLRECVDATA;
	pCurlRecvData->pRecvData = (char*)malloc(512);
	if (nMode == 1)
	{
		bNeedDownUpgrade = FALSE;
		if (!SendPostDataAndRecvBackInfo1(POSTURL, GetVersionInfoCMD, curl_recvbackinfo_function, pCurlRecvData))
		{
			goto label01;
		}
	//	cout << pCurlRecvData->pRecvData << endl;
	}else if(nMode == 2)
	{
		if (!SendPostDataAndRecvBackInfo1(POSTURL, UPGRADEEXE, curl_recvbackinfo_function, pCurlRecvData))
		{
			goto label01;
		}
	}

	char newVersion[32] = {0};
	char downloadUrl[256] = {0};
	char md5Value[256] = {0};
	if(!GetJsonValue(pCurlRecvData->pRecvData, "data", newVersion))
	{
		goto label01;
	}
	if(!GetJsonValue(pCurlRecvData->pRecvData, "download_url", downloadUrl))
	{
		goto label01;
	}
	if(!GetJsonValue(pCurlRecvData->pRecvData, "md5", md5Value))
	{
		goto label01;
	}
	
	free(pCurlRecvData->pRecvData);
	delete pCurlRecvData;
	pCurlRecvData = NULL;

	if (nMode == 1 && CompareVersion(LocalVersion, newVersion))
	{
		myprintf("local is the newest version");
		return TRUE;
	}

	char FormatingDownLoadUrl[256] = {0};
	UrlFormating(downloadUrl, "\\", FormatingDownLoadUrl);

	_makepath(szPath, NULL, TempPath, pszItem, NULL);

	if (nMode == 1)
	{
		bNeedDownUpgrade = TRUE;
		memset(ClientPath, 0x00, MAX_PATH);
		memcpy(ClientPath, szPath, strlen(szPath));
	}
	myprintf("Downloading...");
	
	while (!needdownload(szPath, md5Value))
	{
		if (!GetDownFile(FormatingDownLoadUrl, szPath))
		{
			myprintf("Download failed");
			return FALSE;
		}
	}
	myprintf("now the version is the newest");

	if (nMode == 2)
	{
		try
		{
			vector<string> exename;
			exename.clear();
			Get_Ward_Exe_Name(exename);
			closeAllWardExe(exename);

			char LocalPath[MAX_PATH] = {0};
			GetFileFullPath(LocalPath);

			char ExeParam[1024] = {0};
			char *pSrc = "%d \"%s\" \"%s\"";
			DWORD pid = GetCurrentProcessId();
			sprintf(ExeParam, pSrc, pid, ClientPath, LocalPath);
		
			ShellExecute(0, "open", szPath, ExeParam, NULL, SW_SHOWNORMAL);
		}
		catch(...)
		{
			myprintf("upgrade failed");
			return FALSE;
		}
	}

	return TRUE;

label01:
	free(pCurlRecvData->pRecvData);
	delete pCurlRecvData;
	pCurlRecvData = NULL;
//	system("pause");
	return FALSE;
}

BOOL GetDownFile(char* pUrl, char* pFileName)
{
	CURL *curl;
	CURLcode res;
	FILE* fptr;
	struct curl_slist *http_header = NULL;

	if ((fptr = fopen(pFileName,"wb")) == NULL)
	{
		fprintf(stderr,"fopen file error:%s\n",pFileName);
		return FALSE;
	}

	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr,"curl init failed\n");
		fclose(fptr);
		return FALSE;
	}

	curl_easy_setopt(curl,CURLOPT_URL,pUrl); //url地址
//	curl_easy_setopt(curl, CURLOPT_USERPWD, "administrator:Disikeji123");
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_flie); //对返回的数据进行操作的函数地址
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,fptr); //这是write_data的第四个参数值
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, FALSE);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);// 超时单位s
//	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

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
		fclose(fptr);
		curl_easy_cleanup(curl);
		return FALSE;
	}
	fclose(fptr);
	curl_easy_cleanup(curl);
	return TRUE;
}




BOOL Curl_GET(const char* pUrl, pCurlWriteFunction pCWFunc, void* pResponseInfo)
{
	CURL* pCurl = NULL;
	CURLcode code = CURLE_OK;

	pCurl = curl_easy_init();
	if (NULL == pCurl)
	{
		printf("Curl_GET curl_easy_init failed\n");
		return FALSE;
	}

	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, FALSE);
	curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, FALSE);

	curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(pCurl, CURLOPT_URL, pUrl);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, pCWFunc);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, pResponseInfo);
	curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 600 * 3);

	code = curl_easy_perform(pCurl);
	if (CURLE_OK != code)
	{
		printf("Curl_GET curl_easy_perform failed with error: %d\n", code);
		printf("Error code type ：CURLcode");
		curl_easy_cleanup(pCurl);
		return FALSE;
	}

	curl_easy_cleanup(pCurl);

	return TRUE;
}

size_t CWFunc_DownloadFile(void* pInfo, size_t size, size_t nmemb, void* file)
{
	FILE* pFile = (FILE*)file;
	fwrite(pInfo, 1, size * nmemb, pFile);

	return size * nmemb;
}

BOOL Curl_DownloadFile(const char* url, const char* downPath)
{
	FILE* pFile = NULL;
	fopen_s(&pFile, downPath, "wb");
	if (NULL == pFile)
	{
		printf("打开文件下载目录失败\n");
		return FALSE;
	}
	if (!Curl_GET(url, CWFunc_DownloadFile, pFile))
	{
		printf("文件下载失败\n");
		goto error;
	}
	printf("文件 %s 下载完成\n", downPath);

	fclose(pFile);

	return TRUE;

error:
	if (NULL != pFile)
		fclose(pFile);
	return FALSE;
}

BOOL DeleteDirectoryByFullName(const char* pFilename)
{
	char* pCmdRetinfo = NULL;
	char cCmd[MAX_PATH] = { 0 };
	sprintf_s(cCmd, "rd /s /q %s", pFilename);
	if (!Execmd(cCmd, &pCmdRetinfo))
	{
		printf("执行命令： %s 时失败\n", cCmd);
		return FALSE;
	}

	if (NULL != pCmdRetinfo)
	{
		printf("%s\n", pCmdRetinfo);
		free(pCmdRetinfo);
	}

	return TRUE;
}