#include "global.h"

/* ******************************************************************************
** Types definitions
** RU: Определение типов
*/
#define CALL0(x) (((void(*)(void))x)())
typedef struct ParmSymDef {
	wchar_t		*name;
	void		*pointer;
} ParmSym, *ParmSymPtr;

typedef struct IniKeyDef {
	wchar_t		*section;
	wchar_t		*key;
	wchar_t		*pointer;
	wchar_t		*defval;
} IniKey, *IniKeyPtr;

/* ******************************************************************************
** Global variables
** RU: Глобальные переменные
*/
wchar_t g_ServicePath[MAX_PATH];

wchar_t g_ServiceName[MAX_STR];
wchar_t g_ServiceDisplayName[MAX_STR];
wchar_t g_ServiceDescription[MAX_STR];
DWORD   g_ServiceStartType;
wchar_t g_ServiceAccount[MAX_STR];
wchar_t g_ServicePassword[MAX_STR];

wchar_t g_NginxPath[MAX_STR];
wchar_t g_NginxExecutable[MAX_STR];
wchar_t g_NginxArgs[MAX_STR];
wchar_t g_NginxExecPath[MAX_STR];

bool    g_PhpCgiStatus;
wchar_t g_PhpCgiPath[MAX_STR];
wchar_t g_PhpCgiExecutable[MAX_STR];
wchar_t g_PhpCgiArgs[MAX_STR];
wchar_t g_PhpCgiExecPath[MAX_STR];

/* ******************************************************************************
** Local variables
** RU: Локальные переменные
*/
static wchar_t l_ServiceStartType[MAX_STR];
static wchar_t l_PhpCgiStatus[MAX_STR];


/**##***************************************************************************
 * PRIVATE FUNCTIONS \ ЛОКАЛЬНЫЕ ФУНКЦИИ                                       *
 *******************************************************************************/


/* ******************************************************************************
** Check Windows version
** RU: Проверяем версию винды
*/
static bool IsNT(bool *is2k)
{
	OSVERSIONINFOEX osvi;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (GetVersionEx((POSVERSIONINFO)&osvi) == 0)
		return false;

	*is2k = (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0);

	return ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
		(osvi.dwMajorVersion > 4));
}

/* ******************************************************************************
** Load configuration file and prepare service parameters
** RU: Загружаем файл конфигурации и подготавливаем параметры службы
*/
static void LoadConfFile(const IniKey *first)
{
	const IniKey *ik = first;
	wchar_t inifile[MAX_PATH];

	wcscpy(inifile, g_ServicePath);
	wcscpy(((wchar_t*)(inifile) + wcslen(inifile) - 3), L"ini");

	while (ik->pointer)
	{
		GetPrivateProfileString(ik->section, ik->key, ik->defval,
			ik->pointer, MAX_STR-1, inifile);
		ik++;
	}

	if (_wcsicmp(l_ServiceStartType, L"auto") == 0)
		g_ServiceStartType = SERVICE_AUTO_START;
	else if (_wcsicmp(l_ServiceStartType, L"demand") == 0)
		g_ServiceStartType = SERVICE_DEMAND_START;
	else if (_wcsicmp(l_ServiceStartType, L"disabled") == 0)
		g_ServiceStartType = SERVICE_DISABLED;
	else 
		g_ServiceStartType = SERVICE_AUTO_START;

	if (_wcsicmp(l_PhpCgiStatus, L"off") == 0 || _wcsicmp(l_PhpCgiStatus, L"false") == 0)
		g_PhpCgiStatus = false;
	else if (_wcsicmp(l_PhpCgiStatus, L"on") == 0 || _wcsicmp(l_PhpCgiStatus, L"true") == 0)
		g_PhpCgiStatus = true;
	else
		g_PhpCgiStatus = false;

	swprintf(g_NginxExecPath, MAX_PATH, L"%ls\\%ls", g_NginxPath, g_NginxExecutable);

	if (g_PhpCgiStatus == true)
		swprintf(g_PhpCgiExecPath, MAX_PATH, L"%ls\\%ls", g_PhpCgiPath, g_PhpCgiExecutable);
}

/* ******************************************************************************
** Checkup application parameters
** RU: Проверка параметров приложения
*/
static bool ParmCheck(int argc, wchar_t **argv, const ParmSym *first)
{
	bool bRet = false;
	const ParmSym *ps = first;

	if ((argc > 1) && (*argv[1] == L'-' || (*argv[1] == L'/')))
	{
		while (ps->pointer)
		{
			if (_wcsicmp(ps->name, argv[1] + 1) == 0)
			{
				bRet = true;
				break;
			}
			ps++;
		} /* while */

		if (bRet == true)
			CALL0(ps->pointer);

	} /* if */
	return bRet;
}

