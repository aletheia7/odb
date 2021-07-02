/* $Id: OdbSupport.cpp,v 1.36 2005/12/22 04:44:42 rtwitty Exp $ */
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
#include "OdbtpSock.h"
#include "OdbSupport.h"
#include "ODBTPService.h"
#include "UTF8Support.h"

#define ABS_MAX_PARAMS 256

void DisplayMSSQLDiag( OdbHandle* pOdb, OdbDiagHandler DiagHandler );
void DisplayMSSQLDiagW( OdbHandle* pOdb, OdbDiagHandlerW DiagHandler );

//////////////////////////////////////////////////////////////////////
// COdbtpCon Class Implementation

BOOL    COdbtpCon::m_bBadConnCheck = FALSE;
BOOL    COdbtpCon::m_bEnableProcCache = FALSE;
int     COdbtpCon::m_nMaxQrys = 4;
OdbEnv* COdbtpCon::m_pEnv = NULL;
ULONG   COdbtpCon::m_ulConnectTimeout = 0;
ULONG   COdbtpCon::m_ulFetchRowCount = 0;
ULONG   COdbtpCon::m_ulQueryTimeout = 0;

COdbtpCon::COdbtpCon()
{
    m_bInUse = FALSE;
    m_usType = ODB_CON_NORMAL;
    m_bSentId = FALSE;
    m_nPoolIndex = -1;
    m_u64ReleaseTime = 0;
    m_szId[0] = 0;
    m_ppQrys = NULL;
    m_ppProcs = NULL;
    m_nMaxProcs = 0;
    m_nTotalProcs = 0;
}

COdbtpCon::~COdbtpCon()
{
    if( m_ppQrys )
    {
        DeleteQrys();
        delete[] m_ppQrys;
    }
    FreeProcCache();
}

BOOL COdbtpCon::AddProc( COdbProc* pProc )
{
    if( !(pProc->m_bCached = m_ulAttrCacheProcs ? TRUE : FALSE) )
        return( TRUE );

    if( m_nTotalProcs == m_nMaxProcs )
    {
        if( m_nMaxProcs == 0 )
        {
            m_nMaxProcs = 8;
            m_ppProcs = (COdbProc**)malloc( sizeof(COdbProc*) * m_nMaxProcs );
        }
        else
        {
            m_nMaxProcs += 4;
            m_ppProcs = (COdbProc**)realloc( m_ppProcs,
                                             sizeof(COdbProc*) * m_nMaxProcs );
        }
        if( !m_ppProcs ) return( FALSE );
    }
    m_ppProcs[m_nTotalProcs++] = pProc;
    return( TRUE );
}

void COdbtpCon::DeleteQrys()
{
    if( m_ppQrys )
    {
        for( int n = 0; n < m_nMaxQrys; n++ )
        {
            if( !m_ppQrys[n] ) continue;
            delete m_ppQrys[n];
            m_ppQrys[n] = NULL;
        }
    }
}

BOOL COdbtpCon::DetachQrys()
{
    if( m_ppQrys )
    {
        for( int n = 0; n < m_nMaxQrys; n++ )
            if( m_ppQrys[n] && !m_ppQrys[n]->Detach() ) return( FALSE );
    }
    return( TRUE );
}

void COdbtpCon::DisplayDiag( OdbDiagHandler DiagHandler )
{
    if( m_ulAttrDriver != ODB_DRIVER_MSSQL ||
        (m_nLastResult != SQL_SUCCESS_WITH_INFO &&
         m_nLastResult != SQL_ERROR) )
    {
        OdbCon::DisplayDiag( DiagHandler );
    }
    else
    {
        ::DisplayMSSQLDiag( this, DiagHandler );
    }
}

void COdbtpCon::DisplayDiagW( OdbDiagHandlerW DiagHandler )
{
    if( m_ulAttrDriver != ODB_DRIVER_MSSQL ||
        (m_nLastResult != SQL_SUCCESS_WITH_INFO &&
         m_nLastResult != SQL_ERROR) )
    {
        OdbCon::DisplayDiagW( DiagHandler );
    }
    else
    {
        ::DisplayMSSQLDiagW( this, DiagHandler );
    }
}

BOOL COdbtpCon::Drop()
{
    if( Connected() )
    {
        DeleteQrys();
        FreeProcCache();
        if( m_ulAttrTransactions != ODB_TXN_NONE ) Rollback();
        ResetAttributes();
        if( !Disconnect() ) return( FALSE );
    }
    m_bInUse = FALSE;
    m_bSentId = FALSE;
    m_u64ReleaseTime = 0;
    m_szId[0] = 0;
    m_ulAttrDriver = ODB_DRIVER_UNKNOWN;

    return( TRUE );
}

void COdbtpCon::FreeProcCache()
{
    if( m_ppProcs )
    {
        for( int n = 0; n < m_nTotalProcs; n++ ) delete m_ppProcs[n];
        free( m_ppProcs );
        m_ppProcs = NULL;
        m_nTotalProcs = 0;
        m_nMaxProcs = 0;
    }
}

BOOL COdbtpCon::GetAttribute( COdbtpSock* pSock )
{
    long   lAttr;
    USHORT usType;

    if( !pSock->ExtractRequest( (PULONG)&lAttr ) ||
        !pSock->ExtractRequest( &usType ) )
    {
        return( pSock->SendResponseText( ODBTP_INVALID, "Invalid GETATTR request" ) );
    }
    if( usType == 0 )
    {
        ULONG  ulVal;
        USHORT usVal;

        switch( lAttr )
        {
            case ODB_ATTR_DRIVER:
                ulVal = m_ulAttrDriver;
                break;

            case ODB_ATTR_FETCHROWCOUNT:
                ulVal = m_ulAttrFetchRowCount;
                break;

            case ODB_ATTR_TRANSACTIONS:
                ulVal = m_ulAttrTransactions;
                break;

            case ODB_ATTR_DESCRIBEPARAMS:
                ulVal = m_ulAttrDescribeParams;
                break;

            case ODB_ATTR_UNICODESQL:
                ulVal = m_ulAttrUnicodeSQL;
                break;

            case ODB_ATTR_FULLCOLINFO:
                ulVal = m_ulAttrFullColInfo;
                break;

            case ODB_ATTR_MAPCHARTOWCHAR:
                ulVal = m_ulAttrMapCharToWChar;
                break;

            case ODB_ATTR_VARDATASIZE:
                ulVal = m_ulAttrVarDataSize;
                break;

            case ODB_ATTR_CACHEPROCS:
                ulVal = m_ulAttrCacheProcs;
                break;

            case ODB_ATTR_RIGHTTRIMTEXT:
                ulVal = m_ulAttrRightTrimText;
                break;

            case ODB_ATTR_QUERYTIMEOUT:
                ulVal = m_ulAttrQueryTimeout;
                break;

            case ODB_ATTR_OICLEVEL:
                GetInfo( SQL_ODBC_INTERFACE_CONFORMANCE, &ulVal );
                break;

            case ODB_ATTR_TXNCAPABLE:
                GetInfo( SQL_TXN_CAPABLE, &usVal );
                ulVal = usVal != SQL_TC_NONE ? 1 : 0;
                break;

            default:
                return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unknown numeric attribute" ) );
        }
        if( !pSock->SendResponse( ODBTP_ATTRIBUTE, ulVal, FALSE ) )
            return( FALSE );
    }
    else
    {
        char*  psz = NULL;
        char   sz[256];
        ULONG  ulLen;
        USHORT usInfo;

        switch( lAttr )
        {
            case ODB_ATTR_DSN:           usInfo = SQL_DATA_SOURCE_NAME; break;
            case ODB_ATTR_DRIVERNAME:    usInfo = SQL_DRIVER_NAME; break;
            case ODB_ATTR_DRIVERVER:     usInfo = SQL_DRIVER_VER; break;
            case ODB_ATTR_DRIVERODBCVER: usInfo = SQL_DRIVER_ODBC_VER; break;
            case ODB_ATTR_DBMSNAME:      usInfo = SQL_DBMS_NAME; break;
            case ODB_ATTR_DBMSVER:       usInfo = SQL_DBMS_VER; break;
            case ODB_ATTR_SERVERNAME:    usInfo = SQL_SERVER_NAME; break;
            case ODB_ATTR_USERNAME:      usInfo = SQL_USER_NAME; break;
            case ODB_ATTR_DATABASENAME:  usInfo = SQL_DATABASE_NAME; break;

            default:
                return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unknown string attribute" ) );
        }
        if( !psz )
        {
            if( !GetInfo( usInfo, sz, sizeof(sz) ) ) return( FALSE );
            psz = sz;
        }
        ulLen = strlen( psz );
        if( !pSock->SendResponse( ODBTP_ATTRIBUTE, ulLen, FALSE ) )
            return( FALSE );
        if( ulLen && !pSock->SendResponse( ODBTP_ATTRIBUTE, psz, ulLen, FALSE ) )
            return( FALSE );
    }
    return( pSock->SendResponse( ODBTP_ATTRIBUTE ) );
}

COdbProc* COdbtpCon::GetProc( char* pszCatalog, char* pszSchema,
                              char* pszProcedure )
{
    int       n;
    COdbProc* pProc;

    if( !m_ulAttrCacheProcs ) return( NULL );

    if( !pszCatalog ) pszCatalog = "";
    if( !pszSchema ) pszSchema = "";

    for( n = 0; n < m_nTotalProcs; n++ )
    {
        pProc = m_ppProcs[n];
        if( pProc->m_bUnicode ) continue;

        if( !strcmp( pProc->m_szCatalog, pszCatalog ) )
            if( !strcmp( pProc->m_szSchema, pszSchema ) )
                if( !strcmp( pProc->m_szProcedure, pszProcedure ) )
                    return( pProc );
    }
    return( NULL );
}

COdbProc* COdbtpCon::GetProcW( wchar_t* pszCatalog, wchar_t* pszSchema,
                               wchar_t* pszProcedure )
{
    int       n;
    COdbProc* pProc;

    if( !m_ulAttrCacheProcs ) return( NULL );

    if( !pszCatalog ) pszCatalog = L"";
    if( !pszSchema ) pszSchema = L"";

    for( n = 0; n < m_nTotalProcs; n++ )
    {
        pProc = m_ppProcs[n];
        if( !pProc->m_bUnicode ) continue;

        if( !wcscmp( pProc->m_pszCatalogW, pszCatalog ) )
            if( !wcscmp( pProc->m_pszSchemaW, pszSchema ) )
                if( !wcscmp( pProc->m_pszProcedureW, pszProcedure ) )
                    return( pProc );
    }
    return( NULL );
}

COdbtpQry* COdbtpCon::GetQry( int nId )
{
    COdbtpQry* pQry;

    if( nId >= m_nMaxQrys ) return( NULL );

    if( !(pQry = m_ppQrys[nId]) )
    {
        if( !(pQry = new COdbtpQry) ) return( NULL );
        m_ppQrys[nId] = pQry;
    }
    if( !pQry->Attach( this ) ) return( NULL );
    return( pQry );
}

BOOL COdbtpCon::Init( USHORT usType, int nPoolIndex )
{
    if( !Allocate( m_pEnv ) ) return( FALSE );

    if( m_ulConnectTimeout ) SetConnectTimeout( m_ulConnectTimeout );
    if( m_ulQueryTimeout ) SetQueryTimeout( m_ulQueryTimeout );

    if( m_ppQrys )
    {
        DeleteQrys();
        delete[] m_ppQrys;
    }
    if( !(m_ppQrys = new COdbtpQry*[m_nMaxQrys]) )
    {
        m_nLastResult = ODB_ERR_MEMORY;
        return( ProcessLastResult() );
    }
    for( int n = 0; n < m_nMaxQrys; n++ ) m_ppQrys[n] = NULL;
    FreeProcCache();
    m_bInUse = FALSE;
    m_bSentId = FALSE;
    m_nPoolIndex = nPoolIndex;
    m_usType = usType;
    m_u64ReleaseTime = 0;
    m_szId[0] = 0;

    m_ulAttrQueryTimeout = m_ulQueryTimeout;
    m_ulAttrFetchRowCount = m_ulFetchRowCount;
    m_ulAttrTransactions = ODB_TXN_NONE;
    m_ulAttrDescribeParams = 1;
    m_ulAttrUnicodeSQL = 0;
    m_ulAttrFullColInfo = 0;
    m_ulAttrMapCharToWChar = 0;
    m_ulAttrVarDataSize = 0x10000;
    m_ulAttrCacheProcs = m_bEnableProcCache ? 1 : 0;

    return( TRUE );
}

BOOL COdbtpCon::IsGood()
{
    COdbtpQry* pQry;

    if( !Connected() ) return( FALSE );
    if( m_usType != ODB_CON_NORMAL || !m_bBadConnCheck ) return( TRUE );
    if( !(pQry = GetQry( 0 )) ) return( FALSE );
    if( pQry->Execute( "SELECT 1" ) && pQry->DumpResults() ) return( TRUE );

    pQry->DisplayDiag( CODBTPService::LogOdbDiag );

    return( FALSE );
}

BOOL COdbtpCon::PostConnectInit()
{
    char sz[64];

    if( !GetInfo( SQL_DRIVER_NAME, sz, 63 ) ) return( FALSE );

    if( !strnicmp( sz, "SQLSRV", 6 ) )
        m_ulAttrDriver = ODB_DRIVER_MSSQL;
    else if( !strnicmp( sz, "ODBCJT", 6 ) )
        m_ulAttrDriver = ODB_DRIVER_JET;
    else if( !strnicmp( sz, "VFPODBC", 7 ) )
        m_ulAttrDriver = ODB_DRIVER_FOXPRO;
    else if( !strnicmp( sz, "SQORA", 5 ) )
        m_ulAttrDriver = ODB_DRIVER_ORACLE;
    else if( !strnicmp( sz, "SY", 2 ) )
        m_ulAttrDriver = ODB_DRIVER_SYBASE;
    else if( !strnicmp( sz, "MYODBC", 6 ) )
        m_ulAttrDriver = ODB_DRIVER_MYSQL;
    else if( !strnicmp( sz, "DB2", 3 ) )
        m_ulAttrDriver = ODB_DRIVER_DB2;
    else
        m_ulAttrDriver = ODB_DRIVER_UNKNOWN;

    return( TRUE );
}

