#pragma once
#include "np_golbal_header.h"

void _stdcall np_ontimer_anal_ip(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void _stdcall np_ontimer_report(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void _stdcall np_ontimer_checkversion(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void _stdcall np_ontimer_checklinking(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);

void _stdcall np_ontimer_heartbeat(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime);