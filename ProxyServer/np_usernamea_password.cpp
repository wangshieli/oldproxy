#include "np_usernamea_password.h"
#include "np_tool_function.h"
#include "base64.h"

#define ITEM_01 "Proxy-Authorization: "
#define ITEM_02 "proxy-authorization: "
#define ITEM_03 "PROXY-AUTHORIZATION: "

char password[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'g', 
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', '+', '-', '*', '^', '%', '$', '#', '@', 
	'!', '?', '(', ')', '[', ']', '{', '}', '=', '_'
};

char username[] = {
	'w', 'x', 'y', 'z', 'A', 'B', 'C',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 
	'r', 's', 't', 'O', 'P', 'Q', 'R', 
	'S', 'T', 'U', 'V', 'W', 'X', '0', 
	'1', '2', '6', '7', '8', '9', 'a', 
	'b', 'c', 'd', 'e', 'f', 'g', 'h', 
	'i', 'g', 'u', 'v', 'D', 'E', 'F', 
	'G', 'H', 'I', 'J', 'K', 'L', 'M', 
	'N', 'Y', 'Z', '3', '4', '5'
};

#define COUNT_OF_USERNAME sizeof(username)/sizeof(char)
#define COUNT_OF_PASSWORD sizeof(password)/sizeof(char)
int ArrayIndexOfUsername[COUNT_OF_USERNAME];
int ArrayIndexOfPassword[COUNT_OF_PASSWORD];

void GetUsernameAndPassword()
{
#if IS_TEST
	memcpy(g_username, "administrator", strlen("administrator")+1);
	memcpy(g_password, "qwd123", strlen("qwd123")+1);
#else
	GetRandIndex(ArrayIndexOfUsername, COUNT_OF_USERNAME);
	int i = 0;
	for (i = 0; i < 8; i++)
	{
		g_username[i] = username[ArrayIndexOfUsername[i]];
	}
	i++;
	g_username[i] = '\0';
	GetRandIndex(ArrayIndexOfPassword, COUNT_OF_PASSWORD);
	for (i = 0; i < 12; i++)
	{
		g_password[i] = password[ArrayIndexOfPassword[i]];
	}
	i++;
	g_password[i] = '\0';
#endif
}

void spliteUP(char *p, char* username, char* password, int len)
{
	char *p1 = strstr(p, ":");
	if (p1 != NULL)
	{
		int n = p1 - p;
		memcpy(username, p, n);
		p = p1+1;
		spliteUP(p, username, password, len - n - 1);
	}else
	{
		memcpy(password, p, len);
	}
}

BOOL VerifyBasic(char *pInfo)
{
	char *pAuthorization = strstr(pInfo, ITEM_01);
	if (NULL == pAuthorization)
	{
		pAuthorization = strstr(pInfo, ITEM_02);
		if (NULL == pAuthorization)
		{
			pAuthorization = strstr(pInfo, ITEM_03);
			if (NULL == pAuthorization)
				return FALSE;
		}
	}
	char *pEnd = strstr(pAuthorization, "\r\n");
	if (NULL == pEnd)
		return FALSE;
	int tempLen = strlen("Proxy-Authorization: Basic ");
	int valueLen = pEnd - pAuthorization - tempLen;
	pAuthorization += tempLen;

	char pszSrc[32] = {0};
	int len_src = Base64Decode(pszSrc, pAuthorization, valueLen);
	//if (strncmp("YWRtaW5pc3RyYXRvcjpxd2QxMjM=", pAuthorization, strlen("YWRtaW5pc3RyYXRvcjpxd2QxMjM=")) == 0)
	//	return TRUE;
	char username[256] = {0};
	char password[256] = {0};
	spliteUP(pszSrc, username, password, valueLen);
	if (strcmp(username, g_username) != 0 || strcmp(password, g_password) != 0)
		return FALSE;
	return TRUE;
}

void Return401(SOCKET sClient)
{
	char *p = "HTTP/1.1 407 Proxy Authentication Required\r\n"
			  "Server: proxy-v%s\r\n"
		      "Proxy-Authenticate: Basic realm=\"proxy\"\r\n"
			  "Connection: Close\r\n"
			  "Proxy-Connection: Close\r\n"
			  "Content-Length: 0\r\n"
			  "\r\n";
	char retInfo[256];
	sprintf_s(retInfo, p, CURRENT_VERSION);
// Content-Type: text/html
// Content-Length: xxx
	send(sClient, retInfo, strlen(retInfo), 0);
}