BOOL COdbtpCon::Release()
{
    if( !Connected() ) return( Drop() );

    if( m_usType != ODB_CON_RESERVED )
    {
        if( !DetachQrys() ) return( FALSE );

        if( m_ulAttrTransactions != ODB_TXN_NONE && !Rollback() )
            return( FALSE );
        if( !ResetAttributes() ) return( FALSE );
    }
    else if( !m_bSentId )
    {
        return( Drop() );
    }
    m_nLastResult = SQL_SUCCESS;
    m_bInUse = FALSE;
    ::GetSystemTimeAsFileTime( (LPFILETIME)&m_u64ReleaseTime );

    return( TRUE );
}

BOOL COdbtpCon::ResetAttributes()
{
    if( m_ulAttrQueryTimeout != m_ulQueryTimeout )
    {
        m_ulAttrQueryTimeout = m_ulQueryTimeout;
        if( !SetQueryTimeout( m_ulQueryTimeout ) ) return( FALSE );
    }
    m_ulAttrFetchRowCount = m_ulFetchRowCount;

    if( m_ulAttrTransactions != ODB_TXN_NONE )
    {
        m_ulAttrTransactions = ODB_TXN_NONE;
        if( !EnableManualCommit( FALSE ) ) return( FALSE );
    }
    m_ulAttrDescribeParams = 1;
    m_ulAttrUnicodeSQL = 0;
    m_ulAttrFullColInfo = 0;
    m_ulAttrMapCharToWChar = 0;
    m_ulAttrVarDataSize = 0x10000;
    m_ulAttrCacheProcs = m_bEnableProcCache ? 1 : 0;
    m_ulAttrRightTrimText = 0;

    return( TRUE );
}

BOOL COdbtpCon::SendId( COdbtpSock* pSock )
{
    if( m_usType != ODB_CON_RESERVED )
        return( pSock->SendResponse( ODBTP_CONNECTID, "", 0 ) );
    m_bSentId = TRUE;
    return( pSock->SendResponse( ODBTP_CONNECTID, m_szId, strlen(m_szId) ) );
}

BOOL COdbtpCon::SetAttribute( COdbtpSock* pSock )
{
    long   lAttr;
    USHORT usType;

    if( !pSock->ExtractRequest( (PULONG)&lAttr ) ||
        !pSock->ExtractRequest( &usType ) )
    {
        return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETATTR request" ) );
    }
    if( usType == 0 )
    {
        ULONG ulVal;

        if( !pSock->ExtractRequest( &ulVal ) )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETATTR request" ) );

        switch( lAttr )
        {
            case ODB_ATTR_FETCHROWCOUNT:
                m_ulAttrFetchRowCount = ulVal;
                break;

            case ODB_ATTR_TRANSACTIONS:
                switch( ulVal )
                {
                    case ODB_TXN_NONE:
                        if( !EnableManualCommit( FALSE ) ) return( FALSE );
                        break;

                    case ODB_TXN_READUNCOMMITTED:
                        if( !UseReadUncommittedTrans() ) return( FALSE );
                        break;

                    case ODB_TXN_READCOMMITTED:
                        if( !UseReadCommittedTrans() ) return( FALSE );
                        break;

                    case ODB_TXN_REPEATABLEREAD:
                        if( !UseRepeatableReadTrans() ) return( FALSE );
                        break;

                    case ODB_TXN_SERIALIZABLE:
                        if( !UseSerializableTrans() ) return( FALSE );
                        break;

                    case ODB_TXN_DEFAULT:
                        if( !EnableManualCommit() ) return( FALSE );
                        break;

                    default:
                        return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unknown transaction type" ) );
                }
                m_ulAttrTransactions = ulVal;
                break;

            case ODB_ATTR_DESCRIBEPARAMS:
                m_ulAttrDescribeParams = ulVal ? 1 : 0;
                break;

            case ODB_ATTR_UNICODESQL:
                m_ulAttrUnicodeSQL = ulVal ? 1 : 0;
                if( m_ulAttrDriver == ODB_DRIVER_JET )
                    m_ulAttrMapCharToWChar = m_ulAttrUnicodeSQL;
                break;

            case ODB_ATTR_FULLCOLINFO:
                m_ulAttrFullColInfo = ulVal ? 1 : 0;
                break;

            case ODB_ATTR_MAPCHARTOWCHAR:
                m_ulAttrMapCharToWChar = ulVal ? 1 : 0;
                break;

            case ODB_ATTR_VARDATASIZE:
                if( ulVal < 1000000000 )
                    m_ulAttrVarDataSize = ulVal != 0 ? ulVal : 0x10000;
                else
                    m_ulAttrVarDataSize = 1000000000;
                break;

            case ODB_ATTR_CACHEPROCS:
                if( m_bEnableProcCache )
                {
                    if( !(m_ulAttrCacheProcs = ulVal ? 1 : 0) )
                        FreeProcCache();
                }
                break;

            case ODB_ATTR_RIGHTTRIMTEXT:
                m_ulAttrRightTrimText = ulVal ? 1 : 0;
                break;

            case ODB_ATTR_QUERYTIMEOUT:
                m_ulAttrQueryTimeout = ulVal;
                SetQueryTimeout( m_ulAttrQueryTimeout );
                SetTimeoutForQrys( m_ulAttrQueryTimeout );
                break;

            case ODB_ATTR_DRIVER:
            case ODB_ATTR_OICLEVEL:
            case ODB_ATTR_TXNCAPABLE:
                return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Attribute is read only" ) );

            default:
                return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unknown numeric attribute" ) );
        }
    }
    else
    {
        PSTR  pszVal;
        ULONG ulLen;

        if( !pSock->ExtractRequest( &ulLen ) ||
            (ulLen && !pSock->ExtractRequest( (PBYTE*)&pszVal, ulLen )) )
        {
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETATTR request" ) );
        }
        switch( lAttr )
        {
            case ODB_ATTR_DSN:
            case ODB_ATTR_DRIVERNAME:
            case ODB_ATTR_DRIVERVER:
            case ODB_ATTR_DRIVERODBCVER:
            case ODB_ATTR_DBMSNAME:
            case ODB_ATTR_DBMSVER:
            case ODB_ATTR_SERVERNAME:
            case ODB_ATTR_USERNAME:
            case ODB_ATTR_DATABASENAME:
                return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Attribute is read only" ) );

            default:
                return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unknown string attribute" ) );
        }
    }
    return( pSock->SendResponse( ODBTP_OK ) );
}

BOOL COdbtpCon::SetTimeoutForQrys( ULONG ulTimeout )
{
    if( m_ppQrys )
    {
        for( int n = 0; n < m_nMaxQrys; n++ )
        {
            if( m_ppQrys[n] && m_ppQrys[n]->Allocated() )
                m_ppQrys[n]->SetTimeout( ulTimeout );
        }
    }
    return( TRUE );
}

//////////////////////////////////////////////////////////////////////
// COdbData Class Implementation

COdbData::COdbData()
{
    m_bBound = FALSE;
    m_pData = NULL;
    m_sSqlType = 0;
    m_ulColSize = 0;
    m_sDecDigits = 0;
    m_sNullable = 0;
    m_sDefaultOdbType = 0;
    m_sOdbType = 0;
    m_pszNameW = (wchar_t*)m_szName;
}

COdbData::~COdbData()
{
    if( m_pData ) delete m_pData;
}

BOOL COdbData::Alloc( short sOdbType, long lLen, ULONG ulVarDataSize )
{
    ULONG ulColSize;

    if( !m_pData || m_pData->m_nType != sOdbType )
    {
        if( m_pData ) delete m_pData;

        switch( sOdbType )
        {
            case ODB_BINARY:    m_pData = (OdbDATA*)new OdbBINARY; break;
            case ODB_BIGINT:    m_pData = (OdbDATA*)new OdbBIGINT; break;
            case ODB_UBIGINT:   m_pData = (OdbDATA*)new OdbUBIGINT; break;
            case ODB_BIT:       m_pData = (OdbDATA*)new OdbBIT; break;
            case ODB_CHAR:      m_pData = (OdbDATA*)new OdbCHAR; break;
            case ODB_DATE:      m_pData = (OdbDATA*)new OdbDATE; break;
            case ODB_DATETIME:  m_pData = (OdbDATA*)new OdbDATETIME; break;
            case ODB_DOUBLE:    m_pData = (OdbDATA*)new OdbDOUBLE; break;
            case ODB_GUID:      m_pData = (OdbDATA*)new OdbGUID; break;
            case ODB_INT:       m_pData = (OdbDATA*)new OdbINT; break;
            case ODB_UINT:      m_pData = (OdbDATA*)new OdbUINT; break;
            case ODB_NUMERIC:   m_pData = (OdbDATA*)new OdbNUMERIC; break;
            case ODB_REAL:      m_pData = (OdbDATA*)new OdbREAL; break;
            case ODB_SMALLINT:  m_pData = (OdbDATA*)new OdbSMALLINT; break;
            case ODB_USMALLINT: m_pData = (OdbDATA*)new OdbUSMALLINT; break;
            case ODB_TIME:      m_pData = (OdbDATA*)new OdbTIME; break;
            case ODB_TINYINT:   m_pData = (OdbDATA*)new OdbTINYINT; break;
            case ODB_UTINYINT:  m_pData = (OdbDATA*)new OdbUTINYINT; break;
            case ODB_WCHAR:     m_pData = (OdbDATA*)new OdbWCHAR; break;
            default:            m_pData = (OdbDATA*)new OdbCHAR;
        }
        if( !m_pData ) return( FALSE );
    }
    switch( m_pData->m_nType )
    {
        case ODB_BINARY:
        case ODB_CHAR:
        case ODB_WCHAR:
            switch( m_sSqlType )
            {
                // Prevents numeric overflow for smalldatetime
                case SQL_DATE:
                case SQL_TIME:
                case SQL_TIMESTAMP:
                case SQL_TYPE_DATE:
                case SQL_TYPE_TIME:
                case SQL_TYPE_TIMESTAMP: ulColSize = 32; break;

                default: ulColSize = m_ulColSize ? m_ulColSize : 256;
            }
            if( ulColSize > ulVarDataSize ) ulColSize = ulVarDataSize;
            if( lLen < (long)ulColSize ) lLen = (long)ulColSize;
            if( m_pData->m_nType != ODB_BINARY ) lLen += 2;

            if( m_pData->m_nType != ODB_WCHAR )
            {
                if( lLen > m_pData->m_nMaxLen &&
                    !(((OdbVARDATA*)m_pData)->Realloc( lLen )) )
                {
                    return( FALSE );
                }
            }
            else
            {
                if( (lLen * (long)sizeof(wchar_t)) > m_pData->m_nMaxLen &&
                    !(((OdbWVARDATA*)m_pData)->Realloc( lLen )) )
                {
                    return( FALSE );
                }
            }
    }
    return( TRUE );
}

//////////////////////////////////////////////////////////////////////
// COdbColData Class Implementation

COdbColData::COdbColData()
{
    strcpy( m_szName, "" );
    m_sNameLen = 0;
}

COdbColData::~COdbColData()
{
}

//////////////////////////////////////////////////////////////////////
// COdbParamData Class Implementation

COdbParamData::COdbParamData()
{
    m_bDescribed = FALSE;
    m_usType = ODB_PARAM_NONE;
    m_pszTypeW = (wchar_t*)m_szType;
}

COdbParamData::~COdbParamData()
{
}

//////////////////////////////////////////////////////////////////////
// COdbtpQry Class Implementation

COdbtpQry::COdbtpQry()
{
    m_pCon = NULL;

    m_pColData = NULL;
    m_sMaxCols = 0;
    m_sTotalCols = 0;

    m_pParamData = NULL;
    m_sMaxParams = 0;
    m_sTotalParams = 0;

    m_usCursorType = ODB_CURSOR_FORWARD;
    m_usCursorConcur = ODB_CONCUR_DEFAULT;
    m_bBookmarksOn = FALSE;
    m_bCheckColBinds = FALSE;
    m_bBoundCols = FALSE;
    m_bBoundParams = FALSE;
    m_bGotResult = FALSE;
    m_bGotRow = FALSE;
}

COdbtpQry::~COdbtpQry()
{
    if( m_pColData ) delete[] m_pColData;
    if( m_pParamData ) delete[] m_pParamData;
}

BOOL COdbtpQry::Attach( COdbtpCon* pCon )
{
    if( m_pCon != pCon )
    {
        if( !Allocate( pCon ) ) return( FALSE );
        m_pCon = pCon;
        if( m_pCon->m_ulAttrDriver == ODB_DRIVER_MSSQL )
            SetAttr( SQL_SOPT_SS_HIDDEN_COLUMNS, SQL_HC_ON );
    }
    SetUserData( pCon->GetUserData() );
    return( TRUE );
}

BOOL COdbtpQry::BindColData( USHORT usCol, short sOdbType, long lLen )
{
    COdbColData* pCol = (m_pColData + usCol - 1);
    OdbDATA*     pData;

    pCol->m_bBound = FALSE;

    if( !pCol->Alloc( sOdbType, lLen, m_pCon->m_ulAttrVarDataSize ) )
    {
        m_nLastResult = ODB_ERR_MEMORY;
        return( ProcessLastResult() );
    }
    pData = pCol->m_pData;

    if( !BindCol( usCol, pData->m_nType, pData->m_pVal,
                  pData->m_nMaxLen, &pData->m_nLI ) )
    {
        return( FALSE );
    }
    m_bBoundCols = TRUE;
    pCol->m_bBound = TRUE;
    pCol->m_sOdbType = sOdbType;
    return( TRUE );
}

