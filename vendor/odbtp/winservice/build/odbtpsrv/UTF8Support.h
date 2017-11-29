/* $Id: UTF8Support.h,v 1.2 2004/06/02 20:12:21 rtwitty Exp $ */
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
// OdbtpSock.h: interface for the COdbtpSock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_UTF8SUPPORT_H_)
#define _UTF8SUPPORT_H_

UINT     UCS2Len( const char* pszUTF8 );
wchar_t* UTF8Convert( const char* pszUTF8 );
int      UTF8Decode( wchar_t* pchUCS2, const char* pchUTF8 );
int      UTF8Encode( char* pchUTF8, wchar_t chUCS2 );
UINT     UTF8Len( const wchar_t* pszUCS2 );

#endif
