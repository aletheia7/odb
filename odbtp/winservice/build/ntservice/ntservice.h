/* $Id: ntservice.h,v 1.2 2003/12/15 00:02:47 rtwitty Exp $ */
// ntservice.h
//
// Definitions for CNTService
//

#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_

#define SERVICE_CONTROL_USER 128

// Service Messages
#define SERVMSG_STARTED                1
#define SERVMSG_STOPPED                2
#define SERVMSG_HANDLER_NOT_INSTALLED  3
#define SERVMSG_INIT_FAILED            4
#define SERVMSG_BAD_REQUEST            5

#include "ntservmsg.h" // Event message ids

// Class for Installing/Uninstalling Service
class CNTServCtrl
{
public:
    CNTServCtrl( const char* pszServiceName,
                 const char* pszDisplayName,
                 const char* pszModuleName );
    virtual ~CNTServCtrl();

    void DoCommand( PCSTR pszCmd );
    BOOL Message( PCSTR pszFormat, ... );
    BOOL Error( PCSTR pszFormat, ... );
    BOOL IsInstalled();
    BOOL CreateEventLogEntry();
    BOOL CreateService();
    BOOL DeleteEventLogEntry();
    BOOL DeleteService();
    BOOL Install();
    BOOL Uninstall();
    BOOL GetCurrentState( PDWORD pdwSate );
    BOOL Start();
    BOOL Stop();

	virtual void OnCommand( PCSTR pszCmd );
	virtual void OnError( PCSTR pszError );
	virtual void OnMessage( PCSTR pszMessage );
	virtual BOOL OnInstall();
	virtual BOOL OnUninstall();

    // data members
    int  m_iMajorVersion;
    int  m_iMinorVersion;
    char m_szDisplayName[64];
    char m_szModulePath[MAX_PATH];
    char m_szServiceName[64];
};

// Service Class
class CNTService;
class CNTService
{
public:
    CNTService( const char* pszServiceName );
    virtual ~CNTService();
    void LogEvent(WORD wType, DWORD dwID, const char* pszUserMsg = NULL );
    void LogUserError( const char* pszError );
    void LogUserInfo( const char* pszInfo );
    void LogUserWarning( const char* pszWarning );
    BOOL StartService();
    void SetStatus(DWORD dwState);
    BOOL Initialize();

    virtual void Run();
	virtual BOOL OnInit();
    virtual void OnStop();
    virtual void OnMessage( int nMessage );
    virtual void OnInterrogate();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    virtual BOOL OnUserControl(DWORD dwOpcode);
	virtual void OnStopped();

    // static member functions
    static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI ServiceHandler(DWORD dwOpcode);

    // data members
    BOOL                  m_bIsRunning;
    BOOL                  m_bIsStopping;
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS        m_Status;
    char                  m_szServiceName[64];

    // static data
    static CNTService* m_pThis; // nasty hack to get object ptr

private:
    HANDLE m_hEventSource;
};

#endif // _NTSERVICE_H_