BOOL COdbtpQry::BindColData( COdbtpSock* pSock )
{
    long   lLen;
    short  sOdbType;
    USHORT usCol;

    do
    {
        if( !pSock->ExtractRequest( &usCol ) ||
            !pSock->ExtractRequest( (PUSHORT)&sOdbType ) ||
            !pSock->ExtractRequest( (PULONG)&lLen ) )
        {
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid BINDCOL request" ) );
        }
        if( usCol == 0 || (short)usCol > m_sTotalCols )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid column number" ) );

        if( !BindColData( usCol, sOdbType, lLen ) ) return( FALSE );
    }
    while( pSock->GetExtractRequestSize() != 0 );

    return( pSock->SendResponse( ODBTP_QUERYOK ) );
}

BOOL COdbtpQry::BindParamData( COdbtpSock* pSock )
{
    BOOL           bDescribed = FALSE;
    long           lLen;
    OdbDATA*       pData;
    COdbParamData* pParam;
    short          sDecDigits;
    short          sOdbType;
    short          sSqlType;
    short          sType;
    ULONG          ulColSize;
    USHORT         usParam;
    USHORT         usType;

    do
    {
        if( !pSock->ExtractRequest( &usParam ) ||
            !pSock->ExtractRequest( &usType ) ||
            !pSock->ExtractRequest( (PUSHORT)&sOdbType ) ||
            !pSock->ExtractRequest( (PULONG)&lLen ) )
        {
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid BINDPARAM request" ) );
        }
        if( usType & ODB_PARAM_DESCRIBED )
        {
            if( !pSock->ExtractRequest( (PUSHORT)&sSqlType ) ||
                !pSock->ExtractRequest( &ulColSize ) ||
                !pSock->ExtractRequest( (PUSHORT)&sDecDigits ) )
            {
                return( pSock->SendResponseText( ODBTP_INVALID, "Invalid BINDPARAM request" ) );
            }
            bDescribed = TRUE;
            usType ^= ODB_PARAM_DESCRIBED;
        }
        if( usParam == 0 || (short)usParam > m_sTotalParams )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid parameter number" ) );

        pParam = (m_pParamData + usParam - 1);
        pParam->m_bBound = FALSE;
        pParam->m_sOdbType = 0;
        pParam->m_usType = ODB_PARAM_NONE;

        if( bDescribed )
        {
            pParam->m_sSqlType = sSqlType;
            pParam->m_ulColSize = ulColSize;
            pParam->m_sDecDigits = sDecDigits;
        }
        else if( !pParam->m_bDescribed )
        {
            return( pSock->SendResponseText( ODBTP_INVALID, "Parameter must be described" ) );
        }
        switch( usType )
        {
            case ODB_PARAM_INPUT:     sType = SQL_PARAM_INPUT; break;
            case ODB_PARAM_INOUT:     sType = SQL_PARAM_INPUT_OUTPUT; break;
            case ODB_PARAM_OUTPUT:
            case ODB_PARAM_RETURNVAL: sType = SQL_PARAM_OUTPUT; break;

            default:
                return( pSock->SendResponseText( ODBTP_INVALID, "Invalid parameter bind type" ) );
        }
        if( !pParam->Alloc( sOdbType, lLen, m_pCon->m_ulAttrVarDataSize ) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
        pData = pParam->m_pData;

        if( !BindParam( usParam, sType, pData->m_nType, pParam->m_sSqlType,
                        pParam->m_ulColSize, pParam->m_sDecDigits,
                        pData->m_pVal, pData->m_nMaxLen, &pData->m_nLI ) )
        {
            return( FALSE );
        }
        m_bBoundParams = TRUE;
        pParam->m_bBound = TRUE;
        pParam->m_sOdbType = sOdbType;
        pParam->m_usType = usType;
    }
    while( pSock->GetExtractRequestSize() != 0 );

    return( pSock->SendResponse( ODBTP_QUERYOK ) );
}

void COdbtpQry::ClearError()
{
    m_nLastResult = SQL_SUCCESS;
}

BOOL COdbtpQry::Detach()
{
    if( !m_pCon ) return( TRUE );
    m_pCon = NULL;

    UnBindColData();
    if( m_pColData ) delete[] m_pColData;
    m_pColData = NULL;
    m_sMaxCols = 0;
    m_sTotalCols = 0;

    UnBindParamData();
    if( m_pParamData ) delete[] m_pParamData;
    m_pParamData = NULL;
    m_sMaxParams = 0;
    m_sTotalParams = 0;

    m_bCheckColBinds = FALSE;
    m_bBoundCols = FALSE;
    m_bBoundParams = FALSE;
    m_bGotResult = FALSE;
    m_bGotRow = FALSE;
    m_usCursorType = ODB_CURSOR_FORWARD;
    m_usCursorConcur = ODB_CONCUR_DEFAULT;
    m_bBookmarksOn = FALSE;

    return( Free() );
}

void COdbtpQry::DisplayDiag( OdbDiagHandler DiagHandler )
{
    if( !m_pCon || m_pCon->m_ulAttrDriver != ODB_DRIVER_MSSQL ||
        (m_nLastResult != SQL_SUCCESS_WITH_INFO &&
         m_nLastResult != SQL_ERROR) )
    {
        OdbQry::DisplayDiag( DiagHandler );
    }
    else
    {
        ::DisplayMSSQLDiag( this, DiagHandler );
    }
}

void COdbtpQry::DisplayDiagW( OdbDiagHandlerW DiagHandler )
{
    if( !m_pCon || m_pCon->m_ulAttrDriver != ODB_DRIVER_MSSQL ||
        (m_nLastResult != SQL_SUCCESS_WITH_INFO &&
         m_nLastResult != SQL_ERROR) )
    {
        OdbQry::DisplayDiagW( DiagHandler );
    }
    else
    {
        ::DisplayMSSQLDiagW( this, DiagHandler );
    }
}

BOOL COdbtpQry::DoRowOperation( COdbtpSock* pSock )
{
    BOOL   bOK;
    USHORT usOp;

    if( !pSock->ExtractRequest( &usOp ) )
        return( pSock->SendResponseText( ODBTP_INVALID, "Invalid ROWOP request" ) );

    if( usOp == ODB_ROW_GETSTATUS )
    {
        ULONG ulStatus;

        if( RowSuccess() || RowInfo() )
            ulStatus = ODB_ROWSTAT_SUCCESS;
        else if( RowUpdated() )
            ulStatus = ODB_ROWSTAT_UPDATED;
        else if( RowDeleted() )
            ulStatus = ODB_ROWSTAT_DELETED;
        else if( RowAdded() )
            ulStatus = ODB_ROWSTAT_ADDED;
        else if( RowNone() )
            ulStatus = ODB_ROWSTAT_NOROW;
        else if( RowError() )
            ulStatus = ODB_ROWSTAT_ERROR;
        else
            ulStatus = ODB_ROWSTAT_UNKNOWN;
        return( pSock->SendResponse( ODBTP_ROWSTATUS, ulStatus ) );
    }
    switch( usOp )
    {
        case ODB_ROW_REFRESH: bOK = RefreshRow(); break;
        case ODB_ROW_UPDATE:  bOK = UpdateRow(); break;
        case ODB_ROW_DELETE:  bOK = DeleteRow(); break;
        case ODB_ROW_ADD:     bOK = AddRow(); break;
        case ODB_ROW_LOCK:    bOK = LockRow(); break;
        case ODB_ROW_UNLOCK:  bOK = UnlockRow(); break;

        case ODB_ROW_BOOKMARK:
            if( !m_bBookmarksOn )
                return( pSock->SendResponseText( ODBTP_INVALID, "Bookmarks not enabled" ) );
            m_RowBookmark = m_Bookmark;
            bOK = SetBookmark( m_RowBookmark );
            break;

        default:
            return( pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unknown row operation" ) );
    }
    if( !bOK ) return( FALSE );
    return( pSock->SendResponse( ODBTP_QUERYOK ) );
}

BOOL COdbtpQry::GetColInfo()
{
    if( m_pCon->m_ulAttrUnicodeSQL ) return( GetColInfoW() );

    int          nComputed = 0;
    COdbColData* pCol;
    short        sCol;

    if( m_bBookmarksOn && !SetBookmark( NULL ) ) return( FALSE );

    if( m_sTotalCols > m_sMaxCols )
    {
        if( m_pColData ) delete[] m_pColData;
        m_sMaxCols = m_sTotalCols + (m_sTotalCols % 16) + 16;
        if( !(m_pColData = new COdbColData[m_sMaxCols]) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
    }
    for( sCol = 1; sCol <= m_sTotalCols; sCol++ )
    {
        pCol = (m_pColData + sCol - 1);
        if( !OdbQry::GetColInfo( sCol, pCol->m_szName, sizeof(pCol->m_szName),
                                 &pCol->m_sSqlType, &pCol->m_ulColSize,
                                 &pCol->m_sDecDigits, &pCol->m_sNullable ) )
        {
            return( FALSE );
        }
        if( pCol->m_szName[0] == 0 )
        {
            if( nComputed == 0 )
                strcpy( pCol->m_szName, "computed" );
            else
                sprintf( pCol->m_szName, "computed%i", nComputed );
            nComputed++;
        }
        pCol->m_bBound = FALSE;
        pCol->m_sNameLen = strlen(pCol->m_szName);
        pCol->m_sDefaultOdbType = GetOdbDataType( pCol->m_sSqlType );
        pCol->m_sOdbType = pCol->m_sDefaultOdbType;
    }
    m_bCheckColBinds = TRUE;
    return( TRUE );
}

BOOL COdbtpQry::GetColInfoW()
{
    int          nComputed = 0;
    COdbColData* pCol;
    short        sCol;

    if( m_bBookmarksOn && !SetBookmark( NULL ) ) return( FALSE );

    if( m_sTotalCols > m_sMaxCols )
    {
        if( m_pColData ) delete[] m_pColData;
        m_sMaxCols = m_sTotalCols + (m_sTotalCols % 16) + 16;
        if( !(m_pColData = new COdbColData[m_sMaxCols]) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResultW() );
        }
    }
    for( sCol = 1; sCol <= m_sTotalCols; sCol++ )
    {
        pCol = (m_pColData + sCol - 1);
        if( !OdbQry::GetColInfoW( sCol, pCol->m_pszNameW, sizeof(pCol->m_szName) / sizeof(wchar_t),
                                  &pCol->m_sSqlType, &pCol->m_ulColSize,
                                  &pCol->m_sDecDigits, &pCol->m_sNullable ) )
        {
            return( FALSE );
        }
        if( pCol->m_pszNameW[0] == 0 )
        {
            if( nComputed == 0 )
                wcscpy( pCol->m_pszNameW, L"computed" );
            else
                swprintf( pCol->m_pszNameW, L"computed%i", nComputed );
            nComputed++;
        }
        pCol->m_bBound = FALSE;
        pCol->m_sNameLen = UTF8Len(pCol->m_pszNameW);
        pCol->m_sDefaultOdbType = GetOdbDataType( pCol->m_sSqlType );
        pCol->m_sOdbType = pCol->m_sDefaultOdbType;
    }
    m_bCheckColBinds = TRUE;
    return( TRUE );
}

short COdbtpQry::GetOdbDataType( short sSqlDataType )
{
    switch( sSqlDataType )
    {
        case SQL_BIT:            return( ODB_BIT );
        case SQL_TINYINT:        return( ODB_UTINYINT );
        case SQL_SMALLINT:       return( ODB_SMALLINT );
        case SQL_INTEGER:        return( ODB_INT );
        case SQL_BIGINT:         return( ODB_BIGINT );
        case SQL_REAL:           return( ODB_REAL );
        case SQL_FLOAT:
        case SQL_DOUBLE:         return( ODB_DOUBLE );
        case SQL_DATE:
        case SQL_TIME:
        case SQL_TIMESTAMP:
        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP: return( ODB_DATETIME );
        case SQL_GUID:           return( ODB_GUID );
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:  return( ODB_BINARY );
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR: return( ODB_WCHAR );
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            return( m_pCon->m_ulAttrMapCharToWChar ? ODB_WCHAR : ODB_CHAR );
    }
    return( sSqlDataType ? ODB_CHAR : 0 );
}

short COdbtpQry::GetOdbParamType( short sSqlParamType )
{
    switch( sSqlParamType )
    {
        case SQL_PARAM_INPUT:        return( ODB_PARAM_INPUT );
        case SQL_PARAM_INPUT_OUTPUT: return( ODB_PARAM_INOUT );
        case SQL_PARAM_OUTPUT:       return( ODB_PARAM_OUTPUT );
        case SQL_RETURN_VALUE:       return( ODB_PARAM_RETURNVAL );
        case SQL_RESULT_COL:         return( ODB_PARAM_RESULTCOL );
    }
    return( ODB_PARAM_NONE );
}

BOOL COdbtpQry::GetParamData( COdbtpSock* pSock )
{
    LONG           lLI;
    OdbDATA*       pData;
    COdbParamData* pParam;
    USHORT         usParam;

    do
    {
        if( !pSock->ExtractRequest( &usParam ) )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid GETPARAM request" ) );

        if( usParam == 0 || (short)usParam > m_sTotalParams )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid parameter number" ) );

        pParam = (m_pParamData + usParam - 1);

        if( !pParam->m_bBound )
            return( pSock->SendResponseText( ODBTP_INVALID, "Unbound parameter" ) );

        pData = pParam->m_pData;

        if( !pSock->SendResponse( ODBTP_PARAMDATA, usParam, FALSE ) )
            return( FALSE );

        if( pData->m_nLI > 0 )
        {
            BOOL bTruncated = FALSE;
            LONG lActualLen;

            switch( pData->m_nType )
            {
                case ODB_CHAR:
                    if( pData->m_nLI >= pData->m_nMaxLen )
                    {
                        bTruncated = TRUE;
                        lActualLen = pData->m_nLI;
                        ((OdbCHAR*)pData)->SyncLen();
                    }
                    if( m_pCon->m_ulAttrRightTrimText )
                    {
                        char* psz = (char*)pData->m_pVal;
                        int   n = pData->m_nLI;

                        for( n--; n >= 0 && psz[n] == ' '; n-- );
                        n++;

                        if( pData->m_nLI != n )
                        {
                            bTruncated = FALSE;
                            psz[n] = 0;
                            ((OdbCHAR*)pData)->SyncLen();
                        }
                    }
                    break;

                case ODB_WCHAR:
                    if( pData->m_nLI >= pData->m_nMaxLen )
                    {
                        bTruncated = TRUE;
                        lActualLen = pData->m_nLI / sizeof(wchar_t);
                        ((OdbWCHAR*)pData)->SyncLen();
                    }
                    if( m_pCon->m_ulAttrRightTrimText )
                    {
                        wchar_t* psz = (wchar_t*)pData->m_pVal;
                        int      n = pData->m_nLI / sizeof(wchar_t);

                        for( n--; n >= 0 && psz[n] == L' '; n-- );
                        n++;

                        if( pData->m_nLI != (int)(n * sizeof(wchar_t)) )
                        {
                            bTruncated = FALSE;
                            psz[n] = 0;
                            ((OdbWCHAR*)pData)->SyncLen();
                        }
                    }
                    break;

                case ODB_BINARY:
                    if( pData->m_nLI > pData->m_nMaxLen )
                    {
                        bTruncated = TRUE;
                        lActualLen = pData->m_nLI;
                        ((OdbBINARY*)pData)->Len( pData->m_nMaxLen );
                    }
                    break;
            }
            if( bTruncated )
            {
                if( !pSock->SendResponse( ODBTP_PARAMDATA, (ULONG)0xE0000001, FALSE ) )
                    return( FALSE );
                if( !pSock->SendResponse( ODBTP_PARAMDATA, (ULONG)lActualLen, FALSE ) )
                    return( FALSE );
            }
        }
        else if( pData->m_nLI == 0 )
        {
            switch( pData->m_nType )
            {
                case ODB_CHAR:
                case ODB_WCHAR:
                case ODB_BINARY: break;

                default: pData->m_nLI = -1;
            }
        }
        if( pData->m_nLI > 0 && pData->m_nType == ODB_WCHAR )
            lLI = ((OdbWCHAR*)pData)->UTF8Len();
        else
            lLI = pData->m_nLI;
        if( !pSock->SendResponse( ODBTP_PARAMDATA, (ULONG)lLI, FALSE ) )
            return( FALSE );

        if( pData->m_nLI <= 0 ) continue;

        switch( pData->m_nType )
        {
            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, *((PUSHORT)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, *((PULONG)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, *((unsigned __int64*)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_REAL:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, *((float*)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_DOUBLE:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, *((double*)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_DATETIME:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, (SQL_TIMESTAMP_STRUCT*)pData->m_pVal, FALSE ) )
                    return( FALSE );
                break;

            case ODB_GUID:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, (SQLGUID*)pData->m_pVal, FALSE ) )
                    return( FALSE );
                break;

            case ODB_WCHAR:
                if( !SendWCharData( pSock, ODBTP_PARAMDATA, (OdbWCHAR*)pData ) )
                    return( FALSE );
                break;

            default:
                if( !pSock->SendResponse( ODBTP_PARAMDATA, pData->m_pVal, pData->m_nLI, FALSE ) )
                    return( FALSE );
        }
    }
    while( pSock->GetExtractRequestSize() != 0 );

    return( pSock->SendResponse( ODBTP_PARAMDATA ) );
}

BOOL COdbtpQry::GetParamInfo()
{
    COdbParamData* pParam;
    short          sParam;

    if( m_sTotalParams > m_sMaxParams )
    {
        if( m_pParamData ) delete[] m_pParamData;
        m_sMaxParams = m_sTotalParams + (m_sTotalParams % 8) + 8;
        if( !(m_pParamData = new COdbParamData[m_sMaxParams]) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
    }
    for( sParam = 1; sParam <= m_sTotalParams; sParam++ )
    {
        pParam = (m_pParamData + sParam - 1);
        if( !m_pCon->m_ulAttrDescribeParams ||
            !OdbQry::GetParamInfo( sParam,
                                   &pParam->m_sSqlType, &pParam->m_ulColSize,
                                   &pParam->m_sDecDigits, &pParam->m_sNullable ) )
        {
            pParam->m_bDescribed = FALSE;
            pParam->m_sSqlType = 0;
            pParam->m_ulColSize = 0;
            pParam->m_sDecDigits = 0;
            pParam->m_sNullable = 0;
        }
        else
        {
            pParam->m_bDescribed = TRUE;
        }
        pParam->m_bBound = FALSE;
        pParam->m_usType = ODB_PARAM_NONE;
        pParam->m_sDefaultOdbType = GetOdbDataType( pParam->m_sSqlType );
        pParam->m_sOdbType = 0;
    }
    m_nLastResult = SQL_SUCCESS;
    return( ProcessLastResult() );
}

BOOL COdbtpQry::GetResultInfo()
{
    m_bCheckColBinds = FALSE;
    m_sTotalCols = 0;
    m_lAffectedRows = 0;

    if( Info() )
    {
        m_bGotResult = TRUE;
        return( TRUE );
    }
    if( !GetNumResultCols( &m_sTotalCols ) )
    {
        m_sTotalCols = 0;
        m_nLastResult = SQL_SUCCESS;
    }
    if( m_sTotalCols == 0 )
    {
        if( !GetAffectedRowCount( &m_lAffectedRows ) )
        {
            m_lAffectedRows = -1;
            m_nLastResult = SQL_SUCCESS;
        }
    }
    else
    {
        if( !GetColInfo() ) return( FALSE );
        m_ulFetchRowCount = m_pCon->m_ulAttrFetchRowCount;
        if( m_ulFetchRowCount == 0 ) m_ulFetchRowCount = 0xFFFFFFFF;
    }
    m_bGotResult = TRUE;
    return( TRUE );
}

BOOL COdbtpQry::GetRow( USHORT usFetchType, long lFetchParam )
{
    m_bGotRow = FALSE;

    if( m_sTotalCols == 0 ) return( TRUE );

    if( m_bCheckColBinds )
    {
        COdbColData* pCol;
        short        sCol;

        if( m_bBookmarksOn )
        {
            if( !BindCol( (SQLUSMALLINT)0, m_Bookmark ) ) return( FALSE );
            m_bBoundCols = TRUE;
        }
        for( sCol = 1; sCol <= m_sTotalCols; sCol++ )
        {
            pCol = (m_pColData + sCol - 1);
            if( pCol->m_bBound ) continue;
            if( !BindColData( sCol, pCol->m_sDefaultOdbType ) ) return( FALSE );
        }
        m_bCheckColBinds = FALSE;
    }
    if( m_usCursorType == ODB_CURSOR_FORWARD )
    {
        if( !Fetch() ) return( FALSE );
    }
    else
    {
        BOOL bOK;

        switch( usFetchType )
        {
            case ODB_FETCH_FIRST:
                bOK = FetchFirst();
                break;

            case ODB_FETCH_LAST:
                bOK = FetchLast();
                break;

            case ODB_FETCH_PREV:
                bOK = FetchPrev();
                break;

            case ODB_FETCH_ABS:
                bOK = FetchAbs( lFetchParam );
                break;

            case ODB_FETCH_REL:
                bOK = FetchRel( lFetchParam );
                break;

            case ODB_FETCH_BOOKMARK:
                bOK = FetchViaBookmark( lFetchParam );
                break;

            default:
                bOK = FetchNext();
                break;
        }
        if( !bOK ) return( FALSE );
    }
    if( NoData() )
    {
        if( m_usCursorType == ODB_CURSOR_FORWARD && !UnBindColData() )
            return( FALSE );
    }
    else
    {
        m_bGotRow = TRUE;
    }
    return( TRUE );
}

BOOL COdbtpQry::PrepareProc( COdbtpSock* pSock, PSTR pszCatalog,
                             PSTR pszSchema, PSTR pszProcedure )
{
    BOOL           bNotJet = m_pCon->m_ulAttrDriver != ODB_DRIVER_JET;
    long           lLen;
    OdbDATA*       pData;
    COdbParamData* pParam;
    COdbProc*      pProc;
    SProcParam*    pProcParam;
    char*          psz;
    char*          pszCall;
    short          sParam;
    short          sTotalBindable = 0;
    short          sType;

    if( m_bGotResult )
    {
        if( !UnBindColData() ) return( FALSE );
        DumpResults();
        m_bGotResult = FALSE;
        m_bGotRow = FALSE;
    }
    if( !UnBindParamData() ) return( FALSE );

    if( !(pProc = m_pCon->GetProc( pszCatalog, pszSchema, pszProcedure )) )
    {
        OdbCHAR     odbCatalog;
        OdbCHAR     odbSchema;
        OdbCHAR     odbProcedure;
        OdbCHAR     odbName;
        OdbSMALLINT odbType;
        OdbSMALLINT odbSqlType;
        OdbCHAR     odbSqlTypeName;
        OdbINT      odbColSize;
        OdbSMALLINT odbDecDigits;
        OdbSMALLINT odbNullable;

        char szCatalog[256];
        char szSchema[256];
        char szProcedure[256];

        m_nLastResult = ::SQLProcedureColumns( m_hODBC,
            (SQLCHAR*)pszCatalog, SQL_NTS,
            (SQLCHAR*)pszSchema, SQL_NTS,
            (SQLCHAR*)pszProcedure, SQL_NTS,
            NULL, SQL_NTS );

        if( !ProcessLastResult() ) return( FALSE );

        if( !(pProc = new COdbProc) ||
            !pProc->Init( pszCatalog, pszSchema, pszProcedure ) )
        {
            if( pProc ) delete pProc;
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
        if( !BindCol( 1, odbCatalog ) ||
            !BindCol( 2, odbSchema ) ||
            !BindCol( 3, odbProcedure ) ||
            !BindCol( 4, odbName ) ||
            !BindCol( 5, odbType ) ||
            !BindCol( 6, odbSqlType ) ||
            !BindCol( 7, odbSqlTypeName ) ||
            !BindCol( 8, odbColSize ) ||
            !BindCol( 10, odbDecDigits ) ||
            !BindCol( 12, odbNullable ) )
        {
            delete pProc;
            return( FALSE );
        }
        szProcedure[0] = 0;

        while( Fetch() && !NoData() )
        {
            if( szProcedure[0] )
            {
                if( odbCatalog != szCatalog ||
                    odbSchema != szSchema ||
                    odbProcedure != szProcedure )
                {
                    delete pProc;
                    return( pSock->SendResponseText( ODBTP_INVALID, "Ambiguous procedure name" ) );
                }
            }
            else
            {
                strcpy( szCatalog, odbCatalog );
                strcpy( szSchema, odbSchema );
                strcpy( szProcedure, odbProcedure );
            }
            if( !(pProcParam = pProc->NewParam()) )
            {
                delete pProc;
                m_nLastResult = ODB_ERR_MEMORY;
                return( ProcessLastResult() );
            }
            if( odbName.Null() ) odbName = "";
            if( odbSqlTypeName.Null() ) odbSqlTypeName = "";

            strncpy( pProcParam->m_szName, odbName, 128 );
            pProcParam->m_szName[128] = 0;
            pProcParam->m_sNameLen = strlen( pProcParam->m_szName );
            pProcParam->m_usType = GetOdbParamType( odbType );
            pProcParam->m_sSqlType = odbSqlType;
            strncpy( pProcParam->m_szType, odbSqlTypeName, 128 );
            pProcParam->m_szType[128] = 0;
            pProcParam->m_sTypeLen = strlen( pProcParam->m_szType );
            pProcParam->m_ulColSize = (SQLINTEGER)odbColSize;
            pProcParam->m_sDecDigits = odbDecDigits;
            pProcParam->m_sNullable = odbNullable;
        }
        if( Error() )
        {
            delete pProc;
            return( FALSE );
        }
        UnBindCols();
        DumpResults();

        if( !m_pCon->AddProc( pProc ) )
        {
            delete pProc;
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
    }
    if( (m_sTotalParams = (short)pProc->m_nTotalParams) )
    {
        if( m_sTotalParams > m_sMaxParams )
        {
            if( m_pParamData ) delete[] m_pParamData;
            m_sMaxParams = m_sTotalParams + (m_sTotalParams % 8) + 8;
            if( !(m_pParamData = new COdbParamData[m_sMaxParams]) )
            {
                if( !pProc->m_bCached ) delete pProc;
                m_nLastResult = ODB_ERR_MEMORY;
                return( ProcessLastResult() );
            }
        }
        for( sParam = 0; sParam < m_sTotalParams; sParam++ )
        {
            pParam = (m_pParamData + sParam);
            pProcParam = (pProc->m_pParams + sParam);

            strcpy( pParam->m_szName, pProcParam->m_szName );
            pParam->m_sNameLen = pProcParam->m_sNameLen;
            pParam->m_usType = pProcParam->m_usType;
            pParam->m_sSqlType = pProcParam->m_sSqlType;
            strcpy( pParam->m_szType, pProcParam->m_szType );
            pParam->m_sTypeLen = pProcParam->m_sTypeLen;
            pParam->m_ulColSize = pProcParam->m_ulColSize;
            pParam->m_sDecDigits = pProcParam->m_sDecDigits;
            pParam->m_sNullable = pProcParam->m_sNullable;

            pParam->m_bDescribed = pParam->m_sSqlType != 0;
            pParam->m_bBound = FALSE;
            pParam->m_sDefaultOdbType = GetOdbDataType( pParam->m_sSqlType );
            pParam->m_sOdbType = 0;

            if( (pParam->m_usType & ODB_PARAM_INOUT) ) sTotalBindable++;
        }
    }
    lLen = 0;
    if( m_sTotalParams ) lLen += 6 + (m_sTotalParams * 2);
    if( pszCatalog ) lLen += 3 + strlen( pszCatalog );
    if( pszSchema ) lLen += 3 + strlen( pszSchema );
    lLen += 3 + strlen( pszProcedure );
    lLen += (lLen % 16) + 16;

    if( !(pszCall = (char*)malloc( lLen * sizeof(char) )) )
    {
        if( !pProc->m_bCached ) delete pProc;
        m_nLastResult = ODB_ERR_MEMORY;
        return( ProcessLastResult() );
    }
    psz = pszCall;
    if( bNotJet ) *(psz++) = '{';

    if( sTotalBindable && m_pParamData->m_usType == ODB_PARAM_RETURNVAL )
    {
        strcpy( psz, "? = call " );
        psz += 9;
        sParam = 1;
    }
    else
    {
        strcpy( psz, bNotJet ? "call " : "EXEC " );
        psz += 5;
        sParam = 0;
    }
    if( pszCatalog )
    {
        sprintf( psz, "[%s].", pszCatalog );
        psz += strlen( psz );
    }
    if( pszSchema )
    {
        sprintf( psz, "[%s].", pszSchema );
        psz += strlen( psz );
    }
    else if( pszCatalog )
    {
        *(psz++) = '.';
    }
    sprintf( psz, "[%s]", pszProcedure );
    psz += strlen( psz );

    for( ; sParam < sTotalBindable; sParam++ )
    {
        if( *(psz-1) != '?' )
            *(psz++) = bNotJet ? '(' : ' ';
        else
            *(psz++) = ',';
        *(psz++) = '?';
    }
    if( bNotJet )
    {
        if( *(psz-1) == '?' ) *(psz++) = ')';
        *(psz++) = '}';
    }
    *psz = 0;

    if( !Prepare( pszCall ) )
    {
        if( !pProc->m_bCached ) delete pProc;
        free( pszCall );
        return( FALSE );
    }
    free( pszCall );
    m_bPreparedProc = TRUE;

    for( sParam = 1; sParam <= m_sTotalParams; sParam++ )
    {
        pParam = (m_pParamData + sParam - 1);

        switch( pParam->m_usType )
        {
            case ODB_PARAM_INPUT:     sType = SQL_PARAM_INPUT; break;
            case ODB_PARAM_INOUT:     sType = SQL_PARAM_INPUT_OUTPUT; break;
            case ODB_PARAM_OUTPUT:
            case ODB_PARAM_RETURNVAL: sType = SQL_PARAM_OUTPUT; break;

            default:
                sType = 0;
        }
        if( !pParam->m_bDescribed || sType == 0 ) continue;

        if( !pParam->Alloc( pParam->m_sDefaultOdbType,
                            0, m_pCon->m_ulAttrVarDataSize ) )
        {
            if( !pProc->m_bCached ) delete pProc;
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
        pData = pParam->m_pData;

        if( !BindParam( (USHORT)sParam, sType,
                        pData->m_nType, pParam->m_sSqlType,
                        pParam->m_ulColSize, pParam->m_sDecDigits,
                        pData->m_pVal, pData->m_nMaxLen, &pData->m_nLI ) )
        {
            if( !pProc->m_bCached ) delete pProc;
            return( FALSE );
        }
        m_bBoundParams = TRUE;
        pParam->m_bBound = TRUE;
        pParam->m_sOdbType = pParam->m_sDefaultOdbType;
        pData->m_nLI = SQL_DEFAULT_PARAM;
    }
    if( !pProc->m_bCached ) delete pProc;

    return( TRUE );
}

BOOL COdbtpQry::PrepareProcW( COdbtpSock* pSock, wchar_t* pszCatalog,
                              wchar_t* pszSchema, wchar_t* pszProcedure )
{
    BOOL           bNotJet = m_pCon->m_ulAttrDriver != ODB_DRIVER_JET;
    long           lLen;
    OdbDATA*       pData;
    COdbParamData* pParam;
    COdbProc*      pProc;
    SProcParam*    pProcParam;
    wchar_t*       psz;
    wchar_t*       pszCall;
    short          sParam;
    short          sTotalBindable = 0;
    short          sType;

    if( m_bGotResult )
    {
        if( !UnBindColData() ) return( FALSE );
        DumpResults();
        m_bGotResult = FALSE;
        m_bGotRow = FALSE;
    }
    if( !UnBindParamData() ) return( FALSE );

    if( !(pProc = m_pCon->GetProcW( pszCatalog, pszSchema, pszProcedure )) )
    {
        OdbWCHAR    odbCatalog;
        OdbWCHAR    odbSchema;
        OdbWCHAR    odbProcedure;
        OdbWCHAR    odbName;
        OdbSMALLINT odbType;
        OdbSMALLINT odbSqlType;
        OdbWCHAR    odbSqlTypeName;
        OdbINT      odbColSize;
        OdbSMALLINT odbDecDigits;
        OdbSMALLINT odbNullable;

        wchar_t szCatalog[256];
        wchar_t szSchema[256];
        wchar_t szProcedure[256];

        m_nLastResult = ::SQLProcedureColumnsW( m_hODBC,
            (SQLWCHAR*)pszCatalog, SQL_NTS,
            (SQLWCHAR*)pszSchema, SQL_NTS,
            (SQLWCHAR*)pszProcedure, SQL_NTS,
            NULL, SQL_NTS );

        if( !ProcessLastResult() ) return( FALSE );

        if( !(pProc = new COdbProc) ||
            !pProc->InitW( pszCatalog, pszSchema, pszProcedure ) )
        {
            if( pProc ) delete pProc;
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
        if( !BindCol( 1, odbCatalog ) ||
            !BindCol( 2, odbSchema ) ||
            !BindCol( 3, odbProcedure ) ||
            !BindCol( 4, odbName ) ||
            !BindCol( 5, odbType ) ||
            !BindCol( 6, odbSqlType ) ||
            !BindCol( 7, odbSqlTypeName ) ||
            !BindCol( 8, odbColSize ) ||
            !BindCol( 10, odbDecDigits ) ||
            !BindCol( 12, odbNullable ) )
        {
            delete pProc;
            return( FALSE );
        }
        szProcedure[0] = 0;

        while( Fetch() && !NoData() )
        {
            if( szProcedure[0] )
            {
                if( odbCatalog != szCatalog ||
                    odbSchema != szSchema ||
                    odbProcedure != szProcedure )
                {
                    delete pProc;
                    return( pSock->SendResponseText( ODBTP_INVALID, "Ambiguous procedure name" ) );
                }
            }
            else
            {
                wcscpy( szCatalog, odbCatalog );
                wcscpy( szSchema, odbSchema );
                wcscpy( szProcedure, odbProcedure );
            }
            if( !(pProcParam = pProc->NewParam()) )
            {
                delete pProc;
                m_nLastResult = ODB_ERR_MEMORY;
                return( ProcessLastResult() );
            }
            if( odbName.Null() ) odbName = L"";
            if( odbSqlTypeName.Null() ) odbSqlTypeName = L"";

            wcsncpy( pProcParam->m_pszNameW, odbName, 64 );
            pProcParam->m_pszNameW[64] = 0;
            pProcParam->m_sNameLen = UTF8Len( pProcParam->m_pszNameW );
            pProcParam->m_usType = GetOdbParamType( odbType );
            pProcParam->m_sSqlType = odbSqlType;
            wcsncpy( pProcParam->m_pszTypeW, odbSqlTypeName, 64 );
            pProcParam->m_pszTypeW[64] = 0;
            pProcParam->m_sTypeLen = UTF8Len( pProcParam->m_pszTypeW );
            pProcParam->m_ulColSize = (SQLINTEGER)odbColSize;
            pProcParam->m_sDecDigits = odbDecDigits;
            pProcParam->m_sNullable = odbNullable;
        }
        if( Error() )
        {
            delete pProc;
            return( FALSE );
        }
        UnBindCols();
        DumpResults();

        if( !m_pCon->AddProc( pProc ) )
        {
            delete pProc;
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
    }
    if( (m_sTotalParams = (short)pProc->m_nTotalParams) )
    {
        if( m_sTotalParams > m_sMaxParams )
        {
            if( m_pParamData ) delete[] m_pParamData;
            m_sMaxParams = m_sTotalParams + (m_sTotalParams % 8) + 8;
            if( !(m_pParamData = new COdbParamData[m_sMaxParams]) )
            {
                if( !pProc->m_bCached ) delete pProc;
                m_nLastResult = ODB_ERR_MEMORY;
                return( ProcessLastResult() );
            }
        }
        for( sParam = 0; sParam < m_sTotalParams; sParam++ )
        {
            pParam = (m_pParamData + sParam);
            pProcParam = (pProc->m_pParams + sParam);

            wcscpy( pParam->m_pszNameW, pProcParam->m_pszNameW );
            pParam->m_sNameLen = pProcParam->m_sNameLen;
            pParam->m_usType = pProcParam->m_usType;
            pParam->m_sSqlType = pProcParam->m_sSqlType;
            wcscpy( pParam->m_pszTypeW, pProcParam->m_pszTypeW );
            pParam->m_sTypeLen = pProcParam->m_sTypeLen;
            pParam->m_ulColSize = pProcParam->m_ulColSize;
            pParam->m_sDecDigits = pProcParam->m_sDecDigits;
            pParam->m_sNullable = pProcParam->m_sNullable;

            pParam->m_bDescribed = pParam->m_sSqlType != 0;
            pParam->m_bBound = FALSE;
            pParam->m_sDefaultOdbType = GetOdbDataType( pParam->m_sSqlType );
            pParam->m_sOdbType = 0;

            if( (pParam->m_usType & ODB_PARAM_INOUT) ) sTotalBindable++;
        }
    }
    lLen = 0;
    if( m_sTotalParams ) lLen += 6 + (m_sTotalParams * 2);
    if( pszCatalog ) lLen += 3 + wcslen( pszCatalog );
    if( pszSchema ) lLen += 3 + wcslen( pszSchema );
    lLen += 3 + wcslen( pszProcedure );
    lLen += (lLen % 16) + 16;

    if( !(pszCall = (wchar_t*)malloc( lLen * sizeof(wchar_t) )) )
    {
        if( !pProc->m_bCached ) delete pProc;
        m_nLastResult = ODB_ERR_MEMORY;
        return( ProcessLastResultW() );
    }
    psz = pszCall;
    if( bNotJet ) *(psz++) = L'{';

    if( sTotalBindable && m_pParamData->m_usType == ODB_PARAM_RETURNVAL )
    {
        wcscpy( psz, L"? = call " );
        psz += 9;
        sParam = 1;
    }
    else
    {
        wcscpy( psz, bNotJet ? L"call " : L"EXEC " );
        psz += 5;
        sParam = 0;
    }
    if( pszCatalog )
    {
        swprintf( psz, L"[%s].", pszCatalog );
        psz += wcslen( psz );
    }
    if( pszSchema )
    {
        swprintf( psz, L"[%s].", pszSchema );
        psz += wcslen( psz );
    }
    else if( pszCatalog )
    {
        *(psz++) = L'.';
    }
    swprintf( psz, L"[%s]", pszProcedure );
    psz += wcslen( psz );

    for( ; sParam < sTotalBindable; sParam++ )
    {
        if( *(psz-1) != L'?' )
            *(psz++) = bNotJet ? L'(' : L' ';
        else
            *(psz++) = L',';
        *(psz++) = L'?';
    }
    if( bNotJet )
    {
        if( *(psz-1) == L'?' ) *(psz++) = L')';
        *(psz++) = L'}';
    }
    *psz = 0;

    if( !PrepareW( pszCall ) )
    {
        if( !pProc->m_bCached ) delete pProc;
        free( pszCall );
        return( FALSE );
    }
    free( pszCall );
    m_bPreparedProc = TRUE;

    for( sParam = 1; sParam <= m_sTotalParams; sParam++ )
    {
        pParam = (m_pParamData + sParam - 1);

        switch( pParam->m_usType )
        {
            case ODB_PARAM_INPUT:     sType = SQL_PARAM_INPUT; break;
            case ODB_PARAM_INOUT:     sType = SQL_PARAM_INPUT_OUTPUT; break;
            case ODB_PARAM_OUTPUT:
            case ODB_PARAM_RETURNVAL: sType = SQL_PARAM_OUTPUT; break;

            default:
                sType = 0;
        }
        if( !pParam->m_bDescribed || sType == 0 ) continue;

        if( !pParam->Alloc( pParam->m_sDefaultOdbType,
                            0, m_pCon->m_ulAttrVarDataSize ) )
        {
            if( !pProc->m_bCached ) delete pProc;
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResultW() );
        }
        pData = pParam->m_pData;

        if( !BindParam( (USHORT)sParam, sType,
                        pData->m_nType, pParam->m_sSqlType,
                        pParam->m_ulColSize, pParam->m_sDecDigits,
                        pData->m_pVal, pData->m_nMaxLen, &pData->m_nLI ) )
        {
            if( !pProc->m_bCached ) delete pProc;
            return( FALSE );
        }
        m_bBoundParams = TRUE;
        pParam->m_bBound = TRUE;
        pParam->m_sOdbType = pParam->m_sDefaultOdbType;
        pData->m_nLI = SQL_DEFAULT_PARAM;
    }
    if( !pProc->m_bCached ) delete pProc;

    return( TRUE );
}

BOOL COdbtpQry::SendAffectedRowCount( COdbtpSock* pSock )
{
    if( !GetAffectedRowCount( &m_lAffectedRows ) ) return( FALSE );
    return( pSock->SendResponse( ODBTP_ROWCOUNT, (ULONG)m_lAffectedRows ) );
}

BOOL COdbtpQry::SendColInfo( COdbtpSock* pSock )
{
    if( m_pCon->m_ulAttrUnicodeSQL ) return( SendColInfoW( pSock ) );

    short        n;
    COdbColData* pCol;
    USHORT       usCode;

    usCode = m_pCon->m_ulAttrFullColInfo ? ODBTP_COLINFOEX : ODBTP_COLINFO;

    if( !pSock->SendResponse( usCode, (USHORT)m_sTotalCols, FALSE ) )
        return( FALSE );

    for( n = 0; n < m_sTotalCols; n++ )
    {
        pCol = (m_pColData + n);
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sNameLen, FALSE ) )
            return( FALSE );
        if( pCol->m_sNameLen != 0 &&
            !pSock->SendResponse( usCode, pCol->m_szName, pCol->m_sNameLen, FALSE ) )
        {
            return( FALSE );
        }
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sSqlType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sDefaultOdbType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sOdbType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, pCol->m_ulColSize, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sDecDigits, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sNullable, FALSE ) )
            return( FALSE );
        if( m_pCon->m_ulAttrFullColInfo && !SendColInfoEx( pSock, n + 1 ) )
            return( FALSE );
    }
    return( pSock->SendResponse( usCode ) );
}

BOOL COdbtpQry::SendColInfoW( COdbtpSock* pSock )
{
    short        n;
    COdbColData* pCol;
    USHORT       usCode;

    usCode = m_pCon->m_ulAttrFullColInfo ? ODBTP_COLINFOEX : ODBTP_COLINFO;

    if( !pSock->SendResponse( usCode, (USHORT)m_sTotalCols, FALSE ) )
        return( FALSE );

    for( n = 0; n < m_sTotalCols; n++ )
    {
        pCol = (m_pColData + n);
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sNameLen, FALSE ) )
            return( FALSE );
        if( pCol->m_sNameLen != 0 &&
            !pSock->SendResponseW( usCode, pCol->m_pszNameW, FALSE ) )
        {
            return( FALSE );
        }
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sSqlType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sDefaultOdbType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sOdbType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, pCol->m_ulColSize, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sDecDigits, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pCol->m_sNullable, FALSE ) )
            return( FALSE );
        if( m_pCon->m_ulAttrFullColInfo && !SendColInfoExW( pSock, n + 1 ) )
            return( FALSE );
    }
    return( pSock->SendResponse( usCode ) );
}

