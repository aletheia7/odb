/* $Id: ntservice.cpp,v 1.3 2003/12/15 00:02:47 rtwitty Exp $ */
// NTService.cpp
//
// Implementation of CNTServCtrl & CNTService

#include <windows.h>
#include <stdio.h>
#include "ntservice.h"

CNTServCtrl::CNTServCtrl( const char* pszServiceName,
                          const char* pszDisplayName,
                          const char* pszModuleName   )
{
    // Set the service name, display name and default version
    strncpy(m_szServiceName, pszServiceName, sizeof(m_szServiceName)-1);
    m_szServiceName[sizeof(m_szServiceName)-1] = 0;

    if( pszDisplayName )
    {
        strncpy(m_szDisplayName, pszDisplayName, sizeof(m_szDisplayName)-1);
        m_szDisplayName[sizeof(m_szDisplayName)-1] = 0;
    }
    else
    {
        strcpy( m_szDisplayName, m_szServiceName );
    }
    ::GetModuleFileName( NULL, m_szModulePath, sizeof(m_szModulePath) );

    if( pszModuleName )
    {
        char* ptr = strrchr( m_szModulePath, '\\' );
        strcpy( ptr + 1, pszModuleName );
    }
    m_iMajorVersion = 1;
    m_iMinorVersion = 0;
}

CNTServCtrl::~CNTServCtrl()
{
}

void CNTServCtrl::DoCommand( PCSTR pszCmd )
{
    DWORD dwState;

    if( !stricmp( pszCmd, "install" ) )
    {
        if( IsInstalled() )
            Message( "Service is already installed." );
        else if( Install() )
            Message( "Service was successfully installed." );
        else
            Error( "Unable to install service." );
    }
    else if( !stricmp( pszCmd, "uninstall" ) )
    {
        if( !IsInstalled() )
            Message( "Service is not installed." );
        else if( !GetCurrentState( &dwState ) )
            Error( "Unable to get current state of service." );
        else if( dwState != SERVICE_STOPPED && !Stop() )
            Error( "Unable to stop service." );
        else if( Uninstall() )
            Message( "Service was successfully uninstalled." );
        else
            Error( "Unable to uninstall service." );
    }
    else if( !stricmp( pszCmd, "start" ) )
    {
        if( !IsInstalled() )
            Message( "Service is not installed." );
        else if( !GetCurrentState( &dwState ) )
            Error( "Unable to get current state of service." );
        else if( dwState == SERVICE_RUNNING )
            Message( "Service has already been started." );
        else if( Start() )
            Message( "Service was started successfully." );
        else
            Error( "Unable to start service." );
    }
    else if( !stricmp( pszCmd, "stop" ) )
    {
        if( !IsInstalled() )
            Message( "Service is not installed." );
        else if( !GetCurrentState( &dwState ) )
            Error( "Unable to get current state of service." );
        else if( dwState == SERVICE_STOPPED )
            Message( "Service has already been stopped." );
        else if( Stop() )
            Message( "Service was stopped successfully." );
        else
            Error( "Unable to stop service." );
    }
    else if( !stricmp( pszCmd, "restart" ) )
    {
        if( !IsInstalled() )
            Message( "Service is not installed." );
        else if( !GetCurrentState( &dwState ) )
            Error( "Unable to get current state of service." );
        else if( dwState != SERVICE_STOPPED && !Stop() )
            Error( "Unable to stop service." );
        else if( Start() )
            Message( "Service was restarted successfully." );
        else
            Error( "Unable to restart service." );
    }
    else if( !stricmp( pszCmd, "status" ) )
    {
        if( !IsInstalled() )
            Message( "Service is not installed." );
        else if( !GetCurrentState( &dwState ) )
            Error( "Unable to get current state of service." );
        else if( dwState == SERVICE_RUNNING )
            Message( "Service is running." );
        else
            Message( "Service is not running." );
    }
    else if( !stricmp( pszCmd, "version" ) )
    {
        Message( "%s Version %i.%i",
                 m_szServiceName, m_iMajorVersion, m_iMinorVersion );
    }
    else
    {
        OnCommand( pszCmd );
    }
}

