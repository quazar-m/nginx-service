#include "global.h"

/* ******************************************************************************
** Local variables
** RU: Локальные переменные
*/
static SERVICE_STATUS_HANDLE l_StatusHandle;
static SERVICE_STATUS l_Status;
static HANDLE l_watchThread = NULL;


/**##***************************************************************************
 * PRIVATE FUNCTIONS \ ЛОКАЛЬНЫЕ ФУНКЦИИ                                       *
 *******************************************************************************/

/* ******************************************************************************
** Change service status
** RU: Изменение режима работы службы
*/
static bool ServiceChangeStatus(DWORD state)
{
	static DWORD checkpoint = 1;

	l_Status.dwCurrentState = state;
	l_Status.dwCheckPoint = ((state == SERVICE_RUNNING) ||
		(state == SERVICE_STOPPED)) ? 0 : checkpoint++;

	return SetServiceStatus(l_StatusHandle, &l_Status);
}

/* ******************************************************************************
** Event handler (stop nginx and php-cgi)
** RU: Обработчик сообщений (останов. nginx и php-cgi)
*/
static void WINAPI ServiceCtrlHandler(DWORD state)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	wchar_t pidfile[MAX_PATH];
	char pidinfo[50];
	DWORD rb, pid;
	HANDLE hProc, hPidFile;

	if (state == SERVICE_CONTROL_STOP || state == SERVICE_CONTROL_SHUTDOWN)
	{
		ServiceChangeStatus(SERVICE_STOP_PENDING);

		ZeroMemory(&si, sizeof(STARTUPINFO));
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		if (!CreateProcess(g_NginxExecPath, L" -s stop ", NULL, NULL, FALSE,
				NORMAL_PRIORITY_CLASS, NULL, g_NginxPath, &si, &pi))
		{
			WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't stop nginx process");
			ServiceChangeStatus(SERVICE_STOPPED);
			ExitProcess(1);
		}

		if (g_PhpCgiStatus == true)
		{
			ZeroMemory(pidfile, MAX_PATH);
			swprintf(pidfile, MAX_PATH, L"%ls\\php.pid", g_PhpCgiPath);

			if (!(hPidFile = CreateFile(pidfile, 
						GENERIC_READ, 
						FILE_SHARE_READ,
						NULL, 
						OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL, NULL)))
			{
				WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't read PHP-GCI process information");
				ServiceChangeStatus(SERVICE_STOPPED);
				ExitProcess(1);
			}


			ReadFile(hPidFile, pidinfo, 50, &rb, NULL);
			pid = strtol(pidinfo, NULL, 10);

			if ( l_watchThread )
				TerminateThread(l_watchThread, 0);

			if ((hProc = OpenProcess(PROCESS_TERMINATE, 0, pid)) != NULL)
			{
				TerminateProcess(hProc, 0);
				CloseHandle(hProc);
			}
			
			CloseHandle(hPidFile);
			DeleteFile(pidfile);
		}

		ServiceChangeStatus(SERVICE_STOPPED);
	}
}

/* ******************************************************************************
** Start PHP-CGI process
** RU: Запуск PHP-CGI процесса
*/
static HANDLE StartPHPCGI(void) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	wchar_t args[1024], pidfile[MAX_PATH];
	HANDLE hPidFile;
	char pidinfo[50];
	DWORD bw;

	ZeroMemory(args, 1024);
	swprintf(args, 1024, L" %ls ", g_PhpCgiArgs);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	if (!CreateProcess(g_PhpCgiExecPath, args, NULL, NULL, FALSE,
			NORMAL_PRIORITY_CLASS, NULL, g_PhpCgiPath, &si, &pi))
	{
		WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't start PHP-GCI process");
		ServiceChangeStatus(SERVICE_STOPPED);
		ExitProcess(1);
	}


	ZeroMemory(pidfile, MAX_PATH);
	swprintf(pidfile, MAX_PATH, L"%ls\\php.pid", g_PhpCgiPath);

	if (!(hPidFile = CreateFile(pidfile, 
				GENERIC_WRITE, 
				FILE_SHARE_READ,
				NULL, 
				CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, NULL)))
	{
		WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't write PHP-GCI process information");
		ServiceChangeStatus(SERVICE_STOPPED);
		ExitProcess(1);
	}

	sprintf(pidinfo, "%d", pi.dwProcessId);
	WriteFile(hPidFile, pidinfo, strlen(pidinfo), &bw, NULL);

	CloseHandle(hPidFile);

	return pi.hProcess;
}

/* Watch PHPCGI process terminate
 */
static DWORD WINAPI WatchPHPCGI(PVOID param) {
	HANDLE hProc;
	while ( 1 ) {
		hProc = StartPHPCGI();
		WaitForSingleObject(hProc, INFINITE);
	}
	return 0;
}