BOOL COdbtpQry::SendColInfoEx( COdbtpSock* pSock, short sCol )
{
    char   szVal[1024];
    ULONG  ulFlags = 0;
    USHORT usLen;

    if( GetColAttr( sCol, SQL_DESC_TYPE_NAME, szVal, 1023 ) )
        usLen = strlen( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponse( ODBTP_COLINFOEX, szVal, usLen, FALSE ) )
        return( FALSE );

    if( GetColAttr( sCol, SQL_DESC_TABLE_NAME, szVal, 1023 ) )
        usLen = strlen( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponse( ODBTP_COLINFOEX, szVal, usLen, FALSE ) )
        return( FALSE );

    if( GetColAttr( sCol, SQL_DESC_SCHEMA_NAME, szVal, 1023 ) )
        usLen = strlen( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponse( ODBTP_COLINFOEX, szVal, usLen, FALSE ) )
        return( FALSE );

    if( GetColAttr( sCol, SQL_DESC_CATALOG_NAME, szVal, 1023 ) )
        usLen = strlen( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponse( ODBTP_COLINFOEX, szVal, usLen, FALSE ) )
        return( FALSE );

    if( GetColAttr( sCol, SQL_DESC_BASE_COLUMN_NAME, szVal, 1023 ) )
        usLen = strlen( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponse( ODBTP_COLINFOEX, szVal, usLen, FALSE ) )
        return( FALSE );

    if( GetColAttr( sCol, SQL_DESC_BASE_TABLE_NAME, szVal, 1023 ) )
        usLen = strlen( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponse( ODBTP_COLINFOEX, szVal, usLen, FALSE ) )
        return( FALSE );

    SetColInfoFlags( sCol, &ulFlags );

    m_nLastResult = SQL_SUCCESS;
    ProcessLastResult();

    if( !pSock->SendResponse( ODBTP_COLINFOEX, ulFlags, FALSE ) )
        return( FALSE );

    return( TRUE );
}

