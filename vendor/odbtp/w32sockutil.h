/* $Id: w32sockutil.h,v 1.5 2004/05/19 23:22:28 rtwitty Exp $ */
#ifndef _W32SOCKUTIL_H_
#define _W32SOCKUTIL_H_

#include <winsock.h>

#ifdef __cplusplus
extern "C" {
#endif

u_long  get_netaddr( const char* address );
char*   get_sock_error_text( int sock_error );
int     set_nonblk_mode( SOCKET sock, u_long mode );
int     sock_close( SOCKET sock );
SOCKET  sock_connect( const char* address, u_short port, int* p_timeout,
                      int* p_sock_error, int* p_lookup_failed );
SOCKET  sock_create(void);
int     sock_init( LPWSADATA lpWSAData );
int     sock_read( SOCKET sock, char* buf, size_t count, int* p_timeout,
                   int* p_sock_error );
int     sock_send( SOCKET sock, const char* buf, size_t count, int* p_timeout,
                   int* p_sock_error );
int     sock_uninit(void);
int     sock_wait( SOCKET sock, int* p_timeout, int for_read,
                   int* p_sock_error );

#ifdef __cplusplus
}
#endif

#endif