/* ******************************************************************************
** Print application parametes list
** RU: Вывод списка параметров
*/
static void PrintHelp(void)
{
	wprintf(L"Parameters:\n"                \
		" -?        print this page\n"      \
		" -install  to install service\n"   \
		" -remove   to remove service\n"    \
		" -start    to start service\n"     \
		" -stop     to stop service\n"
	);
}

/* ******************************************************************************
** Extract file path
** RU: Извлечение пути файла
*/
static int ExtractFilePath(const wchar_t *filename, wchar_t* out_path, int buf_len)
{
	wchar_t *slash_pos = wcsrchr(filename, L'\\');
	if (!slash_pos)
		wcsrchr(filename, L'/');

	if (!slash_pos)
		return 0;

	int path_len = slash_pos - filename;
	int chars2copy = (path_len >= buf_len) ? buf_len - 1 : path_len;

	if (out_path)
	{
		wcsncpy(out_path, filename, chars2copy);
		out_path[chars2copy] = 0;
	}
     
	return path_len+1;
}


/**##***************************************************************************
 * PUBLIC FUNCTIONS \ ГЛОБАЛЬНЫЕ ФУНКЦИИ                                       *
 *******************************************************************************/


/* ******************************************************************************
** Get error description and print
** RU: Получение описания ошибки и вывод
*/
void PrintErrorMsg(wchar_t *msg)
{
	DWORD err_code;
	PTCHAR err_msg = NULL;

	err_code = GetLastError();

	if (!FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL,
		err_code,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
		(PTCHAR)&err_msg,
		0, NULL
	)) return;

	wprintf(L"%ls w/err 0x%08lx: %ls", msg, err_code, err_msg);

	LocalFree(err_msg);
}

/* ******************************************************************************
** Application entrypoint
** RU: Точка входа приложения
*/
int wmain(int argc, wchar_t **argv)
{
	wchar_t current_directory[MAX_PATH];
	bool is2k = false;

	if (IsNT(&is2k) == false)
		return 1;

	/* get module name and extract current directory */
	GetModuleFileName(NULL, g_ServicePath, MAX_PATH);
	ExtractFilePath(g_ServicePath, current_directory, MAX_PATH);

	/* application args array */
	/* RU: массив параметров приложения */
	const ParmSym parms[] = {
		{L"?",          PTR(&PrintHelp)             },
		{L"install",    PTR(&MngInstallService)     },
		{L"remove",     PTR(&MngRemoveService)      },
		{L"start",      PTR(&MngStartService)       },
		{L"stop",       PTR(&MngStopService)        },
		{L"service",    PTR(&ServiceInitalization)  },
		{NULL,          NULL                        }
	};

	/* confuration file keys array */
	/* RU: массив ключей конфигурационного файла */
	const IniKey inikeys[] = {
		/* SERVICE section */
		{L"service", L"name",           PTR(&g_ServiceName),                \
		        L"nginx_service"                                            },
		{L"service", L"display_name",   PTR(&g_ServiceDisplayName),         \
		        L"Nginx Web Server control service"                         },
		{L"service", L"description",    PTR(&g_ServiceDescription), NULL    },
		{L"service", L"start_type",     PTR(&l_ServiceStartType),           \
		        L"auto" /* auto | demand | disabled */                      },
		{L"service", L"account",        PTR(&g_ServiceAccount),             \
		        (!is2k) ? L"NT AUTHORITY\\LocalService" : L".\\LocalSystem" },
		{L"service", L"password",       PTR(&g_ServicePassword),    NULL    },

		/* NGINX section */
		{L"nginx",   L"path",           PTR(&g_NginxPath),                  \
		        current_directory                                           },
		{L"nginx",   L"executable",     PTR(&g_NginxExecutable),            \
		        L"nginx.exe"                                                },
		{L"nginx",   L"parameters",     PTR(&g_NginxArgs),          NULL    },

		/* PHP-GCI section */
		{L"php-cgi", L"status",         PTR(&l_PhpCgiStatus),       L"off"  },
		{L"php-cgi", L"path",           PTR(&g_PhpCgiPath),                 \
				current_directory                                           },
		{L"php-cgi", L"executable",     PTR(&g_PhpCgiExecutable),           \
				L"php-cgi.exe"												},
		{L"php-cgi", L"parameters",		PTR(&g_PhpCgiArgs),					\
				L"-b 127.0.0.1:9123"										},

		{NULL,       NULL,              NULL,                       NULL    }
	};

	LoadConfFile(inikeys);

	if (ParmCheck(argc, argv, parms) == false)
		PrintHelp();

	return 0;
}