BOOL COdbtpQry::SendColInfoExW( COdbtpSock* pSock, short sCol )
{
    wchar_t szVal[1024];
    ULONG   ulFlags = 0;
    USHORT  usLen;

    if( GetColAttrW( sCol, SQL_DESC_TYPE_NAME, szVal, 1022 ) )
        usLen = UTF8Len( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponseW( ODBTP_COLINFOEX, szVal, FALSE ) )
        return( FALSE );

    if( GetColAttrW( sCol, SQL_DESC_TABLE_NAME, szVal, 1022 ) )
        usLen = UTF8Len( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponseW( ODBTP_COLINFOEX, szVal, FALSE ) )
        return( FALSE );

    if( GetColAttrW( sCol, SQL_DESC_SCHEMA_NAME, szVal, 1022 ) )
        usLen = UTF8Len( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponseW( ODBTP_COLINFOEX, szVal, FALSE ) )
        return( FALSE );

    if( GetColAttrW( sCol, SQL_DESC_CATALOG_NAME, szVal, 1022 ) )
        usLen = UTF8Len( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponseW( ODBTP_COLINFOEX, szVal, FALSE ) )
        return( FALSE );

    if( GetColAttrW( sCol, SQL_DESC_BASE_COLUMN_NAME, szVal, 1022 ) )
        usLen = UTF8Len( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponseW( ODBTP_COLINFOEX, szVal, FALSE ) )
        return( FALSE );

    if( GetColAttrW( sCol, SQL_DESC_BASE_TABLE_NAME, szVal, 1022 ) )
        usLen = UTF8Len( szVal );
    else
        usLen = 0;
    if( !pSock->SendResponse( ODBTP_COLINFOEX, usLen, FALSE ) )
        return( FALSE );
    if( usLen && !pSock->SendResponseW( ODBTP_COLINFOEX, szVal, FALSE ) )
        return( FALSE );

    SetColInfoFlags( sCol, &ulFlags );

    m_nLastResult = SQL_SUCCESS;
    ProcessLastResultW();

    if( !pSock->SendResponse( ODBTP_COLINFOEX, ulFlags, FALSE ) )
        return( FALSE );

    return( TRUE );
}

