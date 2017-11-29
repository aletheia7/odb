/* $Id: odbtypes.h,v 1.3 2004/08/15 01:49:24 rtwitty Exp $ */
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
#ifndef ODBTYPES_H
#define ODBTYPES_H

class OdbBuffer;
class OdbDATA;
class OdbVARDATA;
class OdbBIGINT;
class OdbUBIGINT;
class OdbBINARY;
class OdbBIT;
class OdbBOOKMARK;
class OdbVARBOOKMARK;
class OdbCHAR;
class OdbDATE;
class OdbDATETIME;
class OdbDOUBLE;
class OdbGUID;
class OdbINT;
class OdbUINT;
class OdbNUMERIC;
class OdbREAL;
class OdbSMALLINT;
class OdbUSMALLINT;
class OdbTIME;
class OdbTINYINT;
class OdbUTINYINT;
class OdbWVARDATA;
class OdbWCHAR;

/////////////////////////////////////////////////////////////////////////////
class OdbBuffer
{
public:
    SQLPOINTER  m_pBuf;
    SQLSMALLINT m_nRef;

    OdbBuffer();
    virtual ~OdbBuffer();

    SQLPOINTER  Alloc( SQLINTEGER nSize );
    void        Free();
    SQLPOINTER  Ptr();
    SQLPOINTER  Realloc( SQLINTEGER nSize );
    SQLSMALLINT RefDec();
    SQLSMALLINT RefInc();
};

class OdbDATA
{
public:
    SQLINTEGER  m_nLI;
    SQLINTEGER  m_nMaxLen;
    SQLINTEGER  m_nSavedLen;
    SQLSMALLINT m_nType;
    SQLPOINTER  m_pVal;

    OdbDATA( SQLSMALLINT m_nType = 0 );
    virtual ~OdbDATA();

    void       Clear();
    BOOL       Default() const;
    void       Default( BOOL bDefault );
    BOOL       Ignore() const;
    void       Ignore( BOOL bIgnore );
    SQLINTEGER Indicator() const;
    SQLINTEGER Indicator( SQLINTEGER nNewVal );
    void       Init( SQLPOINTER pVal, SQLINTEGER nLen, SQLINTEGER nMaxLen = 0 );
    BOOL       Null() const;
    void       Null( BOOL bNull );
    SQLINTEGER MaxLen() const;
    SQLINTEGER RestoreLen();
    SQLPOINTER Ptr() const;
};

class OdbVARDATA : public OdbDATA
{
public:
    OdbBuffer* m_Buffer;
    char*      m_Val;

    OdbVARDATA( SQLINTEGER nMaxLen = 0, SQLSMALLINT m_nType = 0 );
    virtual ~OdbVARDATA();

    BOOL       Alloc( SQLINTEGER nMaxLen );
    void       Free();
    SQLINTEGER Len() const;
    SQLINTEGER Len( SQLINTEGER nNewVal );
    BOOL       NoTotal() const;
    BOOL       Realloc( SQLINTEGER nMaxLen );
    void       SetLenAtExec( SQLINTEGER nVal );
};

class OdbBIGINT : public OdbDATA
{
public:
    SQLBIGINT m_Val;

    OdbBIGINT();
    virtual ~OdbBIGINT();

    operator SQLBIGINT() const { return( m_Val ); }
    const OdbBIGINT& operator=( OdbBIGINT& src );
    const OdbBIGINT& operator=( SQLBIGINT src );
    const OdbBIGINT& operator+=( SQLBIGINT src );
    const OdbBIGINT& operator-=( SQLBIGINT src );
    const OdbBIGINT& operator*=( SQLBIGINT src );
    const OdbBIGINT& operator/=( SQLBIGINT src );
    const OdbBIGINT& operator&=( SQLBIGINT src );
    const OdbBIGINT& operator|=( SQLBIGINT src );
    const OdbBIGINT& operator^=( SQLBIGINT src );
};

class OdbUBIGINT : public OdbDATA
{
public:
    SQLUBIGINT m_Val;

    OdbUBIGINT();
    virtual ~OdbUBIGINT();

