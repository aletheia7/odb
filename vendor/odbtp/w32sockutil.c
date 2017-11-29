/* $Id: w32sockutil.c,v 1.8 2004/05/19 23:22:00 rtwitty Exp $ */
#define _WINSOCKAPI_
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "w32sockutil.h"

u_long get_netaddr( const char* address )
{
    struct hostent* host;
    int             n_try = 0;
    const char*     ptr;

    /* First try it as aaa.bbb.ccc.ddd. */
    for( ptr = address; *ptr && (isdigit((int)*ptr) || *ptr == '.'); ptr++ );
    if( *ptr == 0 ) return( inet_addr( address ) );

    do {
        if( n_try ) Sleep( 5000 );

        if( (host = gethostbyname( address )) ) {
            u_long sin_addr;
            memcpy( &sin_addr, (void *)host->h_addr_list[0], sizeof(u_long) );
            return( sin_addr );
        }
        if( WSAGetLastError() != WSATRY_AGAIN ) break;
    }
    while( ++n_try < 3 );

    return( INADDR_NONE );
}

char* get_sock_error_text( int sock_error )
{
    switch( sock_error ) {
        case 0:                  return( "Error 0" );
        case WSAEINTR:           return( "Interrupted system call" );
        case WSAEBADF:           return( "Bad file number" );
        case WSAEACCES:          return( "Permission denied" );
        case WSAEFAULT:          return( "Bad address" );
        case WSAEINVAL:          return( "Invalid argument" );
        case WSAEMFILE:          return( "Too many open files" );
        case WSAEWOULDBLOCK:     return( "Resource temporarily unavailable" );
        case WSAEINPROGRESS:     return( "Operation now in progress" );
        case WSAEALREADY:        return( "Operation already in progress" );
        case WSAENOTSOCK:        return( "Socket operation on non-socket" );
        case WSAEDESTADDRREQ:    return( "Destination address required" );
        case WSAEMSGSIZE:        return( "Message too long" );
        case WSAEPROTOTYPE:      return( "Protocol wrong type for socket" );
        case WSAENOPROTOOPT:     return( "Option not supported by protocol" );
        case WSAEPROTONOSUPPORT: return( "Protocol not supported" );
        case WSAESOCKTNOSUPPORT: return( "Socket type not supported" );
        case WSAEOPNOTSUPP:      return( "Operation not supported on transport endpoint" );
        case WSAEPFNOSUPPORT:    return( "Protocol family not supported" );
        case WSAEAFNOSUPPORT:    return( "Address family not supported by protocol family" );
        case WSAEADDRINUSE:      return( "Address already in use" );
        case WSAEADDRNOTAVAIL:   return( "Cannot assign requested address" );
        case WSAENETDOWN:        return( "Network is down" );
        case WSAENETUNREACH:     return( "Network is unreachable" );
        case WSAENETRESET:       return( "Network dropped connection because of reset" );
        case WSAECONNABORTED:    return( "Software caused connection abort" );
        case WSAECONNRESET:      return( "Connection reset by peer" );
        case WSAENOBUFS:         return( "No buffer space available" );
        case WSAEISCONN:         return( "Transport endpoint is already connected" );
        case WSAENOTCONN:        return( "Transport endpoint is not connected" );
        case WSAESHUTDOWN:       return( "Cannot send after socket shutdown" );
        case WSAETOOMANYREFS:    return( "Too many references: cannot splice" );
        case WSAETIMEDOUT:       return( "Connection timed out" );
        case WSAECONNREFUSED:    return( "Connection refused" );
        case WSAELOOP:           return( "Number of symbolic links encountered during path name traversal exceeds MAXSYMLINKS" );
        case WSAENAMETOOLONG:    return( "File name too long" );
        case WSAEHOSTDOWN:       return( "Host is down" );
        case WSAEHOSTUNREACH:    return( "No route to host" );
        case WSAENOTEMPTY:       return( "Directory not empty" );
        case WSAEUSERS:          return( "Too many users" );
        case WSAEDQUOT:          return( "Disc quota exceeded" );
        case WSAESTALE:          return( "Stale NFS file handle" );
        case WSAEREMOTE:         return( "Object is remote" );
    }
    return( "Unknown error" );
}

int set_nonblk_mode( SOCKET sock, u_long mode )
{
    return( ioctlsocket( sock, FIONBIO, &mode ) );
}

int sock_close( SOCKET sock )
{
    return( closesocket( sock ) );
}