BOOL COdbtpQry::SendCursor( COdbtpSock* pSock )
{
    BYTE byBookmarksOn = m_bBookmarksOn ? 1 : 0;

    if( !pSock->SendResponse( ODBTP_CURSOR, m_usCursorType, FALSE ) ||
        !pSock->SendResponse( ODBTP_CURSOR, m_usCursorConcur, FALSE ) )
    {
        return( FALSE );
    }
    return( pSock->SendResponse( ODBTP_CURSOR, byBookmarksOn ) );
}

BOOL COdbtpQry::SendParamInfo( COdbtpSock* pSock )
{
    BOOL           bOK;
    COdbParamData* pParam;
    short          sParam;
    USHORT         usCode;

    usCode = m_bPreparedProc ? ODBTP_PARAMINFOEX : ODBTP_PARAMINFO;

    if( !pSock->SendResponse( usCode, (USHORT)m_sTotalParams, FALSE ) )
        return( FALSE );

    for( sParam = 1; sParam <= m_sTotalParams; sParam++ )
    {
        pParam = (m_pParamData + sParam - 1);

        if( !pSock->SendResponse( usCode, pParam->m_usType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sSqlType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sDefaultOdbType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sOdbType, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, pParam->m_ulColSize, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sDecDigits, FALSE ) )
            return( FALSE );
        if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sNullable, FALSE ) )
            return( FALSE );

        if( usCode == ODBTP_PARAMINFOEX )
        {
            if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sNameLen, FALSE ) )
                return( FALSE );

            if( pParam->m_sNameLen != 0 )
            {
                if( m_pCon->m_ulAttrUnicodeSQL )
                    bOK = pSock->SendResponseW( usCode, pParam->m_pszNameW, FALSE );
                else
                    bOK = pSock->SendResponse( usCode, pParam->m_szName, pParam->m_sNameLen, FALSE );
                if( !bOK ) return( FALSE );
            }
            if( !pSock->SendResponse( usCode, (USHORT)pParam->m_sTypeLen, FALSE ) )
                return( FALSE );

            if( pParam->m_sTypeLen != 0 )
            {
                if( m_pCon->m_ulAttrUnicodeSQL )
                    bOK = pSock->SendResponseW( usCode, pParam->m_pszTypeW, FALSE );
                else
                    bOK = pSock->SendResponse( usCode, pParam->m_szType, pParam->m_sTypeLen, FALSE );
                if( !bOK ) return( FALSE );
            }
        }
    }
    return( pSock->SendResponse( usCode ) );
}

BOOL COdbtpQry::SendResultInfo( COdbtpSock* pSock, OdbDiagHandler dhSendInfo )
{
    m_bSentInfo = FALSE;
    if( m_sTotalCols > 0 ) return( SendColInfo( pSock ) );
    if( !Info() ) return( SendAffectedRowCount( pSock ) );
    DisplayDiag( dhSendInfo );
    m_bSentInfo = TRUE;
    return( pSock->GetError() == TCPERR_NONE );
}

BOOL COdbtpQry::SendResultInfoW( COdbtpSock* pSock, OdbDiagHandlerW dhSendInfo )
{
    if( m_sTotalCols > 0 ) return( SendColInfoW( pSock ) );
    if( !Info() ) return( SendAffectedRowCount( pSock ) );
    DisplayDiagW( dhSendInfo );
    return( pSock->GetError() == TCPERR_NONE );
}