    operator SQLUBIGINT() const { return( m_Val ); }
    const OdbUBIGINT& operator=( OdbUBIGINT& src );
    const OdbUBIGINT& operator=( SQLUBIGINT src );
    const OdbUBIGINT& operator+=( SQLUBIGINT src );
    const OdbUBIGINT& operator-=( SQLUBIGINT src );
    const OdbUBIGINT& operator*=( SQLUBIGINT src );
    const OdbUBIGINT& operator/=( SQLUBIGINT src );
    const OdbUBIGINT& operator&=( SQLUBIGINT src );
    const OdbUBIGINT& operator|=( SQLUBIGINT src );
    const OdbUBIGINT& operator^=( SQLUBIGINT src );
};

class OdbBINARY : public OdbVARDATA
{
public:

    OdbBINARY( SQLINTEGER nMaxLen = 0 );
    virtual ~OdbBINARY();

    operator SQLCHAR*() { return( (SQLCHAR*)m_Val ); }
    const OdbBINARY& operator=( OdbBINARY& src );
    SQLCHAR& operator[]( int nIndex );

    BOOL Set( SQLCHAR* src, SQLINTEGER nLen );
};

class OdbBIT : public OdbDATA
{
public:
    SQLCHAR m_Val;

    OdbBIT();
    virtual ~OdbBIT();

    operator SQLCHAR() const { return( m_Val ); }
    const OdbBIT& operator=( OdbBIT& src );
    const OdbBIT& operator=( SQLCHAR src );
    const OdbBIT& operator&=( SQLCHAR src );
    const OdbBIT& operator|=( SQLCHAR src );
    const OdbBIT& operator^=( SQLCHAR src );
};

class OdbBOOKMARK : public OdbDATA
{
public:
    BOOKMARK m_Val;

    OdbBOOKMARK();
    virtual ~OdbBOOKMARK();

    operator BOOKMARK() const { return( m_Val ); }
    const OdbBOOKMARK& operator=( OdbBOOKMARK& src );
    const OdbBOOKMARK& operator=( BOOKMARK src );
};

class OdbVARBOOKMARK : public OdbVARDATA
{
public:

    OdbVARBOOKMARK( SQLINTEGER nMaxLen = 0 );
    virtual ~OdbVARBOOKMARK();

    operator SQLPOINTER() { return( (SQLPOINTER)m_Val ); }
    const OdbVARBOOKMARK& operator=( OdbVARBOOKMARK& src );

    BOOL Set( SQLPOINTER src, SQLINTEGER nLen );
};

class OdbCHAR : public OdbVARDATA
{
public:

    OdbCHAR( SQLINTEGER nMaxLen = 0 );
    virtual ~OdbCHAR();

    operator char*() { return( m_Val ); }
    const OdbCHAR& operator=( const OdbCHAR& src );
    const OdbCHAR& operator=( const char* src );
    const OdbCHAR& operator=( SQLDOUBLE src );
    const OdbCHAR& operator=( SQLINTEGER src );
    const OdbCHAR& operator=( SQLUINTEGER src );
    const OdbCHAR& operator+=( const OdbCHAR& src );
    const OdbCHAR& operator+=( const char* src );
    char& operator[]( int nIndex );

    BOOL           Blank() const;
    SQLINTEGER     Compare( const char* src, SQLINTEGER nSrc = -1 ) const;
    BOOL           Concat( const char* src, SQLINTEGER nSrc = -1 );
    BOOL           Copy( const char* src, SQLINTEGER nSrc = -1 );
    BOOL           CopyConcat( const char* cpysrc, const char* catsrc );
    const OdbCHAR& MakeLower();
    const OdbCHAR& MakeUpper();
    SQLINTEGER     MaxStringLen() const;
    const OdbCHAR& Sprintf( const char* Format, ... );
    SQLINTEGER     SyncLen();
    SQLDOUBLE      ToDouble() const;
    SQLINTEGER     ToInt() const;
    SQLUINTEGER    ToUInt() const;
    const OdbCHAR& TrimLeft();
    const OdbCHAR& TrimRight();

