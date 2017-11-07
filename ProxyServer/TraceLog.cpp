/*
* Copyright (c) 2013, Beijing Watertek Information Technology Co.,Ltd.
* All rights reserved.
*
* �ļ�����: TraceLog.cpp
* �ļ�ժҪ: ������־
*
* ��ǰ�汾: 1.0��˰���з��飬2013-12-16
*            a) ������ʵ��
*
* ��ʷ�汾: 
*/

#include <time.h>
#include "TraceLog.h"

/*
* ��������: TCLib_Asc2Hex
* ��������: 16����ת��ΪASCII
* ��    ��:
*            pbBin        - �ֽ�������
*            pcAsc        - �ַ�������
*            dwLen        - ���ݳ���
* �� �� ֵ: NO
*/
void TCLib_Hex2Asc(IN const BYTE *pbBin, IN DWORD dwLen, OUT char *pcAsc)
{
    BYTE *pbP;
    BYTE *pbQ;

    pbP = (BYTE *)pcAsc;
    pbQ = (BYTE *)pbBin;

    while(dwLen--)
    {
        *pbP = ((*pbQ) & 0xF0) >> 4;
        if (*pbP >= 10)
        {
            *pbP += ('A' - 10);
        }
        else
        {
            *pbP += '0';
        }

        pbP++;
        *pbP = (*pbQ) & 0x0F;
        if(*pbP >= 10)
        {
            *pbP += ('A' - 10);
        }
        else
        {
            *pbP += '0';
        }
        pbP++;
        pbQ++;
    }

    *pbP = 0;

    return;
}

//���캯����������־�ļ���Ĭ��·��
CTraceLog::CTraceLog() 
{
    //��ʼ���ٽ���
    ::InitializeCriticalSection(&m_crit);
}

CTraceLog::~CTraceLog()
{
    //�ͷ����ٽ���
    ::DeleteCriticalSection(&m_crit);
}

void CTraceLog::InitLogPath(const char *path)
{
    //memcpy(m_acLogPath, path, strlen(path));
	strcpy(m_acLogPath, path);
}