BOOL CNTServCtrl::Error( PCSTR pszFormat, ... )
{
    char    sz[512];
    va_list args;

    va_start( args, pszFormat );
    vsprintf( sz, pszFormat, args );
    va_end( args );

    OnError( sz );

    return FALSE;
}

BOOL CNTServCtrl::Message( PCSTR pszFormat, ... )
{
    char    sz[512];
    va_list args;

    va_start( args, pszFormat );
    vsprintf( sz, pszFormat, args );
    va_end( args );

    OnMessage( sz );

    return TRUE;
}

void CNTServCtrl::OnCommand( PCSTR pszCmd )
{
    Message( "Invalid command.  Valid commands are install, uninstall "
                 "start, stop, restart, status and version" );
}

void CNTServCtrl::OnError( PCSTR pszError )
{
    ::MessageBox( NULL, pszError, m_szDisplayName, MB_OK | MB_ICONERROR );
}

void CNTServCtrl::OnMessage( PCSTR pszMessage )
{
    ::MessageBox( NULL, pszMessage, m_szDisplayName, MB_OK | MB_ICONINFORMATION );
}

////////////////////////////////////////////////////////////////////////////////////////
// Install/uninstall routines

// Test if the service is currently installed
BOOL CNTServCtrl::IsInstalled()
{
    BOOL bResult = FALSE;

    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (hSCM) {

        // Try to open the service
        SC_HANDLE hService = ::OpenService(hSCM,
                                           m_szServiceName,
                                           SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }

        ::CloseServiceHandle(hSCM);
    }

    return bResult;
}

BOOL CNTServCtrl::CreateEventLogEntry()
{
    // make registry entries to support logging messages
    // Add the source name as a subkey under the Application
    // key in the EventLog service portion of the registry.
    char szKey[1024];
    HKEY hKey = NULL;
    strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(szKey, m_szServiceName);
    if (::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) != ERROR_SUCCESS)
        return FALSE;

    char szFilePath[_MAX_PATH];

    ::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));

    // Add the Event ID message-file name to the 'EventMessageFile' subkey.
    ::RegSetValueEx(hKey,
                    "EventMessageFile",
                    0,
                    REG_EXPAND_SZ,
                    (CONST BYTE*)szFilePath,
                    strlen(szFilePath) + 1);

    // Set the supported types flags.
    DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    ::RegSetValueEx(hKey,
                    "TypesSupported",
                    0,
                    REG_DWORD,
                    (CONST BYTE*)&dwData,
                     sizeof(DWORD));

    ::RegCloseKey(hKey);

    return TRUE;
}

BOOL CNTServCtrl::CreateService()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    // Create the service
    SC_HANDLE hService = ::CreateService(hSCM,
                                         m_szServiceName,
                                         m_szDisplayName,
                                         SERVICE_ALL_ACCESS,
                                         SERVICE_WIN32_OWN_PROCESS,
                                         SERVICE_AUTO_START,
                                         SERVICE_ERROR_NORMAL,
                                         m_szModulePath,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL);
    if (!hService) {
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return( TRUE );
}

BOOL CNTServCtrl::DeleteEventLogEntry()
{
    // Delete registry entries that support logging messages
    char szKey[1024];
    strcpy(szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat(szKey, m_szServiceName);
    ::RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
    return( TRUE );
}

BOOL CNTServCtrl::DeleteService()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    BOOL bResult = FALSE;
    SC_HANDLE hService = ::OpenService(hSCM,
                                       m_szServiceName,
                                       DELETE);

    if (hService) {
        if (::DeleteService(hService)) bResult = TRUE;
        ::CloseServiceHandle(hService);
    }
    ::CloseServiceHandle(hSCM);
    return( bResult );
}

BOOL CNTServCtrl::Install()
{
    if( !CreateService() )
        return( FALSE );

    if( !CreateEventLogEntry() )
    {
        DeleteService();
        return( FALSE );
    }
    if( !OnInstall() )
    {
        DeleteService();
        DeleteEventLogEntry();
        return( FALSE );
    }
    return( TRUE );
}

BOOL CNTServCtrl::Uninstall()
{
    if( !OnUninstall() ) return( FALSE );
    if( !DeleteService() ) return( FALSE );
    DeleteEventLogEntry();

    return( TRUE );
}

BOOL CNTServCtrl::OnInstall()
{
    return( TRUE );
}

