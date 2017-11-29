/* $Id: odb.cpp,v 1.7 2004/06/02 20:12:20 rtwitty Exp $ */
/*
    Odb - ODBC class library

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
#include <sql.h>
#include <sqlext.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ODB_BUILD
#include "odbtypes.h"
#include "odb.h"

/////////////////////////////////////////////////////////////////////////////
// OdbSrc - Odb Source Class

OdbSrc::OdbSrc()
{
    Clear();
}

OdbSrc::~OdbSrc()
{
}

void OdbSrc::Clear()
{
    szAppend[0] = 0;
    szAppend[sizeof(szAppend) - 1] = 0;
    szDATABASE[0] = 0;
    szDATABASE[sizeof(szDATABASE) - 1] = 0;
    szDBQ[0] = 0;
    szDBQ[sizeof(szDBQ) - 1] = 0;
    szDEFAULTDIR[0] = 0;
    szDEFAULTDIR[sizeof(szDEFAULTDIR) - 1] = 0;
    szDRIVER[0] = 0;
    szDRIVER[sizeof(szDRIVER) - 1] = 0;
    szDSN[0] = 0;
    szDSN[sizeof(szDSN) - 1] = 0;
    szFIL[0] = 0;
    szFIL[sizeof(szFIL) - 1] = 0;
    szFILEDSN[0] = 0;
    szFILEDSN[sizeof(szFILEDSN) - 1] = 0;
    szPWD[0] = 0;
    szPWD[sizeof(szPWD) - 1] = 0;
    szSAVEFILE[0] = 0;
    szSAVEFILE[sizeof(szSAVEFILE) - 1] = 0;
    szSERVER[0] = 0;
    szSERVER[sizeof(szSERVER) - 1] = 0;
    szUID[0] = 0;
    szUID[sizeof(szUID) - 1] = 0;
}

void OdbSrc::GetAppend( char* pszAppend )
{
    strcpy( pszAppend, szAppend );
}

BOOL OdbSrc::GetConnectString( char* pszConnect, int nBufLen )
{
    int   nLen;
    char* ptr = pszConnect;
    char  szTemp[256];

    *ptr = 0;

    if( szSAVEFILE[0] )
    {
        sprintf( szTemp, "SAVEFILE=%s;", szSAVEFILE );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szDSN[0] )
    {
        sprintf( szTemp, "DSN=%s;", szDSN );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szFILEDSN[0] )
    {
        sprintf( szTemp, "FILEDSN=%s;", szFILEDSN );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szDRIVER[0] )
    {
        sprintf( szTemp, "DRIVER={%s};", szDRIVER );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szFIL[0] )
    {
        sprintf( szTemp, "FIL=%s;", szFIL );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szDEFAULTDIR[0] )
    {
        sprintf( szTemp, "DEFAULTDIR=%s;", szDEFAULTDIR );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szDBQ[0] )
    {
        sprintf( szTemp, "DBQ=%s;", szDBQ );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szSERVER[0] )
    {
        sprintf( szTemp, "SERVER=%s;", szSERVER );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szDATABASE[0] )
    {
        sprintf( szTemp, "DATABASE=%s;", szDATABASE );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szUID[0] )
    {
        sprintf( szTemp, "UID=%s;", szUID );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szPWD[0] )
    {
        sprintf( szTemp, "PWD=%s;", szPWD );
        if( (nLen = strlen( szTemp )) >= nBufLen ) return( FALSE );
        strcpy( ptr, szTemp );
        ptr += nLen;
        nBufLen -= nLen;
    }
    if( szAppend[0] )
    {
        if( (int)strlen( szAppend ) >= nBufLen ) return( FALSE );
        strcpy( ptr, szAppend );
    }
    return( TRUE );
}

void OdbSrc::GetDATABASE( char* pszDATABASE )
{
    strcpy( pszDATABASE, szDATABASE );
}

void OdbSrc::GetDBQ( char* pszDBQ )
{
    strcpy( pszDBQ, szDBQ );
}

void OdbSrc::GetDEFAULTDIR( char* pszDEFAULTDIR )
{
    strcpy( pszDEFAULTDIR, szDEFAULTDIR );
}

void OdbSrc::GetDRIVER( char* pszDRIVER )
{
    strcpy( pszDRIVER, szDRIVER );
}

void OdbSrc::GetDSN( char* pszDSN )
{
    strcpy( pszDSN, szDSN );
}

void OdbSrc::GetFIL( char* pszFIL )
{
    strcpy( pszFIL, szFIL );
}

void OdbSrc::GetFILEDSN( char* pszFILEDSN )
{
    strcpy( pszFILEDSN, szFILEDSN );
}

void OdbSrc::GetPWD( char* pszPWD )
{
    strcpy( pszPWD, szPWD );
}

void OdbSrc::GetSAVEFILE( char* pszSAVEFILE )
{
    strcpy( pszSAVEFILE, szSAVEFILE );
}

void OdbSrc::GetSERVER( char* pszSERVER )
{
    strcpy( pszSERVER, szSERVER );
}

void OdbSrc::GetUID( char* pszUID )
{
    strcpy( pszUID, szUID );
}

void OdbSrc::SetAppend( const char* pszAppend )
{
    strncpy( szAppend, pszAppend, sizeof(szAppend) - 1 );
}

void OdbSrc::SetDATABASE( const char* pszDATABASE )
{
    strncpy( szDATABASE, pszDATABASE, sizeof(szDATABASE) - 1 );
}

void OdbSrc::SetDBQ( const char* pszDBQ )
{
    strncpy( szDBQ, pszDBQ, sizeof(szDBQ) - 1 );
}

void OdbSrc::SetDEFAULTDIR( const char* pszDEFAULTDIR )
{
    strncpy( szDEFAULTDIR, pszDEFAULTDIR, sizeof(szDEFAULTDIR) - 1 );
}

void OdbSrc::SetDRIVER( const char* pszDRIVER )
{
    strncpy( szDRIVER, pszDRIVER, sizeof(szDRIVER) - 1 );
}

void OdbSrc::SetFILEDSN( const char* pszFILEDSN )
{
    strncpy( szFILEDSN, pszFILEDSN, sizeof(szFILEDSN) - 1 );
}

void OdbSrc::SetFIL( const char* pszFIL )
{
    strncpy( szFIL, pszFIL, sizeof(szFIL) - 1 );
}

void OdbSrc::SetPWD( const char* pszPWD )
{
    strncpy( szPWD, pszPWD, sizeof(szPWD) - 1 );
}

void OdbSrc::SetSAVEFILE( const char* pszSAVEFILE )
{
    strncpy( szSAVEFILE, pszSAVEFILE, sizeof(szSAVEFILE) - 1 );
}

void OdbSrc::SetSERVER( const char* pszSERVER )
{
    strncpy( szSERVER, pszSERVER, sizeof(szSERVER) - 1 );
}

void OdbSrc::SetUID( const char* pszUID )
{
    strncpy( szUID, pszUID, sizeof(szUID) - 1 );
}

/////////////////////////////////////////////////////////////////////////////
// OdbHandle - Odb Handle Class

OdbHandle::OdbHandle( SQLSMALLINT nHandleType )
{
    m_hODBC = NULL;
    m_pOdbParent = NULL;
    m_nHandleType = nHandleType;
    m_nLastResult = SQL_SUCCESS;
    m_dhErr = DefaultDiagHandler;
    m_dhErrW = DefaultDiagHandlerW;
    m_dhMsg = DefaultDiagHandler;
    m_dhMsgW = DefaultDiagHandlerW;
}

OdbHandle::~OdbHandle()
{
    m_dhErr = NULL;
    m_dhErrW = NULL;
    m_dhMsg = NULL;
    m_dhMsgW = NULL;
    Free();
}

BOOL OdbHandle::Allocate( OdbHandle* pOdbParent )
{
    if( m_hODBC ) return( TRUE );

    SQLHANDLE hODBCParent;

    m_pOdbParent = pOdbParent;

    if( pOdbParent )
    {
        hODBCParent = pOdbParent->m_hODBC;
        if( m_dhErr == DefaultDiagHandler ) m_dhErr = pOdbParent->m_dhErr;
        if( m_dhErrW == DefaultDiagHandlerW ) m_dhErrW = pOdbParent->m_dhErrW;
        if( m_dhMsg == DefaultDiagHandler ) m_dhMsg = pOdbParent->m_dhMsg;
        if( m_dhMsgW == DefaultDiagHandlerW ) m_dhMsgW = pOdbParent->m_dhMsgW;
    }
    else
    {
        hODBCParent = SQL_NULL_HANDLE;
    }
    m_nLastResult = ::SQLAllocHandle( m_nHandleType, hODBCParent, &m_hODBC );
    if( !ProcessLastResult() ) return( FALSE );

    return( OnAllocate() );
}

BOOL OdbHandle::Allocate( OdbHandle& OdbParent )
{
    return( Allocate( &OdbParent ) );
}

BOOL OdbHandle::Allocated()
{
    return( m_hODBC != NULL );
}

int OdbHandle::DefaultDiagHandler( OdbHandle* pOdb, const char* szState,
                                   SQLINTEGER nCode, const char* szText )
{
    static char szDispBuffer[1025];
    static BOOL bFirstMessageBox;
    static BOOL bFirstRun = TRUE;

    HWND  hWnd = GetForegroundWindow();
    int   iSize;
    char* pszTitle;
    char  szBuffer[SQL_SQLSTATE_SIZE+SQL_MAX_MESSAGE_LENGTH+32];

    if( !szState )
    {
        if( bFirstMessageBox )
            pszTitle = pOdb->m_nLastResult >= 0 ? "Odb Info" : "Odb Error";
        else
            pszTitle = pOdb->m_nLastResult >= 0 ? "Odb Info (cont.)" : "Odb Error (cont.)";

        ::MessageBox( hWnd, szDispBuffer, pszTitle,
                      MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );

        bFirstRun = TRUE;
        return( -1 );
    }
    if( bFirstRun )
    {
        szDispBuffer[0] = 0;
        bFirstMessageBox = TRUE;
        bFirstRun = FALSE;
    }
    sprintf( szBuffer, "[%s][%li]%s", szState, nCode, szText );

    iSize = strlen( szDispBuffer );

    if( iSize && (iSize + strlen(szBuffer) + 2) > 1024 )
    {
        if( bFirstMessageBox )
        {
            pszTitle = pOdb->m_nLastResult >= 0 ? "Odb Info" : "Odb Error";
            bFirstMessageBox = FALSE;
        }
        else
        {
            pszTitle = pOdb->m_nLastResult >= 0 ? "Odb Info (cont.)" : "Odb Error (cont.)";
        }
        ::MessageBox( hWnd, szDispBuffer, pszTitle,
                      MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
        strcpy( szDispBuffer, szBuffer );
    }
    else
    {
        if( iSize ) strcat( szDispBuffer, "\n\n" );
        strcat( szDispBuffer, szBuffer );
    }
    return( -1 );
}

int OdbHandle::DefaultDiagHandlerW( OdbHandle* pOdb, const wchar_t* szState,
                                    SQLINTEGER nCode, const wchar_t* szText )
{
    static wchar_t szDispBuffer[1025];
    static BOOL    bFirstMessageBox;
    static BOOL    bFirstRun = TRUE;

    HWND     hWnd = GetForegroundWindow();
    int      iSize;
    wchar_t* pszTitle;
    wchar_t  szBuffer[SQL_SQLSTATE_SIZE+SQL_MAX_MESSAGE_LENGTH+32];

    if( !szState )
    {
        if( bFirstMessageBox )
            pszTitle = pOdb->m_nLastResult >= 0 ? L"Odb Info" : L"Odb Error";
        else
            pszTitle = pOdb->m_nLastResult >= 0 ? L"Odb Info (cont.)" : L"Odb Error (cont.)";

        ::MessageBoxW( hWnd, szDispBuffer, pszTitle,
                       MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );

        bFirstRun = TRUE;
        return( -1 );
    }
    if( bFirstRun )
    {
        szDispBuffer[0] = 0;
        bFirstMessageBox = TRUE;
        bFirstRun = FALSE;
    }
    swprintf( szBuffer, L"[%s][%li]%s", szState, nCode, szText );

    iSize = wcslen( szDispBuffer );

    if( iSize && (iSize + wcslen(szBuffer) + 2) > 1024 )
    {
        if( bFirstMessageBox )
        {
            pszTitle = pOdb->m_nLastResult >= 0 ? L"Odb Info" : L"Odb Error";
            bFirstMessageBox = FALSE;
        }
        else
        {
            pszTitle = pOdb->m_nLastResult >= 0 ? L"Odb Info (cont.)" : L"Odb Error (cont.)";
        }
        ::MessageBoxW( hWnd, szDispBuffer, pszTitle,
                       MB_OK | MB_ICONINFORMATION | MB_APPLMODAL );
        wcscpy( szDispBuffer, szBuffer );
    }
    else
    {
        if( iSize ) wcscat( szDispBuffer, L"\n\n" );
        wcscat( szDispBuffer, szBuffer );
    }
    return( -1 );
}

void OdbHandle::DisplayDiag( OdbDiagHandler DiagHandler )
{
    SQLHANDLE   hODBC;
    SQLSMALLINT nHandleType;

    if( m_hODBC )
    {
        hODBC = m_hODBC;
        nHandleType = m_nHandleType;
    }
    else if( m_pOdbParent )
    {
        hODBC = m_pOdbParent->m_hODBC;
        nHandleType = m_pOdbParent->m_nHandleType;
    }
    else
    {
        hODBC = NULL;
        nHandleType = NULL;
    }
    OdbDiagHandler dh = DiagHandler ? DiagHandler : DefaultDiagHandler;

    if( m_nLastResult == SQL_SUCCESS_WITH_INFO || m_nLastResult == SQL_ERROR )
    {
        BOOL           bDoEnd = TRUE;
        SQLINTEGER     nCode;
        SQLSMALLINT    nMsgLen;
        SQLRETURN      nResult;
        SQLSMALLINT    nMsgNum = 1;
        SQLCHAR        szState[SQL_SQLSTATE_SIZE+1];
        SQLCHAR        szText[SQL_MAX_MESSAGE_LENGTH+1];

        while( (nResult = ::SQLGetDiagRec( nHandleType, hODBC, nMsgNum++,
                                           szState, &nCode, szText,
                                           SQL_MAX_MESSAGE_LENGTH, &nMsgLen ))
               != SQL_NO_DATA )
        {
		   	if( nResult == SQL_ERROR || nResult == SQL_INVALID_HANDLE )
		   	    break;

            if( !(bDoEnd = (*dh)( this, (char*)szState, nCode, (char*)szText )) )
                break;
        }
        if( bDoEnd ) (*dh)( this, NULL, 0, NULL );
    }
    else
    {
        char* pszText;

        switch( m_nLastResult )
        {
            case SQL_SUCCESS:
                pszText = "Operation was Successful."; break;
            case SQL_NO_DATA:
                pszText = "No Data was returned."; break;
            case SQL_NEED_DATA:
                pszText = "Need Data."; break;
            case SQL_STILL_EXECUTING:
                pszText = "Still Executing."; break;
            case SQL_INVALID_HANDLE:
                pszText = "Invalid Handle."; break;
            case ODB_ERR_MEMORY:
                pszText = "Memory Allocation Error."; break;
            case ODB_ERR_COLNAME:
                pszText = "Bad Column Name."; break;
            case ODB_ERR_UNALLOC:
                pszText = "Cannot use Unallocated data."; break;
            case ODB_ERR_VALUE:
                pszText = "Invalid Value."; break;
            case ODB_ERR_APICALL:
                pszText = "Invalid or unsupported API call."; break;
            default:
                pszText = "No diagnostic available for last result.";
        }
        if( (*dh)( this, "ODBEM", 0, pszText ) ) (*dh)( this, NULL, 0, NULL );
    }
}

void OdbHandle::DisplayDiagW( OdbDiagHandlerW DiagHandler )
{
    SQLHANDLE   hODBC;
    SQLSMALLINT nHandleType;

    if( m_hODBC )
    {
        hODBC = m_hODBC;
        nHandleType = m_nHandleType;
    }
    else if( m_pOdbParent )
    {
        hODBC = m_pOdbParent->m_hODBC;
        nHandleType = m_pOdbParent->m_nHandleType;
    }
    else
    {
        hODBC = NULL;
        nHandleType = NULL;
    }
    OdbDiagHandlerW dh = DiagHandler ? DiagHandler : DefaultDiagHandlerW;

    if( m_nLastResult == SQL_SUCCESS_WITH_INFO || m_nLastResult == SQL_ERROR )
    {
        BOOL           bDoEnd = TRUE;
        SQLINTEGER     nCode;
        SQLSMALLINT    nMsgLen;
        SQLRETURN      nResult;
        SQLSMALLINT    nMsgNum = 1;
        SQLWCHAR       szState[SQL_SQLSTATE_SIZE+1];
        SQLWCHAR       szText[SQL_MAX_MESSAGE_LENGTH+1];

        while( (nResult = ::SQLGetDiagRecW( nHandleType, hODBC, nMsgNum++,
                                            szState, &nCode, szText,
                                            SQL_MAX_MESSAGE_LENGTH, &nMsgLen ))
               != SQL_NO_DATA )
        {
		   	if( nResult == SQL_ERROR || nResult == SQL_INVALID_HANDLE )
		   	    break;

            if( !(bDoEnd = (*dh)( this, (wchar_t*)szState, nCode, (wchar_t*)szText )) )
                break;
        }
        if( bDoEnd ) (*dh)( this, NULL, 0, NULL );
    }
    else
    {
        wchar_t* pszText;

        switch( m_nLastResult )
        {
            case SQL_SUCCESS:
                pszText = L"Operation was Successful."; break;
            case SQL_NO_DATA:
                pszText = L"No Data was returned."; break;
            case SQL_NEED_DATA:
                pszText = L"Need Data."; break;
            case SQL_STILL_EXECUTING:
                pszText = L"Still Executing."; break;
            case SQL_INVALID_HANDLE:
                pszText = L"Invalid Handle."; break;
            case ODB_ERR_MEMORY:
                pszText = L"Memory Allocation Error."; break;
            case ODB_ERR_COLNAME:
                pszText = L"Bad Column Name."; break;
            case ODB_ERR_UNALLOC:
                pszText = L"Cannot use Unallocated data."; break;
            case ODB_ERR_VALUE:
                pszText = L"Invalid Value."; break;
            case ODB_ERR_APICALL:
                pszText = L"Invalid or unsupported API call."; break;
            default:
                pszText = L"No diagnostic available for last result.";
        }
        if( (*dh)( this, L"ODBEM", 0, pszText ) ) (*dh)( this, NULL, 0, NULL );
    }
}

BOOL OdbHandle::Error()
{
    return( m_nLastResult < 0 );
}

BOOL OdbHandle::Error( char* szErrorState, SQLINTEGER nErrorCode )
{
    if( m_nLastResult != SQL_ERROR ) return( FALSE );

    SQLHANDLE   hODBC;
    SQLSMALLINT nHandleType;

    if( m_hODBC )
    {
        hODBC = m_hODBC;
        nHandleType = m_nHandleType;
    }
    else if( m_pOdbParent )
    {
        hODBC = m_pOdbParent->m_hODBC;
        nHandleType = m_pOdbParent->m_nHandleType;
    }
    SQLINTEGER  nCode;
    SQLSMALLINT nMsgLen;
    SQLRETURN   nResult;
    SQLSMALLINT nMsgNum = 1;
    SQLCHAR     szState[SQL_SQLSTATE_SIZE+1];

    while( (nResult = ::SQLGetDiagRec( nHandleType, hODBC, nMsgNum++,
                                       szState, &nCode, NULL, 0, &nMsgLen ))
        != SQL_NO_DATA )
    {
		if( nResult == SQL_ERROR || nResult == SQL_INVALID_HANDLE ) break;
        if( !strcmp( (char*)szState, szErrorState ) )
            if( nErrorCode == 0 || nCode == nErrorCode ) return( TRUE );
    }
    return( FALSE );
}

BOOL OdbHandle::Free()
{
    if( m_hODBC )
    {
        m_nLastResult = ::SQLFreeHandle( m_nHandleType, m_hODBC );
        if( !ProcessLastResult() ) return( FALSE );
        m_hODBC = NULL;
    }
    return( TRUE );
}

BOOL OdbHandle::GetAttr( SQLINTEGER nAttr, char* pValue, SQLINTEGER nValLen, SQLINTEGER* pnActLen )
{
    SQLINTEGER nActLen;
    if( !pnActLen ) pnActLen = &nActLen;

    switch( m_nHandleType )
    {
        case SQL_HANDLE_ENV:
            m_nLastResult = ::SQLGetEnvAttr( m_hODBC, nAttr, (SQLPOINTER)pValue, nValLen, pnActLen );
            break;

        case SQL_HANDLE_DBC:
            m_nLastResult = ::SQLGetConnectAttr( m_hODBC, nAttr, (SQLPOINTER)pValue, nValLen, pnActLen );
            break;

        case SQL_HANDLE_STMT:
            m_nLastResult = ::SQLGetStmtAttr( m_hODBC, nAttr, (SQLPOINTER)pValue, nValLen, pnActLen );
            break;
    }
    return( ProcessLastResult() );
}

BOOL OdbHandle::GetAttr( SQLINTEGER nAttr, SQLPOINTER* pValue )
{
    return( GetAttr( nAttr, (char*)pValue, SQL_IS_POINTER ) );
}

BOOL OdbHandle::GetAttr( SQLINTEGER nAttr, SQLINTEGER* pValue )
{
    return( GetAttr( nAttr, (char*)pValue, SQL_IS_INTEGER ) );
}

BOOL OdbHandle::GetAttr( SQLINTEGER nAttr, SQLUINTEGER* pValue )
{
    return( GetAttr( nAttr, (char*)pValue, SQL_IS_UINTEGER ) );
}

void* OdbHandle::GetUserData()
{
    return( m_pUserData );
}

BOOL OdbHandle::Info()
{
    return( m_nLastResult == SQL_SUCCESS_WITH_INFO );
}

BOOL OdbHandle::Info( char* szInfoState, SQLINTEGER nInfoCode )
{
    if( m_nLastResult != SQL_SUCCESS_WITH_INFO ) return( FALSE );

    SQLHANDLE   hODBC;
    SQLSMALLINT nHandleType;

    if( m_hODBC )
    {
        hODBC = m_hODBC;
        nHandleType = m_nHandleType;
    }
    else if( m_pOdbParent )
    {
        hODBC = m_pOdbParent->m_hODBC;
        nHandleType = m_pOdbParent->m_nHandleType;
    }
    SQLINTEGER  nCode;
    SQLSMALLINT nMsgLen;
    SQLRETURN   nResult;
    SQLSMALLINT nMsgNum = 1;
    SQLCHAR     szState[SQL_SQLSTATE_SIZE+1];

    while( (nResult = ::SQLGetDiagRec( nHandleType, hODBC, nMsgNum++,
                                       szState, &nCode, NULL, 0, &nMsgLen ))
        != SQL_NO_DATA )
    {
		if( nResult == SQL_ERROR || nResult == SQL_INVALID_HANDLE ) break;
        if( !strcmp( (char*)szState, szInfoState ) )
            if( nInfoCode == 0 || nCode == nInfoCode ) return( TRUE );
    }
    return( FALSE );
}

BOOL OdbHandle::NeedData()
{
    return( m_nLastResult == SQL_NEED_DATA );
}

BOOL OdbHandle::NoData()
{
    return( m_nLastResult == SQL_NO_DATA || m_nLastResult < 0 );
}

BOOL OdbHandle::OnAllocate()
{
    return( TRUE );
}

BOOL OdbHandle::ProcessLastResult()
{
    OdbDiagHandler dh;

    switch( m_nLastResult )
    {
        case SQL_SUCCESS:
        case SQL_NO_DATA:
        case SQL_NEED_DATA:
        case SQL_STILL_EXECUTING: dh = NULL; break;

        default: dh = m_nLastResult < 0 ? m_dhErr : m_dhMsg;
    }
    if( dh ) DisplayDiag( dh );

    return( m_nLastResult >= 0 );
}

BOOL OdbHandle::ProcessLastResultW()
{
    OdbDiagHandlerW dh;

    switch( m_nLastResult )
    {
        case SQL_SUCCESS:
        case SQL_NO_DATA:
        case SQL_NEED_DATA:
        case SQL_STILL_EXECUTING: dh = NULL; break;

        default: dh = m_nLastResult < 0 ? m_dhErrW : m_dhMsgW;
    }
    if( dh ) DisplayDiagW( dh );

    return( m_nLastResult >= 0 );
}

BOOL OdbHandle::SetAttr( SQLINTEGER nAttr, char* pValue, SQLINTEGER nValLen )
{
    switch( m_nHandleType )
    {
        case SQL_HANDLE_ENV:
            m_nLastResult = ::SQLSetEnvAttr( m_hODBC, nAttr, (SQLPOINTER)pValue, nValLen );
            break;

        case SQL_HANDLE_DBC:
            m_nLastResult = ::SQLSetConnectAttr( m_hODBC, nAttr, (SQLPOINTER)pValue, nValLen );
            break;

        case SQL_HANDLE_STMT:
            m_nLastResult = ::SQLSetStmtAttr( m_hODBC, nAttr, (SQLPOINTER)pValue, nValLen );
            break;
    }
    return( ProcessLastResult() );
}

BOOL OdbHandle::SetAttr( SQLINTEGER nAttr, SQLPOINTER Value )
{
    return( SetAttr( nAttr, (char*)Value, SQL_IS_POINTER ) );
}

BOOL OdbHandle::SetAttr( SQLINTEGER nAttr, SQLINTEGER Value )
{
    return( SetAttr( nAttr, (char*)Value, SQL_IS_INTEGER ) );
}

BOOL OdbHandle::SetAttr( SQLINTEGER nAttr, SQLUINTEGER Value )
{

    return( SetAttr( nAttr, (char*)Value, SQL_IS_UINTEGER ) );
}

void OdbHandle::SetErrDiagHandler( OdbDiagHandler DiagHandler )
{
    m_dhErr = DiagHandler;
}

void OdbHandle::SetErrDiagHandlerW( OdbDiagHandlerW DiagHandler )
{
    m_dhErrW = DiagHandler;
}

void OdbHandle::SetMsgDiagHandler( OdbDiagHandler DiagHandler )
{
    m_dhMsg = DiagHandler;
}

void OdbHandle::SetMsgDiagHandlerW( OdbDiagHandlerW DiagHandler )
{
    m_dhMsgW = DiagHandler;
}

void OdbHandle::SetUserData( void* pUserData )
{
    m_pUserData = pUserData;
}

BOOL OdbHandle::StillExecuting()
{
    return( m_nLastResult == SQL_STILL_EXECUTING );
}

/////////////////////////////////////////////////////////////////////////////
// OdbEnv - Odb Environment Class

OdbEnv::OdbEnv( BOOL bAllocate ) : OdbHandle( SQL_HANDLE_ENV )
{
    if( bAllocate ) Allocate( NULL );
}

OdbEnv::~OdbEnv()
{
}

BOOL OdbEnv::Commit()
{
    m_nLastResult = ::SQLEndTran( m_nHandleType, m_hODBC, SQL_COMMIT );
    return( ProcessLastResult() );
}

BOOL OdbEnv::GetDrivers( char* pszName, SQLSMALLINT nName, char* pszAttrs, SQLSMALLINT nAttrs, BOOL bFirst )
{
    SQLUSMALLINT nDirection;
    SQLSMALLINT  nLen1;
    SQLSMALLINT  nLen2;

    nDirection = (SQLUSMALLINT)(bFirst ? SQL_FETCH_FIRST : SQL_FETCH_NEXT);
    m_nLastResult = ::SQLDrivers( m_hODBC, nDirection,
                                  (SQLCHAR*)pszName, nName, &nLen1,
                                  (SQLCHAR*)pszAttrs, nAttrs, &nLen2 );

    if( m_nLastResult == SQL_NO_DATA || m_nLastResult == SQL_ERROR )
    {
        strcpy( pszName, "" );
        strcpy( pszAttrs, "" );
        return( FALSE );
    }
    return( TRUE );
}

BOOL OdbEnv::GetSources( char* pszName, SQLSMALLINT nName, char* pszDriver, SQLSMALLINT nDriver, BOOL bFirst )
{
    SQLUSMALLINT nDirection;
    SQLSMALLINT  nLen1;
    SQLSMALLINT  nLen2;

    nDirection = (SQLUSMALLINT)(bFirst ? SQL_FETCH_FIRST : SQL_FETCH_NEXT);
    m_nLastResult = ::SQLDataSources( m_hODBC, nDirection,
                                      (SQLCHAR*)pszName, nName, &nLen1,
                                      (SQLCHAR*)pszDriver, nDriver, &nLen2 );

    if( m_nLastResult == SQL_NO_DATA || m_nLastResult == SQL_ERROR )
    {
        strcpy( pszName, "" );
        strcpy( pszDriver, "" );
        return( FALSE );
    }
    return( TRUE );
}

BOOL OdbEnv::OnAllocate()
{
    return( SetAttr( SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3 ) );
}

BOOL OdbEnv::Rollback()
{
    m_nLastResult = ::SQLEndTran( m_nHandleType, m_hODBC, SQL_ROLLBACK );
    return( ProcessLastResult() );
}

/////////////////////////////////////////////////////////////////////////////
// OdbCon - Odb Connection Class

OdbCon::OdbCon( OdbEnv* pEnv ) : OdbHandle( SQL_HANDLE_DBC )
{
    m_bConnected = FALSE;
    m_nQueryTimeout = 0;
    if( pEnv ) Allocate( pEnv );
}

OdbCon::OdbCon( OdbEnv& Env ) : OdbHandle( SQL_HANDLE_DBC )
{
    m_bConnected = FALSE;
    Allocate( Env );
}

OdbCon::~OdbCon()
{
    m_dhErr = NULL;
    m_dhErrW = NULL;
    m_dhMsg = NULL;
    m_dhMsgW = NULL;
    Free();
}

BOOL OdbCon::Commit()
{
    m_nLastResult = ::SQLEndTran( m_nHandleType, m_hODBC, SQL_COMMIT );
    return( ProcessLastResult() );
}

BOOL OdbCon::Connect( char* pszDSN, char* pszUID, char* pszPWD )
{
    m_nLastResult = ::SQLConnect( m_hODBC,
                                  (SQLCHAR*)pszDSN, SQL_NTS,
                                  (SQLCHAR*)pszUID, SQL_NTS,
                                  (SQLCHAR*)pszPWD, SQL_NTS );

    if( !ProcessLastResult() ) return( FALSE );
    m_bConnected = TRUE;
    return( TRUE );
}

BOOL OdbCon::Connect( char* pszInConnect,
                        char* pszOutConnect, SQLSMALLINT nOut,
                        SQLSMALLINT nDriverCompletion, HWND hWnd )
{
    char        szOut[1024];
    SQLSMALLINT nOutActual;

    if( pszOutConnect == NULL || nOut == 0 )
    {
        pszOutConnect = szOut;
        nOut = 1024;
    }
    m_nLastResult = ::SQLDriverConnect( m_hODBC, hWnd,
                                        (SQLCHAR*)pszInConnect, SQL_NTS,
                                        (SQLCHAR*)pszOutConnect, nOut,
                                        &nOutActual, nDriverCompletion );

    if( !ProcessLastResult() ) return( FALSE );
    m_bConnected = TRUE;
    return( TRUE );
}

BOOL OdbCon::Connect( OdbSrc* pSrc,
                      char* pszOutConnect, SQLSMALLINT nOut,
                      SQLSMALLINT nDriverCompletion, HWND hWnd )
{
    char InConnect[1024];

    pSrc->GetConnectString( InConnect, 1024 );
    return( Connect( InConnect, pszOutConnect, nOut, nDriverCompletion, hWnd ) );
}

BOOL OdbCon::Connect( OdbSrc& Src,
                      char* pszOutConnect, SQLSMALLINT nOut,
                      SQLSMALLINT nDriverCompletion, HWND hWnd )
{
    return( Connect( &Src, pszOutConnect, nOut, nDriverCompletion, hWnd ) );
}

BOOL OdbCon::Connected()
{
    return( m_bConnected );
}

BOOL OdbCon::DisableTrans()
{
    return( EnableManualCommit( FALSE ) );
}

BOOL OdbCon::Disconnect()
{
    if( m_bConnected )
    {
        SQLUSMALLINT uTC;

        if( GetInfo( SQL_TXN_CAPABLE, &uTC ) && uTC != SQL_TC_NONE )
        {
            SQLUINTEGER uAttr;

            if( GetAttr( SQL_ATTR_AUTOCOMMIT, &uAttr ) )
            {
                if( uAttr == SQL_AUTOCOMMIT_OFF && !Rollback() )
                    return( FALSE );
            }
        }
        m_nLastResult = ::SQLDisconnect( m_hODBC );
        if( !ProcessLastResult() ) return( FALSE );
        m_bConnected = FALSE;
    }
    return( TRUE );
}

BOOL OdbCon::EnableAsyncMode( BOOL bEnable )
{
    return( SetAttr( SQL_ATTR_ASYNC_ENABLE, bEnable ?
                     SQL_ASYNC_ENABLE_ON : SQL_ASYNC_ENABLE_OFF ) );
}

BOOL OdbCon::EnableManualCommit( BOOL bEnable )
{
    return( SetAttr( SQL_ATTR_AUTOCOMMIT, bEnable ?
                     SQL_AUTOCOMMIT_OFF : SQL_AUTOCOMMIT_ON ) );
}

BOOL OdbCon::Free()
{
    if( m_hODBC )
    {
        return( Disconnect() && OdbHandle::Free() );
    }
    return( TRUE );
}

BOOL OdbCon::GetInfo( SQLUSMALLINT nInfo, char* pValue, SQLSMALLINT nValLen, SQLSMALLINT* pnActLen )
{
    SQLSMALLINT nActLen;
    if( !pnActLen ) pnActLen = &nActLen;
    m_nLastResult = ::SQLGetInfo( m_hODBC, nInfo, (SQLPOINTER)pValue, nValLen, pnActLen );
    return( ProcessLastResult() );
}

BOOL OdbCon::GetInfo( SQLUSMALLINT nInfo, SQLUSMALLINT* pValue )
{
    return( GetInfo( nInfo, (char*)pValue, 0 ) );
}

BOOL OdbCon::GetInfo( SQLUSMALLINT nInfo, SQLUINTEGER* pValue )
{
    return( GetInfo( nInfo, (char*)pValue, 0 ) );
}

BOOL OdbCon::Rollback()
{
    m_nLastResult = ::SQLEndTran( m_nHandleType, m_hODBC, SQL_ROLLBACK );
    return( ProcessLastResult() );
}

BOOL OdbCon::SetConnectTimeout( SQLUINTEGER nSeconds )
{
    return( SetAttr( SQL_LOGIN_TIMEOUT, nSeconds ) );
}

BOOL OdbCon::SetQueryTimeout( SQLUINTEGER nSeconds )
{
    m_nLastResult = SQL_SUCCESS;
    m_nQueryTimeout = nSeconds;
    return( ProcessLastResult() );
}

BOOL OdbCon::UseReadCommittedTrans()
{
    if( !EnableManualCommit() ) return( FALSE );
    return( SetAttr( SQL_ATTR_TXN_ISOLATION, SQL_TXN_READ_COMMITTED ) );
}

BOOL OdbCon::UseReadUncommittedTrans()
{
    if( !EnableManualCommit() ) return( FALSE );
    return( SetAttr( SQL_ATTR_TXN_ISOLATION, SQL_TXN_READ_UNCOMMITTED ) );
}

BOOL OdbCon::UseRepeatableReadTrans()
{
    if( !EnableManualCommit() ) return( FALSE );
    return( SetAttr( SQL_ATTR_TXN_ISOLATION, SQL_TXN_REPEATABLE_READ ) );
}

BOOL OdbCon::UseSerializableTrans()
{
    if( !EnableManualCommit() ) return( FALSE );
    return( SetAttr( SQL_ATTR_TXN_ISOLATION, SQL_TXN_SERIALIZABLE ) );
}

/////////////////////////////////////////////////////////////////////////////
// OdbQry - Odb Query Class

OdbQry::OdbQry( OdbCon* pCon ) : OdbHandle( SQL_HANDLE_STMT )
{
    m_pnRowStatus = &m_nRowStatus;
    m_pnRowOp = NULL;
    if( pCon ) Allocate( pCon );
}

OdbQry::OdbQry( OdbCon& Con ) : OdbHandle( SQL_HANDLE_STMT )
{
    m_pnRowStatus = &m_nRowStatus;
    m_pnRowOp = NULL;
    Allocate( Con );
}

OdbQry::~OdbQry()
{
    m_dhErr = NULL;
    m_dhErrW = NULL;
    m_dhMsg = NULL;
    m_dhMsgW = NULL;
    Free();
}

BOOL OdbQry::AddRow()
{
    m_nLastResult = ::SQLBulkOperations( m_hODBC, SQL_ADD );
    return( ProcessLastResult() );
}

BOOL OdbQry::BindCol( SQLUSMALLINT nCol, SQLSMALLINT nCType, SQLPOINTER pBuffer,
                      SQLINTEGER nBufLen, SQLINTEGER* pnLenOrInd )
{
    m_nLastResult = ::SQLBindCol( m_hODBC, nCol, nCType, pBuffer, nBufLen, pnLenOrInd );
    return( ProcessLastResult() );
}

BOOL OdbQry::BindCol( SQLUSMALLINT nCol, OdbDATA& Data, BOOL bAutoAlloc )
{
    switch( Data.m_nType )
    {
        case SQL_C_CHAR:
        case SQL_C_BINARY:
        case SQL_C_WCHAR:

            if( bAutoAlloc )
            {
                SQLUINTEGER nColSize;

                if( !GetColSize( nCol, &nColSize ) ) return( FALSE );

                if( nColSize > 0x00010000 ) nColSize = 0x00010000;

                if( Data.m_nType != SQL_C_BINARY ) nColSize += 2;

                if( Data.m_nType != SQL_C_WCHAR )
                {
                    if( (SQLINTEGER)nColSize > Data.m_nMaxLen &&
                        !(((OdbVARDATA*)&Data)->Realloc( nColSize )) )
                    {
                        m_nLastResult = ODB_ERR_MEMORY;
                        return( ProcessLastResult() );
                    }
                }
                else
                {
                    if( (SQLINTEGER)(nColSize * sizeof(wchar_t)) > Data.m_nMaxLen &&
                        !(((OdbWVARDATA*)&Data)->Realloc( nColSize )) )
                    {
                        m_nLastResult = ODB_ERR_MEMORY;
                        return( ProcessLastResult() );
                    }
                }
            }
    }
    if( Data.m_nMaxLen == 0 )
    {
        m_nLastResult = ODB_ERR_UNALLOC;
        return( ProcessLastResult() );
    }
    return( BindCol( nCol, Data.m_nType, Data.m_pVal,
                     Data.m_nMaxLen, &Data.m_nLI ) );
}

BOOL OdbQry::BindCol( const char* pszColName, OdbDATA& Data, BOOL bAutoAlloc )
{
    SQLSMALLINT nCol;
    if( !GetColNumber( pszColName, &nCol ) ) return( FALSE );
    return( BindCol( nCol, Data, bAutoAlloc ) );
}

BOOL OdbQry::BindInOutParam( SQLUSMALLINT nParam, OdbDATA& Data, BOOL bAutoAlloc )
{
    return( BindParam( nParam, SQL_PARAM_INPUT_OUTPUT, Data, bAutoAlloc ) );
}

BOOL OdbQry::BindInOutParam( SQLUSMALLINT nParam, OdbDATA& Data, SQLSMALLINT nDataType,
                             SQLUINTEGER nColSize, SQLSMALLINT nDecDigits )
{
    return( BindParam( nParam, SQL_PARAM_INPUT_OUTPUT, Data.m_nType,
                       nDataType, nColSize, nDecDigits,
                       Data.m_pVal, Data.m_nMaxLen, &Data.m_nLI ) );
}

BOOL OdbQry::BindInputParam( SQLUSMALLINT nParam, OdbDATA& Data, BOOL bAutoAlloc )
{
    return( BindParam( nParam, SQL_PARAM_INPUT, Data, bAutoAlloc ) );
}

BOOL OdbQry::BindInputParam( SQLUSMALLINT nParam, OdbDATA& Data, SQLSMALLINT nDataType,
                             SQLUINTEGER nColSize, SQLSMALLINT nDecDigits )
{
    return( BindParam( nParam, SQL_PARAM_INPUT, Data.m_nType,
                       nDataType, nColSize, nDecDigits,
                       Data.m_pVal, Data.m_nMaxLen, &Data.m_nLI ) );
}

BOOL OdbQry::BindOutputParam( SQLUSMALLINT nParam, OdbDATA& Data, BOOL bAutoAlloc )
{
    return( BindParam( nParam, SQL_PARAM_OUTPUT, Data, bAutoAlloc ) );
}

BOOL OdbQry::BindOutputParam( SQLUSMALLINT nParam, OdbDATA& Data, SQLSMALLINT nDataType,
                              SQLUINTEGER nColSize, SQLSMALLINT nDecDigits )
{
    return( BindParam( nParam, SQL_PARAM_OUTPUT, Data.m_nType,
                       nDataType, nColSize, nDecDigits,
                       Data.m_pVal, Data.m_nMaxLen, &Data.m_nLI ) );
}

BOOL OdbQry::BindParam( SQLUSMALLINT nParam, SQLSMALLINT nType, SQLSMALLINT nCType,
                        SQLSMALLINT nDataType, SQLUINTEGER nColSize, SQLSMALLINT nDecDigits,
                        SQLPOINTER pBuffer, SQLINTEGER nBufLen, SQLINTEGER* pnLenOrInd )
{
    m_nLastResult = ::SQLBindParameter( m_hODBC, nParam, nType, nCType,
                                        nDataType, nColSize, nDecDigits,
                                        pBuffer, nBufLen, pnLenOrInd );

    return( ProcessLastResult() );
}

BOOL OdbQry::BindParam( SQLUSMALLINT nParam, SQLSMALLINT nType,
                        OdbDATA& Data, BOOL bAutoAlloc )
{
    SQLUINTEGER nColSize;
    SQLSMALLINT nDataType;
    SQLSMALLINT nDecDigits;
    SQLSMALLINT nNullable;

    if( !GetParamInfo( nParam, &nDataType, &nColSize,
                       &nDecDigits, &nNullable ) )
    {
        return( FALSE );
    }
    switch( Data.m_nType )
    {
        case SQL_C_CHAR:
        case SQL_C_BINARY:
        case SQL_C_WCHAR:

            if( bAutoAlloc )
            {
                if( nColSize > 0x00010000 ) nColSize = 0x00010000;

                if( Data.m_nType != SQL_C_BINARY ) nColSize += 2;

                if( Data.m_nType != SQL_C_WCHAR )
                {
                    if( (SQLINTEGER)nColSize > Data.m_nMaxLen &&
                        !(((OdbVARDATA*)&Data)->Realloc( nColSize )) )
                    {
                        m_nLastResult = ODB_ERR_MEMORY;
                        return( ProcessLastResult() );
                    }
                }
                else
                {
                    if( (SQLINTEGER)(nColSize * sizeof(wchar_t)) > Data.m_nMaxLen &&
                        !(((OdbWVARDATA*)&Data)->Realloc( nColSize )) )
                    {
                        m_nLastResult = ODB_ERR_MEMORY;
                        return( ProcessLastResult() );
                    }
                }
            }
    }
    if( Data.m_nMaxLen == 0 )
    {
        m_nLastResult = ODB_ERR_UNALLOC;
        return( ProcessLastResult() );
    }
    return( BindParam( nParam, nType, Data.m_nType,
                       nDataType, nColSize, nDecDigits,
                       Data.m_pVal, Data.m_nMaxLen, &Data.m_nLI ) );
}

BOOL OdbQry::Cancel()
{
    m_nLastResult = ::SQLCancel( m_hODBC );
    return( ProcessLastResult() );
}

BOOL OdbQry::CloseCursor()
{
    m_nLastResult = ::SQLCloseCursor( m_hODBC );
    return( ProcessLastResult() );
}

BOOL OdbQry::DefineRowset( SQLUINTEGER nSize, SQLUINTEGER nStructSize )
{
    if( m_pnRowOp )
    {
        delete[] m_pnRowStatus;
        delete[] m_pnRowOp;
    }
    if( nSize <= 1 )
    {
        m_pnRowStatus = &m_nRowStatus;
        m_pnRowOp = NULL;
    }
    else
    {
        SQLUINTEGER i;

        if( !(m_pnRowStatus = new SQLUSMALLINT[nSize]) ||
            !(m_pnRowOp = new SQLUSMALLINT[nSize]) )
        {
            m_nLastResult = ODB_ERR_MEMORY;
            ProcessLastResult();
            return( FALSE );
        }
        for( i = 0; i < nSize; i++ ) *(m_pnRowOp + i) = SQL_ROW_PROCEED;
    }
    if( !SetAttr( SQL_ATTR_ROW_ARRAY_SIZE, nSize ) ) return( FALSE );
    if( !SetAttr( SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)m_pnRowStatus ) )
        return( FALSE );
    if( !SetAttr( SQL_ATTR_ROW_OPERATION_PTR, (SQLPOINTER)m_pnRowOp ) )
        return( FALSE );
    return( SetAttr( SQL_ATTR_ROW_BIND_TYPE, nStructSize ) );
}

BOOL OdbQry::DeleteRow( int nRow )
{
    m_nLastResult = ::SQLSetPos( m_hODBC, nRow, SQL_DELETE, SQL_LOCK_NO_CHANGE );
    return( ProcessLastResult() );
}

BOOL OdbQry::DumpResults()
{
    for( ;; )
    {
        m_nLastResult = ::SQLMoreResults( m_hODBC );

        if( m_nLastResult == SQL_NO_DATA ||
            (m_nLastResult < 0 && m_nLastResult != SQL_ERROR) )
        {
            break;
        }
    }
    return( ProcessLastResult() );
}

BOOL OdbQry::EnableAsyncMode( BOOL bEnable )
{
    return( SetAttr( SQL_ATTR_ASYNC_ENABLE, bEnable ?
                     SQL_ASYNC_ENABLE_ON : SQL_ASYNC_ENABLE_OFF ) );
}

BOOL OdbQry::EnableBookmarks( BOOL bEnable )
{
    return( SetAttr( SQL_ATTR_USE_BOOKMARKS, bEnable ?
                     SQL_UB_VARIABLE : SQL_UB_OFF ) );
}

BOOL OdbQry::ExecApiCall( SQLCHAR* pszApiCall )
{
    char*    pszFunc = (char*)(pszApiCall + 2);
    SQLCHAR* pszParams = (SQLCHAR*)pszFunc;

    for( ; *pszParams && *pszParams != '|'; pszParams++ );
    if( *pszParams == '|' ) *(pszParams++) = 0;

    if( !stricmp( pszFunc, "SQLColumns" ) )
    {
        SQLCHAR* pszCatalog = GetApiParam( &pszParams );
        SQLCHAR* pszSchema  = GetApiParam( &pszParams );
        SQLCHAR* pszTable   = GetApiParam( &pszParams );
        SQLCHAR* pszColumn  = GetApiParam( &pszParams );

        m_nLastResult = ::SQLColumns( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            pszColumn, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLColumnPrivileges" ) )
    {
        SQLCHAR* pszCatalog = GetApiParam( &pszParams );
        SQLCHAR* pszSchema  = GetApiParam( &pszParams );
        SQLCHAR* pszTable   = GetApiParam( &pszParams );
        SQLCHAR* pszColumn  = GetApiParam( &pszParams );

        m_nLastResult = ::SQLColumnPrivileges( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            pszColumn, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLForeignKeys" ) )
    {
        SQLCHAR* pszPkCatalog = GetApiParam( &pszParams );
        SQLCHAR* pszPkSchema  = GetApiParam( &pszParams );
        SQLCHAR* pszPkTable   = GetApiParam( &pszParams );
        SQLCHAR* pszFkCatalog = GetApiParam( &pszParams );
        SQLCHAR* pszFkSchema  = GetApiParam( &pszParams );
        SQLCHAR* pszFkTable   = GetApiParam( &pszParams );

        m_nLastResult = ::SQLForeignKeys( m_hODBC,
            pszPkCatalog, SQL_NTS,
            pszPkSchema, SQL_NTS,
            pszPkTable, SQL_NTS,
            pszFkCatalog, SQL_NTS,
            pszFkSchema, SQL_NTS,
            pszFkTable, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLGetTypeInfo" ) )
    {
        SQLSMALLINT nDataType = (SQLSMALLINT)atol( (char*)GetApiParam( &pszParams, TRUE ) );

        m_nLastResult = ::SQLGetTypeInfo( m_hODBC,
            nDataType );
    }
    else if( !stricmp( pszFunc, "SQLPrimaryKeys" ) )
    {
        SQLCHAR* pszCatalog = GetApiParam( &pszParams );
        SQLCHAR* pszSchema  = GetApiParam( &pszParams );
        SQLCHAR* pszTable   = GetApiParam( &pszParams );

        m_nLastResult = ::SQLPrimaryKeys( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLProcedures" ) )
    {
        SQLCHAR* pszCatalog   = GetApiParam( &pszParams );
        SQLCHAR* pszSchema    = GetApiParam( &pszParams );
        SQLCHAR* pszProcedure = GetApiParam( &pszParams );

        m_nLastResult = ::SQLProcedures( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszProcedure, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLProcedureColumns" ) )
    {
        SQLCHAR* pszCatalog   = GetApiParam( &pszParams );
        SQLCHAR* pszSchema    = GetApiParam( &pszParams );
        SQLCHAR* pszProcedure = GetApiParam( &pszParams );
        SQLCHAR* pszColumn    = GetApiParam( &pszParams );

        m_nLastResult = ::SQLProcedureColumns( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszProcedure, SQL_NTS,
            pszColumn, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLSpecialColumns" ) )
    {
        SQLUSMALLINT uColType   = (SQLUSMALLINT)atol( (char*)GetApiParam( &pszParams, TRUE ) );
        SQLCHAR*     pszCatalog = GetApiParam( &pszParams );
        SQLCHAR*     pszSchema  = GetApiParam( &pszParams );
        SQLCHAR*     pszTable   = GetApiParam( &pszParams );
        SQLUSMALLINT uScope     = (SQLUSMALLINT)atol( (char*)GetApiParam( &pszParams, TRUE ) );
        SQLUSMALLINT uNullable  = (SQLUSMALLINT)atol( (char*)GetApiParam( &pszParams, TRUE ) );

        m_nLastResult = ::SQLSpecialColumns( m_hODBC,
            uColType,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            uScope,
            uNullable );
    }
    else if( !stricmp( pszFunc, "SQLTables" ) )
    {
        SQLCHAR* pszCatalog   = GetApiParam( &pszParams );
        SQLCHAR* pszSchema    = GetApiParam( &pszParams );
        SQLCHAR* pszTable     = GetApiParam( &pszParams );
        SQLCHAR* pszTableType = GetApiParam( &pszParams );

        m_nLastResult = ::SQLTables( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            pszTableType, SQL_NTS );
    }
    else if( !stricmp( pszFunc, "SQLTablePrivileges" ) )
    {
        SQLCHAR* pszCatalog = GetApiParam( &pszParams );
        SQLCHAR* pszSchema  = GetApiParam( &pszParams );
        SQLCHAR* pszTable   = GetApiParam( &pszParams );

        m_nLastResult = ::SQLTablePrivileges( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS );
    }
    else
    {
        m_nLastResult = ODB_ERR_APICALL;
    }
    return( ProcessLastResult() );
}

BOOL OdbQry::ExecApiCallW( SQLWCHAR* pszApiCall )
{
    wchar_t*  pszFunc = (wchar_t*)(pszApiCall + 2);
    SQLWCHAR* pszParams = (SQLWCHAR*)pszFunc;

    for( ; *pszParams && *pszParams != L'|'; pszParams++ );
    if( *pszParams == L'|' ) *(pszParams++) = 0;

    if( !wcsicmp( pszFunc, L"SQLColumns" ) )
    {
        SQLWCHAR* pszCatalog = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema  = GetApiParamW( &pszParams );
        SQLWCHAR* pszTable   = GetApiParamW( &pszParams );
        SQLWCHAR* pszColumn  = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLColumnsW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            pszColumn, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLColumnPrivileges" ) )
    {
        SQLWCHAR* pszCatalog = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema  = GetApiParamW( &pszParams );
        SQLWCHAR* pszTable   = GetApiParamW( &pszParams );
        SQLWCHAR* pszColumn  = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLColumnPrivilegesW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            pszColumn, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLForeignKeys" ) )
    {
        SQLWCHAR* pszPkCatalog = GetApiParamW( &pszParams );
        SQLWCHAR* pszPkSchema  = GetApiParamW( &pszParams );
        SQLWCHAR* pszPkTable   = GetApiParamW( &pszParams );
        SQLWCHAR* pszFkCatalog = GetApiParamW( &pszParams );
        SQLWCHAR* pszFkSchema  = GetApiParamW( &pszParams );
        SQLWCHAR* pszFkTable   = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLForeignKeysW( m_hODBC,
            pszPkCatalog, SQL_NTS,
            pszPkSchema, SQL_NTS,
            pszPkTable, SQL_NTS,
            pszFkCatalog, SQL_NTS,
            pszFkSchema, SQL_NTS,
            pszFkTable, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLGetTypeInfo" ) )
    {
        SQLSMALLINT nDataType = (SQLSMALLINT)_wtol( (wchar_t*)GetApiParamW( &pszParams, TRUE ) );

        m_nLastResult = ::SQLGetTypeInfoW( m_hODBC,
            nDataType );
    }
    else if( !wcsicmp( pszFunc, L"SQLPrimaryKeys" ) )
    {
        SQLWCHAR* pszCatalog = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema  = GetApiParamW( &pszParams );
        SQLWCHAR* pszTable   = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLPrimaryKeysW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLProcedures" ) )
    {
        SQLWCHAR* pszCatalog   = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema    = GetApiParamW( &pszParams );
        SQLWCHAR* pszProcedure = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLProceduresW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszProcedure, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLProcedureColumns" ) )
    {
        SQLWCHAR* pszCatalog   = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema    = GetApiParamW( &pszParams );
        SQLWCHAR* pszProcedure = GetApiParamW( &pszParams );
        SQLWCHAR* pszColumn    = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLProcedureColumnsW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszProcedure, SQL_NTS,
            pszColumn, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLSpecialColumns" ) )
    {
        SQLUSMALLINT uColType    = (SQLUSMALLINT)_wtol( (wchar_t*)GetApiParamW( &pszParams, TRUE ) );
        SQLWCHAR*     pszCatalog = GetApiParamW( &pszParams );
        SQLWCHAR*     pszSchema  = GetApiParamW( &pszParams );
        SQLWCHAR*     pszTable   = GetApiParamW( &pszParams );
        SQLUSMALLINT uScope      = (SQLUSMALLINT)_wtol( (wchar_t*)GetApiParamW( &pszParams, TRUE ) );
        SQLUSMALLINT uNullable   = (SQLUSMALLINT)_wtol( (wchar_t*)GetApiParamW( &pszParams, TRUE ) );

        m_nLastResult = ::SQLSpecialColumnsW( m_hODBC,
            uColType,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            uScope,
            uNullable );
    }
    else if( !wcsicmp( pszFunc, L"SQLTables" ) )
    {
        SQLWCHAR* pszCatalog   = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema    = GetApiParamW( &pszParams );
        SQLWCHAR* pszTable     = GetApiParamW( &pszParams );
        SQLWCHAR* pszTableType = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLTablesW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS,
            pszTableType, SQL_NTS );
    }
    else if( !wcsicmp( pszFunc, L"SQLTablePrivileges" ) )
    {
        SQLWCHAR* pszCatalog = GetApiParamW( &pszParams );
        SQLWCHAR* pszSchema  = GetApiParamW( &pszParams );
        SQLWCHAR* pszTable   = GetApiParamW( &pszParams );

        m_nLastResult = ::SQLTablePrivilegesW( m_hODBC,
            pszCatalog, SQL_NTS,
            pszSchema, SQL_NTS,
            pszTable, SQL_NTS );
    }
    else
    {
        m_nLastResult = ODB_ERR_APICALL;
    }
    return( ProcessLastResultW() );
}

BOOL OdbQry::Execute( char* pszSQL )
{
    if( !pszSQL )
        m_nLastResult = ::SQLExecute( m_hODBC );
    else if( *pszSQL != '|' || *(pszSQL + 1) != '|' )
        m_nLastResult = ::SQLExecDirect( m_hODBC, (SQLCHAR*)pszSQL, SQL_NTS );
    else
        return( ExecApiCall( (SQLCHAR*)pszSQL ) );

    return( ProcessLastResult() );
}

BOOL OdbQry::ExecuteW( wchar_t* pszSQL )
{
    if( !pszSQL )
        m_nLastResult = ::SQLExecute( m_hODBC );
    else if( *pszSQL != L'|' || *(pszSQL + 1) != L'|' )
        m_nLastResult = ::SQLExecDirectW( m_hODBC, (SQLWCHAR*)pszSQL, SQL_NTS );
    else
        return( ExecApiCallW( (SQLWCHAR*)pszSQL ) );

    return( ProcessLastResultW() );
}

BOOL OdbQry::Fetch()
{
    m_nLastResult = ::SQLFetch( m_hODBC );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchAbs( SQLINTEGER nRow )
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_ABSOLUTE, nRow );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchFirst()
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_FIRST, 0 );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchLast()
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_LAST, 0 );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchNext()
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_NEXT, 0 );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchPrev()
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_PRIOR, 0 );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchRel( SQLINTEGER nRowOffset )
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_RELATIVE, nRowOffset );
    return( ProcessLastResult() );
}

BOOL OdbQry::FetchViaBookmark( SQLINTEGER nBookmarkRowOffset )
{
    m_nLastResult = ::SQLFetchScroll( m_hODBC, SQL_FETCH_BOOKMARK, nBookmarkRowOffset );
    return( ProcessLastResult() );
}

BOOL OdbQry::Free()
{
    if( m_hODBC )
    {
        if( !DumpResults() ) return( FALSE );

        if( m_pnRowOp )
        {
            delete[] m_pnRowStatus;
            delete[] m_pnRowOp;
        }
        return( OdbHandle::Free() );
    }
    return( TRUE );
}

SQLCHAR* OdbQry::GetApiParam( SQLCHAR** ppszParams, BOOL bNumeric )
{
    SQLCHAR* pszParam;
    SQLCHAR* pszParams = *ppszParams;

    for( pszParam = pszParams; *pszParams && *pszParams != '|'; pszParams++ );
    if( *pszParams == '|' ) *(pszParams++) = 0;
    *ppszParams = pszParams;


    if( *pszParam == 0 ) return( bNumeric ? (SQLCHAR*)"0" : NULL );
    if( !strcmp( (char*)pszParam, "\"\"" ) ) pszParam = (SQLCHAR*)"";

    return( pszParam );
}

SQLWCHAR* OdbQry::GetApiParamW( SQLWCHAR** ppszParams, BOOL bNumeric )
{
    SQLWCHAR* pszParam;
    SQLWCHAR* pszParams = *ppszParams;

    for( pszParam = pszParams; *pszParams && *pszParams != L'|'; pszParams++ );
    if( *pszParams == L'|' ) *(pszParams++) = 0;
    *ppszParams = pszParams;

    if( *pszParam == 0 ) return( bNumeric ? L"0" : NULL );
    if( !wcscmp( (wchar_t*)pszParam, L"\"\"" ) ) pszParam = L"";

    return( pszParam );
}

BOOL OdbQry::GetAffectedRowCount( SQLINTEGER* pnRows )
{
    m_nLastResult = ::SQLRowCount( m_hODBC, pnRows );
    return( ProcessLastResult() );
}

BOOL OdbQry::GetBookmarkSize( SQLUINTEGER* pnSize )
{
    char Name[9];
    return( GetColInfo( 0, Name, 8, NULL, pnSize ) );
}

BOOL OdbQry::GetColAttr( SQLSMALLINT nCol, SQLUSMALLINT nField,
                         SQLINTEGER* pValue )
{
    char        sz[4];
    SQLSMALLINT nActLen;

    m_nLastResult = ::SQLColAttribute( m_hODBC, nCol, nField,
                                       sz, 4, &nActLen, pValue );

    return( ProcessLastResult() );
}

BOOL OdbQry::GetColAttr( SQLSMALLINT nCol, SQLUSMALLINT nField,
                         char* pValue, SQLSMALLINT nValLen,
                         SQLSMALLINT* pnActLen )
{
    SQLINTEGER  n;
    SQLSMALLINT nActLen;

    if( !pnActLen ) pnActLen = &nActLen;

    m_nLastResult = ::SQLColAttribute( m_hODBC, nCol, nField,
                                       pValue, nValLen, pnActLen, &n );

    return( ProcessLastResult() );
}

BOOL OdbQry::GetColAttrW( SQLSMALLINT nCol, SQLUSMALLINT nField,
                          wchar_t* pValue, SQLSMALLINT nValLen,
                          SQLSMALLINT* pnActLen )
{
    SQLINTEGER  n;
    SQLSMALLINT nActLen;

    if( !pnActLen ) pnActLen = &nActLen;

    m_nLastResult = ::SQLColAttributeW( m_hODBC, nCol, nField,
                                        pValue, nValLen, pnActLen, &n );

    return( ProcessLastResultW() );
}

BOOL OdbQry::GetColInfo( SQLSMALLINT nCol, char* pszName, SQLSMALLINT nNameLen,
                           SQLSMALLINT* pnDataType, SQLUINTEGER* pnColSize,
                           SQLSMALLINT* pnDecDigits, SQLSMALLINT* pnNullable )
{
    SQLSMALLINT nActLen, nType, nDD, nNull;
    SQLUINTEGER nSize;
    char        szName[64];

    if( !pszName )
    {
        pszName = szName;
        nNameLen = 64;
    }
    if( !pnDataType ) pnDataType = &nType;
    if( !pnColSize ) pnColSize = &nSize;
    if( !pnDecDigits ) pnDecDigits = &nDD;
    if( !pnNullable ) pnNullable = &nNull;
    m_nLastResult = ::SQLDescribeCol( m_hODBC, nCol, (SQLCHAR*)pszName,
                                      nNameLen, &nActLen, pnDataType, pnColSize,
                                      pnDecDigits, pnNullable );

    return( ProcessLastResult() );
}

BOOL OdbQry::GetColInfoW( SQLSMALLINT nCol, wchar_t* pszName, SQLSMALLINT nNameLen,
                          SQLSMALLINT* pnDataType, SQLUINTEGER* pnColSize,
                          SQLSMALLINT* pnDecDigits, SQLSMALLINT* pnNullable )
{
    SQLSMALLINT nActLen, nType, nDD, nNull;
    SQLUINTEGER nSize;
    wchar_t     szName[64];

    if( !pszName )
    {
        pszName = szName;
        nNameLen = 64;
    }
    if( !pnDataType ) pnDataType = &nType;
    if( !pnColSize ) pnColSize = &nSize;
    if( !pnDecDigits ) pnDecDigits = &nDD;
    if( !pnNullable ) pnNullable = &nNull;
    m_nLastResult = ::SQLDescribeColW( m_hODBC, nCol, (SQLWCHAR*)pszName,
                                       nNameLen, &nActLen, pnDataType, pnColSize,
                                       pnDecDigits, pnNullable );

    return( ProcessLastResultW() );
}

BOOL OdbQry::GetColNumber( const char* pszColName, SQLSMALLINT* pnCol )
{
    SQLSMALLINT n;
    SQLSMALLINT nCols;
    char        szName[64];

    if( !GetNumResultCols( &nCols ) ) return( FALSE );

    for( n = 1; n <= nCols; n++ )
    {
        if( !GetColInfo( n, szName, 64 ) ) return( FALSE );
        if( !stricmp( pszColName, szName ) ) break;
    }
    if( n > nCols )
    {
        m_nLastResult = ODB_ERR_COLNAME;
        return( ProcessLastResult() );
    }
    *pnCol = n;

    return( TRUE );
}

BOOL OdbQry::GetColNumberW( const wchar_t* pszColName, SQLSMALLINT* pnCol )
{
    SQLSMALLINT n;
    SQLSMALLINT nCols;
    wchar_t     szName[64];

    if( !GetNumResultCols( &nCols ) ) return( FALSE );

    for( n = 1; n <= nCols; n++ )
    {
        if( !GetColInfoW( n, szName, 64 ) ) return( FALSE );
        if( !wcsicmp( pszColName, szName ) ) break;
    }
    if( n > nCols )
    {
        m_nLastResult = ODB_ERR_COLNAME;
        return( ProcessLastResultW() );
    }
    *pnCol = n;

    return( TRUE );
}

BOOL OdbQry::GetColSize( SQLSMALLINT nCol, SQLUINTEGER* pnSize )
{
    return( GetColInfo( nCol, NULL, 0, NULL, pnSize ) );
}

BOOL OdbQry::GetColSize( const char* pszColName, SQLUINTEGER* pnSize )
{
    SQLSMALLINT nCol;
    if( !GetColNumber( pszColName, &nCol ) ) return( FALSE );
    return( GetColSize( nCol, pnSize ) );
}

BOOL OdbQry::GetColSizeW( const wchar_t* pszColName, SQLUINTEGER* pnSize )
{
    SQLSMALLINT nCol;
    if( !GetColNumberW( pszColName, &nCol ) ) return( FALSE );
    return( GetColSize( nCol, pnSize ) );
}

BOOL OdbQry::GetColumns( char* pszTable )
{
    if( !m_pOdbParent )
    {
        m_nLastResult = SQL_INVALID_HANDLE;
        return( FALSE );
    }
    SQLCHAR szDatabase[33];
    SQLCHAR szUser[33];

    szDatabase[0] = 0;
    szUser[0] = 0;

    ((OdbCon*)m_pOdbParent)->GetInfo( SQL_DATABASE_NAME, (char*)szDatabase, 33 );
    ((OdbCon*)m_pOdbParent)->GetInfo( SQL_USER_NAME, (char*)szUser, 33 );

    m_nLastResult = ::SQLColumns( m_hODBC, szDatabase, SQL_NTS,
                                  szUser, SQL_NTS,
                                  (SQLCHAR*)pszTable, SQL_NTS,
                                  NULL, 0 );

    return( ProcessLastResult() );
}

BOOL OdbQry::GetCursorRowCount( SQLINTEGER* pnRows )
{
    m_nLastResult = ::SQLGetDiagField( SQL_HANDLE_STMT, m_hODBC, 0,
                                       SQL_DIAG_CURSOR_ROW_COUNT, pnRows,
                                       0, NULL );
    return( ProcessLastResult() );
}

BOOL OdbQry::GetData( SQLUSMALLINT nCol, SQLSMALLINT nCType, SQLPOINTER pBuffer, SQLINTEGER nBufLen, SQLINTEGER* pnLenOrInd )
{
    m_nLastResult = ::SQLGetData( m_hODBC, nCol, nCType, pBuffer, nBufLen, pnLenOrInd );
    return( ProcessLastResult() );
}

BOOL OdbQry::GetData( SQLUSMALLINT nCol, OdbDATA& Data )
{
    return( GetData( nCol, Data.m_nType, Data.m_pVal,
                     Data.m_nMaxLen, &Data.m_nLI ) );
}

BOOL OdbQry::GetParamInfo( SQLUSMALLINT nParam, SQLSMALLINT* pnDataType,
                           SQLUINTEGER* pnColSize, SQLSMALLINT* pnDecDigits,
                           SQLSMALLINT* pnNullable )
{
    m_nLastResult = ::SQLDescribeParam( m_hODBC, nParam, pnDataType,
                                        pnColSize, pnDecDigits,
                                        pnNullable );

    return( ProcessLastResult() );
}

BOOL OdbQry::GetNumParams( SQLSMALLINT* pnParams )
{
    m_nLastResult = ::SQLNumParams( m_hODBC, pnParams );
    return( ProcessLastResult() );
}

BOOL OdbQry::GetNumResultCols( SQLSMALLINT* pnCols )
{
    m_nLastResult = ::SQLNumResultCols( m_hODBC, pnCols );
    return( ProcessLastResult() );
}

BOOL OdbQry::GetParamData( SQLPOINTER* ppData )
{
    m_nLastResult = ::SQLParamData( m_hODBC, ppData );
    return( ProcessLastResult() );
}

BOOL OdbQry::GetTables()
{
    if( !m_pOdbParent )
    {
        m_nLastResult = SQL_INVALID_HANDLE;
        return( FALSE );
    }
//  ((OdbCon*)m_pOdbParent)->GetInfo( SQL_DATABASE_NAME, (char*)szDatabase, 33 );
//  ((OdbCon*)m_pOdbParent)->GetInfo( SQL_USER_NAME, (char*)szUser, 33 );

    m_nLastResult =
      ::SQLTables( m_hODBC, NULL, 0, NULL, 0, NULL, 0, NULL, 0 );

    return( ProcessLastResult() );
}

BOOL OdbQry::GetTypeInfo( SQLSMALLINT nType )
{
    m_nLastResult = ::SQLGetTypeInfo( m_hODBC, nType );
    return( ProcessLastResult() );
}

void OdbQry::IgnoreRow( int nRow )
{
    if( m_pnRowOp )
        *(m_pnRowOp + nRow - 1) = SQL_ROW_IGNORE;
}

BOOL OdbQry::LockRow( int nRow )
{
    m_nLastResult = ::SQLSetPos( m_hODBC, nRow, SQL_POSITION, SQL_LOCK_EXCLUSIVE );
    return( ProcessLastResult() );
}

BOOL OdbQry::MoreResults()
{
    m_nLastResult = ::SQLMoreResults( m_hODBC );
    return( ProcessLastResult() );
}

BOOL OdbQry::OnAllocate()
{
    SQLUINTEGER nTimeout = ((OdbCon*)m_pOdbParent)->GetQueryTimeout();
    if( nTimeout != 0 ) SetTimeout( nTimeout );
    SetAttr( SQL_ATTR_ROW_ARRAY_SIZE, 1UL );
    SetAttr( SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)&m_nRowStatus );
    m_nLastResult = SQL_SUCCESS;
    return( ProcessLastResult() );
}

BOOL OdbQry::PositionRow( int nRow )
{
    m_nLastResult = ::SQLSetPos( m_hODBC, nRow, SQL_POSITION, SQL_LOCK_NO_CHANGE );
    return( ProcessLastResult() );
}

BOOL OdbQry::Prepare( char* pszSQL )
{
    m_nLastResult = ::SQLPrepare( m_hODBC, (SQLCHAR*)pszSQL, SQL_NTS );
    return( ProcessLastResult() );
}

BOOL OdbQry::PrepareW( wchar_t* pszSQL )
{
    m_nLastResult = ::SQLPrepareW( m_hODBC, (SQLWCHAR*)pszSQL, SQL_NTS );
    return( ProcessLastResultW() );
}

void OdbQry::ProcessRow( int nRow )
{
    if( m_pnRowOp )
        *(m_pnRowOp + nRow - 1) = SQL_ROW_PROCEED;
}

BOOL OdbQry::PutData( SQLPOINTER pData, SQLINTEGER nLenOrInd )
{
    m_nLastResult = ::SQLPutData( m_hODBC, pData, nLenOrInd );
    return( ProcessLastResult() );
}

BOOL OdbQry::PutData( OdbDATA& Data )
{
    return( PutData( Data.m_pVal, Data.m_nLI ) );
}

BOOL OdbQry::RefreshRow( int nRow )
{
    m_nLastResult = ::SQLSetPos( m_hODBC, nRow, SQL_REFRESH, SQL_LOCK_NO_CHANGE );
    return( ProcessLastResult() );
}

BOOL OdbQry::RowAdded( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_ADDED );
}

BOOL OdbQry::RowDeleted( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_DELETED );
}

BOOL OdbQry::RowError( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_ERROR );
}

BOOL OdbQry::RowInfo( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_SUCCESS_WITH_INFO );
}

BOOL OdbQry::RowNone( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_NOROW );
}

BOOL OdbQry::RowSuccess( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_SUCCESS );
}

BOOL OdbQry::RowUpdated( int nRow )
{
    return( *(m_pnRowStatus + nRow - 1) == SQL_ROW_UPDATED );
}

BOOL OdbQry::SetBookmark( SQLPOINTER pBookmark )
{
    return( SetAttr( SQL_ATTR_FETCH_BOOKMARK_PTR, pBookmark ) );
}

BOOL OdbQry::SetBookmark( OdbVARBOOKMARK& Bookmark )
{
    return( SetAttr( SQL_ATTR_FETCH_BOOKMARK_PTR, Bookmark.m_pVal ) );
}

BOOL OdbQry::SetTimeout( SQLUINTEGER nSeconds )
{
    return( SetAttr( SQL_ATTR_QUERY_TIMEOUT, nSeconds ) );
}


BOOL OdbQry::UnBindCols()
{
    m_nLastResult = ::SQLFreeStmt( m_hODBC, SQL_UNBIND );
    return( ProcessLastResult() );
}

BOOL OdbQry::UnBindParams()
{
    m_nLastResult = ::SQLFreeStmt( m_hODBC, SQL_RESET_PARAMS );
    return( ProcessLastResult() );
}

BOOL OdbQry::UnlockRow( int nRow )
{
    m_nLastResult = ::SQLSetPos( m_hODBC, nRow, SQL_POSITION, SQL_LOCK_UNLOCK );
    return( ProcessLastResult() );
}

BOOL OdbQry::UpdateRow( int nRow )
{
    m_nLastResult = ::SQLSetPos( m_hODBC, nRow, SQL_UPDATE, SQL_LOCK_NO_CHANGE );
    return( ProcessLastResult() );
}

BOOL OdbQry::UseDynamicCursor( SQLUINTEGER nConcurrency )
{
    if( !SetAttr( SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_DYNAMIC ) )
        return( FALSE );
    return( SetAttr( SQL_ATTR_CONCURRENCY, nConcurrency ) );
}

BOOL OdbQry::UseForwardCursor( SQLUINTEGER nConcurrency )
{
    if( !SetAttr( SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY ) )
        return( FALSE );
    return( SetAttr( SQL_ATTR_CONCURRENCY, nConcurrency ) );
}

BOOL OdbQry::UseKeysetCursor( SQLUINTEGER nConcurrency )
{
    if( !SetAttr( SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_KEYSET_DRIVEN ) )
        return( FALSE );
    return( SetAttr( SQL_ATTR_CONCURRENCY, nConcurrency ) );
}

BOOL OdbQry::UseStaticCursor( SQLUINTEGER nConcurrency )
{
    if( !SetAttr( SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_STATIC ) )
        return( FALSE );
    return( SetAttr( SQL_ATTR_CONCURRENCY, nConcurrency ) );
}

