/* $Id: SockUtils.cpp,v 1.6 2004/06/02 20:12:20 rtwitty Exp $ */
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
#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SockUtils.h"

u_long get_netaddr( const char* address )
{
    struct hostent* host;
    const char*     ptr;

    /* First try it as aaa.bbb.ccc.ddd. */
    for( ptr = address; *ptr && (isdigit(*ptr) || *ptr == '.'); ptr++ );
    if( *ptr == 0 ) return( inet_addr( address ) );

    if( (host = gethostbyname( address )) )
        return( ((struct in_addr *)*host->h_addr_list)->s_addr );

    return( INADDR_NONE );
}

u_short get_netport( const char* service, const char* proto )
{
    const char*     ptr;
    struct servent* serv;

    /* First try it as a number */
    for( ptr = service; *ptr && isdigit(*ptr); ptr++ );
    if( *ptr == 0 ) return( htons( (u_short)atoi( service ) ) );

    /* Try to read it from services database */
    if( (serv = getservbyname( service, proto )) ) return( serv->s_port );

    return( (u_short)-1 );
}

char* get_peer_addr( SOCKET sock, char* buf, size_t len )
{
    struct sockaddr_in addr;
    int                addr_len;
    int                result;

    addr_len = sizeof( addr );
    result = getpeername( sock, (struct sockaddr FAR*)&addr, &addr_len );

    if( result == SOCKET_ERROR ) return( NULL );

    if( len > 0 )
    {
        strncpy( buf, inet_ntoa( addr.sin_addr ), len - 1 );
        buf[len - 1] = 0;
    }
    return( buf );
}

char* get_peer_hostname( SOCKET sock, char* buf, size_t len )
{
    struct sockaddr_in addr;
    int                addr_len;
    struct hostent*    phe;
    int                result;

    addr_len = sizeof( addr );
    result = getpeername( sock, (struct sockaddr FAR*)&addr, &addr_len );

    if( result == SOCKET_ERROR ) return( NULL );

    phe = gethostbyaddr( (const char FAR*)&addr.sin_addr.s_addr,
                         sizeof(addr.sin_addr.s_addr),
                         addr.sin_family );

    if( !phe ) return( NULL );

    if( len > 0 )
    {
        if( phe->h_name )
        {
            strncpy( buf, phe->h_name, len - 1 );
            buf[len - 1] = 0;
        }
        else
        {
            strncpy( buf, inet_ntoa( addr.sin_addr ), len - 1 );
        }
    }
    return( buf );
}

int get_peer_port( SOCKET sock )
{
    struct sockaddr_in addr;
    int                addr_len;
    int                result;
    int                port;

    addr_len = sizeof( addr );
    result = getpeername( sock, (struct sockaddr FAR*)&addr, &addr_len );

    if( result == SOCKET_ERROR ) return( SOCKET_ERROR );

    port = ntohs( addr.sin_port );

    return( port );
}

char* get_sock_addr( SOCKET sock, char* buf, size_t len )
{
    struct sockaddr_in addr;
    int                addr_len;
    int                result;

    addr_len = sizeof( addr );
    result = getsockname( sock, (struct sockaddr FAR*)&addr, &addr_len );

    if( result == SOCKET_ERROR ) return( NULL );

    if( len > 0 )
    {
        strncpy( buf, inet_ntoa( addr.sin_addr ), len - 1 );
        buf[len - 1] = 0;
    }
    return( buf );
}

char* get_sock_hostname( SOCKET sock, char* buf, size_t len )
{
    struct sockaddr_in addr;
    int                addr_len;
    struct hostent*    phe;
    int                result;

    addr_len = sizeof( addr );
    result = getsockname( sock, (struct sockaddr FAR*)&addr, &addr_len );

    if( result == SOCKET_ERROR ) return( NULL );

    phe = gethostbyaddr( (const char FAR*)&addr.sin_addr.s_addr,
                         sizeof(addr.sin_addr.s_addr),
                         addr.sin_family );

    if( !phe ) return( NULL );

    if( len > 0 )
    {
        if( phe->h_name )
        {
            strncpy( buf, phe->h_name, len - 1 );
            buf[len - 1] = 0;
        }
        else
        {
            strncpy( buf, inet_ntoa( addr.sin_addr ), len - 1 );
        }
    }
    return( buf );
}

