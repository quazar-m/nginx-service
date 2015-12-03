#ifndef _GLOBAL_H
#define _GLOBAL_H

#ifndef UNICODE
#	define UNICODE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#ifndef bool
typedef enum {false, true} bool;
#endif

#define PTR(x)	(void*)x
#define MAX_STR	255

extern void PrintErrorMsg(wchar_t *msg);

/* ******************************************************************************
** Global variables
** RU: Глобальные переменные
*/
extern wchar_t g_ServicePath[MAX_PATH];

extern wchar_t g_ServiceName[MAX_STR];
extern wchar_t g_ServiceDisplayName[MAX_STR];
extern wchar_t g_ServiceDescription[MAX_STR];
extern DWORD   g_ServiceStartType;
extern wchar_t g_ServiceAccount[MAX_STR];
extern wchar_t g_ServicePassword[MAX_STR];

extern wchar_t g_NginxPath[MAX_STR];
extern wchar_t g_NginxExecutable[MAX_STR];
extern wchar_t g_NginxArgs[MAX_STR];
extern wchar_t g_NginxExecPath[MAX_STR];

extern bool    g_PhpCgiStatus;
extern wchar_t g_PhpCgiPath[MAX_STR];
extern wchar_t g_PhpCgiExecutable[MAX_STR];
extern wchar_t g_PhpCgiArgs[MAX_STR];
extern wchar_t g_PhpCgiExecPath[MAX_STR];

/* ******************************************************************************
** Manager.c functions
*/
extern void MngInstallService(void);
extern void MngRemoveService(void);
extern void MngStartService(void);
extern void MngStopService(void);

/* ******************************************************************************
** Service.c functions
*/
extern void ServiceInitalization(void);
extern void WriteEventLog(DWORD type, wchar_t *msg);

#endif
