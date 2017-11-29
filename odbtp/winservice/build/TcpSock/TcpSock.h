/* $Id: TcpSock.h,v 1.3 2004/06/02 20:12:20 rtwitty Exp $ */
/*
    TcpSock - TCP/IP sockets class library

    Copyright (C) 2002-2004 Robert E. Twitty <rtwitty@users.sourceforge.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _TCPSOCK_H
#define _TCPSOCK_H

#include <windows.h>

#ifndef TCPSOCK_BUILD
#include "SockUtils.h"
#endif

#define TCPERR_NONE        0
#define TCPERR_INVALID     1
#define TCPERR_VALID       2
#define TCPERR_CONNECT     3
#define TCPERR_LISTEN      4
#define TCPERR_ACCEPT      5
#define TCPERR_READ        6
#define TCPERR_SEND        7
#define TCPERR_WAIT        8
#define TCPERR_TIMEOUT     9
#define TCPERR_GETINFO     10
#define TCPERR_SETMODE     11
#define TCPERR_CLOSE       12

class CThreadAccess
{
public:
    CThreadAccess();
    ~CThreadAccess();

    CRITICAL_SECTION m_CriticalSection;
};

class CThreadAccessLock
{
public:
    CThreadAccessLock( CThreadAccess& ThreadAccess );
    ~CThreadAccessLock();

    LPCRITICAL_SECTION m_pCriticalSection;
};

typedef void (*TcpErrorCallback)( void* pUserData, int nError, LPCSTR pszErrorMessage );

class CTcpSock;
class CTcpSock
{
private:
    SOCKET           m_Sock;
    TcpErrorCallback m_ErrorCallback;
    CThreadAccess    m_SockAccess;
    CThreadAccess    m_ErrorAccess;
    BOOL             m_bNonBlocking;
    int              m_nAcceptTimeout;
    int              m_nConnectTimeout;
    int              m_nError;
    int              m_nReadTimeout;
    int              m_nSendTimeout;
    void*            m_pUserData;
    LPSTR            m_pszPeerAddress;
    LPSTR            m_pszPeerHostname;
    LPSTR            m_pszSockAddress;
    LPSTR            m_pszSockHostname;

public:
    CTcpSock( BOOL bNonBlocking = FALSE );
    virtual ~CTcpSock();

    operator SOCKET();
    CTcpSock& operator=( SOCKET Sock );

    BOOL   Accept( CTcpSock& Sock );
    BOOL   Accept( CTcpSock* pSock );
    BOOL   Accept( SOCKET Sock );
    BOOL   Attach( SOCKET Sock );
    BOOL   Close();
    BOOL   Connect( LPCSTR pszAddress, int nPort );
    BOOL   EnableNonBlocking( BOOL bEnable = TRUE );
    int    GetAcceptTimeout(){ return m_nAcceptTimeout; }
    int    GetConnectTimeout(){ return m_nConnectTimeout; }
    int    GetError(){ return m_nError; }
    int    GetReadTimeout(){ return m_nReadTimeout; }
    int    GetSendTimeout(){ return m_nSendTimeout; }
    LPCSTR GetPeerAddress();
    LPCSTR GetPeerHostname();
    int    GetPeerPort();
    LPCSTR GetSockAddress();
    LPCSTR GetSockHostname();
    int    GetSockPort();
    void*  GetUserData(){ return m_pUserData; }
    BOOL   IsNonBlocking();
    BOOL   IsValid();
    BOOL   Listen( LPCSTR pszAddress = NULL, int nPort = 0, int nBacklog = 1 );
    int    Read( void* pBuf, int nLen, int nStopByte = -1 );
    int    Send( const void* pBuf, int nLen );
    void   SetAcceptTimeout( int nSeconds ){ m_nAcceptTimeout = nSeconds; }
    void   SetConnectTimeout( int nSeconds ){ m_nConnectTimeout = nSeconds; }
    BOOL   SetError( int nError, BOOL bDoCallback = TRUE );
    void   SetReadTimeout( int nSeconds ){ m_nReadTimeout = nSeconds; }
    void   SetSendTimeout( int nSeconds ){ m_nSendTimeout = nSeconds; }
    void   SetUserData( void* pUserData ){ m_pUserData = pUserData; }
    BOOL   Wait( int nSecs = 0, BOOL bForRead = TRUE );

    TcpErrorCallback SetErrorCallback( TcpErrorCallback pCallback );

    virtual LPCSTR GetErrorMessage();

    static BOOL WinsockStartup( LPWSADATA lpWSAData = NULL );
    static BOOL WinsockCleanup();
};

 #endif