    friend OdbCHAR operator+( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend OdbCHAR operator+( const OdbCHAR& s1, const char* s2 );
    friend OdbCHAR operator+( const char* s1, const OdbCHAR& s2 );

    friend BOOL operator==( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend BOOL operator==( const OdbCHAR& s1, const char* s2 );
    friend BOOL operator==( const char* s1, const OdbCHAR& s2 );
    friend BOOL operator!=( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend BOOL operator!=( const OdbCHAR& s1, const char* s2 );
    friend BOOL operator!=( const char* s1, const OdbCHAR& s2 );
    friend BOOL operator<( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend BOOL operator<( const OdbCHAR& s1, const char* s2 );
    friend BOOL operator<( const char* s1, const OdbCHAR& s2 );
    friend BOOL operator>( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend BOOL operator>( const OdbCHAR& s1, const char* s2 );
    friend BOOL operator>( const char* s1, const OdbCHAR& s2 );
    friend BOOL operator<=( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend BOOL operator<=( const OdbCHAR& s1, const char* s2 );
    friend BOOL operator<=( const char* s1, const OdbCHAR& s2 );
    friend BOOL operator>=( const OdbCHAR& s1, const OdbCHAR& s2 );
    friend BOOL operator>=( const OdbCHAR& s1, const char* s2 );
    friend BOOL operator>=( const char* s1, const OdbCHAR& s2 );
};

class OdbDATE : public OdbDATA
{
public:
    SQL_DATE_STRUCT m_Val;

    OdbDATE();
    virtual ~OdbDATE();

    operator SQL_DATE_STRUCT*() { return( &m_Val ); }
    const OdbDATE& operator=( OdbDATE& src );
    const OdbDATE& operator=( SQL_DATE_STRUCT& src );

    SQLSMALLINT Year() const;
    SQLSMALLINT Year( SQLSMALLINT nNewVal );
    SQLUSMALLINT Month() const;
    SQLUSMALLINT Month( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Day() const;
    SQLUSMALLINT Day( SQLUSMALLINT nNewVal );
    void         Today();
};

class OdbDATETIME : public OdbDATA
{
public:
    SQL_TIMESTAMP_STRUCT m_Val;

    OdbDATETIME();
    virtual ~OdbDATETIME();

    operator SQL_TIMESTAMP_STRUCT*() { return( &m_Val ); }
    const OdbDATETIME& operator=( OdbDATETIME& src );
    const OdbDATETIME& operator=( SQL_TIMESTAMP_STRUCT& src );
    const OdbDATETIME& operator=( SYSTEMTIME& src );

    SQLSMALLINT Year() const;
    SQLSMALLINT Year( SQLSMALLINT nNewVal );
    SQLUSMALLINT Month() const;
    SQLUSMALLINT Month( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Day() const;
    SQLUSMALLINT Day( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Hour() const;
    SQLUSMALLINT Hour( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Minute() const;
    SQLUSMALLINT Minute( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Second() const;
    SQLUSMALLINT Second( SQLUSMALLINT nNewVal );
    SQLUINTEGER Fraction() const;
    SQLUINTEGER Fraction( SQLUINTEGER nNewVal );
    void        Today();
    SQLINTEGER  Compare( const OdbDATETIME& dt ) const;
    SQLINTEGER  Compare( const SYSTEMTIME& dt ) const;

    friend BOOL operator==( const OdbDATETIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator==( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 );
    friend BOOL operator==( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator!=( const OdbDATETIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator!=( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 );
    friend BOOL operator!=( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator<( const OdbDATETIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator<( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 );
    friend BOOL operator<( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator>( const OdbDATETIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator>( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 );
    friend BOOL operator>( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator<=( const OdbDATETIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator<=( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 );
    friend BOOL operator<=( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator>=( const OdbDATETIME& dt1, const OdbDATETIME& dt2 );
    friend BOOL operator>=( const OdbDATETIME& dt1, const SYSTEMTIME& dt2 );
    friend BOOL operator>=( const SYSTEMTIME& dt1, const OdbDATETIME& dt2 );
};

class OdbDOUBLE : public OdbDATA
{
public:
    SQLDOUBLE m_Val;

    OdbDOUBLE();
    virtual ~OdbDOUBLE();

    operator SQLDOUBLE() const { return( m_Val ); }
    const OdbDOUBLE& operator=( OdbDOUBLE& src );
    const OdbDOUBLE& operator=( SQLDOUBLE src );
    const OdbDOUBLE& operator+=( SQLDOUBLE src );
    const OdbDOUBLE& operator-=( SQLDOUBLE src );
    const OdbDOUBLE& operator*=( SQLDOUBLE src );
    const OdbDOUBLE& operator/=( SQLDOUBLE src );
};

class OdbGUID : public OdbDATA
{
public:
    SQLGUID m_Val;

    OdbGUID();
    virtual ~OdbGUID();

    operator SQLGUID*() { return( &m_Val ); }
    const OdbGUID& operator=( OdbGUID& src );
    const OdbGUID& operator=( SQLGUID& src );

    SQLUINTEGER    Data1() const;
    SQLUINTEGER    Data1( SQLUINTEGER nNewVal );
    SQLUSMALLINT   Data2() const;
    SQLUSMALLINT   Data2( SQLUSMALLINT nNewVal );
    SQLUSMALLINT   Data3() const;
    SQLUSMALLINT   Data3( SQLUSMALLINT nNewVal );
    const SQLCHAR* Data4() const;
    const SQLCHAR* Data4( const SQLCHAR* pNewVal );
};

class OdbINT : public OdbDATA
{
public:
    SQLINTEGER m_Val;

    OdbINT();
    virtual ~OdbINT();

    operator SQLINTEGER() const { return( m_Val ); }
    const OdbINT& operator=( OdbINT& src );
    const OdbINT& operator=( SQLINTEGER src );
    const OdbINT& operator+=( SQLINTEGER src );
    const OdbINT& operator-=( SQLINTEGER src );
    const OdbINT& operator*=( SQLINTEGER src );
    const OdbINT& operator/=( SQLINTEGER src );
    const OdbINT& operator&=( SQLINTEGER src );
    const OdbINT& operator|=( SQLINTEGER src );
    const OdbINT& operator^=( SQLINTEGER src );
};

class OdbUINT : public OdbDATA
{
public:
    SQLUINTEGER m_Val;

    OdbUINT();
    virtual ~OdbUINT();

    operator SQLUINTEGER() const { return( m_Val ); }
    const OdbUINT& operator=( OdbUINT& src );
    const OdbUINT& operator=( SQLUINTEGER src );
    const OdbUINT& operator+=( SQLUINTEGER src );
    const OdbUINT& operator-=( SQLUINTEGER src );
    const OdbUINT& operator*=( SQLUINTEGER src );
    const OdbUINT& operator/=( SQLUINTEGER src );
    const OdbUINT& operator&=( SQLUINTEGER src );
    const OdbUINT& operator|=( SQLUINTEGER src );
    const OdbUINT& operator^=( SQLUINTEGER src );
};

class OdbNUMERIC : public OdbDATA
{
public:
    SQL_NUMERIC_STRUCT m_Val;

    OdbNUMERIC();
    virtual ~OdbNUMERIC();

    operator SQL_NUMERIC_STRUCT*() { return( &m_Val ); }
    const OdbNUMERIC& operator=( OdbNUMERIC& src );
    const OdbNUMERIC& operator=( SQL_NUMERIC_STRUCT& src );

    SQLCHAR        Precision() const;
    SQLCHAR        Precision( SQLCHAR nNewVal );
    SQLSCHAR       Scale() const;
    SQLSCHAR       Scale( SQLSCHAR nNewVal );
    SQLCHAR        Sign() const;
    SQLCHAR        Sign( SQLCHAR nNewVal );
    const SQLCHAR* Val() const;
    const SQLCHAR* Val( const SQLCHAR* pNewVal );
};

class OdbREAL : public OdbDATA
{
public:
    SQLREAL m_Val;

    OdbREAL();
    virtual ~OdbREAL();

    operator SQLREAL() const { return( m_Val ); }
    const OdbREAL& operator=( OdbREAL& src );
    const OdbREAL& operator=( SQLREAL src );
    const OdbREAL& operator+=( SQLREAL src );
    const OdbREAL& operator-=( SQLREAL src );
    const OdbREAL& operator*=( SQLREAL src );
    const OdbREAL& operator/=( SQLREAL src );
};

class OdbSMALLINT : public OdbDATA
{
public:
    SQLSMALLINT m_Val;

    OdbSMALLINT();
    virtual ~OdbSMALLINT();

    operator SQLSMALLINT() const { return( m_Val ); }
    const OdbSMALLINT& operator=( OdbSMALLINT& src );
    const OdbSMALLINT& operator=( SQLSMALLINT src );
    const OdbSMALLINT& operator+=( SQLSMALLINT src );
    const OdbSMALLINT& operator-=( SQLSMALLINT src );
    const OdbSMALLINT& operator*=( SQLSMALLINT src );
    const OdbSMALLINT& operator/=( SQLSMALLINT src );
    const OdbSMALLINT& operator&=( SQLSMALLINT src );
    const OdbSMALLINT& operator|=( SQLSMALLINT src );
    const OdbSMALLINT& operator^=( SQLSMALLINT src );
};

class OdbUSMALLINT : public OdbDATA
{
public:
    SQLUSMALLINT m_Val;

    OdbUSMALLINT();
    virtual ~OdbUSMALLINT();

    operator SQLUSMALLINT() const { return( m_Val ); }
    const OdbUSMALLINT& operator=( OdbUSMALLINT& src );
    const OdbUSMALLINT& operator=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator+=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator-=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator*=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator/=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator&=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator|=( SQLUSMALLINT src );
    const OdbUSMALLINT& operator^=( SQLUSMALLINT src );
};

class OdbTIME : public OdbDATA
{
public:
    SQL_TIME_STRUCT m_Val;

    OdbTIME();
    virtual ~OdbTIME();

    operator SQL_TIME_STRUCT*() { return( &m_Val ); }
    const OdbTIME& operator=( OdbTIME& src );
    const OdbTIME& operator=( SQL_TIME_STRUCT& src );

    SQLUSMALLINT Hour() const;
    SQLUSMALLINT Hour( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Minute() const;
    SQLUSMALLINT Minute( SQLUSMALLINT nNewVal );
    SQLUSMALLINT Second() const;
    SQLUSMALLINT Second( SQLUSMALLINT nNewVal );
    void         Current();
};

class OdbTINYINT : public OdbDATA
{
public:
    SQLSCHAR m_Val;

    OdbTINYINT();
    virtual ~OdbTINYINT();

    operator SQLSCHAR() const { return( m_Val ); }
    const OdbTINYINT& operator=( OdbTINYINT& src );
    const OdbTINYINT& operator=( SQLSCHAR src );
    const OdbTINYINT& operator+=( SQLSCHAR src );
    const OdbTINYINT& operator-=( SQLSCHAR src );
    const OdbTINYINT& operator*=( SQLSCHAR src );
    const OdbTINYINT& operator/=( SQLSCHAR src );
    const OdbTINYINT& operator&=( SQLSCHAR src );
    const OdbTINYINT& operator|=( SQLSCHAR src );
    const OdbTINYINT& operator^=( SQLSCHAR src );
};

class OdbUTINYINT : public OdbDATA
{
public:
    SQLCHAR m_Val;

    OdbUTINYINT();
    virtual ~OdbUTINYINT();

    operator SQLCHAR() const { return( m_Val ); }
    const OdbUTINYINT& operator=( OdbUTINYINT& src );
    const OdbUTINYINT& operator=( SQLCHAR src );
    const OdbUTINYINT& operator+=( SQLCHAR src );
    const OdbUTINYINT& operator-=( SQLCHAR src );
    const OdbUTINYINT& operator*=( SQLCHAR src );
    const OdbUTINYINT& operator/=( SQLCHAR src );
    const OdbUTINYINT& operator&=( SQLCHAR src );
    const OdbUTINYINT& operator|=( SQLCHAR src );
    const OdbUTINYINT& operator^=( SQLCHAR src );
};

class OdbWVARDATA : public OdbDATA
{
public:
    OdbBuffer* m_Buffer;
    wchar_t*   m_Val;

    OdbWVARDATA( SQLINTEGER nMaxLen = 0, SQLSMALLINT m_nType = 0 );
    virtual ~OdbWVARDATA();

    BOOL       Alloc( SQLINTEGER nMaxLen );
    void       Free();
    SQLINTEGER Len() const;
    SQLINTEGER Len( SQLINTEGER nNewVal );
    BOOL       NoTotal() const;
    BOOL       Realloc( SQLINTEGER nMaxLen );
    void       SetLenAtExec( SQLINTEGER nVal );
};

class OdbWCHAR : public OdbWVARDATA
{
public:

    OdbWCHAR( SQLINTEGER nMaxLen = 0 );
    virtual ~OdbWCHAR();

    operator wchar_t*() { return( m_Val ); }
    const OdbWCHAR& operator=( const OdbWCHAR& src );
    const OdbWCHAR& operator=( const wchar_t* src );
    const OdbWCHAR& operator=( SQLDOUBLE src );
    const OdbWCHAR& operator=( SQLINTEGER src );
    const OdbWCHAR& operator=( SQLUINTEGER src );
    const OdbWCHAR& operator+=( const OdbWCHAR& src );
    const OdbWCHAR& operator+=( const wchar_t* src );
    wchar_t& operator[]( int nIndex );

    BOOL            Blank() const;
    SQLINTEGER      Compare( const wchar_t* src, SQLINTEGER nSrc = -1 ) const;
    BOOL            Concat( const wchar_t* src, SQLINTEGER nSrc = -1 );
    BOOL            Copy( const wchar_t* src, SQLINTEGER nSrc = -1 );
    BOOL            CopyConcat( const wchar_t* cpysrc, const wchar_t* catsrc );
    const OdbWCHAR& MakeLower();
    const OdbWCHAR& MakeUpper();
    SQLINTEGER      MaxStringLen() const;
    const OdbWCHAR& Sprintf( const wchar_t* Format, ... );
    SQLINTEGER      SyncLen();
    SQLDOUBLE       ToDouble() const;
    SQLINTEGER      ToInt() const;
    SQLUINTEGER     ToUInt() const;
    const OdbWCHAR& TrimLeft();
    const OdbWCHAR& TrimRight();
    BOOL            UTF8Concat( const char* src, SQLINTEGER nSrc = -1 );
    BOOL            UTF8Copy( const char* src, SQLINTEGER nSrc = -1 );
    SQLINTEGER      UTF8Decode( SQLINTEGER nIndex, const char* mbchar );
    SQLINTEGER      UTF8Encode( char* mbchar, SQLINTEGER nIndex ) const;
    SQLINTEGER      UTF8Len() const;
    SQLINTEGER      UTF8WideCharLen( const char* str, SQLINTEGER nStr = -1 ) const;

    friend OdbWCHAR operator+( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend OdbWCHAR operator+( const OdbWCHAR& s1, const wchar_t* s2 );
    friend OdbWCHAR operator+( const wchar_t* s1, const OdbWCHAR& s2 );

    friend BOOL operator==( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend BOOL operator==( const OdbWCHAR& s1, const wchar_t* s2 );
    friend BOOL operator==( const wchar_t* s1, const OdbWCHAR& s2 );
    friend BOOL operator!=( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend BOOL operator!=( const OdbWCHAR& s1, const wchar_t* s2 );
    friend BOOL operator!=( const wchar_t* s1, const OdbWCHAR& s2 );
    friend BOOL operator<( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend BOOL operator<( const OdbWCHAR& s1, const wchar_t* s2 );
    friend BOOL operator<( const wchar_t* s1, const OdbWCHAR& s2 );
    friend BOOL operator>( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend BOOL operator>( const OdbWCHAR& s1, const wchar_t* s2 );
    friend BOOL operator>( const wchar_t* s1, const OdbWCHAR& s2 );
    friend BOOL operator<=( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend BOOL operator<=( const OdbWCHAR& s1, const wchar_t* s2 );
    friend BOOL operator<=( const wchar_t* s1, const OdbWCHAR& s2 );
    friend BOOL operator>=( const OdbWCHAR& s1, const OdbWCHAR& s2 );
    friend BOOL operator>=( const OdbWCHAR& s1, const wchar_t* s2 );
    friend BOOL operator>=( const wchar_t* s1, const OdbWCHAR& s2 );
};

#endif
