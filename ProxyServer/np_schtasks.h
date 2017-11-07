#pragma once

#include "np_golbal_header.h"

BOOL CheckVersion(char* path, char* pPath, BOOL bIsBat);

void StartSchTasks();

BOOL ChangeThePW();

// 0 不需要更新 1 需要更新  2 出错
DWORD NewCreateKey_REG(HKEY hProxyExe);

// 0 不需要更新 1 需要更新  2 出错
DWORD OpenExisting_REG(HKEY hProxyExe);

BOOL OpenAdnGetValue_REG();

BOOL InitSchTsks();

void RunClear(const char* _username, const char* _password);