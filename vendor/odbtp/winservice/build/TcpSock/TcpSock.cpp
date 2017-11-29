/* $Id: TcpSock.cpp,v 1.3 2004/06/02 20:12:20 rtwitty Exp $ */
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
#define TCPSOCK_BUILD
#include "TcpSock.h"
#include "SockUtils.h"

CThreadAccess::CThreadAccess()
{
    ::InitializeCriticalSection( &m_CriticalSection );
}

CThreadAccess::~CThreadAccess()
{
    ::DeleteCriticalSection( &m_CriticalSection );
}

CThreadAccessLock::CThreadAccessLock( CThreadAccess& ThreadAccess )
{
    m_pCriticalSection = &ThreadAccess.m_CriticalSection;
    ::EnterCriticalSection( m_pCriticalSection );
}

CThreadAccessLock::~CThreadAccessLock()
{
    ::LeaveCriticalSection( m_pCriticalSection );
}

#define ADDRESS_BUFSIZE  32
#define HOSTNAME_BUFSIZE 64

CTcpSock::CTcpSock( BOOL bNonBlocking )
{
    m_bNonBlocking = bNonBlocking;
    m_Sock = INVALID_SOCKET;
    m_nAcceptTimeout = 0;
    m_nConnectTimeout = 0;
    m_nReadTimeout = 0;
    m_nSendTimeout = 0;
    m_nError = TCPERR_NONE;
    m_ErrorCallback = NULL;
    m_pUserData = NULL;

    m_pszPeerAddress = (LPSTR)malloc( ADDRESS_BUFSIZE );
    m_pszPeerHostname = (LPSTR)malloc( HOSTNAME_BUFSIZE );
    m_pszSockAddress = (LPSTR)malloc( ADDRESS_BUFSIZE );
    m_pszSockHostname = (LPSTR)malloc( HOSTNAME_BUFSIZE );
}

CTcpSock::~CTcpSock()
{
    SetErrorCallback( NULL );
    Close();

    if( m_pszPeerAddress ) free( m_pszPeerAddress );
    if( m_pszPeerHostname ) free( m_pszPeerHostname );
    if( m_pszSockAddress ) free( m_pszSockAddress );
    if( m_pszSockHostname ) free( m_pszSockHostname );
}

CTcpSock::operator SOCKET()
{
    return( m_Sock );
}

CTcpSock& CTcpSock::operator=( SOCKET Sock )
{
    m_Sock = Sock;
    return( *this );
}

BOOL CTcpSock::Accept( CTcpSock& Sock )
{
    return( Accept( Sock.m_Sock ) );
}

BOOL CTcpSock::Accept( CTcpSock* pSock )
{
    return( Accept( pSock->m_Sock ) );
}

BOOL CTcpSock::Accept( SOCKET Sock )
{
    if( IsValid() ) return( SetError( TCPERR_VALID ) );
    if( Sock == INVALID_SOCKET ) return( SetError( TCPERR_INVALID ) );

    int nTO = m_nAcceptTimeout > 0 ? m_nAcceptTimeout : 0;
    m_Sock = sock_accept( Sock, nTO ? &nTO : NULL );
    if( m_Sock == INVALID_SOCKET )
        return( SetError( nTO >= 0 ? TCPERR_ACCEPT : TCPERR_TIMEOUT ) );

    return( EnableNonBlocking( m_bNonBlocking ) );
}

BOOL CTcpSock::Attach( SOCKET Sock )
{
    if( IsValid() ) return( SetError( TCPERR_VALID ) );
    if( Sock == INVALID_SOCKET ) return( SetError( TCPERR_INVALID ) );

    m_Sock = Sock;

    return( EnableNonBlocking( m_bNonBlocking ) );
}

BOOL CTcpSock::Close()
{
    if( m_Sock != INVALID_SOCKET )
    {
        if( sock_close( m_Sock ) == SOCKET_ERROR )
            return( SetError( TCPERR_CLOSE ) );
        m_Sock = INVALID_SOCKET;
    }
    return( SetError( TCPERR_NONE ) );
}

BOOL CTcpSock::Connect( LPCSTR pszAddress, int nPort )
{
    if( IsValid() ) return( SetError( TCPERR_VALID ) );

    int nTO = m_nConnectTimeout > 0 ? m_nConnectTimeout : 0;
    m_Sock = sock_connect( pszAddress, nPort, SOCK_STREAM, nTO ? &nTO : NULL );
    if( m_Sock == INVALID_SOCKET )
        return( SetError( nTO >= 0 ? TCPERR_CONNECT : TCPERR_TIMEOUT ) );

    return( EnableNonBlocking( m_bNonBlocking ) );
}

BOOL CTcpSock::EnableNonBlocking( BOOL bEnable )
{
    m_bNonBlocking = bEnable;

    if( IsValid() )
    {
        u_long nMode = bEnable ? 1 : 0;
        if( set_nonblk_mode( m_Sock, nMode ) )
            return( SetError( TCPERR_SETMODE ) );
    }
    return( SetError( TCPERR_NONE ) );
}

LPCSTR CTcpSock::GetErrorMessage()
{
    switch( GetError() )
    {
        case TCPERR_NONE:    return( "No error" );
        case TCPERR_INVALID: return( "Invalid Socket" );
        case TCPERR_VALID:   return( "Valid Socket" );
        case TCPERR_CONNECT: return( "Connect failed" );
        case TCPERR_LISTEN:  return( "Listen failed" );
        case TCPERR_ACCEPT:  return( "Accept failed" );
        case TCPERR_READ:    return( "Read failed" );
        case TCPERR_SEND:    return( "Send failed" );
        case TCPERR_WAIT:    return( "Wait failed" );
        case TCPERR_TIMEOUT: return( "Timed Out" );
        case TCPERR_GETINFO: return( "Get info failed" );
        case TCPERR_SETMODE: return( "Set mode failed" );
        case TCPERR_CLOSE:   return( "Close failed" );
    }
    return( "Unknown Error" );
}

