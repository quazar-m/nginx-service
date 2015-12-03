#include "global.h"

/* ******************************************************************************
** Types definitions
** RU: Определение типов
*/
typedef struct MngServiceDef {
	SC_HANDLE	sc_manager;
	SC_HANDLE	service;
} MngService, *MngServicePtr;


/**##***************************************************************************
 * PRIVATE FUNCTIONS \ ЛОКАЛЬНЫЕ ФУНКЦИИ                                       *
 *******************************************************************************/


/* ******************************************************************************
** Open SCM database and create service
** RU: Открытие SCM базы и создание службы
*/
static bool MngCreateService(MngService *ms)
{
	wchar_t service_path[MAX_PATH];

	/* Add service run arg */
	/* RU: Добавление флага запуска сервиса */
	wcscpy(service_path, g_ServicePath);
	wcscat(service_path, L" /service");

	/* Open the local default service control manager database */
	/* RU: Открываем локальную базу SCM */
	ms->sc_manager = OpenSCManager(NULL, NULL, 
		SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
	if (ms->sc_manager == NULL)
		return false;

	/* Install the service into SCM */
	/* RU: Установка сервиса в SCM */
	ms ->service = CreateService(
		ms->sc_manager,
		g_ServiceName, 
		g_ServiceDisplayName,
		SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG,
		SERVICE_WIN32_OWN_PROCESS,
		g_ServiceStartType,
		SERVICE_ERROR_NORMAL,
		service_path,
		NULL, NULL,
		L"tcpip\0\0",
		g_ServiceAccount,
		g_ServicePassword
	);
	
	return (ms->service != NULL);
}

/* ******************************************************************************
** Open SCM and service
** RU: Открытие SCM и службы
*/
static bool MngOpenService(MngService *ms, DWORD mode)
{
	/* Open the local default service control manager database */
	/* RU: Открываем локальную базу SCM */
	ms->sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (ms->sc_manager == NULL)
		return false;

	/* Open the service */
	/* RU: Открываем службу */
	ms->service = OpenService(ms->sc_manager, g_ServiceName, 
		SERVICE_QUERY_STATUS | mode);

	return (ms->service != NULL);
}

/* ******************************************************************************
** Close SCM and service handles
** RU: закрытие хендлов SCM и службы
*/
static void MngCloseService(MngService *ms)
{
	if (ms->sc_manager)
		CloseServiceHandle(ms->sc_manager);

	if (ms->service)
		CloseServiceHandle(ms->service);
}

/* ******************************************************************************
** Wait service status
** RU: Ожидание статуса службы
*/
static DWORD WaitServiceStatus(SC_HANDLE handle, SERVICE_STATUS* ss, 
	DWORD status)
{
	while (QueryServiceStatus(handle, ss))
	{
		if (ss->dwCurrentState == status)
		{
			wprintf(L".");
			Sleep(1000);
		} else break;
	} /* while */

	return ss->dwCurrentState;
}


/**##***************************************************************************
 * PUBLIC FUNCTIONS \ ГЛОБАЛЬНЫЕ ФУНКЦИИ                                       *
 *******************************************************************************/


/* ******************************************************************************
** Install the current application as a service to the 
**  local service control manager database
** RU: Установка текущего приложение как службу в локальную SCM базу
*/
void MngInstallService(void)
{
	MngService ms;
	SERVICE_DESCRIPTION sd;

	if (!MngCreateService(&ms))
	{
		PrintErrorMsg(L"Create service failed");
		goto cleanup;
	}

	/* Set service description */
	/* RU: Установка описания службы */
	sd.lpDescription = g_ServiceDescription;
	if (ChangeServiceConfig2(ms.service, SERVICE_CONFIG_DESCRIPTION, 
			&sd) == FALSE)
	{
		PrintErrorMsg(L"Can't change service configuration");
		goto cleanup;
	}

	wprintf(L"%ls is installed.\n", g_ServiceName);

cleanup:
	MngCloseService(&ms);
}

/* ******************************************************************************
** Stop and remove the service from the local service control manager database
** RU: Остановка и удаление службы из локальной SCM базы
*/
void MngRemoveService(void)
{
	MngService ms;
	SERVICE_STATUS ss;

	if (!MngOpenService(&ms, SERVICE_STOP | DELETE))
	{
		PrintErrorMsg(L"Open service failed");
		goto cleanup;
	}

	/* Try to stop the service */
	/* Остановка сервиса */
	if (ControlService(ms.service, SERVICE_CONTROL_STOP, &ss))
	{
		wprintf(L"Stopping %ls.", g_ServiceName);
		Sleep(1000);

		if (WaitServiceStatus(ms.service, 
				&ss, SERVICE_STOP_PENDING) == SERVICE_STOPPED)
			wprintf(L"\n%ls is stopped.\n", g_ServiceName);
		else
			wprintf(L"\n%ls failed to stop.\n", g_ServiceName);
	} /* if */

	/* Now remove the service */
	/* RU: Удаление сервиса */
	if (DeleteService(ms.service) == FALSE)
	{
		PrintErrorMsg(L"Delete service failed");
		goto cleanup;
	}

	wprintf(L"%ls is removed.\n", g_ServiceName);

cleanup:
	MngCloseService(&ms);
}

/* ******************************************************************************
** Start service
** RU: Запуск службы
*/
void MngStartService(void)
{
	MngService ms;
	SERVICE_STATUS ss;

	if (!MngOpenService(&ms, SERVICE_START))
	{
		PrintErrorMsg(L"Open service failed");
		goto cleanup;
	}

	/* Try to start the service */
	/* Запуск службы */
	if (StartService(ms.service, 0, NULL))
	{
		wprintf(L"Starting %ls.", g_ServiceName);
		Sleep(1000);

		if (WaitServiceStatus(ms.service, &ss, 
				SERVICE_START_PENDING) == SERVICE_RUNNING)
			wprintf(L"\n%ls is started.\n", g_ServiceName);
		else
			wprintf(L"\n%ls failed to start.\n", g_ServiceName);
	} else
		PrintErrorMsg(L"Service failed to start");

cleanup:
	MngCloseService(&ms);
}

/* ******************************************************************************
** Stop service
** RU: Остановка службы
*/
void MngStopService(void)
{
	MngService ms;
	SERVICE_STATUS ss;

	if (!MngOpenService(&ms, SERVICE_STOP))
	{
		PrintErrorMsg(L"Open service failed");
		goto cleanup;
	}

	/* Try to stop the service */
	/* Остановка сервиса */
	if (ControlService(ms.service, SERVICE_CONTROL_STOP, &ss))
	{
		wprintf(L"Stopping %ls.", g_ServiceName);
		Sleep(1000);

		if (WaitServiceStatus(ms.service, 
				&ss, SERVICE_STOP_PENDING) == SERVICE_STOPPED)
			wprintf(L"\n%ls is stopped.\n", g_ServiceName);
		else
			wprintf(L"\n%ls failed to stop.\n", g_ServiceName);
	} else
		PrintErrorMsg(L"Service failed to stop");

cleanup:
	MngCloseService(&ms);
}
