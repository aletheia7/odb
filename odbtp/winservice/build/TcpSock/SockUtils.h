/* $Id: SockUtils.h,v 1.2 2004/06/02 20:12:20 rtwitty Exp $ */
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
#ifndef _SOCKUTILS_H_
#define _SOCKUTILS_H_

#include <winsock.h>

#ifdef __cplusplus
extern "C" {
#endif

u_long  get_netaddr( const char* address );
u_short get_netport( const char* service, const char* proto );
char*   get_peer_addr( SOCKET sock, char* buf, size_t len );
char*   get_peer_hostname( SOCKET sock, char* buf, size_t len );
int     get_peer_port( SOCKET sock );
char*   get_sock_addr( SOCKET sock, char* buf, size_t len );
char*   get_sock_hostname( SOCKET sock, char* buf, size_t len );
int     get_sock_port( SOCKET sock );
int     set_nonblk_mode( SOCKET sock, u_long mode );
SOCKET  sock_accept( SOCKET listen_sock, int* p_timeout );
int     sock_close( SOCKET sock );
SOCKET  sock_connect( const char* address, u_short port, int type, int* p_timeout );
SOCKET  sock_create( int type );
int     sock_init( LPWSADATA lpWSAData );
SOCKET  sock_listen( const char* address, u_short port, int type, int backlog );
int     sock_read( SOCKET sock, char* buf, size_t count, int stop_byte, int* p_timeout );
int     sock_readfrom( SOCKET listen_sock, char* buf, size_t count, int* p_timeout );
int     sock_send( SOCKET sock, const char *buf, size_t count, int* p_timeout );
int     sock_sendto( SOCKET sock, const char *buf, size_t count, const char* address, u_short port, int* p_timeout );
int     sock_uninit();
int     sock_wait( SOCKET sock, int timeout, int for_read );

#ifdef __cplusplus
}
#endif

#endif
