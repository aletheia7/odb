/* $Id: odbtypes.cpp,v 1.3 2004/08/15 01:49:24 rtwitty Exp $ */
/*
    Odbtypes - Data type classes used by Odb class library

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
#define STRICT 1
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sql.h>
#include <sqlext.h>

#include "odbtypes.h"

/////////////////////////////////////////////////////////////////////////////
// OdbBuffer - Data buffer

OdbBuffer::OdbBuffer()
{
    m_pBuf = NULL;
    m_nRef = 0;
}

OdbBuffer::~OdbBuffer()
{
    Free();
}

SQLPOINTER OdbBuffer::Alloc( SQLINTEGER nSize )
{
    Free();
    if( (m_pBuf = malloc( nSize )) ) m_nRef = 1;
    return( m_pBuf );
}

void OdbBuffer::Free()
{
    if( m_pBuf )
    {
        free( m_pBuf );
        m_pBuf = NULL;
        m_nRef = 0;
    }
}

SQLPOINTER OdbBuffer::Ptr()
{
    return( m_pBuf );
}

SQLPOINTER OdbBuffer::Realloc( SQLINTEGER nSize )
{
    if( !m_pBuf ) return( Alloc( nSize ) );
    m_pBuf = realloc( m_pBuf, nSize );
    return( m_pBuf );
}

SQLSMALLINT OdbBuffer::RefDec()
{
    return( --m_nRef );
}

SQLSMALLINT OdbBuffer::RefInc()
{
    return( ++m_nRef );
}

/////////////////////////////////////////////////////////////////////////////
// OdbDATA - Odbc Data Type Class

OdbDATA::OdbDATA( SQLSMALLINT nType )
{
    m_nType = nType;
    m_pVal = NULL;
    m_nLI = 0;
    m_nSavedLen = 0;
    m_nMaxLen = 0;
}

OdbDATA::~OdbDATA()
{
}

void OdbDATA::Clear()
{
    if( m_pVal ) memset( m_pVal, 0, m_nMaxLen );
}

BOOL OdbDATA::Default() const
{
    return( m_nLI == SQL_DEFAULT_PARAM );
}

void OdbDATA::Default( BOOL bDefault )
{
    if( bDefault )
    {
        if( m_nLI >= 0 ) m_nSavedLen = m_nLI;
        m_nLI = SQL_DEFAULT_PARAM;
    }
    else if( m_nLI == SQL_DEFAULT_PARAM )
    {
        m_nLI = m_nSavedLen;
    }
}

BOOL OdbDATA::Ignore() const
{
    return( m_nLI == SQL_COLUMN_IGNORE );
}

void OdbDATA::Ignore( BOOL bIgnore )
{
    if( bIgnore )
    {
        if( m_nLI >= 0 ) m_nSavedLen = m_nLI;
        m_nLI = SQL_COLUMN_IGNORE;
    }
    else if( m_nLI == SQL_COLUMN_IGNORE )
    {
        m_nLI = m_nSavedLen;
    }
}

void OdbDATA::Init( SQLPOINTER pVal, SQLINTEGER nLen, SQLINTEGER nMaxLen )
{
    m_pVal = pVal;
    m_nLI = nLen;
    m_nSavedLen = nLen;
    m_nMaxLen = nMaxLen > 0 ? nMaxLen : nLen;
    if( m_pVal ) memset( m_pVal, 0, m_nMaxLen );
    Null(TRUE);
}

SQLINTEGER OdbDATA::Indicator() const
{
    return( m_nLI );
}

SQLINTEGER OdbDATA::Indicator( SQLINTEGER nNewVal )
{
    if( nNewVal >= 0 )
    {
        m_nLI = nNewVal < m_nMaxLen ? nNewVal : m_nMaxLen;
        m_nSavedLen = m_nLI;
    }
    else
    {
        if( m_nLI >= 0 ) m_nSavedLen = m_nLI;
        m_nLI = nNewVal;
    }
    return( m_nLI );
}

SQLINTEGER OdbDATA::MaxLen() const
{
    return( m_nMaxLen );
}

BOOL OdbDATA::Null() const
{
    return( m_nLI == SQL_NULL_DATA );
}

void OdbDATA::Null( BOOL bNull )
{
    if( bNull )
    {
        if( m_nLI >= 0 ) m_nSavedLen = m_nLI;
        m_nLI = SQL_NULL_DATA;
    }
    else if( m_nLI == SQL_NULL_DATA )
    {
        m_nLI = m_nSavedLen;
    }
}

SQLPOINTER OdbDATA::Ptr() const
{
    return( m_pVal );
}

SQLINTEGER OdbDATA::RestoreLen()
{
    return( m_nLI = m_nLI >= 0 ? m_nLI : m_nSavedLen );
}

/////////////////////////////////////////////////////////////////////////////
// OdbVARDATA - Odbc Data Type Class

OdbVARDATA::OdbVARDATA( SQLINTEGER nMaxLen, SQLSMALLINT nType ) : OdbDATA( nType )
{
    m_Buffer = NULL;
    m_Val = NULL;
    if( nMaxLen > 0 ) Alloc( nMaxLen );
}

OdbVARDATA::~OdbVARDATA()
{
    if( m_Buffer && m_Buffer->RefDec() <= 0 ) delete m_Buffer;
}

BOOL OdbVARDATA::Alloc( SQLINTEGER nMaxLen )
{
    Free();
    if( !(m_Buffer = new OdbBuffer) ) return( FALSE );
    if( (m_Val = (char*) m_Buffer->Alloc( nMaxLen )) )
        Init( m_Val, 0, nMaxLen );
    return( m_Val != NULL );
}

void OdbVARDATA::Free()
{
    if( m_Buffer && m_Buffer->RefDec() <= 0 ) delete m_Buffer;

    if( m_Buffer )
    {
        m_Buffer = NULL;
        m_Val = NULL;
        Init( NULL, 0 );
    }
}

SQLINTEGER OdbVARDATA::Len() const
{
    return( m_nLI >= 0 ? m_nLI : m_nSavedLen );
}

SQLINTEGER OdbVARDATA::Len( SQLINTEGER nNewVal )
{
    if( !m_Val ) return( 0 );

    if( m_nType == SQL_C_CHAR )
    {
        if( nNewVal > 0 )
            m_nSavedLen = nNewVal < m_nMaxLen - 1 ? nNewVal : m_nMaxLen - 1;
        else
            m_nSavedLen = 0;
        m_Val[m_nSavedLen] = 0;
    }
    else
    {
        if( nNewVal > 0 )
            m_nSavedLen = nNewVal < m_nMaxLen ? nNewVal : m_nMaxLen;
        else
            m_nSavedLen = 0;
    }
    m_nLI = m_nSavedLen;
    return( m_nLI );
}

BOOL OdbVARDATA::NoTotal() const
{
    return( m_nLI == SQL_NO_TOTAL );
}

BOOL OdbVARDATA::Realloc( SQLINTEGER nMaxLen )
{
    if( !m_Buffer ) return( Alloc( nMaxLen ) );

    if( m_nLI >= 0 && m_nLI != m_nSavedLen ) m_nSavedLen = m_nLI;

    if( nMaxLen > 0 && nMaxLen != m_nMaxLen )
    {
        if( (m_Val = (char*) m_Buffer->Realloc( nMaxLen )) )
        {
            m_pVal = m_Val;
            m_nMaxLen = nMaxLen;
            if( m_nType == SQL_C_CHAR )
            {
                nMaxLen -= 1;
                m_nSavedLen = nMaxLen < m_nSavedLen ? nMaxLen : m_nSavedLen;
                m_Val[m_nSavedLen] = 0;
            }
            else
            {
                m_nSavedLen = nMaxLen < m_nSavedLen ? nMaxLen : m_nSavedLen;
            }
            if( m_nLI >= 0 ) m_nLI = m_nSavedLen;
        }
        else
        {
            Init( NULL, 0 );
        }
    }
    return( m_Val != NULL );
}

void OdbVARDATA::SetLenAtExec( SQLINTEGER nVal )
{
    if( m_nLI >= 0 ) m_nSavedLen = m_nLI;
    m_nLI = SQL_LEN_DATA_AT_EXEC( nVal );
}

/////////////////////////////////////////////////////////////////////////////
// OdbBINARY - Odbc Data Type Class

OdbBINARY::OdbBINARY( SQLINTEGER nMaxLen ) : OdbVARDATA( nMaxLen, SQL_C_BINARY )
{
}

OdbBINARY::~OdbBINARY()
{
}

const OdbBINARY& OdbBINARY::operator=( OdbBINARY& src )
{
    if( src.m_Val )
    {
        if( src.m_nLI >= 0 )
        {
            Set( (SQLCHAR*)src, src.m_nLI );
        }
        else
        {
            Set( (SQLCHAR*)src, src.m_nSavedLen );
            m_nLI = src.m_nLI;
        }
    }
    return( *this );
}

SQLCHAR& OdbBINARY::operator[]( int nIndex )
{
    return( *((SQLCHAR*)(m_Val + nIndex)) );
}

BOOL OdbBINARY::Set( SQLCHAR* src, SQLINTEGER nLen )
{
    if( m_Val == (char*)src ) return( TRUE );

    if( !m_Val || m_nMaxLen < nLen )
        if( !Realloc( nLen ) ) return( FALSE );

    SQLINTEGER i;
    char*      to = m_Val;
    char*      from = (char*)src;

    for( i = 0; i < nLen; i++ ) *(to++) = *(from++);
    Len( nLen );

    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// OdbBIGINT - Odbc Data Type Class

OdbBIGINT::OdbBIGINT() : OdbDATA( SQL_C_SBIGINT )
{
    Init( &m_Val, sizeof(SQLBIGINT) );
}

OdbBIGINT::~OdbBIGINT()
{
}

const OdbBIGINT& OdbBIGINT::operator=( OdbBIGINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator=( SQLBIGINT src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator+=( SQLBIGINT src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator-=( SQLBIGINT src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator*=( SQLBIGINT src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator/=( SQLBIGINT src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator&=( SQLBIGINT src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator|=( SQLBIGINT src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIGINT& OdbBIGINT::operator^=( SQLBIGINT src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbUBIGINT - Odbc Data Type Class

OdbUBIGINT::OdbUBIGINT() : OdbDATA( SQL_C_UBIGINT )
{
    Init( &m_Val, sizeof(SQLUBIGINT) );
}

OdbUBIGINT::~OdbUBIGINT()
{
}

const OdbUBIGINT& OdbUBIGINT::operator=( OdbUBIGINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator=( SQLUBIGINT src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator+=( SQLUBIGINT src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator-=( SQLUBIGINT src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator*=( SQLUBIGINT src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator/=( SQLUBIGINT src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator&=( SQLUBIGINT src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator|=( SQLUBIGINT src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUBIGINT& OdbUBIGINT::operator^=( SQLUBIGINT src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbBIT - Odbc Data Type Class

OdbBIT::OdbBIT() : OdbDATA( SQL_C_BIT )
{
    Init( &m_Val, sizeof(SQLCHAR) );
}

OdbBIT::~OdbBIT()
{
}

const OdbBIT& OdbBIT::operator=( OdbBIT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbBIT& OdbBIT::operator=( SQLCHAR src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIT& OdbBIT::operator&=( SQLCHAR src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIT& OdbBIT::operator|=( SQLCHAR src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbBIT& OdbBIT::operator^=( SQLCHAR src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbBOOKMARK - Odbc Data Type Class

OdbBOOKMARK::OdbBOOKMARK() : OdbDATA( SQL_C_BOOKMARK )
{
    Init( &m_Val, sizeof(BOOKMARK) );
}

OdbBOOKMARK::~OdbBOOKMARK()
{
}

const OdbBOOKMARK& OdbBOOKMARK::operator=( OdbBOOKMARK& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbBOOKMARK& OdbBOOKMARK::operator=( BOOKMARK src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbVARBOOKMARK - Odbc Data Type Class

OdbVARBOOKMARK::OdbVARBOOKMARK( SQLINTEGER nMaxLen ) : OdbVARDATA( nMaxLen, SQL_C_VARBOOKMARK )
{
}

OdbVARBOOKMARK::~OdbVARBOOKMARK()
{
}

const OdbVARBOOKMARK& OdbVARBOOKMARK::operator=( OdbVARBOOKMARK& src )
{
    if( src.m_Val )
    {
        if( src.m_nLI >= 0 )
        {
            Set( (SQLPOINTER)src, src.m_nLI );
        }
        else
        {
            Set( (SQLPOINTER)src, src.m_nSavedLen );
            m_nLI = src.m_nLI;
        }
    }
    return( *this );
}

BOOL OdbVARBOOKMARK::Set( SQLPOINTER src, SQLINTEGER nLen )
{
    if( m_Val == (char*)src ) return( TRUE );

    if( !m_Val || m_nMaxLen < nLen )
        if( !Realloc( nLen ) ) return( FALSE );

    SQLINTEGER i;
    char*      to = m_Val;
    char*      from = (char*)src;

    for( i = 0; i < nLen; i++ ) *(to++) = *(from++);
    Len( nLen );

    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// OdbCHAR - Odbc Data Type Class

OdbCHAR::OdbCHAR( SQLINTEGER nMaxLen ) : OdbVARDATA( nMaxLen, SQL_C_CHAR )
{
}

OdbCHAR::~OdbCHAR()
{
}

const OdbCHAR& OdbCHAR::operator=( const OdbCHAR& src )
{
    if( src.m_Val )
    {
        Copy( src.m_Val );
        if( src.m_nLI < 0 ) m_nLI = src.m_nLI;
    }
    return( *this );
}

const OdbCHAR& OdbCHAR::operator=( const char* src )
{
    Copy( src );
    return( *this );
}

const OdbCHAR& OdbCHAR::operator+=( const OdbCHAR& src )
{
    if( src.m_Val ) Concat( src.m_Val );
    return( *this );
}

const OdbCHAR& OdbCHAR::operator=( SQLDOUBLE src )
{
    char buf[32];
    sprintf( buf, "%.18lg", src );
    Copy( buf );
    return( *this );
}

const OdbCHAR& OdbCHAR::operator=( SQLINTEGER src )
{
    char buf[16];
    sprintf( buf, "%li", src );
    Copy( buf );
    return( *this );
}

const OdbCHAR& OdbCHAR::operator=( SQLUINTEGER src )
{
    char buf[16];
    sprintf( buf, "%lu", src );
    Copy( buf );
    return( *this );
}

const OdbCHAR& OdbCHAR::operator+=( const char* src )
{
    Concat( src );
    return( *this );
}

char& OdbCHAR::operator[]( int nIndex )
{
    return( m_Val[nIndex] );
}

BOOL OdbCHAR::Blank() const
{
    if( m_Val )
    {
        unsigned char* ch = (unsigned char*)m_Val;
        for( ; *ch; ch++ ) if( *ch != ' ' ) return( FALSE );
    }
    return( TRUE );
}

SQLINTEGER OdbCHAR::Compare( const char* src, SQLINTEGER nSrc ) const
{
    if( m_Val == src ) return( 0 );
    if( !m_Val ) return( -1 );
    if( !src ) return( 1 );

    unsigned char* s1 = (unsigned char*)m_Val;
    unsigned char* s2 = (unsigned char*)src;
    SQLINTEGER     n;

    for( n = 0; n != nSrc && (*s1 || *s2); s1++, s2++, n++ )
    {
        if( *s1 < *s2 ) return( -1 );
        if( *s1 > *s2 ) return(  1 );
    }
    return( 0 );
}

BOOL OdbCHAR::Concat( const char* src, SQLINTEGER nSrc )
{
    if( m_Val != src )
    {
        SQLINTEGER nLen = SyncLen();

        if( nSrc < 0 ) nSrc = strlen( src );

        if( !m_Val || (nLen + nSrc) > MaxStringLen() )
            if( !Realloc( nLen + nSrc + 1 ) ) return( FALSE );

        strncpy( (m_Val + nLen), src, nSrc );
        m_Val[nLen + nSrc] = 0;
        SyncLen();
    }
    return( TRUE );
}

BOOL OdbCHAR::Copy( const char* src, SQLINTEGER nSrc )
{
    if( m_Val != src )
    {
        if( nSrc < 0 ) nSrc = strlen( src );

        if( !m_Val || nSrc > MaxStringLen() )
            if( !Realloc( nSrc + 1 ) ) return( FALSE );

        strncpy( m_Val, src, nSrc );
        m_Val[nSrc] = 0;
        SyncLen();
    }
    return( TRUE );
}

BOOL OdbCHAR::CopyConcat( const char* cpysrc, const char* catsrc )
{
    SQLINTEGER nCpy = strlen( cpysrc );
    SQLINTEGER nLen = nCpy + strlen( catsrc );

    if( !m_Val || nLen > MaxStringLen() )
        if( !Realloc( nLen + 1 ) ) return( FALSE );

    strcpy( m_Val, cpysrc );
    strcpy( (m_Val + nCpy), catsrc );
    SyncLen();

    return( TRUE );
}

const OdbCHAR& OdbCHAR::MakeLower()
{
    if( SyncLen() ) _strlwr( m_Val );
    return( *this );
}

const OdbCHAR& OdbCHAR::MakeUpper()
{
    if( SyncLen() ) _strupr( m_Val );
    return( *this );
}

SQLINTEGER OdbCHAR::MaxStringLen() const
{
    SQLINTEGER nMaxStringLen = MaxLen() - 1;
    return( nMaxStringLen > 0 ? nMaxStringLen : 0 );
}

const OdbCHAR& OdbCHAR::Sprintf( const char* Format, ... )
{
    if( m_Val )
    {
        va_list args;
        va_start( args, Format );
        vsprintf( m_Val, Format, args );
        va_end( args );
        SyncLen();
    }
    return( *this );
}

SQLINTEGER OdbCHAR::SyncLen()
{
    if( !m_Val ) return( 0 );
    return( Len( strlen( m_Val ) ) );
}

SQLDOUBLE OdbCHAR::ToDouble() const
{
    if( !m_Val ) return( 0.0 );
    return( atof( m_Val ) );
}

SQLINTEGER OdbCHAR::ToInt() const
{
    if( !m_Val ) return( 0 );
    return( atol( m_Val ) );
}

SQLUINTEGER OdbCHAR::ToUInt() const
{
    if( !m_Val ) return( 0 );

    unsigned char* nptr = (unsigned char*)m_Val;

    char        c;              /* current char */
    char        sign;              /* current char */
    SQLUINTEGER total;         /* current total */

    while( *nptr <= ' ' ) ++nptr;
    c = *nptr++;
    sign = c;
    if( c == '-' || c == '+' ) c = *nptr++;
    total = 0;

    while( c >= '0' && c <= '9' )
    {
        total = 10 * total + (SQLUINTEGER)(c - '0');
        c = *nptr++;
    }
    if( sign == '-' )
    {
        return( (SQLUINTEGER)(-(SQLINTEGER)total) );
    }
    return( total );
}

