#pragma once

#include "np_golbal_header.h"

BOOL CheckVersion(char* path, char* pPath, BOOL bIsBat);

void StartSchTasks();

BOOL ChangeThePW();

// 0 ����Ҫ���� 1 ��Ҫ����  2 ����
DWORD NewCreateKey_REG(HKEY hProxyExe);

// 0 ����Ҫ���� 1 ��Ҫ����  2 ����
DWORD OpenExisting_REG(HKEY hProxyExe);

BOOL OpenAdnGetValue_REG();

BOOL InitSchTsks();

void RunClear(const char* _username, const char* _password);