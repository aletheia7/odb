/* $Id: UTF8Support.cpp,v 1.2 2004/06/02 20:12:21 rtwitty Exp $ */
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
#include "stdafx.h"
#include "UTF8Support.h"

UINT UCS2Len( const char* pszUTF8 )
{
    BYTE cBit;
    BYTE cByte;
    UINT nBytes;
    UINT nLen = 0;

    for( ; *pszUTF8; pszUTF8++ )
    {
        nLen++;
        cByte = (BYTE)*pszUTF8;

        for( cBit = 0x80, nBytes = 0; (cByte & cBit); cBit >>= 1, nBytes++ );

        if( nBytes <= 1 || nBytes > 6 ) continue;

        for( nBytes--; nBytes > 0; nBytes-- )
        {
            if( (*(++pszUTF8) & 0xC0) != 0x80 )
            {
                pszUTF8--;
                break;
            }
        }
    }
    return( nLen );
}

wchar_t* UTF8Convert( const char* pszUTF8 )
{
    UINT     n;
    int      nBytesDecoded;
    UINT     nLen;
    wchar_t* pszUCS2;

    nLen = UCS2Len( pszUTF8 );

    if( !(pszUCS2 = (wchar_t*)malloc( (nLen + 1) * sizeof(wchar_t) )) )
        return( NULL );

    for( n = 0; n < nLen; n++, pszUTF8 += nBytesDecoded )
        nBytesDecoded = UTF8Decode( &pszUCS2[n], pszUTF8 );

    pszUCS2[nLen] = 0;

    return( pszUCS2 );
}

int UTF8Decode( wchar_t* pchUCS2, const char* pchUTF8 )
{
    BYTE cBit;
    BYTE cByte;
    int  nBytes;
    int  nBytesDecoded;
    UINT nUnicode;

    cByte = (BYTE)*pchUTF8;

    for( cBit = 0x80, nBytes = 0; (cByte & cBit); cBit >>= 1, nBytes++ )
        cByte ^= cBit;

    if( nBytes == 0 )
    {
        *pchUCS2 = cByte;
        return( 1 );
    }
    if( nBytes == 1 || nBytes > 6 )
    {
        *pchUCS2 = L'?';
        return( 1 );
    }
    nUnicode = cByte;
    nBytesDecoded = nBytes;

    for( nBytes--; nBytes > 0; nBytes-- )
    {
        if( (*(++pchUTF8) & 0xC0) != 0x80 )
        {
            *pchUCS2 = L'?';
            return( nBytesDecoded - nBytes );
        }
        nUnicode = (nUnicode << 6) + (*pchUTF8 & 0x3F);
    }
    *pchUCS2 = (wchar_t)nUnicode;
    return( nBytesDecoded );
}

int UTF8Encode( char* pchUTF8, wchar_t chUCS2 )
{
    if( chUCS2 < 0x80 )
    {
        *pchUTF8 = (BYTE)chUCS2;
        return( 1 );
    }
    else if( chUCS2 < 0x800 )
    {
        *(pchUTF8++) = (BYTE)(0xC0 | (chUCS2 >> 6));
        *pchUTF8 = (BYTE)(0x80 | (chUCS2 & 0x3F));
        return( 2 );
    }
    *(pchUTF8++) = (BYTE)(0xE0 | (chUCS2 >> 12));
    *(pchUTF8++) = (BYTE)(0x80 | ((chUCS2 >> 6) & 0x3F));
    *pchUTF8 = (BYTE)(0x80 | (chUCS2 & 0x3F));
    return( 3 );
}

UINT UTF8Len( const wchar_t* pszUCS2 )
{
    UINT nUTF8Len = 0;

    for( ; *pszUCS2; pszUCS2++ )
    {
        if( *pszUCS2 < 0x80 )
            nUTF8Len += 1;
        else if( *pszUCS2 < 0x800 )
            nUTF8Len += 2;
        else
            nUTF8Len += 3;
    }
    return( nUTF8Len );
}