const OdbCHAR& OdbCHAR::TrimLeft()
{
    if( SyncLen() )
    {
        unsigned char* from = (unsigned char*)m_Val;
        for( ; *from && *from <= ' '; from++ );

        if( (char*)from != m_Val )
        {
            unsigned char* to = (unsigned char*)m_Val;
            for( ; *from; from++, to++ ) *to = *from;
            *to = *from;
        }
        SyncLen();
    }
    return( *this );
}

const OdbCHAR& OdbCHAR::TrimRight()
{
    if( SyncLen() )
    {
        unsigned char* s = (unsigned char*)(m_Val + Len() - 1);
        for( ; (char*)s != m_Val && *s <= ' '; s-- ) *s = 0;
        if( *s <= ' ' ) *s = 0;
        SyncLen();
    }
    return( *this );
}

OdbCHAR operator+( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    OdbCHAR s;

    if( !s1.m_Val || !s2.m_Val ) return( s );
    s.CopyConcat( s1.m_Val, s2.m_Val );
    if( s.m_Buffer ) s.m_Buffer->RefInc();

    return( s );
}

OdbCHAR operator+( const OdbCHAR& s1, const char* s2 )
{
    OdbCHAR s;

    if( !s1.m_Val || !s2 ) return( s );
    s.CopyConcat( s1.m_Val, s2 );
    if( s.m_Buffer ) s.m_Buffer->RefInc();

    return( s );
}