void CTraceLog::CleanPathFile(const char *path)
{
    /*-----------------------�����ٽ���(д�ļ�)------------- -------------*/ 
    ::EnterCriticalSection(&m_crit);

    char acPathName[MAX_PATH+1] = {0};
    DWORD dwFileCount = 0;
    DWORD dwWhileCount = 0;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA sFindData;
  
    sprintf(acPathName, "%s\\*.*", path);
    hFind = FindFirstFile(acPathName, &sFindData);  
    if(INVALID_HANDLE_VALUE != hFind) 
    {
        dwWhileCount = 1000;
        while(dwWhileCount--) 
        {
            if(FALSE == FindNextFile(hFind, &sFindData))
            {
                break;
            }
            if(FILE_ATTRIBUTE_DIRECTORY == (sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                continue;
            }
            if(0 == dwFileCount)
            {
                sprintf(acPathName, "%s\\%s", path, sFindData.cFileName);
            }
            dwFileCount += 1;
        }

        //����ɾ���׸��ļ�
        if(dwFileCount > LOG_FILE_SIZE)
        {
            DeleteFile(acPathName); 
        }

        FindClose(hFind);    
    }

    ::LeaveCriticalSection(&m_crit); 
    /*-------------------�˳��ٽ���----------------------------------------*/ 
}

void CTraceLog::AddLogInfo(const char *pcHead, const char *pcData)
{
    if(strlen(m_acLogPath) <= 0) return;

    /*-----------------------�����ٽ���(д�ļ�)------------- -------------*/ 
    ::EnterCriticalSection(&m_crit);
    
    //����ӵķ�ʽ������ļ�
    FILE *fp = fopen(m_acLogPath, "a+");
    if(fp)
    {
        //��ǰʱ��
  //      struct tm time1;
		//_getsystime(&time1);
		SYSTEMTIME time1;
		GetLocalTime(&time1);
		fprintf(fp, "%04d-%02d-%02d ", time1.wYear, time1.wMonth, time1.wDay);
		fprintf(fp, "%02d:%02d:%02d ", time1.wHour, time1.wMinute, time1.wSecond);
        //�̺߳�
        DWORD dwThreadId = GetCurrentThreadId();
        fprintf(fp, "%08x ", (unsigned int)dwThreadId);
        //��־��Ϣ
        fprintf(fp, "%s : ", pcHead);
        fprintf(fp, "%s\n", pcData);
        //�ر��ļ�
        fclose(fp);
    }
    
    ::LeaveCriticalSection(&m_crit); 
    /*-------------------�˳��ٽ���----------------------------------------*/ 
}

//////////////////////////////////////////////////////////////////////////////////////////

void TRACELOG_STR(const char *pcPath, const char *pcInfo, const char *pcBuf)
{
    if(GetFileAttributes(pcPath) == 0xffffffff)
    {
        CreateDirectory(pcPath, NULL);
    }

    //struct tm time1;
    //_getsystime(&time1);
	SYSTEMTIME time1;
	GetLocalTime(&time1);
    char acPathName[MAX_PATH] = {0};
	sprintf(acPathName, "%s\\_%04d-%02d-%02d.log", pcPath, time1.wYear, time1.wMonth, time1.wDay);
 
    CTraceLog m_CTraceLog;
    m_CTraceLog.CleanPathFile(pcPath);
    m_CTraceLog.InitLogPath(acPathName);
    
    if((NULL != pcInfo) && (NULL != pcBuf))
    {
        m_CTraceLog.AddLogInfo(pcInfo, pcBuf);
    }        
    else
    {
        m_CTraceLog.AddLogInfo("����", "����ΪNULL");
    }

    return;
}

void TRACELOG_HEX(const char *pcPath, const char *pcInfo, BYTE *pbBuf, DWORD dwLen)
{
    if(GetFileAttributes(pcPath) == 0xffffffff)
    {
        CreateDirectory(pcPath, NULL);
    }

    char *pcTempBuf = NULL;
    pcTempBuf = new char[2*dwLen+1];
    memset(pcTempBuf, 0x00, 2*dwLen+1);
    TCLib_Hex2Asc(pbBuf, dwLen, pcTempBuf);

   /* struct tm time1;
    _getsystime(&time1);*/
	SYSTEMTIME time1;
	GetLocalTime(&time1);
    char acPathName[MAX_PATH] = {0};
	sprintf(acPathName, "%s\\_%04d-%02d-%02d.log", pcPath, time1.wYear, time1.wMonth, time1.wDay);
   
    CTraceLog m_CTraceLog;
    m_CTraceLog.CleanPathFile(pcPath);
    m_CTraceLog.InitLogPath(acPathName);
    
    if((NULL != pcInfo) && (NULL != pbBuf))
    {
        m_CTraceLog.AddLogInfo(pcInfo, pcTempBuf);
    }
    else
    {
        m_CTraceLog.AddLogInfo("����", "����ΪNULL");
    }

    delete[] pcTempBuf;
    return;
}

void TRACELOG_U32(const char *pcPath, const char *pcInfo, DWORD dwValue)
{
    if(GetFileAttributes(pcPath) == 0xffffffff)
    {
        CreateDirectory(pcPath, NULL);
    }
    
    char acTempBuf[32] = {0};
    sprintf(acTempBuf, "%08x", (unsigned int)dwValue);

	SYSTEMTIME time1;
	GetLocalTime(&time1);
    char acPathName[MAX_PATH] = {0};
	sprintf(acPathName, "%s\\_%04d-%02d-%02d.log", pcPath, time1.wYear, time1.wMonth, time1.wDay);

    CTraceLog m_CTraceLog;
    m_CTraceLog.CleanPathFile(pcPath);
    m_CTraceLog.InitLogPath(acPathName);
    
    if(NULL != pcInfo)
    {
        m_CTraceLog.AddLogInfo(pcInfo, acTempBuf);
    }
    else
    {
        m_CTraceLog.AddLogInfo("����", "����ΪNULL");
    }

    return;
}

void TRACELOG_STR_HD(const char *pcPath, HANDLE hdFileName, const char *pcInfo, const char *pcBuf)
{
    if(GetFileAttributes(pcPath) == 0xffffffff)
    {
        CreateDirectory(pcPath, NULL);
    }
    
    char acPathName[MAX_PATH] = {0};
    sprintf(acPathName, "%s\\_%08x.log", pcPath, (unsigned int)hdFileName);

    CTraceLog m_CTraceLog;
    m_CTraceLog.CleanPathFile(pcPath);
    m_CTraceLog.InitLogPath(acPathName);
    
    if((NULL != pcInfo) && (NULL != pcBuf))
    {
        m_CTraceLog.AddLogInfo(pcInfo, pcBuf);
    }        
    else
    {
        m_CTraceLog.AddLogInfo("����", "����ΪNULL");
    }
    
    return;
}

void TRACELOG_HEX_HD(const char *pcPath, HANDLE hdFileName, const char *pcInfo, BYTE *pbBuf, DWORD dwLen)
{
    if(GetFileAttributes(pcPath) == 0xffffffff)
    {
        CreateDirectory(pcPath, NULL);
    }
    
    char *pcTempBuf = NULL;
    pcTempBuf = new char[2*dwLen+1];
    memset(pcTempBuf, 0x00, 2*dwLen+1);
    TCLib_Hex2Asc(pbBuf, dwLen, pcTempBuf);
  
    char acPathName[MAX_PATH] = {0};
    sprintf(acPathName, "%s\\_%08x.log", pcPath, (unsigned int)hdFileName);

    CTraceLog m_CTraceLog;
    m_CTraceLog.CleanPathFile(pcPath);
    m_CTraceLog.InitLogPath(acPathName);
    
    if((NULL != pcInfo) && (NULL != pbBuf))
    {
        m_CTraceLog.AddLogInfo(pcInfo, pcTempBuf);
    }        
    else
    {
        m_CTraceLog.AddLogInfo("����", "����ΪNULL");
    }

    delete[] pcTempBuf;
    return;
}

void TRACELOG_U32_HD(const char *pcPath, HANDLE hdFileName, const char *pcInfo, DWORD dwValue)
{
    if(GetFileAttributes(pcPath) == 0xffffffff)
    {
        CreateDirectory(pcPath, NULL);
    }
    
    char acTempBuf[32] = {0};
    sprintf(acTempBuf, "%08x", (unsigned int)dwValue);

    char acPathName[MAX_PATH] = {0};
    sprintf(acPathName, "%s\\_%08x.log", pcPath, (unsigned int)hdFileName);

    CTraceLog m_CTraceLog;
    m_CTraceLog.CleanPathFile(pcPath);
    m_CTraceLog.InitLogPath(acPathName);    
    
    if(NULL != pcInfo)
    {
        m_CTraceLog.AddLogInfo(pcInfo, acTempBuf);
    }        
    else
    {
        m_CTraceLog.AddLogInfo("����", "����ΪNULL");
    }

    return;
}

