/* $Id: ODBTPService.h,v 1.12 2004/08/04 01:08:09 rtwitty Exp $ */
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
// ODBTPService.h: interface for the CODBTPService class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBTPSERVICE_H__803AD891_4D62_11D6_812D_0050DA0B930B__INCLUDED_)
#define AFX_ODBTPSERVICE_H__803AD891_4D62_11D6_812D_0050DA0B930B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\ntservice\ntservice.h"

class CAccessObject
{
public:
   CAccessObject();
   virtual ~CAccessObject();

    DWORD  m_dwTimeout;
    HANDLE m_hevServiceStopping;
    HANDLE m_hmutAccess;
    HANDLE m_hObjects[2];

    BOOL Capture();
    BOOL Create( HANDLE hevServiceStopping, DWORD dwTimeout, PCSTR pszName );
    BOOL Open( HANDLE hevServiceStopping, DWORD dwTimeout, PCSTR pszName );
    void Release();
};

class CODBTPService;

class CODBTPService : public CNTService
{
public:
	CODBTPService();
	virtual ~CODBTPService();

// Variables
    OdbEnv m_Env;
    BOOL   m_bIsServer;
    DWORD  m_dwThreadCount;
    DWORD  m_dwTotalAccepted;
    HANDLE m_hfileLog;
    HANDLE m_hevClientThreadWaitEvents[3];
    HANDLE m_hevDontBlockThreads;
    HANDLE m_hevChildOK;
    HANDLE m_hevReleaseThread;
    HANDLE m_hevServerOK;
    HANDLE m_hevStopping;

// Registry/Ini Parameter Variables
    DWORD m_dwBadConnCheck;
    DWORD m_dwClientThreadWaitTimeout;
    DWORD m_dwConnectTimeout;
    DWORD m_dwConnPoolSize;
    DWORD m_dwConnPoolTimeout;
    DWORD m_dwEnableLog;
    DWORD m_dwEnableProcCache;
    DWORD m_dwFetchRowCount;
    DWORD m_dwListenBacklog;
    DWORD m_dwLogODBC;
    DWORD m_dwLogReadAndSent;
    DWORD m_dwMaxActiveClientThreads;
    DWORD m_dwMaxClientThreads;
    DWORD m_dwMaxQrysPerConn;
    DWORD m_dwMaxRequestSize;
    DWORD m_dwPort;
    DWORD m_dwQueryTimeout;
    DWORD m_dwReadTimeout;
    DWORD m_dwReservConnPoolSize;
    DWORD m_dwReservConnPoolTimeout;
    DWORD m_dwTransBufferSize;
    char  m_szLogFile[MAX_PATH];

// Database Connection Pool Variables
    COdbtpConPool m_ConPool;
    COdbtpConPool m_ReservConPool;

// Thread Access Variables
    CAccessObject  m_aoLog;
    CAccessObject* m_paoReserved;
    CThreadAccess  m_taOKEvent;
    CThreadAccess  m_taThreadCount;

// General Functions
    BOOL  BlockThread( COdbtpSock* pSock );
    void  DecrementThreadCount( BOOL bServiced );
    BOOL  GetIniFileParameters();
    BOOL  GetRegistryParameters();
    DWORD GetThreadCount();
    void  IncrementThreadCount();
    BOOL  IsBlockingThreads();
    BOOL  IsServer(){ return m_bIsServer; }
    BOOL  IsStopping();
    BOOL  Log( PCSTR pszFormat, ... );
    BOOL  LogChildDiag( PCSTR pszDiag );
    BOOL  LogReadAndSent(){return m_dwLogReadAndSent != 0;}
    BOOL  LogServerDiag( PCSTR pszDiag );
    BOOL  ReadIniFileValue( PSTR pszIniFile, PSTR pszValue,
                            PSTR pszData, DWORD dwSize );
    BOOL  ReadIniFileValue( PSTR pszIniFile, PSTR pszValue, PDWORD pdwData );
    BOOL  ReadRegistryValue( HKEY hKey, LPTSTR pszValue,
                             LPBYTE pData, DWORD dwSize );

// Database Connection Pool Functions
    void       CheckConnTimeouts();
    COdbtpCon* Connect( COdbtpSock* pSock, PSTR pszDBConnect, USHORT usType );
    BOOL       Disconnect( COdbtpCon* pCon, BOOL bDrop );

// Server Funtions
    HANDLE ServerExec();
    BOOL   ServerInit();
    void   ServerRun();


// Child Processing Funtions
    BOOL ChildExec( PVOID pThread, PCSTR pszDBConnect );
    BOOL ChildInit( PVOID pData );
    void ChildRun();

// Service Handler Funtions
    void Run();
	BOOL OnInit();
    void OnStop();
    void OnStopped();
    BOOL OnUserControl( DWORD dwOpcode );

// Thread Procedures
    static DWORD WINAPI ClientThreadProc( LPVOID pParam );
    static DWORD WINAPI MonitorThreadProc( LPVOID pParam );

// Odb Diag Logger
    static int LogOdbDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text );
    static int LogOdbDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text );
};

#endif // !defined(AFX_ODBTPSERVICE_H__803AD891_4D62_11D6_812D_0050DA0B930B__INCLUDED_)