OdbCHAR operator+( const char* s1, const OdbCHAR& s2 )
{
    OdbCHAR s;

    if( !s1 || !s2.m_Val ) return( s );
    s.CopyConcat( s1, s2.m_Val );
    if( s.m_Buffer ) s.m_Buffer->RefInc();

    return( s );
}

BOOL operator==( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) == 0 );
}

BOOL operator==( const OdbCHAR& s1, const char* s2 )
{
    return( s1.Compare( s2 ) == 0 );
}

BOOL operator==( const char* s1, const OdbCHAR& s2 )
{
    return( s2.Compare( s1 ) == 0 );
}

BOOL operator!=( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) != 0 );
}

BOOL operator!=( const OdbCHAR& s1, const char* s2 )
{
    return( s1.Compare( s2 ) != 0 );
}

BOOL operator!=( const char* s1, const OdbCHAR& s2 )
{
    return( s2.Compare( s1 ) != 0 );
}

BOOL operator<( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) < 0 );
}

BOOL operator<( const OdbCHAR& s1, const char* s2 )
{
    return( s1.Compare( s2 ) < 0 );
}

BOOL operator<( const char* s1, const OdbCHAR& s2 )
{
    return( s2.Compare( s1 ) > 0 );
}

BOOL operator>( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) > 0 );
}

BOOL operator>( const OdbCHAR& s1, const char* s2 )
{
    return( s1.Compare( s2 ) > 0 );
}

BOOL operator>( const char* s1, const OdbCHAR& s2 )
{
    return( s2.Compare( s1 ) < 0 );
}

BOOL operator<=( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) <= 0 );
}

BOOL operator<=( const OdbCHAR& s1, const char* s2 )
{
    return( s1.Compare( s2 ) <= 0 );
}

BOOL operator<=( const char* s1, const OdbCHAR& s2 )
{
    return( s2.Compare( s1 ) >= 0 );
}

BOOL operator>=( const OdbCHAR& s1, const OdbCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) >= 0 );
}

BOOL operator>=( const OdbCHAR& s1, const char* s2 )
{
    return( s1.Compare( s2 ) >= 0 );
}