int get_sock_port( SOCKET sock )
{
    struct sockaddr_in addr;
    int                addr_len;
    int                result;
    int                port;

    addr_len = sizeof( addr );
    result = getsockname( sock, (struct sockaddr FAR*)&addr, &addr_len );

    if( result == SOCKET_ERROR ) return( SOCKET_ERROR );

    port = ntohs( addr.sin_port );

    return( port );
}

int set_nonblk_mode( SOCKET sock, u_long mode )
{
    return( ioctlsocket( sock, FIONBIO, &mode ) );
}

SOCKET sock_accept( SOCKET listen_sock, int* p_timeout )
{
    int    rs;
    SOCKET sock;

    if( p_timeout && (rs = sock_wait( listen_sock, *p_timeout, TRUE )) <= 0 )
    {
        if( rs == 0 ) *p_timeout = -1;
        return( INVALID_SOCKET );
    }
    if( (sock = accept( listen_sock, NULL, NULL )) != INVALID_SOCKET )
    {
        struct linger linger = {1, 10};
        setsockopt( sock, SOL_SOCKET, SO_LINGER,
                    (char*)&linger, sizeof(linger) );
    }
    return( sock );
}

int sock_close( SOCKET sock )
{
    return( closesocket( sock ) );
}

SOCKET sock_connect( const char* address, u_short port, int type, int* p_timeout )
{
    struct sockaddr_in addr;
    SOCKET             connect_sock;

    ZeroMemory( (PVOID)&addr, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    if( (addr.sin_addr.s_addr = get_netaddr( address )) == INADDR_NONE )
        return( INVALID_SOCKET );

    if( (connect_sock = sock_create( type )) == INVALID_SOCKET )
        return( INVALID_SOCKET );

    if( type == SOCK_STREAM )
    {
        if( p_timeout )
        {
            fd_set         fds_error;
            fd_set         fds_write;
            int            rs = -1;
            struct timeval tv;

            tv.tv_sec  = *p_timeout;
            tv.tv_usec = 0;

            FD_ZERO( &fds_write );
            FD_SET( connect_sock, &fds_write );
            FD_ZERO( &fds_error );
            FD_SET( connect_sock, &fds_error );

            if( set_nonblk_mode( connect_sock, 1 ) ||
                ( connect( connect_sock, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR &&
                  ( (rs = select( 0, NULL, &fds_write, &fds_error, &tv )) <= 0 ||
                    !FD_ISSET( connect_sock, &fds_write ) ) ) ||
                set_nonblk_mode( connect_sock, 0 ) )
            {
                int last_error = WSAGetLastError();
                sock_close( connect_sock );
                if( rs == 0 ) *p_timeout = -1;
                WSASetLastError( last_error );
                return( INVALID_SOCKET );
            }
        }
        else if( connect( connect_sock, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR )
        {
            int last_error = WSAGetLastError();
            sock_close( connect_sock );
            WSASetLastError( last_error );
            return( INVALID_SOCKET );
        }
        return( connect_sock );
    }
    /* Otherwise, must be for udp, so bind to address. */
    if( bind( connect_sock, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR )
    {
        int last_error = WSAGetLastError();
        sock_close( connect_sock );
        WSASetLastError( last_error );
        return( INVALID_SOCKET );
    }
    return( connect_sock );
}

SOCKET sock_create( int type )
{
    return( socket( AF_INET, type, 0 ) );
}

int sock_init( LPWSADATA lpWSAData )
{
    WSADATA WSAData;
    if( !lpWSAData ) lpWSAData = &WSAData;
    return( WSAStartup( MAKEWORD(2,2), lpWSAData ) );
}

SOCKET sock_listen( const char* address, u_short port, int type, int backlog )
{
    struct sockaddr_in addr;
    SOCKET             listen_sock;
    int                reuse_addr = 1;

    /* Setup internet address information.
       This is used with the bind() call */
    ZeroMemory( (PVOID)&addr, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    if( address )
    {
        if( (addr.sin_addr.s_addr = get_netaddr( address )) == INADDR_NONE )
            return( INVALID_SOCKET );
    }
    else
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    if( (listen_sock = sock_create( type )) == INVALID_SOCKET )
        return( INVALID_SOCKET );

/*  setsockopt( listen_sock, SOL_SOCKET, SO_REUSEADDR,
                (char*)&reuse_addr, sizeof(reuse_addr) );
*/
    if( bind( listen_sock, (struct sockaddr *)&addr,
              sizeof(addr) ) == SOCKET_ERROR )
    {
        int last_error = WSAGetLastError();
        sock_close( listen_sock );
        WSASetLastError( last_error );
        return( INVALID_SOCKET );
    }
    if( type == SOCK_STREAM && listen( listen_sock, backlog ) == SOCKET_ERROR )
    {
        int last_error = WSAGetLastError();
        sock_close( listen_sock );
        WSASetLastError( last_error );
        return( INVALID_SOCKET );
    }
    return( listen_sock );
}

int sock_read( SOCKET sock, char* buf, size_t count, int stop_byte, int* p_timeout )
{
    size_t bytes_read = 0;
    int    rs;

    if( stop_byte == -1 )
    {
        if( p_timeout && (rs = sock_wait( sock, *p_timeout, TRUE )) <= 0 )
        {
            if( rs == 0 ) *p_timeout = -1;
            return( SOCKET_ERROR );
        }
        bytes_read = recv( sock, buf, count, 0 );
        return( bytes_read >= 0 ? bytes_read : SOCKET_ERROR );
    }
    else
    {
        int this_read;

        while( bytes_read < count )
        {
            if( p_timeout && (rs = sock_wait( sock, *p_timeout, TRUE )) <= 0 )
            {
                if( rs == 0 ) *p_timeout = -1;
                return( SOCKET_ERROR );
            }
            this_read = recv( sock, buf, 1, 0 );
            if( this_read == 0 ) return( bytes_read );
            if( this_read == SOCKET_ERROR ) return( SOCKET_ERROR );
            bytes_read++;
            if( *(buf++) == (char)stop_byte ) return( bytes_read );
        }
    }
    return( count );
}

int sock_readfrom( SOCKET listen_sock, char* buf, size_t count, int* p_timeout )
{
    size_t bytes_read = 0;
    int    rs;

    if( p_timeout && (rs = sock_wait( listen_sock, *p_timeout, TRUE )) <= 0 )
    {
        if( rs == 0 ) *p_timeout = -1;
        return( SOCKET_ERROR );
    }
    bytes_read = recvfrom( listen_sock, buf, count, 0, NULL, NULL );
    return( bytes_read >= 0 ? bytes_read : SOCKET_ERROR );
}

int sock_send( SOCKET sock, const char *buf, size_t count, int* p_timeout )
{
    size_t bytes_sent = 0;
    int    rs;
    int    this_send;

    while( bytes_sent < count )
    {
        if( p_timeout && (rs = sock_wait( sock, *p_timeout, FALSE )) <= 0 )
        {
            if( rs == 0 ) *p_timeout = -1;
            return( SOCKET_ERROR );
        }
        this_send = send( sock, buf, count - bytes_sent, 0 );
        if( this_send == SOCKET_ERROR ) return( SOCKET_ERROR );

        bytes_sent += this_send;
        buf += this_send;
    }
    return( count );
}

int sock_sendto( SOCKET sock, const char *buf, size_t count, const char* address, u_short port, int* p_timeout )
{
    struct sockaddr_in addr;
    size_t             bytes_sent = 0;
    int                rs;
    int                this_send;

    ZeroMemory( (PVOID)&addr, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    if( (addr.sin_addr.s_addr = get_netaddr( address )) == INADDR_NONE )
        return( SOCKET_ERROR );

    while( bytes_sent < count )
    {
        if( p_timeout && (rs = sock_wait( sock, *p_timeout, FALSE )) <= 0 )
        {
            if( rs == 0 ) *p_timeout = -1;
            return( SOCKET_ERROR );
        }
        this_send = sendto( sock, buf, count - bytes_sent, 0, (struct sockaddr *)&addr, sizeof(addr) );
        if( this_send == 0 ) return( bytes_sent );
        if( this_send == SOCKET_ERROR ) return( SOCKET_ERROR );

        bytes_sent += this_send;
        buf += this_send;
    }
    return( count );
}

int sock_uninit()
{
    return( WSACleanup() );
}

int sock_wait( SOCKET sock, int timeout, int for_read )
{
    fd_set         fds_test;
    int            rstatus;
    struct timeval tv;

    tv.tv_sec  = timeout;
    tv.tv_usec = 0;

    FD_ZERO( &fds_test );
    FD_SET( sock, &fds_test );

    if( for_read )
        rstatus = select( 0, &fds_test, NULL, NULL, &tv );
    else
        rstatus = select( 0, NULL, &fds_test, NULL, &tv );

    if( rstatus == SOCKET_ERROR )
        return( SOCKET_ERROR );
    if( rstatus == 0 || !FD_ISSET( sock, &fds_test ) )
        return( 0 );
    return( 1 );
}