LPCSTR CTcpSock::GetPeerAddress()
{
    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( NULL );
    }
    if( !get_peer_addr( m_Sock, m_pszPeerAddress, ADDRESS_BUFSIZE ) )
    {
        SetError( TCPERR_GETINFO );
        return( NULL );
    }
    SetError( TCPERR_NONE );
    return( m_pszPeerAddress );
}

LPCSTR CTcpSock::GetPeerHostname()
{
    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( NULL );
    }
    if( !get_peer_hostname( m_Sock, m_pszPeerHostname, HOSTNAME_BUFSIZE ) )
    {
        SetError( TCPERR_GETINFO );
        return( NULL );
    }
    SetError( TCPERR_NONE );
    return( m_pszPeerHostname );
}

int CTcpSock::GetPeerPort()
{
    int nPort;

    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( -1 );
    }
    if( (nPort = get_peer_port( m_Sock )) < 0 )
    {
        SetError( TCPERR_GETINFO );
        return( NULL );
    }
    SetError( TCPERR_NONE );
    return( nPort );
}

LPCSTR CTcpSock::GetSockAddress()
{
    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( NULL );
    }
    if( !get_sock_addr( m_Sock, m_pszSockAddress, ADDRESS_BUFSIZE ) )
    {
        SetError( TCPERR_GETINFO );
        return( NULL );
    }
    SetError( TCPERR_NONE );
    return( m_pszSockAddress );
}

LPCSTR CTcpSock::GetSockHostname()
{
    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( NULL );
    }
    if( !get_sock_hostname( m_Sock, m_pszSockHostname, HOSTNAME_BUFSIZE ) )
    {
        SetError( TCPERR_GETINFO );
        return( NULL );
    }
    SetError( TCPERR_NONE );
    return( m_pszSockHostname );
}

int CTcpSock::GetSockPort()
{
    int nPort;

    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( -1 );
    }
    if( (nPort = get_sock_port( m_Sock )) < 0 )
    {
        SetError( TCPERR_GETINFO );
        return( NULL );
    }
    SetError( TCPERR_NONE );
    return( nPort );
}

BOOL CTcpSock::IsNonBlocking()
{
    return( m_bNonBlocking );
}

BOOL CTcpSock::IsValid()
{
    return( m_Sock != INVALID_SOCKET );
}

BOOL CTcpSock::Listen( LPCSTR pszAddress, int nPort, int nBacklog )
{
    if( IsValid() ) return( SetError( TCPERR_VALID ) );
    m_Sock = sock_listen( pszAddress, nPort, SOCK_STREAM, nBacklog );
    if( m_Sock == INVALID_SOCKET ) return( SetError( TCPERR_LISTEN ) );
    return( EnableNonBlocking( m_bNonBlocking ) );
}

int CTcpSock::Read( void* pBuf, int nLen, int nStopByte )
{
    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( -1 );
    }
    if( nLen == 0 )
    {
        SetError( TCPERR_NONE );
        return( 0 );
    }
    int nTO = m_nReadTimeout > 0 ? m_nReadTimeout : 0;
    int nRead = sock_read( m_Sock, (char*)pBuf, nLen, nStopByte, nTO ? &nTO : NULL );
    SetError( nRead >= 0 ? TCPERR_NONE :
                           (nTO >= 0 ? TCPERR_READ : TCPERR_TIMEOUT) );

    return( nRead );
}

int CTcpSock::Send( const void* pBuf, int nLen )
{
    if( !IsValid() )
    {
        SetError( TCPERR_INVALID );
        return( -1 );
    }
    if( nLen == 0 )
    {
        SetError( TCPERR_NONE );
        return( 0 );
    }
    int nTO = m_nSendTimeout > 0 ? m_nSendTimeout : 0;
    int nSent = sock_send( m_Sock, (const char*)pBuf, nLen, nTO ? &nTO : NULL );
    SetError( nSent == nLen ? TCPERR_NONE :
                              (nTO >= 0 ? TCPERR_SEND : TCPERR_TIMEOUT) );

    return( nSent );
}

BOOL CTcpSock::SetError( int nError, BOOL bDoCallback )
{
    if( (m_nError = nError) != TCPERR_NONE )
    {
        if( bDoCallback && m_ErrorCallback )
            m_ErrorCallback( m_pUserData, m_nError, GetErrorMessage() );
        return( FALSE );
    }
    return( TRUE );
}

TcpErrorCallback CTcpSock::SetErrorCallback( TcpErrorCallback pCallback )
{
    TcpErrorCallback pOldFunc = m_ErrorCallback;
    m_ErrorCallback = pCallback;
    return( pOldFunc );
}

BOOL CTcpSock::Wait( int nSecs, BOOL bForRead )
{
    if( !IsValid() ) return( SetError( TCPERR_INVALID ) );

    int nRet = sock_wait( m_Sock, nSecs, bForRead );
    SetError( nRet >= 0 ? TCPERR_NONE : TCPERR_WAIT );
    return( nRet > 0 ? TRUE : FALSE );
}

BOOL CTcpSock::WinsockStartup( LPWSADATA lpWSAData )
{
    return( !sock_init( lpWSAData ) );
}

BOOL CTcpSock::WinsockCleanup()
{
    return( !sock_uninit() );
}