BOOL operator>=( const char* s1, const OdbCHAR& s2 )
{
    return( s2.Compare( s1 ) <= 0 );
}

/////////////////////////////////////////////////////////////////////////////
// OdbDATE - Odbc Data Type Class

OdbDATE::OdbDATE() : OdbDATA( SQL_C_TYPE_DATE )
{
    Init( &m_Val, sizeof(SQL_DATE_STRUCT) );
}

OdbDATE::~OdbDATE()
{
}

const OdbDATE& OdbDATE::operator=( OdbDATE& src )
{
    *this = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbDATE& OdbDATE::operator=( SQL_DATE_STRUCT& src )
{
    memcpy( &m_Val, &src, sizeof(SQL_DATE_STRUCT) );
    m_nLI = m_nSavedLen;
    return( *this );
}

SQLSMALLINT OdbDATE::Year() const
{
    return( m_Val.year );
}

SQLSMALLINT OdbDATE::Year( SQLSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.year = nNewVal );
}

SQLUSMALLINT OdbDATE::Month() const
{
    return( m_Val.month );
}

SQLUSMALLINT OdbDATE::Month( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.month = nNewVal );
}

SQLUSMALLINT OdbDATE::Day() const
{
    return( m_Val.day );
}

SQLUSMALLINT OdbDATE::Day( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.day = nNewVal );
}

void OdbDATE::Today()
{
    SYSTEMTIME st;

    ::GetLocalTime( &st );

    m_Val.year = st.wYear;
    m_Val.month = st.wMonth;
    m_Val.day = st.wDay;

    m_nLI = m_nSavedLen;
}

/////////////////////////////////////////////////////////////////////////////
// OdbDATETIME - Odbc Data Type Class

OdbDATETIME::OdbDATETIME() : OdbDATA( SQL_C_TYPE_TIMESTAMP )
{
    Init( &m_Val, sizeof(SQL_TIMESTAMP_STRUCT) );
}

OdbDATETIME::~OdbDATETIME()
{
}

const OdbDATETIME& OdbDATETIME::operator=( OdbDATETIME& src )
{
    *this = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbDATETIME& OdbDATETIME::operator=( SQL_TIMESTAMP_STRUCT& src )
{
    memcpy( &m_Val, &src, sizeof(SQL_TIMESTAMP_STRUCT) );
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbDATETIME& OdbDATETIME::operator=( SYSTEMTIME& src )
{
    m_Val.year = src.wYear;
    m_Val.month = src.wMonth;
    m_Val.day = src.wDay;
    m_Val.hour = src.wHour;
    m_Val.minute = src.wMinute;
    m_Val.second = src.wSecond;
    m_Val.fraction = 0;

    m_nLI = m_nSavedLen;

    return( *this );
}

SQLSMALLINT OdbDATETIME::Year() const
{
    return( m_Val.year );
}

SQLSMALLINT OdbDATETIME::Year( SQLSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.year = nNewVal );
}

SQLUSMALLINT OdbDATETIME::Month() const
{
    return( m_Val.month );
}

SQLUSMALLINT OdbDATETIME::Month( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.month = nNewVal );
}

SQLUSMALLINT OdbDATETIME::Day() const
{
    return( m_Val.day );
}

SQLUSMALLINT OdbDATETIME::Day( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.day = nNewVal );
}

SQLUSMALLINT OdbDATETIME::Hour() const
{
    return( m_Val.hour );
}

SQLUSMALLINT OdbDATETIME::Hour( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.hour = nNewVal );
}

SQLUSMALLINT OdbDATETIME::Minute() const
{
    return( m_Val.minute );
}

SQLUSMALLINT OdbDATETIME::Minute( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.minute = nNewVal );
}

SQLUSMALLINT OdbDATETIME::Second() const
{
    return( m_Val.second );
}

SQLUSMALLINT OdbDATETIME::Second( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.second = nNewVal );
}

SQLUINTEGER OdbDATETIME::Fraction() const
{
    return( m_Val.fraction );
}

SQLUINTEGER OdbDATETIME::Fraction( SQLUINTEGER nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.fraction = nNewVal );
}

void OdbDATETIME::Today()
{
    SYSTEMTIME st;

    ::GetLocalTime( &st );

    m_Val.year = st.wYear;
    m_Val.month = st.wMonth;
    m_Val.day = st.wDay;
    m_Val.hour = st.wHour;
    m_Val.minute = st.wMinute;
    m_Val.second = st.wSecond;
    m_Val.fraction = 0;

    m_nLI = m_nSavedLen;
}

SQLINTEGER OdbDATETIME::Compare( const OdbDATETIME& dt ) const
{
    if( m_Val.year > dt.m_Val.year ) return( 1 );
    if( m_Val.year < dt.m_Val.year ) return( -1 );
    if( m_Val.month > dt.m_Val.month ) return( 1 );
    if( m_Val.month < dt.m_Val.month ) return( -1 );
    if( m_Val.day > dt.m_Val.day ) return( 1 );
    if( m_Val.day < dt.m_Val.day ) return( -1 );
    if( m_Val.hour > dt.m_Val.hour ) return( 1 );
    if( m_Val.hour < dt.m_Val.hour ) return( -1 );
    if( m_Val.minute > dt.m_Val.minute ) return( 1 );
    if( m_Val.minute < dt.m_Val.minute ) return( -1 );
    if( m_Val.second > dt.m_Val.second ) return( 1 );
    if( m_Val.second < dt.m_Val.second ) return( -1 );
    return( 0 );
}

SQLINTEGER OdbDATETIME::Compare( const SYSTEMTIME& dt ) const
{
    if( m_Val.year > (SQLSMALLINT)dt.wYear ) return( 1 );
    if( m_Val.year < (SQLSMALLINT)dt.wYear ) return( -1 );
    if( m_Val.month > (SQLUSMALLINT)dt.wMonth ) return( 1 );
    if( m_Val.month < (SQLUSMALLINT)dt.wMonth ) return( -1 );
    if( m_Val.day > (SQLUSMALLINT)dt.wDay ) return( 1 );
    if( m_Val.day < (SQLUSMALLINT)dt.wDay ) return( -1 );
    if( m_Val.hour > (SQLUSMALLINT)dt.wHour ) return( 1 );
    if( m_Val.hour < (SQLUSMALLINT)dt.wHour ) return( -1 );
    if( m_Val.minute > (SQLUSMALLINT)dt.wMinute ) return( 1 );
    if( m_Val.minute < (SQLUSMALLINT)dt.wMinute ) return( -1 );
    if( m_Val.second > (SQLUSMALLINT)dt.wSecond ) return( 1 );
    if( m_Val.second < (SQLUSMALLINT)dt.wSecond ) return( -1 );
    return( 0 );
}

BOOL operator==( const OdbDATETIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt1.Compare( dt2 ) == 0 );
}

BOOL operator==( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 )
{
    return( dt1.Compare( dt2 ) == 0 );
}

BOOL operator==( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt2.Compare( dt1 ) == 0 );
}

BOOL operator!=( const OdbDATETIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt1.Compare( dt2 ) != 0 );
}

BOOL operator!=( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 )
{
    return( dt1.Compare( dt2 ) != 0 );
}

BOOL operator!=( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt2.Compare( dt1 ) != 0 );
}

BOOL operator<( const OdbDATETIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt1.Compare( dt2 ) < 0 );
}

BOOL operator<( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 )
{
    return( dt1.Compare( dt2 ) < 0 );
}

BOOL operator<( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt2.Compare( dt1 ) > 0 );
}

BOOL operator>( const OdbDATETIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt1.Compare( dt2 ) > 0 );
}

