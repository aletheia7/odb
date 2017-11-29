/* $Id: sockutil.c,v 1.12 2004/06/20 20:11:19 rtwitty Exp $ */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#ifdef NETWARE
#include <sys/select.h>
#endif
#include "sockutil.h"

u_long get_netaddr( const char* address )
{
    struct hostent* host;
    int             n_try = 0;
    const char*     ptr;

    /* First try it as aaa.bbb.ccc.ddd. */
    for( ptr = address; *ptr && (isdigit((int)*ptr) || *ptr == '.'); ptr++ );
    if( *ptr == 0 ) return( inet_addr( address ) );

#ifdef TRY_AGAIN
    do {
        if( n_try ) sleep( 5 );
#endif

        if( (host = gethostbyname( address )) ) {
            u_long sin_addr;
            memcpy( &sin_addr, (void *)host->h_addr_list[0], sizeof(u_long) );
            return( sin_addr );
        }

#ifdef TRY_AGAIN
        if( h_errno != TRY_AGAIN ) break;
    }
    while( ++n_try < 3 );
#endif

    return( INADDR_NONE );
}

char* get_sock_error_text( int sock_error )
{
    char* errstr = strerror( sock_error );
    return( errstr ? errstr : "Unknown error" );
}

void ignore_sigpipe(void)
{
    struct sigaction sig;

    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset( &sig.sa_mask );
    sigaction( SIGPIPE, &sig, NULL );
}

int set_nonblk_mode( SOCKET sock, u_long mode )
{
    int sock_flags = fcntl( sock, F_GETFL );
    if( mode )
        sock_flags |= O_NONBLOCK;
    else
        sock_flags &= ~O_NONBLOCK;
    return( fcntl( sock, F_SETFL, sock_flags ) );
}

int sock_close( SOCKET sock )
{
    return( close( sock ) );
}

SOCKET sock_connect( const char* address, u_short port, int* p_timeout,
                     int* p_sock_error, int* p_lookup_failed )
{
    struct sockaddr_in addr;
    SOCKET             connect_sock;
    struct linger      linger = {1, 0};

    *p_sock_error = 0;

    memset( (char*) &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    if( (addr.sin_addr.s_addr = get_netaddr( address )) == INADDR_NONE ) {
        *p_lookup_failed = -1;
        return( INVALID_SOCKET );
    }
    *p_lookup_failed = 0;

    if( (connect_sock = sock_create()) < 0 ) {
        *p_sock_error = errno;
        return( INVALID_SOCKET );
    }
    setsockopt( connect_sock, SOL_SOCKET, SO_LINGER,
                (char*)&linger, sizeof(linger) );

    if( p_timeout ) {
        int addr_len = sizeof(addr);

        set_nonblk_mode( connect_sock, 1 );

        if( connect( connect_sock, (struct sockaddr*)&addr, sizeof(addr) ) < 0 &&
            ( sock_wait( connect_sock, p_timeout, FALSE, p_sock_error ) <= 0 ||
              getpeername( connect_sock, (struct sockaddr*)&addr, &addr_len ) ) )
        {
            if( *p_sock_error == 0 && *p_timeout >= 0 ) {
                connect( connect_sock, (struct sockaddr*)&addr, sizeof(addr) );
                *p_sock_error = errno;
            }
            sock_close( connect_sock );
            return( INVALID_SOCKET );
        }
        set_nonblk_mode( connect_sock, 0 );
    }
    else {
        int rs;

        do {
            rs = connect( connect_sock, (struct sockaddr*)&addr, sizeof(addr) );

            if( rs < 0 && errno != EINTR ) {
                *p_sock_error = errno;
                sock_close( connect_sock );
                return( INVALID_SOCKET );
            }
        }
        while( rs < 0 );
    }
    return( connect_sock );
}

SOCKET sock_create(void)
{
    return( socket( AF_INET, SOCK_STREAM, 0 ) );
}

int sock_read( SOCKET sock, char* buf, size_t count, int* p_timeout,
                      int* p_sock_error )
{
    size_t bytes_read = 0;

    *p_sock_error = 0;

    do {
        if( p_timeout && sock_wait( sock, p_timeout, TRUE, p_sock_error ) <= 0 )
            return( SOCKET_ERROR );

        bytes_read = read( sock, buf, count );
        if( bytes_read < 0 && errno != EINTR ) {
            *p_sock_error = errno;
            return( SOCKET_ERROR );
        }
    }
    while( bytes_read < 0 );

    return( bytes_read );
}

int sock_send( SOCKET sock, const char* buf, size_t count, int* p_timeout,
                      int* p_sock_error )
{
    size_t bytes_sent = 0;
    int    this_send;
    int    timeout = 0;

    *p_sock_error = 0;

    if( p_timeout ) timeout = *p_timeout;

    while( bytes_sent < count ) {
        if( p_timeout ) *p_timeout = timeout;

        do {
            if( p_timeout && sock_wait( sock, p_timeout, FALSE, p_sock_error ) <= 0 )
                return( SOCKET_ERROR );

            this_send = write( sock, buf, count - bytes_sent );
            if( this_send < 0 && errno != EINTR ) {
                *p_sock_error = errno;
                return( SOCKET_ERROR );
            }
        }
        while( this_send <= 0 );

        bytes_sent += this_send;
        buf += this_send;
    }
    return( count );
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

    time_start = time(NULL);
    time_now = time_start;

    do {
        tv.tv_sec  = *p_timeout - (time_now - time_start);
        tv.tv_usec = 0;

        FD_ZERO( &fds_test );
        FD_SET( sock, &fds_test );

        if( for_read )
            rstatus = select( sock + 1, &fds_test, NULL, NULL, &tv );
        else
            rstatus = select( sock + 1, NULL, &fds_test, NULL, &tv );

        if( rstatus < 0 && errno != EINTR ) {
            *p_sock_error = errno;
            return( SOCKET_ERROR );
        }
        time_now = time(NULL);
    }
    while( rstatus < 0 && (time_now - time_start) < *p_timeout );

    if( rstatus == 0 ) {
        *p_timeout = -1;
        return( 0 );
    }
    if( (*p_timeout -= (time_now - time_start)) < 0 ) *p_timeout = 0;

    return( 1 );
}
