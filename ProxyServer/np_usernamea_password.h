#pragma once 
#include "np_golbal_header.h"

void GetUsernameAndPassword();

BOOL VerifyBasic(char *pInfo);

void Return401(SOCKET sClient);