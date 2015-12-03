Nginx service configuration file HOWTO
--------------------------------------------------------------------------------
Nginx service configuration file is standart INI file. Filename assigned by
exebutable name - (executable_name.)[exe\ini]

Section "service":
  - name - Service name in SCM database, default value is "nginx_service"
  - display_name - Display name in Service manager (Administration->Services),
    default is "Nginx Web Server control service"
  - description - Description of service, default NULL
  - start_type - Startup service type, values: auto, demand or disabled.
    Default id auto.
  - account - Service account. Default: "NT AUTHORITY\\LocalService" 
    for WinXP\Win2k3\Vista\Win7, for Win2k - ".\LocalSystem"
  - password - Service account password. Default is NULL.

Section "nginx":
  - path - Path to nginx executable, default assign by service
  - executable - nginx executable name, default is "nginx.exe"
  - parameters - additional parameters to run nginx, default is NULL

Section "php-cgi":
  - status - Enable PHP-CGI, default is "off"
  - path - Path to PHP directory
  - executable - PHP-CGI executable name, default is "php-cgi.exe"
  - parameters - additional parameters to run PHP-GCI, default is NULL
  