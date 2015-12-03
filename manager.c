#include "global.h"

/* ******************************************************************************
** Types definitions
** RU: ����������� �����
*/
typedef struct MngServiceDef {
	SC_HANDLE	sc_manager;
	SC_HANDLE	service;
} MngService, *MngServicePtr;


/**##***************************************************************************
 * PRIVATE FUNCTIONS \ ��������� �������                                       *
 *******************************************************************************/


/* ******************************************************************************
** Open SCM database and create service
** RU: �������� SCM ���� � �������� ������
*/
static bool MngCreateService(MngService *ms)
{
	wchar_t service_path[MAX_PATH];

	/* Add service run arg */
	/* RU: ���������� ����� ������� ������� */
	wcscpy(service_path, g_ServicePath);
	wcscat(service_path, L" /service");

	/* Open the local default service control manager database */
	/* RU: ��������� ��������� ���� SCM */
	ms->sc_manager = OpenSCManager(NULL, NULL, 
		SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
	if (ms->sc_manager == NULL)
		return false;

	/* Install the service into SCM */
	/* RU: ��������� ������� � SCM */
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
** RU: �������� SCM � ������
*/
static bool MngOpenService(MngService *ms, DWORD mode)
{
	/* Open the local default service control manager database */
	/* RU: ��������� ��������� ���� SCM */
	ms->sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (ms->sc_manager == NULL)
		return false;

	/* Open the service */
	/* RU: ��������� ������ */
	ms->service = OpenService(ms->sc_manager, g_ServiceName, 
		SERVICE_QUERY_STATUS | mode);

	return (ms->service != NULL);
}

/* ******************************************************************************
** Close SCM and service handles
** RU: �������� ������� SCM � ������
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
** RU: �������� ������� ������
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
 * PUBLIC FUNCTIONS \ ���������� �������                                       *
 *******************************************************************************/


/* ******************************************************************************
** Install the current application as a service to the 
**  local service control manager database
** RU: ��������� �������� ���������� ��� ������ � ��������� SCM ����
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
	/* RU: ��������� �������� ������ */
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
** RU: ��������� � �������� ������ �� ��������� SCM ����
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
	/* ��������� ������� */
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
	/* RU: �������� ������� */
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
** RU: ������ ������
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
	/* ������ ������ */
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
** RU: ��������� ������
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
	/* ��������� ������� */
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