BOOL CNTServCtrl::OnUninstall()
{
    return( TRUE );
}

BOOL CNTServCtrl::GetCurrentState( PDWORD pdwState )
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if( !hSCM ) return FALSE;

    BOOL bResult = FALSE;
    SC_HANDLE hService = ::OpenService(hSCM,
                                       m_szServiceName,
                                       SERVICE_QUERY_STATUS);

    if( hService )
    {
        SERVICE_STATUS ss;
        if( ::QueryServiceStatus( hService, &ss ) )
        {
            *pdwState = ss.dwCurrentState;
            bResult = TRUE;
        }
        ::CloseServiceHandle( hService );
    }
    ::CloseServiceHandle( hSCM );
    return( bResult );
}

BOOL CNTServCtrl::Start()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if( !hSCM ) return FALSE;

    BOOL bResult = FALSE;
    SC_HANDLE hService = ::OpenService(hSCM,
                                       m_szServiceName,
                                       SERVICE_START);

    if( hService )
    {
        if( ::StartService( hService, 0, NULL ) ) bResult = TRUE;
        ::CloseServiceHandle( hService );
    }
    ::CloseServiceHandle( hSCM );
    return( bResult );
}

BOOL CNTServCtrl::Stop()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if( !hSCM ) return FALSE;

    BOOL bResult = FALSE;
    SC_HANDLE hService = ::OpenService(hSCM,
                                       m_szServiceName,
                                       SERVICE_QUERY_STATUS|SERVICE_STOP);

    if( hService )
    {
        SERVICE_STATUS ss;
        if( ::ControlService( hService, SERVICE_CONTROL_STOP, &ss ) )
        {
            while( ss.dwCurrentState != SERVICE_STOPPED &&
                   ::QueryServiceStatus( hService, &ss ) )
            {
                ::Sleep( 1000 );
            }
            if( ss.dwCurrentState == SERVICE_STOPPED ) bResult = TRUE;
        }
        ::CloseServiceHandle( hService );
    }
    ::CloseServiceHandle( hSCM );
    return( bResult );
}

////////////////////////////////////////////////////////////////////////
// CNTService

// static variables
CNTService* CNTService::m_pThis = NULL;

CNTService::CNTService( const char* pszServiceName )
{
    // copy the address of the current object so we can access it from
    // the static member callback functions.
    // WARNING: This limits the application to only one CNTService object.
    m_pThis = this;

    // Set the service
    strncpy(m_szServiceName, pszServiceName, sizeof(m_szServiceName)-1);
    m_szServiceName[sizeof(m_szServiceName)-1] = 0;

    m_hEventSource = NULL;

    // set up the initial service status
    m_hServiceStatus = NULL;
    m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_Status.dwCurrentState = SERVICE_STOPPED;
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    m_bIsRunning = FALSE;
    m_bIsStopping = FALSE;
}

CNTService::~CNTService()
{
}

void CNTService::LogEvent(WORD wType, DWORD dwID, const char* pszUserMsg )
{
    const char* ps[1];
    ps[0] = pszUserMsg;

    // Check the event source has been registered and if
    // not then register it now
    if (!m_hEventSource) {
        m_hEventSource = ::RegisterEventSource( NULL, m_szServiceName );
    }

    if (m_hEventSource) {
        ::ReportEvent(m_hEventSource,
                      wType,
                      0,
                      dwID,
                      NULL, // sid
                      pszUserMsg != NULL ? 1 : 0,
                      0,
                      ps,
                      NULL);
    }
}

void CNTService::LogUserError( const char* pszError )
{
    LogEvent( EVENTLOG_ERROR_TYPE, EVMSG_USERERROR, pszError );
}

void CNTService::LogUserInfo( const char* pszInfo )
{
    LogEvent( EVENTLOG_INFORMATION_TYPE, EVMSG_USERINFO, pszInfo );
}