SOCKET sock_connect( const char* address, u_short port, int* p_timeout,
                     int* p_sock_error, int* p_lookup_failed )
{
    struct sockaddr_in addr;
    SOCKET             connect_sock;
    struct linger      linger = {1, 0};

    *p_sock_error = 0;

    ZeroMemory( (PVOID)&addr, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    if( (addr.sin_addr.s_addr = get_netaddr( address )) == INADDR_NONE ) {
        *p_lookup_failed = -1;
        return( INVALID_SOCKET );
    }
    *p_lookup_failed = 0;

    if( (connect_sock = sock_create()) == INVALID_SOCKET ) {
        *p_sock_error = WSAGetLastError();
        return( INVALID_SOCKET );
    }
    setsockopt( connect_sock, SOL_SOCKET, SO_LINGER,
                (char*)&linger, sizeof(linger) );

    if( p_timeout ) {
        fd_set         fds_error;
        fd_set         fds_write;
        int            rs = -1;
        int            time_now;
        int            time_start;
        struct timeval tv;

        tv.tv_sec  = *p_timeout;
        tv.tv_usec = 0;

        FD_ZERO( &fds_write );
        FD_SET( connect_sock, &fds_write );
        FD_ZERO( &fds_error );
        FD_SET( connect_sock, &fds_error );

        set_nonblk_mode( connect_sock, 1 );
        time_start = time(NULL);

        if( connect( connect_sock, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR &&
            ( (rs = select( 0, NULL, &fds_write, &fds_error, &tv )) <= 0 ||
              !FD_ISSET( connect_sock, &fds_write ) ) )
        {
            if( rs > 0 ) {
                int len = sizeof(int);
                getsockopt( connect_sock, SOL_SOCKET, SO_ERROR,
                            (char*)p_sock_error, &len );
            }
            else if( rs == 0 ) {
                *p_timeout = -1;
            }
            else {
                *p_sock_error = WSAGetLastError();
            }
            sock_close( connect_sock );
            return( INVALID_SOCKET );
        }
        time_now = time(NULL);
        set_nonblk_mode( connect_sock, 0 );

        if( (*p_timeout -= (time_now - time_start)) < 0 ) *p_timeout = 0;
    }
    else if( connect( connect_sock, (struct sockaddr *)&addr, sizeof(addr) ) == SOCKET_ERROR ) {
        *p_sock_error = WSAGetLastError();
        sock_close( connect_sock );
        return( INVALID_SOCKET );
    }
    return( connect_sock );
}

SOCKET sock_create(void)
{
    return( socket( AF_INET, SOCK_STREAM, 0 ) );
}

int sock_init( LPWSADATA lpWSAData )
{
    WSADATA WSAData;
    if( !lpWSAData ) lpWSAData = &WSAData;
    return( WSAStartup( MAKEWORD(2,2), lpWSAData ) );
}

int sock_read( SOCKET sock, char* buf, size_t count, int* p_timeout,
                      int* p_sock_error )
{
    size_t bytes_read = 0;

    *p_sock_error = 0;

    if( p_timeout && sock_wait( sock, p_timeout, TRUE, p_sock_error ) <= 0 )
        return( SOCKET_ERROR );

    if( (bytes_read = recv( sock, buf, count, 0 )) < 0 ) {
        *p_sock_error = WSAGetLastError();
        return( SOCKET_ERROR );
    }
    return( bytes_read );
}

int sock_send( SOCKET sock, const char *buf, size_t count, int* p_timeout,
                      int* p_sock_error )
{
    size_t bytes_sent = 0;
    int    this_send;
    int    timeout = 0;

    *p_sock_error = 0;

    if( p_timeout ) timeout = *p_timeout;

    while( bytes_sent < count ) {
        if( p_timeout && sock_wait( sock, p_timeout, FALSE, p_sock_error ) <= 0 )
            return( SOCKET_ERROR );

        this_send = send( sock, buf, count - bytes_sent, 0 );
        if( this_send == SOCKET_ERROR ) {
            *p_sock_error = WSAGetLastError();
            return( SOCKET_ERROR );
        }
        if( p_timeout && this_send > 0 ) *p_timeout = timeout;

        bytes_sent += this_send;
        buf += this_send;
    }
    return( count );
}

int sock_uninit(void)
{
    return( WSACleanup() );
}

int sock_wait( SOCKET sock, int* p_timeout, int for_read,
                      int* p_sock_error )
{
    fd_set         fds_test;
    int            rstatus;
    int            time_now;
    int            time_start;
    struct timeval tv;

    *p_sock_error = 0;

    tv.tv_sec  = *p_timeout;
    tv.tv_usec = 0;

    FD_ZERO( &fds_test );
    FD_SET( sock, &fds_test );

    time_start = time(NULL);

    if( for_read )
        rstatus = select( 0, &fds_test, NULL, NULL, &tv );
    else
        rstatus = select( 0, NULL, &fds_test, NULL, &tv );

    if( rstatus == SOCKET_ERROR ) {
        *p_sock_error = WSAGetLastError();
        return( SOCKET_ERROR );
    }
    if( rstatus == 0 ) {
        *p_timeout = -1;
        return( 0 );
    }
    time_now = time(NULL);

    if( (*p_timeout -= (time_now - time_start)) < 0 ) *p_timeout = 0;

    return( 1 );
}