BOOL operator>( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 )
{
    return( dt1.Compare( dt2 ) > 0 );
}

BOOL operator>( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt2.Compare( dt1 ) < 0 );
}

BOOL operator<=( const OdbDATETIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt1.Compare( dt2 ) <= 0 );
}

BOOL operator<=( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 )
{
    return( dt1.Compare( dt2 ) <= 0 );
}

BOOL operator<=( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt2.Compare( dt1 ) >= 0 );
}

BOOL operator>=( const OdbDATETIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt1.Compare( dt2 ) >= 0 );
}

BOOL operator>=( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 )
{
    return( dt1.Compare( dt2 ) >= 0 );
}

BOOL operator>=( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 )
{
    return( dt2.Compare( dt1 ) <= 0 );
}

/////////////////////////////////////////////////////////////////////////////
// OdbDOUBLE - Odbc Data Type Class

OdbDOUBLE::OdbDOUBLE() : OdbDATA( SQL_C_DOUBLE )
{
    Init( &m_Val, sizeof(SQLDOUBLE) );
}

OdbDOUBLE::~OdbDOUBLE()
{
}

const OdbDOUBLE& OdbDOUBLE::operator=( OdbDOUBLE& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbDOUBLE& OdbDOUBLE::operator=( SQLDOUBLE src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbDOUBLE& OdbDOUBLE::operator+=( SQLDOUBLE src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbDOUBLE& OdbDOUBLE::operator-=( SQLDOUBLE src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbDOUBLE& OdbDOUBLE::operator*=( SQLDOUBLE src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbDOUBLE& OdbDOUBLE::operator/=( SQLDOUBLE src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbGUID - Odbc Data Type Class

OdbGUID::OdbGUID() : OdbDATA( SQL_C_GUID )
{
    Init( &m_Val, sizeof(SQLGUID) );
}

OdbGUID::~OdbGUID()
{
}

const OdbGUID& OdbGUID::operator=( OdbGUID& src )
{
    *this = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbGUID& OdbGUID::operator=( SQLGUID& src )
{
    memcpy( &m_Val, &src, sizeof(SQLGUID) );
    m_nLI = m_nSavedLen;
    return( *this );
}

SQLUINTEGER OdbGUID::Data1() const
{
    return( m_Val.Data1 );
}

SQLUINTEGER OdbGUID::Data1( SQLUINTEGER nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.Data1 = nNewVal );
}

SQLUSMALLINT OdbGUID::Data2() const
{
    return( m_Val.Data2 );
}

SQLUSMALLINT OdbGUID::Data2( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.Data2 = nNewVal );
}

SQLUSMALLINT OdbGUID::Data3() const
{
    return( m_Val.Data3 );
}

SQLUSMALLINT OdbGUID::Data3( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.Data3 = nNewVal );
}

const SQLCHAR* OdbGUID::Data4() const
{
    return( m_Val.Data4 );
}

const SQLCHAR* OdbGUID::Data4( const SQLCHAR* pNewVal )
{
    memcpy( m_Val.Data4, pNewVal, 8 );
    return( m_Val.Data4 );
}


/////////////////////////////////////////////////////////////////////////////
// OdbINT - Odbc Data Type Class

OdbINT::OdbINT() : OdbDATA( SQL_C_SLONG )
{
    Init( &m_Val, sizeof(SQLINTEGER) );
}

OdbINT::~OdbINT()
{
}

const OdbINT& OdbINT::operator=( OdbINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbINT& OdbINT::operator=( SQLINTEGER src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator+=( SQLINTEGER src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator-=( SQLINTEGER src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator*=( SQLINTEGER src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator/=( SQLINTEGER src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator&=( SQLINTEGER src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator|=( SQLINTEGER src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbINT& OdbINT::operator^=( SQLINTEGER src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbUINT - Odbc Data Type Class

OdbUINT::OdbUINT() : OdbDATA( SQL_C_ULONG )
{
    Init( &m_Val, sizeof(SQLUINTEGER) );
}

OdbUINT::~OdbUINT()
{
}

const OdbUINT& OdbUINT::operator=( OdbUINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbUINT& OdbUINT::operator=( SQLUINTEGER src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator+=( SQLUINTEGER src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator-=( SQLUINTEGER src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator*=( SQLUINTEGER src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator/=( SQLUINTEGER src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator&=( SQLUINTEGER src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator|=( SQLUINTEGER src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUINT& OdbUINT::operator^=( SQLUINTEGER src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbNUMERIC - Odbc Data Type Class

OdbNUMERIC::OdbNUMERIC() : OdbDATA( SQL_C_NUMERIC )
{
    Init( &m_Val, sizeof(SQL_NUMERIC_STRUCT) );
}

OdbNUMERIC::~OdbNUMERIC()
{
}

const OdbNUMERIC& OdbNUMERIC::operator=( OdbNUMERIC& src )
{
    *this = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbNUMERIC& OdbNUMERIC::operator=( SQL_NUMERIC_STRUCT& src )
{
    memcpy( &m_Val, &src, sizeof(SQL_NUMERIC_STRUCT) );
    m_nLI = m_nSavedLen;
    return( *this );
}

SQLCHAR OdbNUMERIC::Precision() const
{
    return( m_Val.precision );
}

SQLCHAR OdbNUMERIC::Precision( SQLCHAR nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.precision = nNewVal );
}

SQLSCHAR OdbNUMERIC::Scale() const
{
    return( m_Val.scale );
}

SQLSCHAR OdbNUMERIC::Scale( SQLSCHAR nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.scale = nNewVal );
}

SQLCHAR OdbNUMERIC::Sign() const
{
    return( m_Val.sign );
}

SQLCHAR OdbNUMERIC::Sign( SQLCHAR nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.sign = nNewVal );
}

const SQLCHAR* OdbNUMERIC::Val() const
{
    return( m_Val.val );
}

const SQLCHAR* OdbNUMERIC::Val( const SQLCHAR* pNewVal )
{
    memcpy( m_Val.val, pNewVal, SQL_MAX_NUMERIC_LEN );
    return( m_Val.val );
}

/////////////////////////////////////////////////////////////////////////////
// OdbREAL - Odbc Data Type Class

OdbREAL::OdbREAL() : OdbDATA( SQL_C_FLOAT )
{
    Init( &m_Val, sizeof(SQLREAL) );
}

OdbREAL::~OdbREAL()
{
}

const OdbREAL& OdbREAL::operator=( OdbREAL& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbREAL& OdbREAL::operator=( SQLREAL src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbREAL& OdbREAL::operator+=( SQLREAL src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbREAL& OdbREAL::operator-=( SQLREAL src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbREAL& OdbREAL::operator*=( SQLREAL src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbREAL& OdbREAL::operator/=( SQLREAL src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbSMALLINT - Odbc Data Type Class

OdbSMALLINT::OdbSMALLINT() : OdbDATA( SQL_C_SSHORT )
{
    Init( &m_Val, sizeof(SQLSMALLINT) );
}

OdbSMALLINT::~OdbSMALLINT()
{
}

const OdbSMALLINT& OdbSMALLINT::operator=( OdbSMALLINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator=( SQLSMALLINT src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator+=( SQLSMALLINT src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator-=( SQLSMALLINT src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator*=( SQLSMALLINT src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator/=( SQLSMALLINT src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator&=( SQLSMALLINT src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator|=( SQLSMALLINT src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbSMALLINT& OdbSMALLINT::operator^=( SQLSMALLINT src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbUSMALLINT - Odbc Data Type Class

OdbUSMALLINT::OdbUSMALLINT() : OdbDATA( SQL_C_USHORT )
{
    Init( &m_Val, sizeof(SQLUSMALLINT) );
}

OdbUSMALLINT::~OdbUSMALLINT()
{
}

const OdbUSMALLINT& OdbUSMALLINT::operator=( OdbUSMALLINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator=( SQLUSMALLINT src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator+=( SQLUSMALLINT src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator-=( SQLUSMALLINT src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator*=( SQLUSMALLINT src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator/=( SQLUSMALLINT src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator&=( SQLUSMALLINT src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator|=( SQLUSMALLINT src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUSMALLINT& OdbUSMALLINT::operator^=( SQLUSMALLINT src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbTIME - Odbc Data Type Class

OdbTIME::OdbTIME() : OdbDATA( SQL_C_TYPE_TIME )
{
    Init( &m_Val, sizeof(SQL_TIME_STRUCT) );
}

OdbTIME::~OdbTIME()
{
}

const OdbTIME& OdbTIME::operator=( OdbTIME& src )
{
    *this = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbTIME& OdbTIME::operator=( SQL_TIME_STRUCT& src )
{
    memcpy( &m_Val, &src, sizeof(SQL_TIME_STRUCT) );
    m_nLI = m_nSavedLen;
    return( *this );
}

SQLUSMALLINT OdbTIME::Hour() const
{
    return( m_Val.hour );
}

SQLUSMALLINT OdbTIME::Hour( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.hour = nNewVal );
}

SQLUSMALLINT OdbTIME::Minute() const
{
    return( m_Val.minute );
}

SQLUSMALLINT OdbTIME::Minute( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.minute = nNewVal );
}

SQLUSMALLINT OdbTIME::Second() const
{
    return( m_Val.second );
}

SQLUSMALLINT OdbTIME::Second( SQLUSMALLINT nNewVal )
{
    m_nLI = m_nSavedLen;
    return( m_Val.second = nNewVal );
}

void OdbTIME::Current()
{
    SYSTEMTIME st;

    ::GetLocalTime( &st );

    m_Val.hour = st.wHour;
    m_Val.minute = st.wMinute;
    m_Val.second = st.wSecond;

    m_nLI = m_nSavedLen;
}

/////////////////////////////////////////////////////////////////////////////
// OdbTINYINT - Odbc Data Type Class

OdbTINYINT::OdbTINYINT() : OdbDATA( SQL_C_STINYINT )
{
    Init( &m_Val, sizeof(SQLSCHAR) );
}

OdbTINYINT::~OdbTINYINT()
{
}

const OdbTINYINT& OdbTINYINT::operator=( OdbTINYINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator=( SQLSCHAR src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator+=( SQLSCHAR src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator-=( SQLSCHAR src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator*=( SQLSCHAR src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator/=( SQLSCHAR src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator&=( SQLSCHAR src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator|=( SQLSCHAR src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbTINYINT& OdbTINYINT::operator^=( SQLSCHAR src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbUTINYINT - Odbc Data Type Class

OdbUTINYINT::OdbUTINYINT() : OdbDATA( SQL_C_UTINYINT )
{
    Init( &m_Val, sizeof(SQLCHAR) );
}

OdbUTINYINT::~OdbUTINYINT()
{
}

const OdbUTINYINT& OdbUTINYINT::operator=( OdbUTINYINT& src )
{
    m_Val = src.m_Val;
    m_nLI = src.m_nLI;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator=( SQLCHAR src )
{
    m_Val = src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator+=( SQLCHAR src )
{
    m_Val += src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator-=( SQLCHAR src )
{
    m_Val -= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator*=( SQLCHAR src )
{
    m_Val *= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator/=( SQLCHAR src )
{
    m_Val /= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator&=( SQLCHAR src )
{
    m_Val &= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator|=( SQLCHAR src )
{
    m_Val |= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

const OdbUTINYINT& OdbUTINYINT::operator^=( SQLCHAR src )
{
    m_Val ^= src;
    m_nLI = m_nSavedLen;
    return( *this );
}

/////////////////////////////////////////////////////////////////////////////
// OdbWVARDATA - Odbc Data Type Class

OdbWVARDATA::OdbWVARDATA( SQLINTEGER nMaxLen, SQLSMALLINT nType ) : OdbDATA( nType )
{
    m_Buffer = NULL;
    m_Val = NULL;
    if( nMaxLen > 0 ) Alloc( nMaxLen );
}

OdbWVARDATA::~OdbWVARDATA()
{
    if( m_Buffer && m_Buffer->RefDec() <= 0 ) delete m_Buffer;
}

BOOL OdbWVARDATA::Alloc( SQLINTEGER nMaxLen )
{
    nMaxLen *= sizeof(wchar_t);
    Free();
    if( !(m_Buffer = new OdbBuffer) ) return( FALSE );
    if( (m_Val = (wchar_t*) m_Buffer->Alloc( nMaxLen )) )
        Init( m_Val, 0, nMaxLen );
    return( m_Val != NULL );
}

void OdbWVARDATA::Free()
{
    if( m_Buffer && m_Buffer->RefDec() <= 0 ) delete m_Buffer;

    if( m_Buffer )
    {
        m_Buffer = NULL;
        m_Val = NULL;
        Init( NULL, 0 );
    }
}

SQLINTEGER OdbWVARDATA::Len() const
{
    return( (m_nLI >= 0 ? m_nLI : m_nSavedLen) / sizeof(wchar_t) );
}

SQLINTEGER OdbWVARDATA::Len( SQLINTEGER nNewVal )
{
    if( !m_Val ) return( 0 );

    SQLINTEGER nMaxLen = m_nMaxLen - sizeof(wchar_t);

    nNewVal *= sizeof(wchar_t);

    if( nNewVal > 0 )
        m_nSavedLen = nNewVal < nMaxLen ? nNewVal : nMaxLen;
    else
        m_nSavedLen = 0;
    m_Val[m_nSavedLen / sizeof(wchar_t)] = 0;
    m_nLI = m_nSavedLen;
    return( m_nLI / sizeof(wchar_t) );
}

BOOL OdbWVARDATA::NoTotal() const
{
    return( m_nLI == SQL_NO_TOTAL );
}

BOOL OdbWVARDATA::Realloc( SQLINTEGER nMaxLen )
{
    if( !m_Buffer ) return( Alloc( nMaxLen ) );

    nMaxLen *= sizeof(wchar_t);

    if( m_nLI >= 0 && m_nLI != m_nSavedLen ) m_nSavedLen = m_nLI;

    if( nMaxLen > 0 && nMaxLen != m_nMaxLen )
    {
        if( (m_Val = (wchar_t*)m_Buffer->Realloc( nMaxLen )) )
        {
            m_pVal = m_Val;
            m_nMaxLen = nMaxLen;
            nMaxLen -= sizeof(wchar_t);
            m_nSavedLen = nMaxLen < m_nSavedLen ? nMaxLen : m_nSavedLen;
            m_Val[m_nSavedLen / sizeof(wchar_t)] = 0;
            if( m_nLI >= 0 ) m_nLI = m_nSavedLen;
        }
        else
        {
            Init( NULL, 0 );
        }
    }
    return( m_Val != NULL );
}

void OdbWVARDATA::SetLenAtExec( SQLINTEGER nVal )
{
    if( m_nLI >= 0 ) m_nSavedLen = m_nLI;
    m_nLI = SQL_LEN_DATA_AT_EXEC( nVal );
}

/////////////////////////////////////////////////////////////////////////////
// OdbWCHAR - Odbc Data Type Class

OdbWCHAR::OdbWCHAR( SQLINTEGER nMaxLen ) : OdbWVARDATA( nMaxLen, SQL_C_WCHAR )
{
}

OdbWCHAR::~OdbWCHAR()
{
}

const OdbWCHAR& OdbWCHAR::operator=( const OdbWCHAR& src )
{
    if( src.m_Val )
    {
        Copy( src.m_Val );
        if( src.m_nLI < 0 ) m_nLI = src.m_nLI;
    }
    return( *this );
}

const OdbWCHAR& OdbWCHAR::operator=( const wchar_t* src )
{
    Copy( src );
    return( *this );
}

const OdbWCHAR& OdbWCHAR::operator+=( const OdbWCHAR& src )
{
    if( src.m_Val ) Concat( src.m_Val );
    return( *this );
}

const OdbWCHAR& OdbWCHAR::operator=( SQLDOUBLE src )
{
    wchar_t buf[32];
    swprintf( buf, L"%.18lg", src );
    Copy( buf );
    return( *this );
}

const OdbWCHAR& OdbWCHAR::operator=( SQLINTEGER src )
{
    wchar_t buf[16];
    swprintf( buf, L"%li", src );
    Copy( buf );
    return( *this );
}

const OdbWCHAR& OdbWCHAR::operator=( SQLUINTEGER src )
{
    wchar_t buf[16];
    swprintf( buf, L"%lu", src );
    Copy( buf );
    return( *this );
}

const OdbWCHAR& OdbWCHAR::operator+=( const wchar_t* src )
{
    Concat( src );
    return( *this );
}

wchar_t& OdbWCHAR::operator[]( int nIndex )
{
    return( m_Val[nIndex] );
}

BOOL OdbWCHAR::Blank() const
{
    if( m_Val )
    {
        wchar_t* ch = (wchar_t*)m_Val;
        for( ; *ch; ch++ ) if( *ch != L' ' ) return( FALSE );
    }
    return( TRUE );
}

SQLINTEGER OdbWCHAR::Compare( const wchar_t* src, SQLINTEGER nSrc ) const
{
    if( m_Val == src ) return( 0 );
    if( !m_Val ) return( -1 );
    if( !src ) return( 1 );

    wchar_t*   s1 = (wchar_t*)m_Val;
    wchar_t*   s2 = (wchar_t*)src;
    SQLINTEGER n;

    for( n = 0; n != nSrc && (*s1 || *s2); s1++, s2++, n++ )
    {
        if( *s1 < *s2 ) return( -1 );
        if( *s1 > *s2 ) return(  1 );
    }
    return( 0 );
}

BOOL OdbWCHAR::Concat( const wchar_t* src, SQLINTEGER nSrc )
{
    if( m_Val != src )
    {
        SQLINTEGER nLen = SyncLen();

        if( nSrc < 0 ) nSrc = wcslen( src );

        if( !m_Val || (nLen + nSrc) > MaxStringLen() )
            if( !Realloc( nLen + nSrc + 1 ) ) return( FALSE );

        wcsncpy( (m_Val + nLen), src, nSrc );
        m_Val[nLen + nSrc] = 0;
        SyncLen();
    }
    return( TRUE );
}

BOOL OdbWCHAR::Copy( const wchar_t* src, SQLINTEGER nSrc )
{
    if( m_Val != src )
    {
        if( nSrc < 0 ) nSrc = wcslen( src );

        if( !m_Val || nSrc > MaxStringLen() )
            if( !Realloc( nSrc + 1 ) ) return( FALSE );

        wcsncpy( m_Val, src, nSrc );
        m_Val[nSrc] = 0;
        SyncLen();
    }
    return( TRUE );
}

BOOL OdbWCHAR::CopyConcat( const wchar_t* cpysrc, const wchar_t* catsrc )
{
    SQLINTEGER nCpy = wcslen( cpysrc );
    SQLINTEGER nLen = nCpy + wcslen( catsrc );

    if( !m_Val || nLen > MaxStringLen() )
        if( !Realloc( nLen + 1 ) ) return( FALSE );

    wcscpy( m_Val, cpysrc );
    wcscpy( (m_Val + nCpy), catsrc );
    SyncLen();

    return( TRUE );
}

const OdbWCHAR& OdbWCHAR::MakeLower()
{
    if( SyncLen() ) _wcslwr( m_Val );
    return( *this );
}

const OdbWCHAR& OdbWCHAR::MakeUpper()
{
    if( SyncLen() ) _wcsupr( m_Val );
    return( *this );
}

SQLINTEGER OdbWCHAR::MaxStringLen() const
{
    SQLINTEGER nMaxStringLen = (MaxLen() / sizeof(wchar_t)) - 1;
    return( nMaxStringLen > 0 ? nMaxStringLen : 0 );
}

const OdbWCHAR& OdbWCHAR::Sprintf( const wchar_t* Format, ... )
{
    if( m_Val )
    {
        va_list args;
        va_start( args, Format );
        vswprintf( m_Val, Format, args );
        va_end( args );
        SyncLen();
    }
    return( *this );
}

SQLINTEGER OdbWCHAR::SyncLen()
{
    if( !m_Val ) return( 0 );
    return( Len( wcslen( m_Val ) ) );
}

SQLDOUBLE OdbWCHAR::ToDouble() const
{
    int  n;
    char szVal[80];

    if( !m_Val ) return( 0.0 );
    for( n = 0; n < 79 && m_Val[n] && m_Val[n] < 256; n++ )
        szVal[n] = (char)m_Val[n];
    szVal[n] = 0;
    return( atof( szVal ) );
}

SQLINTEGER OdbWCHAR::ToInt() const
{
    if( !m_Val ) return( 0 );
    return( _wtol( m_Val ) );
}

SQLUINTEGER OdbWCHAR::ToUInt() const
{
    if( !m_Val ) return( 0 );

    wchar_t* nptr = (wchar_t*)m_Val;

    wchar_t     c;              /* current char */
    wchar_t     sign;              /* current char */
    SQLUINTEGER total;         /* current total */

    while( *nptr <= L' ' ) ++nptr;
    c = *nptr++;
    sign = c;
    if( c == L'-' || c == L'+' ) c = *nptr++;
    total = 0;

    while( c >= L'0' && c <= L'9' )
    {
        total = 10 * total + (SQLUINTEGER)(c - L'0');
        c = *nptr++;
    }
    if( sign == L'-' )
    {
        return( (SQLUINTEGER)(-(SQLINTEGER)total) );
    }
    return( total );
}

const OdbWCHAR& OdbWCHAR::TrimLeft()
{
    if( SyncLen() )
    {
        wchar_t* from = (wchar_t*)m_Val;
        for( ; *from && *from <= L' '; from++ );

        if( (wchar_t*)from != m_Val )
        {
            wchar_t* to = (wchar_t*)m_Val;
            for( ; *from; from++, to++ ) *to = *from;
            *to = *from;
        }
        SyncLen();
    }
    return( *this );
}

const OdbWCHAR& OdbWCHAR::TrimRight()
{
    if( SyncLen() )
    {
        wchar_t* s = (wchar_t*)(m_Val + Len() - 1);
        for( ; (wchar_t*)s != m_Val && *s <= L' '; s-- ) *s = 0;
        if( *s <= L' ' ) *s = 0;
        SyncLen();
    }
    return( *this );
}

BOOL OdbWCHAR::UTF8Concat( const char* src, SQLINTEGER nSrc )
{
    SQLINTEGER n = SyncLen();
    SQLINTEGER nBytesDecoded;
    SQLINTEGER nLen;

    if( nSrc < 0 ) nSrc = strlen( src );
    nLen = n + UTF8WideCharLen( src, nSrc );

    if( !m_Val || nLen > MaxStringLen() )
        if( !Realloc( nLen + 1 ) ) return( FALSE );

    for( ; n < nLen; n++, src += nBytesDecoded )
        nBytesDecoded = UTF8Decode( n, src );

    m_Val[nLen] = 0;
    SyncLen();

    return( TRUE );
}

BOOL OdbWCHAR::UTF8Copy( const char* src, SQLINTEGER nSrc )
{
    SQLINTEGER n;
    SQLINTEGER nBytesDecoded;
    SQLINTEGER nLen;

    if( nSrc < 0 ) nSrc = strlen( src );
    nLen = UTF8WideCharLen( src, nSrc );

    if( !m_Val || nLen > MaxStringLen() )
        if( !Realloc( nLen + 1 ) ) return( FALSE );

    for( n = 0; n < nLen; n++, src += nBytesDecoded )
        nBytesDecoded = UTF8Decode( n, src );

    m_Val[nLen] = 0;
    SyncLen();

    return( TRUE );
}

SQLINTEGER OdbWCHAR::UTF8Decode( SQLINTEGER nIndex, const char* mbchar )
{
    BYTE        cBit;
    BYTE        cByte;
    SQLINTEGER  nBytes;
    SQLINTEGER  nBytesDecoded;
    SQLUINTEGER nUnicode;

    cByte = (BYTE)*mbchar;

    for( cBit = 0x80, nBytes = 0; (cByte & cBit); cBit >>= 1, nBytes++ )
        cByte ^= cBit;

    if( nBytes == 0 )
    {
        m_Val[nIndex] = cByte;
        return( 1 );
    }
    if( nBytes == 1 || nBytes > 6 )
    {
        m_Val[nIndex] = L'?';
        return( 1 );
    }
    nUnicode = cByte;
    nBytesDecoded = nBytes;

    for( nBytes--; nBytes > 0; nBytes-- )
    {
        if( (*(++mbchar) & 0xC0) != 0x80 )
        {
            m_Val[nIndex] = L'?';
            return( nBytesDecoded - nBytes );
        }
        nUnicode = (nUnicode << 6) + (*mbchar & 0x3F);
    }
    m_Val[nIndex] = (wchar_t)nUnicode;
    return( nBytesDecoded );
}

SQLINTEGER OdbWCHAR::UTF8Encode( char* mbchar, SQLINTEGER nIndex ) const
{
    SQLUINTEGER nUnicode = m_Val[nIndex];

    if( nUnicode < 0x80 )
    {
        *mbchar = (BYTE)nUnicode;
        return( 1 );
    }
    else if( nUnicode < 0x800 )
    {
        *(mbchar++) = (BYTE)(0xC0 | (nUnicode >> 6));
        *mbchar = (BYTE)(0x80 | (nUnicode & 0x3F));
        return( 2 );
    }
    *(mbchar++) = (BYTE)(0xE0 | (nUnicode >> 12));
    *(mbchar++) = (BYTE)(0x80 | ((nUnicode >> 6) & 0x3F));
    *mbchar = (BYTE)(0x80 | (nUnicode & 0x3F));
    return( 3 );
}

SQLINTEGER OdbWCHAR::UTF8Len() const
{
    SQLINTEGER n;
    SQLINTEGER nLen = Len();
    SQLINTEGER nUTF8Len = 0;

    for( n = 0; n < nLen; n++ )
    {
        if( m_Val[n] < 0x80 )
            nUTF8Len += 1;
        else if( m_Val[n] < 0x800 )
            nUTF8Len += 2;
        else
            nUTF8Len += 3;
    }
    return( nUTF8Len );
}

SQLINTEGER OdbWCHAR::UTF8WideCharLen( const char* str, SQLINTEGER nStr ) const
{
    BYTE       cBit;
    BYTE       cByte;
    SQLINTEGER nBytes;
    SQLINTEGER nLen = 0;

    if( nStr < 0 ) nStr = strlen( str );

    for( ; nStr > 0; str++, nStr-- )
    {
        nLen++;
        cByte = (BYTE)*str;

        for( cBit = 0x80, nBytes = 0; (cByte & cBit); cBit >>= 1, nBytes++ );

        if( nBytes <= 1 || nBytes > 6 ) continue;

        for( nBytes--; nBytes > 0; nBytes-- )
        {
            if( (*(++str) & 0xC0) != 0x80 )
            {
                str--;
                break;
            }
            nStr--;
        }
    }
    return( nLen );
}

OdbWCHAR operator+( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    OdbWCHAR s;

    if( !s1.m_Val || !s2.m_Val ) return( s );
    s.CopyConcat( s1.m_Val, s2.m_Val );
    if( s.m_Buffer ) s.m_Buffer->RefInc();

    return( s );
}

OdbWCHAR operator+( const OdbWCHAR& s1, const wchar_t* s2 )
{
    OdbWCHAR s;

    if( !s1.m_Val || !s2 ) return( s );
    s.CopyConcat( s1.m_Val, s2 );
    if( s.m_Buffer ) s.m_Buffer->RefInc();

    return( s );
}

OdbWCHAR operator+( const wchar_t* s1, const OdbWCHAR& s2 )
{
    OdbWCHAR s;

    if( !s1 || !s2.m_Val ) return( s );
    s.CopyConcat( s1, s2.m_Val );
    if( s.m_Buffer ) s.m_Buffer->RefInc();

    return( s );
}

BOOL operator==( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) == 0 );
}

BOOL operator==( const OdbWCHAR& s1, const wchar_t* s2 )
{
    return( s1.Compare( s2 ) == 0 );
}

BOOL operator==( const wchar_t* s1, const OdbWCHAR& s2 )
{
    return( s2.Compare( s1 ) == 0 );
}

BOOL operator!=( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) != 0 );
}

BOOL operator!=( const OdbWCHAR& s1, const wchar_t* s2 )
{
    return( s1.Compare( s2 ) != 0 );
}

BOOL operator!=( const wchar_t* s1, const OdbWCHAR& s2 )
{
    return( s2.Compare( s1 ) != 0 );
}

BOOL operator<( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) < 0 );
}

BOOL operator<( const OdbWCHAR& s1, const wchar_t* s2 )
{
    return( s1.Compare( s2 ) < 0 );
}

BOOL operator<( const wchar_t* s1, const OdbWCHAR& s2 )
{
    return( s2.Compare( s1 ) > 0 );
}

BOOL operator>( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) > 0 );
}

BOOL operator>( const OdbWCHAR& s1, const wchar_t* s2 )
{
    return( s1.Compare( s2 ) > 0 );
}

BOOL operator>( const wchar_t* s1, const OdbWCHAR& s2 )
{
    return( s2.Compare( s1 ) < 0 );
}

BOOL operator<=( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) <= 0 );
}

BOOL operator<=( const OdbWCHAR& s1, const wchar_t* s2 )
{
    return( s1.Compare( s2 ) <= 0 );
}

BOOL operator<=( const wchar_t* s1, const OdbWCHAR& s2 )
{
    return( s2.Compare( s1 ) >= 0 );
}

BOOL operator>=( const OdbWCHAR& s1, const OdbWCHAR& s2 )
{
    return( s1.Compare( s2.m_Val ) >= 0 );
}

BOOL operator>=( const OdbWCHAR& s1, const wchar_t* s2 )
{
    return( s1.Compare( s2 ) >= 0 );
}

BOOL operator>=( const wchar_t* s1, const OdbWCHAR& s2 )
{
    return( s2.Compare( s1 ) <= 0 );
}