void CNTService::LogUserWarning( const char* pszWarning )
{
    LogEvent( EVENTLOG_WARNING_TYPE, EVMSG_USERWARNING, pszWarning );
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration

BOOL CNTService::StartService()
{
    SERVICE_TABLE_ENTRY st[] = {
        {m_szServiceName, ServiceMain},
        {NULL, NULL}
    };
    BOOL b = ::StartServiceCtrlDispatcher(st);
    return b;
}

// static member function (callback)
void CNTService::ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    // Get a pointer to the C++ object
    CNTService* pService = m_pThis;

    // Register the control request handler
    pService->m_Status.dwCurrentState = SERVICE_START_PENDING;
    pService->m_hServiceStatus = RegisterServiceCtrlHandler(pService->m_szServiceName,
                                                            ServiceHandler);
    if (pService->m_hServiceStatus == NULL) {
        pService->OnMessage( SERVMSG_HANDLER_NOT_INSTALLED );
        return;
    }
    // Start the initialisation

    pService->SetStatus(SERVICE_START_PENDING);

    if (pService->Initialize()) {

        // Do the real work.
        // When the Run function returns, the service has stopped.
        pService->SetStatus( SERVICE_RUNNING );
        pService->m_Status.dwWin32ExitCode = 0;
        pService->m_Status.dwCheckPoint = 0;
        pService->m_Status.dwWaitHint = 0;
        pService->OnMessage( SERVMSG_STARTED );
        pService->m_bIsRunning = TRUE;
        pService->Run();
        pService->m_bIsRunning = FALSE;
    }
    pService->OnStopped();
    // Tell the service manager we are stopped
    pService->SetStatus(SERVICE_STOPPED);
    pService->OnMessage( SERVMSG_STOPPED );
}

///////////////////////////////////////////////////////////////////////////////////////////
// status functions

void CNTService::SetStatus(DWORD dwState)
{
    m_Status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_Status);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Service initialization

BOOL CNTService::Initialize()
{
    // Start the initialization
    // Perform the actual initialization
    BOOL bResult = OnInit();

    // Set final state
    m_Status.dwWin32ExitCode = GetLastError();
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    if (!bResult) {
        OnMessage( SERVMSG_INIT_FAILED );
        return FALSE;
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// main function to do the real work of the service

// This function performs the main work of the service.
// When this function returns the service has stopped.
void CNTService::Run()
{
    while( !m_bIsStopping ) Sleep(1000);
}

//////////////////////////////////////////////////////////////////////////////////////
// Control request handlers

// static member function (callback) to handle commands from the
// service control manager
void CNTService::ServiceHandler(DWORD dwOpcode)
{
    // Get a pointer to the object
    CNTService* pService = m_pThis;

    switch (dwOpcode) {
    case SERVICE_CONTROL_STOP: // 1
        pService->SetStatus(SERVICE_STOP_PENDING);
        pService->m_bIsStopping = TRUE;
        pService->OnStop();
        pService->m_bIsStopping = FALSE;
        break;

    case SERVICE_CONTROL_PAUSE: // 2
        pService->OnPause();
        break;

    case SERVICE_CONTROL_CONTINUE: // 3
        pService->OnContinue();
        break;

    case SERVICE_CONTROL_INTERROGATE: // 4
        pService->OnInterrogate();
        break;

    case SERVICE_CONTROL_SHUTDOWN: // 5
        pService->OnShutdown();
        break;

    default:
        if (dwOpcode >= SERVICE_CONTROL_USER) {
            if (!pService->OnUserControl(dwOpcode)) {
                pService->OnMessage( SERVMSG_BAD_REQUEST );
            }
        } else {
            pService->OnMessage( SERVMSG_BAD_REQUEST );
        }
        break;
    }
    // Report current status
    ::SetServiceStatus(pService->m_hServiceStatus, &pService->m_Status);
}

// Called when the service is first initialized
BOOL CNTService::OnInit()
{
	return TRUE;
}

// Called when the service control manager wants to stop the service
void CNTService::OnStop()
{
    while( m_bIsRunning ) Sleep(1000);
}

// called when the service is interrogated
void CNTService::OnInterrogate()
{
}

// called when the service is paused
void CNTService::OnPause()
{
}

// called when the service is continued
void CNTService::OnContinue()
{
}

// called when the service is shut down
void CNTService::OnShutdown()
{
}

// Called when the service is stopped
void CNTService::OnStopped()
{
}

// called when the service gets a user control message
BOOL CNTService::OnUserControl(DWORD dwOpcode)
{
    return FALSE; // say not handled
}

void CNTService::OnMessage( int nMessage )
{
}