BOOL COdbtpQry::SendRow( COdbtpSock* pSock, BOOL bFinal )
{
    LONG     lLI;
    short    n;
    OdbDATA* pData;

    for( n = 0; n < m_sTotalCols; n++ )
    {
        pData = (m_pColData + n)->m_pData;

        if( pData->m_nLI > 0 )
        {
            BOOL bTruncated = FALSE;
            LONG lActualLen;

            switch( pData->m_nType )
            {
                case ODB_CHAR:
                    if( pData->m_nLI >= pData->m_nMaxLen )
                    {
                        bTruncated = TRUE;
                        lActualLen = pData->m_nLI;
                        ((OdbCHAR*)pData)->SyncLen();
                    }
                    if( m_pCon->m_ulAttrRightTrimText )
                    {
                        char* psz = (char*)pData->m_pVal;
                        int   n = pData->m_nLI;

                        for( n--; n >= 0 && psz[n] == ' '; n-- );
                        n++;

                        if( pData->m_nLI != n )
                        {
                            bTruncated = FALSE;
                            psz[n] = 0;
                            ((OdbCHAR*)pData)->SyncLen();
                        }
                    }
                    break;

                case ODB_WCHAR:
                    if( pData->m_nLI >= pData->m_nMaxLen )
                    {
                        bTruncated = TRUE;
                        lActualLen = pData->m_nLI / sizeof(wchar_t);
                        ((OdbWCHAR*)pData)->SyncLen();
                    }
                    if( m_pCon->m_ulAttrRightTrimText )
                    {
                        wchar_t* psz = (wchar_t*)pData->m_pVal;
                        int      n = pData->m_nLI / sizeof(wchar_t);

                        for( n--; n >= 0 && psz[n] == L' '; n-- );
                        n++;

                        if( pData->m_nLI != (int)(n * sizeof(wchar_t)) )
                        {
                            bTruncated = FALSE;
                            psz[n] = 0;
                            ((OdbWCHAR*)pData)->SyncLen();
                        }
                    }
                    break;

                case ODB_BINARY:
                    if( pData->m_nLI > pData->m_nMaxLen )
                    {
                        bTruncated = TRUE;
                        lActualLen = pData->m_nLI;
                        ((OdbBINARY*)pData)->Len( pData->m_nMaxLen );
                    }
                    break;
            }
            if( bTruncated )
            {
                if( !pSock->SendResponse( ODBTP_ROWDATA, (ULONG)0xE0000001, FALSE ) )
                    return( FALSE );
                if( !pSock->SendResponse( ODBTP_ROWDATA, (ULONG)lActualLen, FALSE ) )
                    return( FALSE );
            }
        }
        else if( pData->m_nLI == 0 )
        {
            switch( pData->m_nType )
            {
                case ODB_CHAR:
                case ODB_WCHAR:
                case ODB_BINARY: break;

                default: pData->m_nLI = -1;
            }
        }
        if( pData->m_nLI > 0 && pData->m_nType == ODB_WCHAR )
            lLI = ((OdbWCHAR*)pData)->UTF8Len();
        else
            lLI = pData->m_nLI;
        if( !pSock->SendResponse( ODBTP_ROWDATA, (ULONG)lLI, FALSE ) )
            return( FALSE );

        if( pData->m_nLI <= 0 ) continue;

        switch( pData->m_nType )
        {
            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( !pSock->SendResponse( ODBTP_ROWDATA, *((PUSHORT)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( !pSock->SendResponse( ODBTP_ROWDATA, *((PULONG)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( !pSock->SendResponse( ODBTP_ROWDATA, *((unsigned __int64*)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_REAL:
                if( !pSock->SendResponse( ODBTP_ROWDATA, *((float*)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_DOUBLE:
                if( !pSock->SendResponse( ODBTP_ROWDATA, *((double*)pData->m_pVal), FALSE ) )
                    return( FALSE );
                break;

            case ODB_DATETIME:
                if( !pSock->SendResponse( ODBTP_ROWDATA, (SQL_TIMESTAMP_STRUCT*)pData->m_pVal, FALSE ) )
                    return( FALSE );
                break;

            case ODB_GUID:
                if( !pSock->SendResponse( ODBTP_ROWDATA, (SQLGUID*)pData->m_pVal, FALSE ) )
                    return( FALSE );
                break;

            case ODB_WCHAR:
                if( !SendWCharData( pSock, ODBTP_ROWDATA, (OdbWCHAR*)pData ) )
                    return( FALSE );
                break;

            default:
                if( !pSock->SendResponse( ODBTP_ROWDATA, pData->m_pVal, pData->m_nLI, FALSE ) )
                    return( FALSE );
        }
    }
    return( bFinal ? pSock->SendResponse( ODBTP_ROWDATA ) : TRUE );
}

BOOL COdbtpQry::SendWCharData( COdbtpSock* pSock, USHORT usCode, OdbWCHAR* pData )
{
    int  b;
    int  n;
    int  nLen = pData->Len();
    char sz[264];

    for( b = 0, n = 0; n < nLen; n++ )
    {
        if( b >= 256 )
        {
            if( !pSock->SendResponse( usCode, sz, b, FALSE ) ) return( FALSE );
            b = 0;
        }
        b += pData->UTF8Encode( &sz[b], n );
    }
    if( b != 0 ) return( pSock->SendResponse( usCode, sz, b, FALSE ) );
    return( TRUE );
}

BOOL COdbtpQry::SetColData( COdbtpSock* pSock )
{
    long           lLI;
    COdbColData*   pCol;
    OdbDATA*       pData;
    SQLPOINTER     pVal;
    PBYTE          pbyData;
    USHORT         usCol;

    do
    {
        if( !pSock->ExtractRequest( &usCol ) ||
            !pSock->ExtractRequest( (PULONG)&lLI ) )
        {
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
        }
        if( usCol == 0 || (short)usCol > m_sTotalCols )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid column number" ) );

        pCol = (m_pColData + usCol - 1);

        if( !pCol->m_bBound )
            return( pSock->SendResponseText( ODBTP_INVALID, "Unbound column" ) );

        pData = pCol->m_pData;
        pVal = pData->m_pVal;

        if( lLI < 0 )
        {
            pData->Indicator( lLI );
            continue;
        }
        switch( pData->m_nType )
        {
            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (PUSHORT)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (PULONG)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (unsigned __int64*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_REAL:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (float*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_DOUBLE:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (double*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_DATETIME:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (SQL_TIMESTAMP_STRUCT*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_GUID:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (SQLGUID*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                break;

            case ODB_CHAR:
                if( !pSock->ExtractRequest( &pbyData, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );

                if( !((OdbCHAR*)pData)->Copy( (char*)pbyData, lLI ) )
                {
                    m_nLastResult = ODB_ERR_MEMORY;
                    return( ProcessLastResult() );
                }
                break;

            case ODB_WCHAR:
                if( !pSock->ExtractRequest( &pbyData, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );

                if( !((OdbWCHAR*)pData)->UTF8Copy( (char*)pbyData, lLI ) )
                {
                    m_nLastResult = ODB_ERR_MEMORY;
                    return( ProcessLastResult() );
                }
                break;

            case ODB_BINARY:
                if( !pSock->ExtractRequest( &pbyData, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );

                if( !((OdbBINARY*)pData)->Set( pbyData, lLI ) )
                {
                    m_nLastResult = ODB_ERR_MEMORY;
                    return( ProcessLastResult() );
                }
                break;

            default:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
                if( !pSock->ExtractRequest( (PBYTE)pData->m_pVal, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCOL request" ) );
        }
        pData->RestoreLen();

        if( pVal != pData->m_pVal ) // Rebind if buffer changed.
        {
            if( !BindCol( usCol, pData->m_nType, pData->m_pVal,
                          pData->m_nMaxLen, &pData->m_nLI ) )
            {
                return( FALSE );
            }
        }
    }
    while( pSock->GetExtractRequestSize() != 0 );

    return( pSock->SendResponse( ODBTP_QUERYOK ) );
}

BOOL COdbtpQry::SetColInfoFlags( short sCol, PULONG pulFlags )
{
    SQLINTEGER nVal;

    *pulFlags = 0;

    if( GetColAttr( sCol, SQL_DESC_NULLABLE, &nVal ) && nVal == SQL_NO_NULLS )
        *pulFlags |= ODB_COLINFO_NOTNULL;

    if( GetColAttr( sCol, SQL_DESC_UNSIGNED, &nVal ) && nVal )
        *pulFlags |= ODB_COLINFO_UNSIGNED;

    if( GetColAttr( sCol, SQL_DESC_AUTO_UNIQUE_VALUE, &nVal ) && nVal )
        *pulFlags |= ODB_COLINFO_AUTONUMBER;

    if( m_pCon->m_ulAttrDriver == ODB_DRIVER_MSSQL )
    {
        if( GetColAttr( sCol, SQL_CA_SS_COLUMN_KEY, &nVal ) && nVal )
            *pulFlags |= ODB_COLINFO_PRIMARYKEY;

        if( GetColAttr( sCol, SQL_CA_SS_COLUMN_HIDDEN, &nVal ) && nVal )
            *pulFlags |= ODB_COLINFO_HIDDEN;
    }
    return( TRUE );
}

BOOL COdbtpQry::GetNextResult()
{
    m_bGotResult = FALSE;
    m_bGotRow = FALSE;

    if( m_bSentInfo )
    {
        m_bSentInfo = FALSE;
        m_nLastResult = SQL_SUCCESS;
    }
    else
    {
        if( !UnBindColData() ) return( FALSE );
        if( !MoreResults() ) return( FALSE );
        if( NoData() ) return( TRUE );
    }
    return( GetResultInfo() );
}

BOOL COdbtpQry::SetCursor( COdbtpSock* pSock )
{
    BOOL   bOK;
    BYTE   byEnableBookmarks;
    ULONG  ulConcur;
    USHORT usConcur;
    USHORT usType;

    if( !pSock->ExtractRequest( &usType ) ||
        !pSock->ExtractRequest( &usConcur ) ||
        !pSock->ExtractRequest( &byEnableBookmarks ) )
    {
        return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETCURSOR request" ) );
    }
    if( m_bGotResult )
    {
        if( !UnBindColData() ) return( FALSE );
        DumpResults();
        m_bGotResult = FALSE;
        m_bGotRow = FALSE;
    }
    if( byEnableBookmarks )
    {
        if( !m_bBookmarksOn && !EnableBookmarks( TRUE ) ) return( FALSE );
        m_bBookmarksOn = TRUE;
    }
    else
    {
        if( m_bBookmarksOn && !EnableBookmarks( FALSE ) ) return( FALSE );
        m_bBookmarksOn = FALSE;
    }
    switch( usConcur )
    {
        case ODB_CONCUR_READONLY: ulConcur = odbConcurReadOnly; break;
        case ODB_CONCUR_LOCK:     ulConcur = odbConcurLock; break;
        case ODB_CONCUR_ROWVER:   ulConcur = odbConcurRowver; break;
        case ODB_CONCUR_VALUES:   ulConcur = odbConcurValues; break;
        default:                  ulConcur = 0;
    }
    switch( usType )
    {
        case ODB_CURSOR_KEYSET:
            if( ulConcur )
                bOK = UseKeysetCursor( ulConcur );
            else
                bOK = UseKeysetCursor();
            break;

        case ODB_CURSOR_DYNAMIC:
            if( ulConcur )
                bOK = UseDynamicCursor( ulConcur );
            else
                bOK = UseDynamicCursor();
            break;

        case ODB_CURSOR_STATIC:
            if( ulConcur )
                bOK = UseStaticCursor( ulConcur );
            else
                bOK = UseStaticCursor();
            break;

        default:
            if( ulConcur )
                bOK = UseForwardCursor( ulConcur );
            else
                bOK = UseForwardCursor();
            usType = ODB_CURSOR_FORWARD;
    }
    if( !bOK ) return( FALSE );
    m_usCursorType = usType;
    m_usCursorConcur = ulConcur ? usConcur : ODB_CONCUR_DEFAULT;
    return( SendCursor( pSock ) );
}

BOOL COdbtpQry::SetParamData( COdbtpSock* pSock )
{
    long           lLI;
    OdbDATA*       pData;
    COdbParamData* pParam;
    SQLPOINTER     pVal;
    PBYTE          pbyData;
    USHORT         usParam;

    do
    {
        if( !pSock->ExtractRequest( &usParam ) ||
            !pSock->ExtractRequest( (PULONG)&lLI ) )
        {
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
        }
        if( usParam == 0 || (short)usParam > m_sTotalParams )
            return( pSock->SendResponseText( ODBTP_INVALID, "Invalid parameter number" ) );

        pParam = (m_pParamData + usParam - 1);

        if( !pParam->m_bBound )
            return( pSock->SendResponseText( ODBTP_INVALID, "Unbound parameter" ) );

        pData = pParam->m_pData;
        pVal = pData->m_pVal;

        if( lLI < 0 )
        {
            pData->Indicator( lLI );
            continue;
        }
        switch( pData->m_nType )
        {
            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (PUSHORT)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (PULONG)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (unsigned __int64*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_REAL:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (float*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_DOUBLE:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (double*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_DATETIME:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (SQL_TIMESTAMP_STRUCT*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_GUID:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (SQLGUID*)pData->m_pVal ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                break;

            case ODB_CHAR:
                if( !pSock->ExtractRequest( &pbyData, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );

                if( !((OdbCHAR*)pData)->Copy( (char*)pbyData, lLI ) )
                {
                    m_nLastResult = ODB_ERR_MEMORY;
                    return( ProcessLastResult() );
                }
                break;

            case ODB_WCHAR:
                if( !pSock->ExtractRequest( &pbyData, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );

                if( !((OdbWCHAR*)pData)->UTF8Copy( (char*)pbyData, lLI ) )
                {
                    m_nLastResult = ODB_ERR_MEMORY;
                    return( ProcessLastResult() );
                }
                break;

            case ODB_BINARY:
                if( !pSock->ExtractRequest( &pbyData, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );

                if( !((OdbBINARY*)pData)->Set( pbyData, lLI ) )
                {
                    m_nLastResult = ODB_ERR_MEMORY;
                    return( ProcessLastResult() );
                }
                break;

            default:
                if( lLI != pData->m_nMaxLen )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
                if( !pSock->ExtractRequest( (PBYTE)pData->m_pVal, lLI ) )
                    return( pSock->SendResponseText( ODBTP_INVALID, "Invalid SETPARAM request" ) );
        }
        pData->RestoreLen();

        if( pVal != pData->m_pVal ) // Rebind if buffer changed.
        {
            short sType;

            switch( pParam->m_usType )
            {
                case ODB_PARAM_INPUT:     sType = SQL_PARAM_INPUT; break;
                case ODB_PARAM_INOUT:     sType = SQL_PARAM_INPUT_OUTPUT; break;
                case ODB_PARAM_OUTPUT:
                case ODB_PARAM_RETURNVAL: sType = SQL_PARAM_OUTPUT; break;
            }
            if( !BindParam( usParam, sType, pData->m_nType, pParam->m_sSqlType,
                            pParam->m_ulColSize, pParam->m_sDecDigits,
                            pData->m_pVal, pData->m_nMaxLen, &pData->m_nLI ) )
            {
                return( FALSE );
            }
        }
    }
    while( pSock->GetExtractRequestSize() != 0 );

    return( pSock->SendResponse( ODBTP_QUERYOK ) );
}

BOOL COdbtpQry::SqlExec( PSTR pszSQL )
{
    if( m_bGotResult )
    {
        if( !UnBindColData() ) return( FALSE );
        if( pszSQL ) if( !UnBindParamData() ) return( FALSE );
        m_bGotResult = FALSE;
        DumpResults();
        m_bGotRow = FALSE;
    }
    if( pszSQL && m_pCon->m_ulAttrUnicodeSQL )
    {
        wchar_t* pszSQLW;

        if( !(pszSQLW = UTF8Convert( pszSQL )) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
        if( !ExecuteW( pszSQLW ) )
        {
            free( pszSQLW );
            return( FALSE );
        }
        free( pszSQLW );
    }
    else
    {
        if( !Execute( pszSQL ) ) return( FALSE );
    }
    if( NoData() ) return( TRUE );
    return( GetResultInfo() );
}

BOOL COdbtpQry::SqlPrep( PSTR pszSQL )
{
    if( m_bGotResult )
    {
        if( !UnBindColData() ) return( FALSE );
        DumpResults();
        m_bGotResult = FALSE;
        m_bGotRow = FALSE;
    }
    if( !UnBindParamData() ) return( FALSE );

    if( m_pCon->m_ulAttrUnicodeSQL )
    {
        wchar_t* pszSQLW;

        if( !(pszSQLW = UTF8Convert( pszSQL )) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            return( ProcessLastResult() );
        }
        if( !PrepareW( pszSQLW ) )
        {
            free( pszSQLW );
            return( FALSE );
        }
        free( pszSQLW );
    }
    else
    {
        if( !Prepare( pszSQL ) ) return( FALSE );
    }
    m_bPreparedProc = FALSE;
    if( !GetNumParams( &m_sTotalParams ) ) return( FALSE );
    if( !GetParamInfo() ) return( FALSE );

    return( TRUE );
}

BOOL COdbtpQry::UnBindColData()
{
    if( m_bBoundCols )
    {
        m_bBoundCols = FALSE;
        if( !UnBindCols() ) return( FALSE );
    }
    return( TRUE );
}

BOOL COdbtpQry::UnBindParamData()
{
    if( m_bBoundParams )
    {
        m_bBoundParams = FALSE;
        if( !UnBindParams() ) return( FALSE );
    }
    m_sTotalParams = 0;
    return( TRUE );
}

//////////////////////////////////////////////////////////////////////
// COdbProc Class Implementation

COdbProc::COdbProc()
{
    m_pParams = NULL;
    m_nMaxParams = 0;
    m_nTotalParams = 0;
    m_pszCatalogW = (wchar_t*)m_szCatalog;
    m_pszSchemaW = (wchar_t*)m_szSchema;
    m_pszProcedureW = (wchar_t*)m_szProcedure;
}

COdbProc::~COdbProc()
{
    if( m_pParams ) free( m_pParams );
}

BOOL COdbProc::Init( char* pszCatalog, char* pszSchema,
                     char* pszProcedure )
{
    int nMaxLen = 163;

    m_nMaxParams = 8;
    m_pParams = (SProcParam*)malloc( sizeof(SProcParam) * m_nMaxParams );
    if( !m_pParams ) return( FALSE );

    m_bUnicode = FALSE;

    if( !pszCatalog ) pszCatalog = "";
    if( !pszSchema ) pszSchema = "";

    strncpy( m_szCatalog, pszCatalog, nMaxLen );
    m_szCatalog[nMaxLen] = 0;

    strncpy( m_szSchema, pszSchema, nMaxLen );
    m_szSchema[nMaxLen] = 0;

    strncpy( m_szProcedure, pszProcedure, nMaxLen );
    m_szProcedure[nMaxLen] = 0;

    return( TRUE );
}

BOOL COdbProc::InitW( wchar_t* pszCatalog, wchar_t* pszSchema,
                      wchar_t* pszProcedure )
{
    int nMaxLen = 81;

    m_nMaxParams = 8;
    m_pParams = (SProcParam*)malloc( sizeof(SProcParam) * m_nMaxParams );
    if( !m_pParams ) return( FALSE );

    m_bUnicode = TRUE;

    if( !pszCatalog ) pszCatalog = L"";
    if( !pszSchema ) pszSchema = L"";

    wcsncpy( m_pszCatalogW, pszCatalog, nMaxLen );
    m_pszCatalogW[nMaxLen] = 0;

    wcsncpy( m_pszSchemaW, pszSchema, nMaxLen );
    m_pszSchemaW[nMaxLen] = 0;

    wcsncpy( m_pszProcedureW, pszProcedure, nMaxLen );
    m_pszProcedureW[nMaxLen] = 0;

    return( TRUE );
}

SProcParam* COdbProc::NewParam()
{
    SProcParam* pParam;

    if( m_nTotalParams == m_nMaxParams )
    {
        m_nMaxParams += 4;
        m_pParams = (SProcParam*)realloc( m_pParams,
                                          sizeof(SProcParam) * m_nMaxParams );
        if( !m_pParams ) return( NULL );
    }
    pParam = m_pParams + m_nTotalParams;

    if( m_bUnicode )
    {
        pParam->m_pszNameW = (wchar_t*)pParam->m_szName;
        pParam->m_pszTypeW = (wchar_t*)pParam->m_szType;
    }
    m_nTotalParams++;

    return( pParam );
}

//////////////////////////////////////////////////////////////////////
// COdbtpConPool Class Implementation

COdbtpConPool::COdbtpConPool()
{
    m_ppCons = NULL;
    m_nTotalCons = 0;
    m_usType = ODB_CON_NORMAL;
}

COdbtpConPool::~COdbtpConPool()
{
    Free();
}

void COdbtpConPool::CheckTimeouts( ULONG ulTimeout )
{
    CThreadAccessLock LockAccess( m_taPool );

    int              n;
    COdbtpCon*       pCon;
    unsigned __int64 u64Time;
    ULONG            ulTimeDiff;

    ::GetSystemTimeAsFileTime( (LPFILETIME)&u64Time );

    for( n = 0; n < m_nTotalCons; n++ )
    {
        pCon = m_ppCons[n];
        if( !pCon || pCon->m_bInUse || pCon->m_u64ReleaseTime == 0 )
            continue;

        ulTimeDiff = (ULONG)( (u64Time - pCon->m_u64ReleaseTime) / 10000000 );
        if( ulTimeDiff >= ulTimeout && !pCon->Drop() ) ReInit( pCon );
    }
}

COdbtpCon* COdbtpConPool::Connect( COdbtpSock* pSock, PSTR pszDBConnect )
{
    BOOL       bCheckId = TRUE;
    BOOL       bSearchPool = TRUE;
    BOOL       bIsReserveId;
    int        n;
    COdbtpCon* pCon;
    COdbtpCon* pReturnCon = NULL;
    PSTR       psz;
    PSTR       pszId = pszDBConnect;

    if( m_usType == ODB_CON_RESERVED )
    {
        for( psz = pszId; *psz; psz++ )
        {
            if( !(*psz >= '0' && *psz <= '9') &&
                !(*psz >= 'A' && *psz <= 'F') )
            {
                bCheckId = FALSE;
                break;
            }
        }
        bIsReserveId = bCheckId;
    }
    else
    {
        bIsReserveId = FALSE;
    }
    if( bSearchPool )
    {
        CThreadAccessLock LockAccess( m_taPool );

        for( n = 0; n < m_nTotalCons; n++ )
        {
            pCon = m_ppCons[n];
            if( !pCon || pCon->m_bInUse ) continue;

            if( bCheckId && !strcmp( pCon->m_szId, pszId ) )
            {
                pReturnCon = pCon;
                break;
            }
            if( !bIsReserveId && !pReturnCon && !pCon->Connected() )
                pReturnCon = pCon;
        }
        if( !pReturnCon )
        {
            if( m_usType != ODB_CON_NORMAL )
            {
                pSock->SendResponseText( ODBTP_NODBCONNECT );
                return( NULL );
            }
            if( !(pReturnCon = new COdbtpCon) ||
                !pReturnCon->Init( ODB_CON_SINGLE, -1 ) )
            {
                if( pReturnCon ) delete pReturnCon;
                pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Memory allocation failed" );
                return( NULL );
            }
        }
        pReturnCon->m_bInUse = TRUE;
    }
    if( pReturnCon->Connected() && !pReturnCon->IsGood() )
    {
        CThreadAccessLock LockAccess( m_taPool );

        if( !pReturnCon->Drop() && !(pReturnCon = ReInit( pReturnCon )) )
        {
            pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Memory allocation failed" );
            return( NULL );
        }
        if( m_usType == ODB_CON_RESERVED )
        {
            pSock->SendResponseText( ODBTP_NODBCONNECT );
            return( NULL );
        }
        pReturnCon->m_bInUse = TRUE;
    }
    if( !pReturnCon->Connected() &&
        pReturnCon->Connect( pszDBConnect ) &&
        pReturnCon->PostConnectInit() )
    {
        if( m_usType == ODB_CON_RESERVED )
        {
            FILETIME ft;
            DWORD    dw1, dw2;

            ::GetSystemTimeAsFileTime( &ft );
            dw1 = (DWORD)rand() * (DWORD)rand();
            dw2 = (DWORD)rand() * (DWORD)rand();
            wsprintf( pReturnCon->m_szId, "%08X%08X%08X%08X",
                      ft.dwHighDateTime, ft.dwLowDateTime, dw1, dw2 );
        }
        else
        {
            strcpy( pReturnCon->m_szId, pszId );
        }
    }
    return( pReturnCon );
}

BOOL COdbtpConPool::Disconnect( COdbtpCon* pCon, BOOL bDrop )
{
    CThreadAccessLock LockAccess( m_taPool );

    if( pCon->m_nPoolIndex < 0 )
    {
        pCon->Drop();
        delete pCon;
    }
    else if( bDrop )
    {
        if( !pCon->Drop() && !ReInit( pCon ) ) return( FALSE );
    }
    else if( !pCon->Release() && !ReInit( pCon ) )
    {
        return( FALSE );
    }
    return( TRUE );
}

void COdbtpConPool::Free()
{
    if( m_ppCons )
    {
        for( int n = 0; n < m_nTotalCons; n++ )
            if( m_ppCons[n] ) delete m_ppCons[n];
        delete[] m_ppCons;
    }
    m_ppCons = NULL;
}

COdbtpCon* COdbtpConPool::GetCon( PCSTR pszId )
{
    CThreadAccessLock LockAccess( m_taPool );
    COdbtpCon*        pCon;

    for( int n = 0; n < m_nTotalCons; n++ )
    {
        if( !(pCon = m_ppCons[n]) ) continue;

        if( !strcmp( pCon->m_szId, pszId ) ) return( pCon );
    }
    return( NULL );
}

BOOL COdbtpConPool::Init( int nTotalCons, USHORT usType )
{
    int n;

    if( !(m_ppCons = new COdbtpCon*[nTotalCons]) ) return( FALSE );
    for( n = 0; n < nTotalCons; n++ ) m_ppCons[n] = NULL;

    for( n = 0; n < nTotalCons; n++ )
    {
        if( !(m_ppCons[n] = new COdbtpCon) ) return( FALSE );
        if( !m_ppCons[n]->Init( usType, n ) ) return( FALSE );
    }
    m_nTotalCons = nTotalCons;
    m_usType = usType;

    return( TRUE );
}

COdbtpCon* COdbtpConPool::ReInit( COdbtpCon* pCon )
{
    int n = pCon->m_nPoolIndex;

    if( m_ppCons[n] == pCon )
    {
        delete m_ppCons[n];
        if( !(m_ppCons[n] = new COdbtpCon) ) return( NULL );
        if( !m_ppCons[n]->Init( m_usType, n ) )
        {
            delete m_ppCons[n];
            m_ppCons[n] = NULL;
            return( NULL );
        }
        return( m_ppCons[n] );
    }
    return( NULL );
}

//////////////////////////////////////////////////////////////////////

void DisplayMSSQLDiag( OdbHandle* pOdb, OdbDiagHandler DiagHandler )
{
    BOOL         bDoEnd = TRUE;
    SQLHANDLE    hODBC = pOdb->m_hODBC;
    SQLINTEGER   nCode;
    SQLSMALLINT  nHandleType = pOdb->m_nHandleType;
    SQLSMALLINT  nLen;
    SQLSMALLINT  nMsgNum = 1;
    SQLRETURN    nResult;
    SQLUSMALLINT nSSLine;
    SQLINTEGER   nSSMsgState;
    SQLINTEGER   nSSSeverity;
    char*        pszText;
    char         szMsg[SQL_MAX_MESSAGE_LENGTH+1];
    char         szState[SQL_SQLSTATE_SIZE+1];
    char         szSSProcName[65];
    char         szSSSrvName[65];
    char         szText[SQL_MAX_MESSAGE_LENGTH+257];

    while( (nResult = SQLGetDiagRec( nHandleType, hODBC, nMsgNum,
                                     (SQLCHAR*)szState, &nCode,
                                     (SQLCHAR*)szMsg,
                                     SQL_MAX_MESSAGE_LENGTH, &nLen ))
           != SQL_NO_DATA )
    {
      	if( nResult == SQL_ERROR || nResult == SQL_INVALID_HANDLE )
       	    break;

        if( (pszText = strstr( szMsg, "][SQL Server]" )) )
        {
            if( nCode != 0 )
            {
                pszText += 13;

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_SEVERITY,
                                           &nSSSeverity, SQL_IS_INTEGER,
                                           NULL );

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_MSGSTATE,
                                           &nSSMsgState, SQL_IS_INTEGER,
                                           NULL );

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_SRVNAME,
                                           szSSSrvName,
                                           sizeof(szSSSrvName) - sizeof(char),
                                           &nLen );

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_PROCNAME,
                                           szSSProcName,
                                           sizeof(szSSProcName) - sizeof(char),
                                           &nLen );

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_LINE,
                                           &nSSLine, SQL_IS_USMALLINT,
                                           NULL );

                if( szSSProcName[0] != 0 )
                {
                    sprintf( szText,
                             "[SQL Server][Level %i, State %i, "
                             "Server %s, Procedure %s, Line %u]%s",
                             nSSSeverity, nSSMsgState, szSSSrvName,
                             szSSProcName, nSSLine, pszText );
                }
                else
                {
                    sprintf( szText,
                             "[SQL Server][Level %i, State %i, "
                             "Server %s, Line %u]%s",
                             nSSSeverity, nSSMsgState, szSSSrvName,
                             nSSLine, pszText );
                }
                pszText = szText;
            }
            else
            {
                pszText++;
            }
        }
        else
        {
            pszText = szMsg;
        }
        if( !(bDoEnd = (*DiagHandler)( pOdb, szState, nCode, pszText )) )
            break;

        nMsgNum++;
    }
    if( bDoEnd ) (*DiagHandler)( pOdb, NULL, 0, NULL );
}

void DisplayMSSQLDiagW( OdbHandle* pOdb, OdbDiagHandlerW DiagHandler )
{
    BOOL         bDoEnd = TRUE;
    SQLHANDLE    hODBC = pOdb->m_hODBC;
    SQLINTEGER   nCode;
    SQLSMALLINT  nHandleType = pOdb->m_nHandleType;
    SQLSMALLINT  nLen;
    SQLSMALLINT  nMsgNum = 1;
    SQLRETURN    nResult;
    SQLUSMALLINT nSSLine;
    SQLINTEGER   nSSMsgState;
    SQLINTEGER   nSSSeverity;
    wchar_t*     pszText;
    wchar_t      szMsg[SQL_MAX_MESSAGE_LENGTH+1];
    wchar_t      szState[SQL_SQLSTATE_SIZE+1];
    wchar_t      szSSProcName[65];
    wchar_t      szSSSrvName[65];
    wchar_t      szText[SQL_MAX_MESSAGE_LENGTH+257];

    while( (nResult = SQLGetDiagRecW( nHandleType, hODBC, nMsgNum,
                                      (SQLWCHAR*)szState, &nCode,
                                      (SQLWCHAR*)szMsg,
                                      SQL_MAX_MESSAGE_LENGTH, &nLen ))
           != SQL_NO_DATA )
    {
      	if( nResult == SQL_ERROR || nResult == SQL_INVALID_HANDLE )
       	    break;

        if( (pszText = wcsstr( szMsg, L"][SQL Server]" )) )
        {
            if( nCode != 0 )
            {
                pszText += 13;

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_SEVERITY,
                                           &nSSSeverity, SQL_IS_INTEGER,
                                           NULL );

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_MSGSTATE,
                                           &nSSMsgState, SQL_IS_INTEGER,
                                           NULL );

                nResult = SQLGetDiagFieldW( nHandleType, hODBC, nMsgNum,
                                            SQL_DIAG_SS_SRVNAME,
                                            szSSSrvName,
                                            sizeof(szSSSrvName) - sizeof(wchar_t),
                                            &nLen );

                nResult = SQLGetDiagFieldW( nHandleType, hODBC, nMsgNum,
                                            SQL_DIAG_SS_PROCNAME,
                                            szSSProcName,
                                            sizeof(szSSProcName) - sizeof(wchar_t),
                                            &nLen );

                nResult = SQLGetDiagField( nHandleType, hODBC, nMsgNum,
                                           SQL_DIAG_SS_LINE,
                                           &nSSLine, SQL_IS_USMALLINT,
                                           NULL );

                if( szSSProcName[0] != 0 )
                {
                    swprintf( szText,
                              L"[SQL Server][Level %i, State %i, "
                              L"Server %s, Procedure %s, Line %u]%s",
                              nSSSeverity, nSSMsgState, szSSSrvName,
                              szSSProcName, nSSLine, pszText );
                }
                else
                {
                    swprintf( szText,
                              L"[SQL Server][Level %i, State %i, "
                              L"Server %s, Line %u]%s",
                              nSSSeverity, nSSMsgState, szSSSrvName,
                              nSSLine, pszText );
                }
                pszText = szText;
            }
            else
            {
                pszText++;
            }
        }
        else
        {
            pszText = szMsg;
        }
        if( !(bDoEnd = (*DiagHandler)( pOdb, szState, nCode, pszText )) )
            break;

        nMsgNum++;
    }
    if( bDoEnd ) (*DiagHandler)( pOdb, NULL, 0, NULL );
}