/* ******************************************************************************
** Service entry point. It registers the handler function for the service and
**  start the service (run nginx and php-cgi)
** RU: Точка входа службы. Регистрирует обработчик сообщений сервиса и 
**  последующий запуск (Запуск nginx и php-cgi)
*/
static void WINAPI ServiceMain(DWORD argc, PWSTR *argv)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	wchar_t args[1024], pidfile[MAX_PATH];
	HANDLE hPidFile;
	char pidinfo[50];
	DWORD bw;

	l_StatusHandle = RegisterServiceCtrlHandler(g_ServiceName,
		ServiceCtrlHandler);
	if (l_StatusHandle == NULL)
	{
		WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't register control handler");
		ServiceChangeStatus(SERVICE_STOPPED);
		ExitProcess(1);
	}

	ServiceChangeStatus(SERVICE_START_PENDING);

	ZeroMemory(args, 1024);
	swprintf(args, 1024, L" %ls ", g_NginxArgs);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	if (!CreateProcess(g_NginxExecPath, args, NULL, NULL, FALSE,
			NORMAL_PRIORITY_CLASS, NULL, g_NginxPath, &si, &pi))
	{
		WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't start nginx process");
		ServiceChangeStatus(SERVICE_STOPPED);
		ExitProcess(1);
	}
#if 0
	if (g_PhpCgiStatus == true)
	{
		ZeroMemory(args, 1024);
		swprintf(args, 1024, L" %ls ", g_PhpCgiArgs);

		ZeroMemory(&si, sizeof(STARTUPINFO));
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		if (!CreateProcess(g_PhpCgiExecPath, args, NULL, NULL, FALSE,
				NORMAL_PRIORITY_CLASS, NULL, g_PhpCgiPath, &si, &pi))
		{
			WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't start PHP-GCI process");
			ServiceChangeStatus(SERVICE_STOPPED);
			ExitProcess(1);
		}


		ZeroMemory(pidfile, MAX_PATH);
		swprintf(pidfile, MAX_PATH, L"%ls\\php.pid", g_PhpCgiPath);

		if (!(hPidFile = CreateFile(pidfile, 
					GENERIC_WRITE, 
					FILE_SHARE_READ,
					NULL, 
					CREATE_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, NULL)))
		{
			WriteEventLog(EVENTLOG_ERROR_TYPE, L"Can't write PHP-GCI process information");
			ServiceChangeStatus(SERVICE_STOPPED);
			ExitProcess(1);
		}

		sprintf(pidinfo, "%d", pi.dwProcessId);
		WriteFile(hPidFile, pidinfo, strlen(pidinfo), &bw, NULL);

		CloseHandle(hPidFile);
	}
#else
	if (g_PhpCgiStatus == true)
		l_watchThread = CreateThread(NULL, 0, WatchPHPCGI, NULL, 0, NULL);
#endif

	ServiceChangeStatus(SERVICE_RUNNING);
}


/**##***************************************************************************
 * PUBLIC FUNCTIONS \ ГЛОБАЛЬНЫЕ ФУНКЦИИ                                       *
 *******************************************************************************/


/* ******************************************************************************
** Log a message to the Application event log
** RU: Запись сообщений с ошибками в журнал приложений и служб
*/
void WriteEventLog(DWORD type, wchar_t *msg)
{
	HANDLE event_src;
	LPCWSTR strings[2];
	DWORD last_error;
	PTCHAR sys_err_text = NULL;
	wchar_t buffer[MAX_STR];

	last_error = GetLastError();

	/* get system error description */
	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL,
		last_error,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
		(PTCHAR)&sys_err_text,
		0, NULL))
	{
		swprintf(buffer, MAX_STR, L"%ls w/err 0x%08lx: %ls", 
		msg, last_error, sys_err_text);

		LocalFree(sys_err_text);

		/* report message to app event log */
		event_src = RegisterEventSource(NULL, g_ServiceName);
		if (event_src)
		{
			strings[0] = g_ServiceName;
			strings[1] = buffer;

			ReportEvent(event_src, type, 0, 0, NULL, 2, 0, strings, NULL);
			DeregisterEventSource(event_src);
		}
	}
}

/* ******************************************************************************
** Service initialization
** RU: Инициальзация запуска службы
*/
void ServiceInitalization(void)
{
	const SERVICE_TABLE_ENTRY ste[] = {
		{g_ServiceName, ServiceMain},
		{NULL, NULL}
	};

	l_StatusHandle              = NULL;
	l_Status.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
	l_Status.dwCurrentState     = SERVICE_START_PENDING;
	l_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	l_Status.dwWin32ExitCode    = NO_ERROR;
	l_Status.dwServiceSpecificExitCode = 0;
	l_Status.dwCheckPoint       = 0;
	l_Status.dwWaitHint         = 0;

	if (!StartServiceCtrlDispatcher(ste))
		PrintErrorMsg(L"Service failed to run");
}
