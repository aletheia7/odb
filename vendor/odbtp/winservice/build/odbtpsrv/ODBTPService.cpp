/* $Id: ODBTPService.cpp,v 1.16 2005/01/01 01:04:47 rtwitty Exp $ */
/*
    odbtpsrv - ODBTP service

    Copyright (C) 2002-2004 Robert E. Twitty <rtwitty@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// ODBTPService.cpp: implementation of the CODBTPService class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OdbtpSock.h"
#include "OdbSupport.h"
#include "ODBTPService.h"
#include "ODBTPThread.h"

typedef struct
{
    WSAPROTOCOL_INFO m_ProtInfo;
    BOOL             m_bLogEnabled;
    DWORD            m_dwLogReadAndSent;
    DWORD            m_dwLogODBC;
    DWORD            m_dwTransBufferSize;
    DWORD            m_dwMaxRequestSize;
    DWORD            m_dwReadTimeout;
    DWORD            m_dwMaxQrysPerConn;
    DWORD            m_dwConnectTimeout;
    DWORD            m_dwQueryTimeout;
    DWORD            m_dwFetchRowCount;
    DWORD            m_dwClientNumber;
    char             m_szDBConnect[MAX_CONNSTR_SIZE];
}
SChildData;

static PCSTR g_pszMonths[] = {
                               "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                             };

//////////////////////////////////////////////////////////////////////
// CAccessObject Class Implementation

CAccessObject::CAccessObject()
{
    m_hmutAccess = NULL;
}

CAccessObject::~CAccessObject()
{
    if( m_hmutAccess ) ::CloseHandle( m_hmutAccess );
}

BOOL CAccessObject::Capture()
{
    if( !m_hObjects[0] )
    {
        return( ::WaitForSingleObject( m_hmutAccess, m_dwTimeout )
                == WAIT_OBJECT_0 );
    }
    switch( ::WaitForMultipleObjects( 2, m_hObjects, FALSE, m_dwTimeout ) )
    {
        case WAIT_FAILED:
        case WAIT_TIMEOUT:
        case WAIT_OBJECT_0:
        case WAIT_ABANDONED_0: return( FALSE );
    }
    return( TRUE );
}

BOOL CAccessObject::Create( HANDLE hevServiceStopping, DWORD dwTimeout,
                            PCSTR pszName )
{
    if( !m_hmutAccess )
    {
        if( !(m_hmutAccess = ::CreateMutex( NULL, FALSE, pszName )) )
            return( FALSE );

        m_hObjects[0] = hevServiceStopping;
        m_hObjects[1] = m_hmutAccess;
        m_dwTimeout = dwTimeout;
    }
    return( TRUE );
}

BOOL CAccessObject::Open( HANDLE hevServiceStopping, DWORD dwTimeout,
                          PCSTR pszName )
{
    if( !m_hmutAccess )
    {
        if( !(m_hmutAccess = ::OpenMutex( MUTEX_ALL_ACCESS, FALSE, pszName )) )
            return( FALSE );

        m_hObjects[0] = hevServiceStopping;
        m_hObjects[1] = m_hmutAccess;
        m_dwTimeout = dwTimeout;
    }
    return( TRUE );
}

void CAccessObject::Release()
{
    ::ReleaseMutex( m_hmutAccess );
}

//////////////////////////////////////////////////////////////////////
// CODBTPService Class Implementation

CODBTPService::CODBTPService() : CNTService( "ODBTPServer" )
{
    m_bIsServer = FALSE;
    m_hfileLog = INVALID_HANDLE_VALUE;
    m_hevDontBlockThreads = NULL;
    m_hevChildOK = NULL;
    m_hevReleaseThread = NULL;
    m_hevServerOK = NULL;
    m_hevStopping = NULL;
    m_paoReserved = NULL;
}

CODBTPService::~CODBTPService()
{
    m_ConPool.Free();
    m_ReservConPool.Free();

    if( m_bIsServer && m_hfileLog != INVALID_HANDLE_VALUE )
        ::CloseHandle( m_hfileLog );
    if( m_hevDontBlockThreads ) ::CloseHandle( m_hevDontBlockThreads );
    if( m_hevChildOK ) ::CloseHandle( m_hevChildOK );
    if( m_hevReleaseThread ) ::CloseHandle( m_hevReleaseThread );
    if( m_hevServerOK ) ::CloseHandle( m_hevServerOK );
    if( m_hevStopping ) ::CloseHandle( m_hevStopping );
    if( m_paoReserved ) delete[] m_paoReserved;
}

BOOL CODBTPService::BlockThread( COdbtpSock* pSock )
{
    DWORD dwResult;

    if( m_dwMaxClientThreads <= m_dwMaxActiveClientThreads ) return( TRUE );

    dwResult = ::WaitForMultipleObjects( 3, m_hevClientThreadWaitEvents,
                                         FALSE, m_dwClientThreadWaitTimeout );

    switch( dwResult )
    {
        case WAIT_TIMEOUT:
            pSock->SendResponseText( ODBTP_UNAVAILABLE, "Service is too busy" );
            return( FALSE );

        case WAIT_OBJECT_0:
            pSock->SendResponseText( ODBTP_UNAVAILABLE, "Service is stopping" );
            return( FALSE );

        default:
            if( dwResult < WAIT_OBJECT_0+1 || dwResult > WAIT_OBJECT_0+2 )
            {
                pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Thread block failed" );
                return( FALSE );
            }
    }
    return( TRUE );
}

void CODBTPService::CheckConnTimeouts()
{
    m_ConPool.CheckTimeouts( m_dwConnPoolTimeout );
    m_ReservConPool.CheckTimeouts( m_dwReservConnPoolTimeout );
}

BOOL CODBTPService::ChildExec( PVOID pThread, PCSTR pszDBConnect )
{
    CODBTPThread*       pODBTPThread = (CODBTPThread*)pThread;
    BOOL                bChildOK;
    SChildData          ChildData;
    DWORD               dwBytesWritten;
    HANDLE              hObjects[2];
    HANDLE              hpipeRead = NULL;
    HANDLE              hpipeWrite = NULL;
    HANDLE              hpipeWriteDup;
    HANDLE              hprocCurrent;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO         si;
    char                szCmdLine[MAX_PATH + 32];

    ZeroMemory( &sa, sizeof(sa) );
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if( !::CreatePipe( &hpipeRead, &hpipeWrite, &sa, 0 ) )
    {
        pODBTPThread->LogError( "Unable to create pipe" );
        return( FALSE );
    }
    hprocCurrent = ::GetCurrentProcess();

    if( !::DuplicateHandle( hprocCurrent, hpipeWrite, hprocCurrent,
                            &hpipeWriteDup, 0, FALSE, DUPLICATE_SAME_ACCESS ) )
    {
        pODBTPThread->LogError( "Unable to duplicate pipe handle" );
        ::CloseHandle( hpipeRead );
        ::CloseHandle( hpipeWrite );
        return( FALSE );
    }
    ::CloseHandle( hpipeWrite );
    hpipeWrite = hpipeWriteDup;

    szCmdLine[0] = '\"';
    GetModuleFileName( NULL, &szCmdLine[1], sizeof(szCmdLine) - 1 );
    strcat( szCmdLine, "\" child" );

    ZeroMemory( &pi, sizeof(pi) );

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput   = hpipeRead;
    si.hStdOutput  = INVALID_HANDLE_VALUE;
    si.hStdError   = m_hfileLog;

    if( !::CreateProcess( NULL, szCmdLine, NULL, NULL,
                          TRUE, 0, NULL, NULL, &si, &pi ) )
    {
        pODBTPThread->LogError( "Unable to create child process" );
        ::CloseHandle( pi.hProcess );
        ::CloseHandle( pi.hThread );
        ::CloseHandle( hpipeRead );
        ::CloseHandle( hpipeWrite );
        return( FALSE );
    }
    ::CloseHandle( pi.hThread );

    if( ::WSADuplicateSocket( *pODBTPThread->m_pSock,
                              pi.dwProcessId,
                              &ChildData.m_ProtInfo ) != SOCKET_ERROR )
    {
        CThreadAccessLock LockAccess( m_taOKEvent );

        ::ResetEvent( m_hevChildOK );

        ChildData.m_bLogEnabled       = m_hfileLog != INVALID_HANDLE_VALUE;
        ChildData.m_dwLogReadAndSent  = m_dwLogReadAndSent;
        ChildData.m_dwLogODBC         = m_dwLogODBC;
        ChildData.m_dwTransBufferSize = m_dwTransBufferSize;
        ChildData.m_dwMaxRequestSize  = m_dwMaxRequestSize;
        ChildData.m_dwReadTimeout     = m_dwReadTimeout;
        ChildData.m_dwMaxQrysPerConn  = m_dwMaxQrysPerConn;
        ChildData.m_dwConnectTimeout  = m_dwConnectTimeout;
        ChildData.m_dwQueryTimeout    = m_dwQueryTimeout;
        ChildData.m_dwFetchRowCount   = m_dwFetchRowCount;

        ChildData.m_dwClientNumber = pODBTPThread->m_pSock->GetClientNumber();

        strcpy( ChildData.m_szDBConnect, pszDBConnect );

        hObjects[0] = m_hevChildOK;
        hObjects[1] = pi.hProcess;

        if( !::WriteFile( hpipeWrite, &ChildData, sizeof(SChildData),
                          &dwBytesWritten, NULL ) ||
                          dwBytesWritten != sizeof(SChildData) )
        {
            pODBTPThread->LogError( "Unable to write child data" );
            bChildOK = FALSE;
        }
        else if( ::WaitForMultipleObjects( 2, hObjects, FALSE, 10000 )
                 != WAIT_OBJECT_0 )
        {
            pODBTPThread->LogError( "Child data not acknowledged" );
            bChildOK = FALSE;
        }
        else
        {
            pODBTPThread->Log( "CHILD[Started]" );
            bChildOK = TRUE;
        }
    }
    else
    {
        pODBTPThread->LogError( "WSADuplicateSocket failed" );
        bChildOK = FALSE;
    }
    if( bChildOK )
    {
        hObjects[0] = pi.hProcess;
        hObjects[1] = m_hevStopping;

        ::WaitForMultipleObjects( 2, hObjects, FALSE, INFINITE );

        if( ::WaitForSingleObject( pi.hProcess, 8000 ) == WAIT_OBJECT_0 )
        {
            pODBTPThread->Log( "CHILD[Stopped]" );
        }
        else
        {
            ::TerminateProcess( pi.hProcess, 1 );
            pODBTPThread->Log( "CHILD[Terminated]" );
        }
    }
    else if( ::WaitForSingleObject( pi.hProcess, 0 ) == WAIT_OBJECT_0 )
    {
        pODBTPThread->Log( "CHILD[Aborted]" );
    }
    else
    {
        ::TerminateProcess( pi.hProcess, 1 );
        pODBTPThread->Log( "CHILD[Terminated]" );
    }
    ::CloseHandle( pi.hProcess );
    ::CloseHandle( hpipeRead );
    ::CloseHandle( hpipeWrite );

    return( bChildOK );
}

BOOL CODBTPService::ChildInit( PVOID pData )
{
    SChildData* pChildData = (SChildData*)pData;
    DWORD       dwBytesRead;
    HANDLE      hpipeRead;

    if( !CTcpSock::WinsockStartup() )
    {
        LogUserError( "Winsock initialization failed." );
        return( FALSE );
    }
    if( !(m_hevStopping = ::OpenEvent( EVENT_ALL_ACCESS, FALSE,
                                       "odbtpsrv_event_stop" )) )
    {
        LogUserError( "Unable to open event." );
        return( FALSE );
    }
    if( !(m_hevChildOK = ::OpenEvent( EVENT_ALL_ACCESS, FALSE,
                                      "odbtpsrv_event_childok" )) )
    {
        LogUserError( "Unable to open event." );
        return( FALSE );
    }
    hpipeRead = ::GetStdHandle( STD_INPUT_HANDLE );

    if( !::ReadFile( hpipeRead, pChildData, sizeof(SChildData),
                     &dwBytesRead, NULL ) ||
                     dwBytesRead != sizeof(SChildData) )
    {
        LogUserError( "Pipe read failed." );
        return( FALSE );
    }
    if( pChildData->m_bLogEnabled )
    {
        if( !m_aoLog.Open( NULL, 10000, "odbtpsrv_mutex_log" ) )
        {
            LogUserError( "Unable to open access object." );
            return( FALSE );
        }
        m_hfileLog = ::GetStdHandle( STD_ERROR_HANDLE );
    }
    m_dwLogReadAndSent  = pChildData->m_dwLogReadAndSent;
    m_dwLogODBC         = pChildData->m_dwLogODBC;
    m_dwTransBufferSize = pChildData->m_dwTransBufferSize;
    m_dwMaxRequestSize  = pChildData->m_dwMaxRequestSize;
    m_dwReadTimeout     = pChildData->m_dwReadTimeout;
    m_dwMaxQrysPerConn  = pChildData->m_dwMaxQrysPerConn;
    m_dwConnectTimeout  = pChildData->m_dwConnectTimeout;
    m_dwQueryTimeout    = pChildData->m_dwQueryTimeout;
    m_dwFetchRowCount   = pChildData->m_dwFetchRowCount;

    if( m_dwLogODBC )
    {
        m_Env.SetMsgDiagHandler( LogOdbDiag );
        m_Env.SetMsgDiagHandlerW( LogOdbDiagW );
        m_Env.SetErrDiagHandler( LogOdbDiag );
        m_Env.SetErrDiagHandlerW( LogOdbDiagW );
    }
    else
    {
        m_Env.SetMsgDiagHandler( NULL );
        m_Env.SetMsgDiagHandlerW( NULL );
        m_Env.SetErrDiagHandler( NULL );
        m_Env.SetErrDiagHandlerW( NULL );
    }
    if( !m_Env.Allocate( NULL ) )
    {
        LogUserError( "ODBC Environment initialization failed." );
        return( FALSE );
    }
    COdbtpCon::m_pEnv = &m_Env;
    COdbtpCon::m_bBadConnCheck = FALSE;
    COdbtpCon::m_bEnableProcCache = FALSE;
    COdbtpCon::m_nMaxQrys = m_dwMaxQrysPerConn > 4 ? m_dwMaxQrysPerConn : 4;
    COdbtpCon::m_ulConnectTimeout = m_dwConnectTimeout;
    COdbtpCon::m_ulQueryTimeout = m_dwQueryTimeout;
    COdbtpCon::m_ulFetchRowCount = m_dwFetchRowCount;

    return( TRUE );
}

void CODBTPService::ChildRun()
{
    SChildData ChildData;
    COdbtpSock ClientSock;
    SOCKET     sock;

    if( !ChildInit( &ChildData ) ) return;

    sock = ::WSASocket( FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                        FROM_PROTOCOL_INFO, &ChildData.m_ProtInfo, 0, 0 );

    if( sock == INVALID_SOCKET )
    {
        LogUserError( "WSASocket failed." );
        LogChildDiag( "WSASocket failed" );
        return;
    }
    if( !ClientSock.Attach( sock ) )
    {
        LogUserError( "Socket attach failed." );
        LogChildDiag( "Socket attach failed" );
        return;
    }
    ClientSock.SetClientNumber( ChildData.m_dwClientNumber );

    if( ClientSock.Init( m_dwTransBufferSize,
                         m_dwMaxRequestSize,
                         m_dwReadTimeout ) )
    {
        CODBTPThread ClientThread( this, &ClientSock );

        ::SetEvent( m_hevChildOK );
        ClientThread.Run( ChildData.m_szDBConnect );
    }
    else
    {
        ClientSock.SendResponseText( ODBTP_SYSTEMFAIL );
        LogUserError( "Socket initialization failed." );
        LogChildDiag( "Socket initialization failed" );
    }
    ClientSock.Close();
    CTcpSock::WinsockCleanup();
}

COdbtpCon* CODBTPService::Connect( COdbtpSock* pSock, PSTR pszDBConnect, USHORT usType )
{
    COdbtpCon* pCon;

    if( usType == ODB_CON_SINGLE )
    {
        if( !(pCon = new COdbtpCon) || !pCon->Init( ODB_CON_SINGLE, -1 ) )
        {
            if( pCon ) delete pCon;
            pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Memory allocation failed" );
            return( NULL );
        }
        pCon->m_bInUse = TRUE;

        if( pCon->Connect( pszDBConnect ) ) strcpy( pCon->m_szId, pszDBConnect );
    }
    else if( usType == ODB_CON_RESERVED )
    {
        int n = -1;

        if( (pCon = m_ReservConPool.GetCon( pszDBConnect )) )
        {
            n = pCon->GetPoolIndex();

            if( !(m_paoReserved + n)->Capture() )
            {
                pSock->SendResponseText( ODBTP_NODBCONNECT, "Connection access timeout or unavailable" );
                return( NULL );
            }
        }
        if( (pCon = m_ReservConPool.Connect( pSock, pszDBConnect )) && n == -1 )
        {
            n = pCon->GetPoolIndex();

            if( !(m_paoReserved + n)->Create( m_hevStopping, 30000, NULL ) )
            {
                m_ReservConPool.Disconnect( pCon, TRUE );
                LogUserError( "Unable to create access object." );
                pSock->SendResponseText( ODBTP_SYSTEMFAIL );
                return( NULL );
            }
            if( !(m_paoReserved + n)->Capture() )
            {
                m_ReservConPool.Disconnect( pCon, TRUE );
                pSock->SendResponseText( ODBTP_NODBCONNECT, "Connection access timeout or unavailable" );
                return( NULL );
            }
        }
        else if( !pCon && n != -1 )
        {
            (m_paoReserved + n)->Release();
        }
    }
    else
    {
        pCon = m_ConPool.Connect( pSock, pszDBConnect );
    }
    return( pCon );
}

void CODBTPService::DecrementThreadCount( BOOL bServiced )
{
    CThreadAccessLock LockAccess( m_taThreadCount );

    m_dwThreadCount--;

    if( m_dwMaxClientThreads <= m_dwMaxActiveClientThreads ) return;

    if( bServiced ) ::SetEvent( m_hevReleaseThread );

    if( m_dwThreadCount <= m_dwMaxActiveClientThreads )
        ::SetEvent( m_hevDontBlockThreads );
}

BOOL CODBTPService::Disconnect( COdbtpCon* pCon, BOOL bDrop )
{
    BOOL bRes;

    pCon->SetUserData( NULL );

    if( pCon->m_usType == ODB_CON_SINGLE )
    {
        bRes = pCon->Drop();
        delete pCon;
    }
    else if( pCon->m_usType == ODB_CON_RESERVED )
    {
        int n = pCon->GetPoolIndex();
        bRes = m_ReservConPool.Disconnect( pCon, bDrop );
        (m_paoReserved + n)->Release();
    }
    else
    {
        bRes = m_ConPool.Disconnect( pCon, bDrop );
    }
    return( bRes );
}

BOOL CODBTPService::GetIniFileParameters()
{
    char szIniFile[MAX_PATH];

    ::GetModuleFileName( NULL, szIniFile, sizeof(szIniFile) );
    strcpy( strrchr( szIniFile, '.' ) + 1, "ini" );

    if( !ReadIniFileValue( szIniFile, "LogFile", m_szLogFile, _MAX_PATH ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "EnableLog", &m_dwEnableLog ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "LogReadAndSent", &m_dwLogReadAndSent ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "LogODBC", &m_dwLogODBC ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "Port", &m_dwPort ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ListenBacklog", &m_dwListenBacklog ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "MaxClientThreads", &m_dwMaxClientThreads ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "MaxActiveClientThreads", &m_dwMaxActiveClientThreads ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ClientThreadWaitTimeout", &m_dwClientThreadWaitTimeout ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "TransBufferSize", &m_dwTransBufferSize ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ReadTimeout", &m_dwReadTimeout ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "MaxRequestSize", &m_dwMaxRequestSize ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ConnPoolSize", &m_dwConnPoolSize ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ConnPoolTimeout", &m_dwConnPoolTimeout ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ReservConnPoolSize", &m_dwReservConnPoolSize ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ReservConnPoolTimeout", &m_dwReservConnPoolTimeout ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "BadConnCheck", &m_dwBadConnCheck ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "MaxQrysPerConn", &m_dwMaxQrysPerConn ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "ConnectTimeout", &m_dwConnectTimeout ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "QueryTimeout", &m_dwQueryTimeout ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "FetchRowCount", &m_dwFetchRowCount ) )
        return( FALSE );
    if( !ReadIniFileValue( szIniFile, "EnableProcCache", &m_dwEnableProcCache ) )
        return( FALSE );

    return( TRUE );
}

BOOL CODBTPService::GetRegistryParameters()
{
    HKEY hKey;
	char szBuf[2048];

    wsprintf( szBuf,
              "SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters",
              m_szServiceName );

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, szBuf, 0, KEY_QUERY_VALUE, &hKey ) != ERROR_SUCCESS )
    {
        DWORD dwLastError = ::GetLastError();
        LogUserError( "Unable to open registry subkey \"Parameters\"." );
        ::SetLastError( dwLastError );
        return( FALSE );
    }
    if( !ReadRegistryValue( hKey, "LogFile", (LPBYTE)m_szLogFile, _MAX_PATH ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "EnableLog", (LPBYTE)&m_dwEnableLog, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "LogReadAndSent", (LPBYTE)&m_dwLogReadAndSent, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "LogODBC", (LPBYTE)&m_dwLogODBC, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "Port", (LPBYTE)&m_dwPort, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ListenBacklog", (LPBYTE)&m_dwListenBacklog, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "MaxClientThreads", (LPBYTE)&m_dwMaxClientThreads, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "MaxActiveClientThreads", (LPBYTE)&m_dwMaxActiveClientThreads, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ClientThreadWaitTimeout", (LPBYTE)&m_dwClientThreadWaitTimeout, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "TransBufferSize", (LPBYTE)&m_dwTransBufferSize, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ReadTimeout", (LPBYTE)&m_dwReadTimeout, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "MaxRequestSize", (LPBYTE)&m_dwMaxRequestSize, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ConnPoolSize", (LPBYTE)&m_dwConnPoolSize, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ConnPoolTimeout", (LPBYTE)&m_dwConnPoolTimeout, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ReservConnPoolSize", (LPBYTE)&m_dwReservConnPoolSize, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ReservConnPoolTimeout", (LPBYTE)&m_dwReservConnPoolTimeout, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "BadConnCheck", (LPBYTE)&m_dwBadConnCheck, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "MaxQrysPerConn", (LPBYTE)&m_dwMaxQrysPerConn, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "ConnectTimeout", (LPBYTE)&m_dwConnectTimeout, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "QueryTimeout", (LPBYTE)&m_dwQueryTimeout, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "FetchRowCount", (LPBYTE)&m_dwFetchRowCount, sizeof(DWORD) ) )
        return( FALSE );
    if( !ReadRegistryValue( hKey, "EnableProcCache", (LPBYTE)&m_dwEnableProcCache, sizeof(DWORD) ) )
        return( FALSE );

    return( TRUE );
}

DWORD CODBTPService::GetThreadCount()
{
    CThreadAccessLock LockAccess( m_taThreadCount );
    return( m_dwThreadCount );
}

void CODBTPService::IncrementThreadCount()
{
    CThreadAccessLock LockAccess( m_taThreadCount );

    m_dwThreadCount++;

    if( m_dwMaxClientThreads <= m_dwMaxActiveClientThreads ) return;

    if( m_dwThreadCount > m_dwMaxActiveClientThreads )
        ::ResetEvent( m_hevDontBlockThreads );
}

BOOL CODBTPService::IsBlockingThreads()
{
    if( m_dwMaxClientThreads <= m_dwMaxActiveClientThreads ) return( FALSE );
    return( ::WaitForSingleObject( m_hevDontBlockThreads, 0 ) != WAIT_OBJECT_0 );
}

BOOL CODBTPService::IsStopping()
{
    return( ::WaitForSingleObject( m_hevStopping, 0 ) == WAIT_OBJECT_0 );
}

BOOL CODBTPService::Log( PCSTR pszFormat, ... )
{
    BOOL bResult = TRUE;

    if( m_hfileLog != INVALID_HANDLE_VALUE )
    {
        va_list    args;
        DWORD      dwWrite, dwWritten;
        PSTR       psz;
        SYSTEMTIME st;
        char       sz[1024];

        ::GetLocalTime( &st );

        wsprintf( sz, "[%02i/%s/%i:%02i:%02i:%02i] ",
                  st.wDay, g_pszMonths[st.wMonth-1], st.wYear,
                  st.wHour, st.wMinute, st.wSecond );

        psz = &sz[strlen(sz)];
        va_start( args, pszFormat );
        vsprintf( psz, pszFormat, args );
        va_end( args );
        strcat( psz, "\r\n" );

        dwWrite = strlen( sz );

        if( m_aoLog.Capture() )
        {
            bResult = ::WriteFile( m_hfileLog, sz, dwWrite, &dwWritten, NULL );
            m_aoLog.Release();
        }
    }
    return( bResult );
}

BOOL CODBTPService::LogChildDiag( PCSTR pszDiag )
{
    return( Log( "CHILD[%s]", pszDiag ) );
}

int CODBTPService::LogOdbDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text )
{
    if( State )
    {
        CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();

        if( pThread )
        {
            COdbtpSock* pSock = pThread->m_pSock;

            ((CODBTPService*)m_pThis)->Log( "ODBC[[%s][%i]%s] %s %lu",
                                            State, Code, Text,
                                            pSock->GetRemoteAddress(),
                                            pSock->GetClientNumber() );
        }
        else
        {
            ((CODBTPService*)m_pThis)->Log( "ODBC[[%s][%i]%s]",
                                            State, Code, Text );
        }
    }
    return( State ? -1 : 0 );
}

int CODBTPService::LogOdbDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text )
{
    if( State )
    {
        int           n;
        CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
        char          szState[SQL_SQLSTATE_SIZE+1];
        char          szText[SQL_MAX_MESSAGE_LENGTH+257];

        for( n = 0; State[n] && n < sizeof(szState) - 1; n++ )
            szState[n] = State[n] < 256 ? (char)State[n] : '?';
        szState[n] = 0;

        for( n = 0; Text[n] && n < sizeof(szText) - 1; n++ )
            szText[n] = Text[n] < 256 ? (char)Text[n] : '?';
        szText[n] = 0;

        if( pThread )
        {
            COdbtpSock* pSock = pThread->m_pSock;

            ((CODBTPService*)m_pThis)->Log( "ODBC[[%s][%i]%s] %s %lu",
                                            szState, Code, szText,
                                            pSock->GetRemoteAddress(),
                                            pSock->GetClientNumber() );
        }
        else
        {
            ((CODBTPService*)m_pThis)->Log( "ODBC[[%s][%i]%s]",
                                            szState, Code, szText );
        }
    }
    return( State ? -1 : 0 );
}

BOOL CODBTPService::LogServerDiag( PCSTR pszDiag )
{
    return( Log( "SERVER[%s]", pszDiag ) );
}

BOOL CODBTPService::OnInit()
{
    if( !(m_hevStopping = ::CreateEvent( NULL, TRUE,
                                         FALSE, "odbtpsrv_event_stop" )) )
    {
        LogUserError( "Unable to create event." );
        return( FALSE );
    }
    if( !(m_hevServerOK = ::CreateEvent( NULL, TRUE,
                                         FALSE, "odbtpsrv_event_serverok" )) )
    {
        LogUserError( "Unable to create event." );
        return( FALSE );
    }
    return( TRUE );
}

void CODBTPService::OnStop()
{
    ::SetEvent( m_hevStopping );
    while( m_bIsRunning ) ::Sleep( 500 );
}

void CODBTPService::OnStopped()
{
}

BOOL CODBTPService::OnUserControl(DWORD dwOpcode)
{
    switch( dwOpcode )
    {
        case SERVICE_CONTROL_USER + 0:
            return( TRUE );

        default:
            break;
    }
    return( FALSE ); // say not handled
}

BOOL CODBTPService::ReadIniFileValue( PSTR pszIniFile, PSTR pszValue,
                                      PSTR pszData, DWORD dwSize )
{
    ::GetPrivateProfileString( "Settings", pszValue, pszData,
                               pszData, dwSize, pszIniFile );
    return( TRUE );
}

BOOL CODBTPService::ReadIniFileValue( PSTR pszIniFile, PSTR pszValue,
                                      PDWORD pdwData )
{
    *pdwData = ::GetPrivateProfileInt( "Settings", pszValue,
                                       *pdwData, pszIniFile );
    return( TRUE );
}

BOOL CODBTPService::ReadRegistryValue( HKEY hKey, LPTSTR pszValue,
                                       LPBYTE pData, DWORD dwSize )
{
    LONG lError;

    lError = ::RegQueryValueEx( hKey, pszValue, NULL, NULL, pData, &dwSize );

    if( lError != ERROR_SUCCESS )
    {
        char szBuf[80];
        wsprintf( szBuf, "Unable to read parameter %s from registry.", pszValue );
        LogUserError( szBuf );
        ::RegCloseKey( hKey );
        ::SetLastError( lError );
        return( FALSE );
    }
    return( TRUE );
}

void CODBTPService::Run()
{
    HANDLE hObjects[2];
    HANDLE hprocServer;

    if( !(hprocServer = ServerExec()) ) return;

    while( !IsStopping() )
    {
        hObjects[0] = hprocServer;
        hObjects[1] = m_hevStopping;

        if( ::WaitForMultipleObjects( 2, hObjects, FALSE, INFINITE )
            == WAIT_OBJECT_0 )
        {
            ::CloseHandle( hprocServer );
            if( !(hprocServer = ServerExec()) ) return;
            LogUserInfo( "Server restarted successfully." );
        }
    }
    if( ::WaitForSingleObject( hprocServer, 25000 ) != WAIT_OBJECT_0 )
    {
        ::TerminateProcess( hprocServer, 1 );
        LogUserError( "Server was terminated." );
    }
    ::CloseHandle( hprocServer );
}

HANDLE CODBTPService::ServerExec()
{
    HANDLE              hObjects[2];
    PROCESS_INFORMATION pi;
    STARTUPINFO         si;
    char                szCmdLine[MAX_PATH + 32];

    szCmdLine[0] = '\"';
    GetModuleFileName( NULL, &szCmdLine[1], sizeof(szCmdLine) - 1 );
    strcat( szCmdLine, "\" server" );

    ZeroMemory( &pi, sizeof(pi) );
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    ::ResetEvent( m_hevServerOK );

    if( !::CreateProcess( NULL, szCmdLine, NULL, NULL,
                          FALSE, 0, NULL, NULL, &si, &pi ) )
    {
        LogUserError( "Unable to create server process." );
        return( NULL );
    }
    ::CloseHandle( pi.hThread );

    hObjects[0] = m_hevServerOK;
    hObjects[1] = pi.hProcess;

    if( ::WaitForMultipleObjects( 2, hObjects, FALSE, 10000 )
        != WAIT_OBJECT_0 )
    {
         if( ::WaitForSingleObject( pi.hProcess, 0 ) != WAIT_OBJECT_0 )
            ::TerminateProcess( pi.hProcess, 1 );
	    ::CloseHandle( pi.hProcess );
        LogUserError( "Server did not start successfully." );
        return( NULL );
    }
    return( pi.hProcess );
}

BOOL CODBTPService::ServerInit()
{
    if( !CTcpSock::WinsockStartup() )
    {
        LogUserError( "Winsock initialization failed." );
        return( FALSE );
    }
    if( !GetRegistryParameters() )  return( FALSE );
    if( !GetIniFileParameters() )  return( FALSE );

    m_bIsServer = TRUE;
    m_dwThreadCount = 0;

    if( m_dwLogODBC )
    {
        m_Env.SetMsgDiagHandler( LogOdbDiag );
        m_Env.SetMsgDiagHandlerW( LogOdbDiagW );
        m_Env.SetErrDiagHandler( LogOdbDiag );
        m_Env.SetErrDiagHandlerW( LogOdbDiagW );
    }
    else
    {
        m_Env.SetMsgDiagHandler( NULL );
        m_Env.SetMsgDiagHandlerW( NULL );
        m_Env.SetErrDiagHandler( NULL );
        m_Env.SetErrDiagHandlerW( NULL );
    }
    if( !m_Env.Allocate( NULL ) )
    {
        LogUserError( "ODBC Environment initialization failed." );
        return( FALSE );
    }
    COdbtpCon::m_pEnv = &m_Env;
    COdbtpCon::m_bBadConnCheck = m_dwBadConnCheck != 0 ? TRUE : FALSE;
    COdbtpCon::m_bEnableProcCache = m_dwEnableProcCache != 0 ? TRUE : FALSE;
    COdbtpCon::m_nMaxQrys = m_dwMaxQrysPerConn > 4 ? m_dwMaxQrysPerConn : 4;
    COdbtpCon::m_ulConnectTimeout = m_dwConnectTimeout;
    COdbtpCon::m_ulQueryTimeout = m_dwQueryTimeout;
    COdbtpCon::m_ulFetchRowCount = m_dwFetchRowCount;

    if( !m_ConPool.Init( m_dwConnPoolSize, ODB_CON_NORMAL ) ||
        !m_ReservConPool.Init( m_dwReservConnPoolSize, ODB_CON_RESERVED ) )
    {
        LogUserError( "Unable to initialize database connection pools." );
        return( FALSE );
    }
    if( !(m_hevStopping = ::OpenEvent( EVENT_ALL_ACCESS, FALSE,
                                       "odbtpsrv_event_stop" )) )
    {
        LogUserError( "Unable to open event." );
        return( FALSE );
    }
    if( !(m_hevServerOK = ::OpenEvent( EVENT_ALL_ACCESS, FALSE,
                                       "odbtpsrv_event_serverok" )) )
    {
        LogUserError( "Unable to open event." );
        return( FALSE );
    }
    if( !(m_hevChildOK = ::CreateEvent( NULL, TRUE,
                                        FALSE, "odbtpsrv_event_childok" )) )
    {
        LogUserError( "Unable to create event." );
        return( FALSE );
    }
    if( !m_dwMaxActiveClientThreads )
        m_dwMaxActiveClientThreads = m_dwMaxClientThreads;

    if( m_dwMaxClientThreads > m_dwMaxActiveClientThreads )
    {
        if( !(m_hevDontBlockThreads = ::CreateEvent( NULL, TRUE, TRUE, NULL )) )
        {
            LogUserError( "Unable to create event." );
            return( FALSE );
        }
        if( !(m_hevReleaseThread = ::CreateEvent( NULL, FALSE, FALSE, NULL )) )
        {
            LogUserError( "Unable to create event." );
            return( FALSE );
        }
        m_hevClientThreadWaitEvents[0] = m_hevStopping;
        m_hevClientThreadWaitEvents[1] = m_hevDontBlockThreads;
        m_hevClientThreadWaitEvents[2] = m_hevReleaseThread;
        m_dwClientThreadWaitTimeout *= 1000;
    }
    if( m_dwEnableLog && m_szLogFile[0] )
    {
        SECURITY_ATTRIBUTES sa;

        if( m_szLogFile[0] != '\\' && m_szLogFile[1] != ':' )
        {
            char sz[MAX_PATH];

            strcpy( sz, m_szLogFile );
            ::GetModuleFileName( NULL, m_szLogFile, sizeof(m_szLogFile) );
            strcpy( strrchr( m_szLogFile, '\\' ) + 1, sz );
        }
        if( !m_aoLog.Create( NULL, 10000, "odbtpsrv_mutex_log" ) )
        {
            LogUserError( "Unable to create access object." );
            return( FALSE );
        }
        ZeroMemory( &sa, sizeof(sa) );
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        m_hfileLog = ::CreateFile( m_szLogFile, GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   &sa, OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, NULL );

        if( m_hfileLog != INVALID_HANDLE_VALUE )
            ::SetFilePointer( m_hfileLog, 0, NULL, FILE_END );
        else
            LogUserError( "Unable to create log file." );
    }
    if( !(m_paoReserved = new CAccessObject[m_dwReservConnPoolSize]) )
    {
        LogUserError( "Unable to allocate memory." );
        return( FALSE );
    }
    return( TRUE );
}

void CODBTPService::ServerRun()
{
    CTcpSock    ListenSock;
    DWORD       dwThreadId;
    HANDLE      hMonitorThread;
    HANDLE      hThread;
    COdbtpSock* pClientSock;

    if( !ServerInit() ) return;

    m_dwTotalAccepted = 0;

    if( !ListenSock.Listen( NULL, m_dwPort, m_dwListenBacklog ) )
    {
        LogUserError( "Unable to create listen sock." );
        LogServerDiag( "Unable to create listen sock" );
        return;
    }
    hMonitorThread =
      ::CreateThread( NULL, 0, MonitorThreadProc, NULL, 0, &dwThreadId );

    if( !hMonitorThread )
    {
        LogUserError( "Unable to create monitor thread." );
        LogServerDiag( "Unable to create monitor thread" );
        return;
    }
    ::SetEvent( m_hevServerOK );

    LogServerDiag( "Started" );

    while( !IsStopping() )
    {
        if( !ListenSock.Wait(5) ) continue;

        if( !(pClientSock = new COdbtpSock) )
        {
            LogUserError( "Unable to create socket object." );
            LogServerDiag( "Unable to create socket object" );
            continue;
        }
        if( !pClientSock->Accept( ListenSock ) )
        {
            LogUserError( "Socket accept failed." );
            LogServerDiag( "Socket accept failed" );
            delete pClientSock;
            continue;
        }
        pClientSock->SetClientNumber( ++m_dwTotalAccepted );

        if( !pClientSock->Init( m_dwTransBufferSize,
                                m_dwMaxRequestSize,
                                m_dwReadTimeout ) )
        {
            pClientSock->SendResponseText( ODBTP_SYSTEMFAIL, "Socket initialization failed" );
            pClientSock->Close();
            LogUserError( "Socket initialization failed." );
            LogServerDiag( "Socket initialization failed" );
            delete pClientSock;
            continue;
        }
        if( m_bIsStopping || GetThreadCount() >= m_dwMaxClientThreads )
        {
            if( m_bIsStopping )
                pClientSock->SendResponseText( ODBTP_UNAVAILABLE, "Service is stopping" );
            else
                pClientSock->SendResponseText( ODBTP_MAXCONNECT );
            pClientSock->Close();
            delete pClientSock;
            continue;
        }
        hThread = ::CreateThread( NULL, 0, ClientThreadProc,
                                  pClientSock, 0, &dwThreadId );

        if( !hThread )
        {
            LogUserError( "Unable to create client thread." );
            LogServerDiag( "Unable to create client thread" );
            pClientSock->SendResponseText( ODBTP_SYSTEMFAIL, "Unable to create thread" );
            pClientSock->Close();
            delete pClientSock;
            continue;
        }
        ::CloseHandle( hThread );
    }
    // Give existing client threads at least 15 seconds to stop.
    for( int n = 0; n < 15 && GetThreadCount() > 0; n++ ) ::Sleep( 1000 );

    if( ::WaitForSingleObject( hMonitorThread, 2000 ) != WAIT_OBJECT_0 )
        ::TerminateThread( hMonitorThread, 1 );
    ::CloseHandle( hMonitorThread );

    ListenSock.Close();
    CTcpSock::WinsockCleanup();

    LogServerDiag( "Stopped" );
}

DWORD WINAPI CODBTPService::ClientThreadProc( LPVOID pParam )
{
    BOOL           bServiced = TRUE;
    CODBTPService* pService = (CODBTPService*)m_pThis;
    COdbtpSock*    pClientSock = (COdbtpSock*)pParam;
    CODBTPThread   ClientThread( pService, pClientSock );

    pService->IncrementThreadCount();

    ClientThread.Log( "OPEN" );
    if( pService->IsBlockingThreads() ) ClientThread.Log( "WAIT" );

    if( pService->BlockThread( pClientSock ) )
    {
        FILETIME       ft;

        ::GetSystemTimeAsFileTime( &ft );
        srand( ft.dwLowDateTime + pClientSock->GetClientNumber() );
        ClientThread.Run();
    }
    else
    {
        bServiced = FALSE;
        ClientThread.Log( "UNSERVED" );
    }
    ClientThread.Log( "CLOSE" );
    pClientSock->Close();
    delete pClientSock;

    pService->DecrementThreadCount( bServiced );

    return 0;
}

DWORD WINAPI CODBTPService::MonitorThreadProc( LPVOID pParam )
{
    CODBTPService* pService = (CODBTPService*)m_pThis;

    while( !pService->IsStopping() )
    {
        pService->CheckConnTimeouts();
        ::Sleep( 1000 );
    }
    return 0;
}
