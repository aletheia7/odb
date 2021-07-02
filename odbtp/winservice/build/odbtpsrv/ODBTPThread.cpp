/* $Id: ODBTPThread.cpp,v 1.15 2004/06/02 20:12:21 rtwitty Exp $ */
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
// ODBTPThread.cpp: implementation of the CODBTPThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OdbtpSock.h"
#include "OdbSupport.h"
#include "ODBTPService.h"
#include "ODBTPThread.h"
#include "UTF8Support.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CODBTPThread::CODBTPThread( CODBTPService* pService, COdbtpSock* pSock )
{
    m_pService = pService;
    m_pSock = pSock;
    m_pCon = NULL;
    m_bGotODBCConnError = FALSE;
    m_bRanChild = FALSE;
    m_bSentPrevDiag = FALSE;
}

CODBTPThread::~CODBTPThread()
{

}

BOOL CODBTPThread::DoBINDCOL( COdbtpQry* pQry )
{
    if( !pQry->BindColData( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );

}

BOOL CODBTPThread::DoBINDPARAM( COdbtpQry* pQry )
{
    if( !pQry->BindParamData( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoCANCELREQ()
{
    return( m_pSock->SendResponse( ODBTP_CANCELRESP ) );
}

BOOL CODBTPThread::DoCOMMIT()
{
    if( !m_pCon )
    {
        m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
        return( FALSE );
    }
    if( !m_pCon->Commit() )
    {
        SendErrorDiag( m_pCon );
        return( m_pSock->GetError() == TCPERR_NONE );
    }
    return( m_pSock->SendResponse( ODBTP_OK ) );
}

BOOL CODBTPThread::DoDROP( COdbtpQry* pQry )
{
    if( !pQry->Detach() )
        return( DoQueryRequestError( pQry ) );
    return( m_pSock->SendResponse( ODBTP_QUERYOK ) );
}

BOOL CODBTPThread::DoEXECUTE( COdbtpQry* pQry )
{
    PSTR  pszSQL;
    ULONG ulLen;

    if( (ulLen = m_pSock->GetExtractRequestSize()) )
        m_pSock->ExtractRequest( (PBYTE*)&pszSQL, ulLen );
    else
        pszSQL = NULL;

    if( !pQry->SqlExec( pszSQL ) )
        return( DoQueryRequestError( pQry ) );
    if( !pQry->GotResult() ) return( m_pSock->SendResponse( ODBTP_NODATA ) );

    if( m_pCon->m_ulAttrUnicodeSQL )
    {
        if( !pQry->SendResultInfoW( m_pSock, SendMessageDiagW ) )
            return( DoQueryRequestError( pQry ) );
    }
    else
    {
        if( !pQry->SendResultInfo( m_pSock, SendMessageDiag ) )
            return( DoQueryRequestError( pQry ) );
    }
    return( TRUE );
}

BOOL CODBTPThread::DoFETCHRESULT( COdbtpQry* pQry )
{
    if( !pQry->GetNextResult() )
        return( DoQueryRequestError( pQry ) );
    if( !pQry->GotResult() ) return( m_pSock->SendResponse( ODBTP_NODATA ) );

    if( m_pCon->m_ulAttrUnicodeSQL )
    {
        if( !pQry->SendResultInfoW( m_pSock, SendMessageDiagW ) )
            return( DoQueryRequestError( pQry ) );
    }
    else
    {
        if( !pQry->SendResultInfo( m_pSock, SendMessageDiag ) )
            return( DoQueryRequestError( pQry ) );
    }
    return( TRUE );
}

BOOL CODBTPThread::DoFETCHROW( COdbtpQry* pQry )
{
    long   lFetchParam;
    ULONG  ulCount;
    ULONG  ulSent;
    ULONG  ulSentSync;
    USHORT usFetchType;

    if( m_pSock->GetExtractRequestSize() != 0 )
    {
        if( !m_pSock->ExtractRequest( &usFetchType ) ||
            !m_pSock->ExtractRequest( (PULONG)&lFetchParam ) )
        {
            return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid LOGIN request" ) );
        }
    }
    else
    {
        usFetchType = ODB_FETCH_DEFAULT;
        lFetchParam = 0;
    }
    if( pQry->GetCursorType() == ODB_CURSOR_FORWARD )
        ulCount = pQry->GetFetchRowCount();
    else
        ulCount = 1;

    if( usFetchType == ODB_FETCH_REREAD )
    {
        if( ulCount > 1 )
            return( m_pSock->SendResponseText( ODBTP_UNSUPPORTED, "Unsupported for fetch row counts greater than 1" ) );
        if( !pQry->GotRow() )
            return( m_pSock->SendResponse( ODBTP_NODATA ) );
        if( !pQry->SendRow( m_pSock, FALSE ) ) return( FALSE );
        ulSent = 1;
    }
    else
    {
        for( ulSent = 0; ulSent < ulCount; ulSent++ )
        {
            if( !pQry->GetRow( usFetchType, lFetchParam ) )
                return( DoQueryRequestError( pQry ) );
            if( !pQry->GotRow() ) break;
            if( !pQry->SendRow( m_pSock, FALSE ) ) return( FALSE );
        }
        if( ulSent == 0 ) return( m_pSock->SendResponse( ODBTP_NODATA ) );
    }
    ulSentSync = 0xF000000F;

    if( !m_pSock->SendResponse( ODBTP_ROWDATA, ulSentSync, FALSE ) )
        return( FALSE );
    return( m_pSock->SendResponse( ODBTP_ROWDATA, ulSent ) );
}

BOOL CODBTPThread::DoGETATTR()
{
    if( !m_pCon )
    {
        m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
        return( FALSE );
    }
    if( !m_pCon->GetAttribute( m_pSock ) && !m_pCon->Error() ) return( FALSE );

    if( m_pCon->Error() )
    {
        SendErrorDiag( m_pCon );
        return( m_pSock->GetError() == TCPERR_NONE );
    }
    return( TRUE );
}

BOOL CODBTPThread::DoGETCOLINFO( COdbtpQry* pQry )
{
    if( !pQry->SendColInfo( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoGETCONNID()
{
    if( m_pCon ) return( m_pCon->SendId( m_pSock ) );
    m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
    return( FALSE );
}

BOOL CODBTPThread::DoGETCURSOR( COdbtpQry* pQry )
{
    if( !pQry->SendCursor( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoGETPARAM( COdbtpQry* pQry )
{
    if( !pQry->GetParamData( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoGETPARAMINFO( COdbtpQry* pQry )
{
    if( !pQry->SendParamInfo( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoGETROWCOUNT( COdbtpQry* pQry )
{
    if( !pQry->SendAffectedRowCount( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoLOGIN( PSTR pszDBConnect )
{
    USHORT usLen;
    USHORT usType;
    char   sz[MAX_CONNSTR_SIZE];

    if( !pszDBConnect )
    {
        if( !m_pSock->ExtractRequest( &usType ) ||
            !m_pSock->ExtractRequest( &usLen ) || usLen >= MAX_CONNSTR_SIZE ||
            !m_pSock->ExtractRequest( (PBYTE)sz, usLen ) )
        {
            return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid LOGIN request" ) );
        }
        sz[usLen] = 0;

        if( m_pCon )
        {
            return( m_pSock->SendResponseText( ODBTP_INVALID, "Already logged in" ) );
        }
        if( usType == ODB_CON_SINGLE )
        {
            if( !m_pService->ChildExec( this, sz ) )
            {
                m_pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Unable to execute child process" );
                return( FALSE );
            }
            m_bRanChild = TRUE;
            return( TRUE );
        }
    }
    else
    {
        strcpy( sz, pszDBConnect );
        usType = ODB_CON_SINGLE;
    }
    if( !(m_pCon = m_pService->Connect( m_pSock, sz, usType )) )
        return( FALSE );

    m_pCon->SetUserData( this );

    if( m_pCon->Error() )
    {
        SendDisconnectDiag( m_pCon );
        m_pService->Disconnect( m_pCon, TRUE );
        m_pCon = NULL;
        return( FALSE );
    }
    else if( m_pCon->Info() )
    {
        SendMessageDiag( m_pCon );
        if( m_pSock->GetError() != TCPERR_NONE )
        {
            m_pService->Disconnect( m_pCon, FALSE );
            m_pCon = NULL;
            return( FALSE );
        }
    }
    else if( !m_pSock->SendResponse( ODBTP_OK ) )
    {
        m_pService->Disconnect( m_pCon, FALSE );
        m_pCon = NULL;
        return( FALSE );
    }
    return( TRUE );
}

BOOL CODBTPThread::DoLOGOUT()
{
    BYTE byDropCon;

    if( !m_pSock->ExtractRequest( &byDropCon ) )
    {
        return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid LOGOUT request" ) );
    }
    if( m_pCon )
    {
        if( m_pService->Disconnect( m_pCon, byDropCon != 0 ) )
            m_pSock->SendResponseText( ODBTP_DISCONNECT );
        else
            m_pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Disconnect Failed" );
        m_pCon = NULL;
    }
    else
    {
        m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
    }
    return( FALSE );
}

BOOL CODBTPThread::DoPREPARE( COdbtpQry* pQry )
{
    PSTR  pszSQL;
    ULONG ulLen;

    if( (ulLen = m_pSock->GetExtractRequestSize()) )
        m_pSock->ExtractRequest( (PBYTE*)&pszSQL, ulLen );
    else
        return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid PREPARE request" ) );

    if( !pQry->SqlPrep( pszSQL ) || !pQry->SendParamInfo( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoPREPAREPROC( COdbtpQry* pQry )
{
    BOOL   bInBrackets = FALSE;
    BOOL   bOK;
    PSTR   psz;
    PSTR   pszFullProcName;
    PSTR   pszCatalog = NULL;
    PSTR   pszSchema = NULL;
    PSTR   pszProcedure = NULL;
    ULONG  ulLen;

    if( (ulLen = m_pSock->GetExtractRequestSize()) )
        m_pSock->ExtractRequest( (PBYTE*)&pszFullProcName, ulLen );
    else
        return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid PREPAREPROC request" ) );

    psz = pszFullProcName + ulLen - 1;

    for( ; psz >= pszFullProcName; psz-- )
    {
        if( (!bInBrackets && *psz == '.') || (bInBrackets && *psz == '[') )
        {
            pszProcedure = psz + 1;
            if( *psz == '[' ) psz--;
            break;
        }
        if( !bInBrackets && *psz == ']' )
        {
            *psz = 0;
            bInBrackets = TRUE;
        }
    }
    if( !pszProcedure ) pszProcedure = pszFullProcName;
    if( *pszProcedure == 0 )
        return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid procedure name" ) );

    if( psz > pszFullProcName && *psz == '.' )
    {
        bInBrackets = FALSE;
        *psz = 0;
        for( psz--; psz >= pszFullProcName; psz-- )
        {
            if( (!bInBrackets && *psz == '.') || (bInBrackets && *psz == '[') )
            {
                pszSchema = psz + 1;
                if( *psz == '[' ) psz--;
                break;
            }
            if( !bInBrackets && *psz == ']' )
            {
                *psz = 0;
                bInBrackets = TRUE;
            }
        }
        if( !pszSchema ) pszSchema = pszFullProcName;
        if( *pszSchema == 0 ) pszSchema = NULL;
    }
    if( psz > pszFullProcName && *psz == '.' )
    {
        bInBrackets = FALSE;
        *psz = 0;
        for( psz--; psz >= pszFullProcName; psz-- )
        {
            if( (!bInBrackets && *psz == '.') || (bInBrackets && *psz == '[') )
            {
                pszCatalog = psz + 1;
                if( *psz == '[' ) psz--;
                break;
            }
            if( !bInBrackets && *psz == ']' )
            {
                *psz = 0;
                bInBrackets = TRUE;
            }
        }
        if( !pszCatalog ) pszCatalog = pszFullProcName;
        if( *pszCatalog == 0 ) pszCatalog = NULL;
    }
    if( m_pCon->m_ulAttrUnicodeSQL )
    {
        wchar_t* pszCatalogW = NULL;
        wchar_t* pszSchemaW = NULL;
        wchar_t* pszProcedureW;

        if( (pszCatalog && !(pszCatalogW = UTF8Convert( pszCatalog ))) ||
            (pszSchema && !(pszSchemaW = UTF8Convert( pszSchema ))) ||
            !(pszProcedureW = UTF8Convert( pszProcedure )) )
        {
            return( m_pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Memory allocation failed" ) );
        }
        bOK = pQry->PrepareProcW( m_pSock, pszCatalogW,
                                  pszSchemaW, pszProcedureW );

        free( pszProcedureW );
        if( pszSchemaW ) free( pszSchemaW );
        if( pszCatalogW ) free( pszCatalogW );
    }
    else
    {
        bOK = pQry->PrepareProc( m_pSock, pszCatalog,
                                 pszSchema, pszProcedure );
    }
    if( !bOK || !pQry->SendParamInfo( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoQueryRequest()
{
    BYTE       byId;
    COdbtpQry* pQry;

    if( !m_pCon )
    {
        m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
        return( FALSE );
    }
    if( !m_pSock->ExtractRequest( &byId ) )
        return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid Query request" ) );
    if( (int)byId >= m_pCon->m_nMaxQrys )
        return( m_pSock->SendResponseText( ODBTP_INVALID, "Invalid Query Id" ) );
    if( !(pQry = m_pCon->GetQry( (int)byId )) )
    {
        m_pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Query access failed" );
        return( FALSE );
    }
    pQry->ClearError();

    switch( m_pSock->GetRequestCode() )
    {
        case ODBTP_EXECUTE:      return( DoEXECUTE( pQry ) );
        case ODBTP_PREPARE:      return( DoPREPARE( pQry ) );
        case ODBTP_BINDCOL:      return( DoBINDCOL( pQry ) );
        case ODBTP_BINDPARAM:    return( DoBINDPARAM( pQry ) );
        case ODBTP_GETPARAM:     return( DoGETPARAM( pQry ) );
        case ODBTP_SETPARAM:     return( DoSETPARAM( pQry ) );
        case ODBTP_FETCHROW:     return( DoFETCHROW( pQry ) );
        case ODBTP_FETCHRESULT:  return( DoFETCHRESULT( pQry ) );
        case ODBTP_GETROWCOUNT:  return( DoGETROWCOUNT( pQry ) );
        case ODBTP_GETCOLINFO:   return( DoGETCOLINFO( pQry ) );
        case ODBTP_GETPARAMINFO: return( DoGETPARAMINFO( pQry ) );
        case ODBTP_GETCURSOR:    return( DoGETCURSOR( pQry ) );
        case ODBTP_SETCURSOR:    return( DoSETCURSOR( pQry ) );
        case ODBTP_SETCOL:       return( DoSETCOL( pQry ) );
        case ODBTP_ROWOP:        return( DoROWOP( pQry ) );
        case ODBTP_PREPAREPROC:  return( DoPREPAREPROC( pQry ) );
        case ODBTP_DROP:         return( DoDROP( pQry ) );
    }
    return( m_pSock->SendResponseText( ODBTP_UNSUPPORTED ) );
}

BOOL CODBTPThread::DoQueryRequestError( COdbtpQry* pQry )
{
    if( pQry->Error() )
    {
        SendErrorDiag( pQry );
        return( m_pSock->GetError() == TCPERR_NONE );
    }
    return( FALSE );
}

BOOL CODBTPThread::DoROLLBACK()
{
    if( !m_pCon )
    {
        m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
        return( FALSE );
    }
    if( !m_pCon->Rollback() )
    {
        SendErrorDiag( m_pCon );
        return( m_pSock->GetError() == TCPERR_NONE );
    }
    return( m_pSock->SendResponse( ODBTP_OK ) );
}

BOOL CODBTPThread::DoROWOP( COdbtpQry* pQry )
{
    if( !pQry->DoRowOperation( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoSETATTR()
{
    if( !m_pCon )
    {
        m_pSock->SendResponseText( ODBTP_VIOLATION, "Not connected" );
        return( FALSE );
    }
    if( !m_pCon->SetAttribute( m_pSock ) && !m_pCon->Error() ) return( FALSE );

    if( m_pCon->Error() )
    {
        SendErrorDiag( m_pCon );
        return( m_pSock->GetError() == TCPERR_NONE );
    }
    return( TRUE );
}

BOOL CODBTPThread::DoSETCOL( COdbtpQry* pQry )
{
    if( !pQry->SetColData( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoSETCURSOR( COdbtpQry* pQry )
{
    if( !pQry->SetCursor( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::DoSETPARAM( COdbtpQry* pQry )
{
    if( !pQry->SetParamData( m_pSock ) )
        return( DoQueryRequestError( pQry ) );
    return( TRUE );
}

BOOL CODBTPThread::Log( PCSTR pszText )
{
    return( m_pService->Log( "%s %s %lu",
                             pszText,
                             m_pSock->GetRemoteAddress(),
                             m_pSock->GetClientNumber() )
          );
}

BOOL CODBTPThread::LogError( PCSTR pszError )
{
    return( m_pService->Log( "ERROR[%s] %s %lu",
                             pszError,
                             m_pSock->GetRemoteAddress(),
                             m_pSock->GetClientNumber() )
          );
}

BOOL CODBTPThread::LogRequest()
{
    return( m_pService->Log( "READ[%02X %lu] %s %lu",
                             m_pSock->GetRequestCode(),
                             m_pSock->GetRequestSize(),
                             m_pSock->GetRemoteAddress(),
                             m_pSock->GetClientNumber() )
          );
}

BOOL CODBTPThread::LogResponse()
{
    return( m_pService->Log( "SENT[%02X %lu] %s %lu",
                             m_pSock->GetResponseCode(),
                             m_pSock->GetResponseSize(),
                             m_pSock->GetRemoteAddress(),
                             m_pSock->GetClientNumber() )
          );
}

BOOL CODBTPThread::LogSockError()
{
    return( LogError( m_pSock->GetErrorMessage() ) );
}

void CODBTPThread::Run( PSTR pszDBConnect )
{
    BOOL  bLogReadAndSent = m_pService->LogReadAndSent();
    BOOL  bOK = TRUE;
    ULONG ulRemainingReadTime = m_pService->m_dwReadTimeout;
    ULONG ulReqCode;

    if( m_pService->IsServer() )
    {
        if( !m_pSock->SendResponseText( ODBTP_OK, ODBTP_VERSION ) )
        {
            LogSockError();
            return;
        }
        if( bLogReadAndSent ) LogResponse();
    }
    if( pszDBConnect )
    {
        bOK = DoLOGIN( pszDBConnect );

        if( m_pSock->GetError() )
            LogSockError();
        else if( bLogReadAndSent )
            LogResponse();
    }
    while( bOK )
    {
        if( m_pService->m_dwReadTimeout )
        {
            if( !m_pSock->Wait(3) )
            {
                if( m_pService->IsStopping() )
                {
                    bOK = FALSE;
                }
                else if( ulRemainingReadTime <= 3 )
                {
                    if( m_pSock->GetError() == TCPERR_NONE )
                        m_pSock->SetError( TCPERR_TIMEOUT );
                    break;
                }
                else
                {
                    ulRemainingReadTime -= 3;
                }
                continue;
            }
            ulRemainingReadTime = m_pService->m_dwReadTimeout;
        }
        if( !m_pSock->ReadRequest() ) break;

        if( bLogReadAndSent ) LogRequest();

        if( m_pService->IsStopping() )
        {
            m_pSock->SendResponseText( ODBTP_UNAVAILABLE, "Service is stopping" );

            if( m_pSock->GetError() )
                LogSockError();
            else if( bLogReadAndSent )
                LogResponse();

            bOK = FALSE;
            break;
        }
        switch( (ulReqCode = m_pSock->GetRequestCode()) )
        {
            case ODBTP_LOGIN:     bOK = DoLOGIN(); break;
            case ODBTP_LOGOUT:    bOK = DoLOGOUT(); break;
            case ODBTP_GETCONNID: bOK = DoGETCONNID(); break;
            case ODBTP_GETATTR:   bOK = DoGETATTR(); break;
            case ODBTP_SETATTR:   bOK = DoSETATTR(); break;
            case ODBTP_COMMIT:    bOK = DoCOMMIT(); break;
            case ODBTP_ROLLBACK:  bOK = DoROLLBACK(); break;
            case ODBTP_CANCELREQ: bOK = DoCANCELREQ(); break;

            default:
                if( (ulReqCode & 0xE0) == 0x20 )
                    bOK = DoQueryRequest();
                else
                    bOK = m_pSock->SendResponseText( ODBTP_UNSUPPORTED );
        }
        if( m_bRanChild ) return;

        if( m_pSock->GetError() )
            LogSockError();
        else if( bLogReadAndSent )
            LogResponse();

        if( m_bGotODBCConnError ) bOK = FALSE;
    }
    if( m_pCon ) m_pService->Disconnect( m_pCon, TRUE );
    if( !bOK ) return;

    LogSockError();

    switch( m_pSock->GetError() )
    {
        case ODBTPERR_MEMORY:
            m_pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Memory allocation failed" );
            break;

        case ODBTPERR_REQUESTSIZE:
            m_pSock->SendResponseText( ODBTP_SYSTEMFAIL, "Request size too large" );
            break;

        case ODBTPERR_PROTOCOL:
            m_pSock->SendResponseText( ODBTP_VIOLATION, "Protocol violation" );
            break;

        default:
            return;
    }
    if( m_pSock->GetError() )
        LogSockError();
    else if( bLogReadAndSent )
        LogResponse();
}

BOOL CODBTPThread::SendDiag( USHORT usRCode, PCSTR pszState, long lCode, PCSTR pszText )
{
    char szStateCode[SQL_SQLSTATE_SIZE+32];

    if( usRCode == ODBTP_ERROR )
    {
        if( pszState && *pszState == '0' && *(pszState + 1) == '8' )
            m_bGotODBCConnError = TRUE;
        if( m_bGotODBCConnError ) usRCode = ODBTP_DISCONNECT;
    }
    if( !pszState )
    {
        m_bSentPrevDiag = FALSE;
        return( m_pSock->SendResponse( usRCode ) );
    }
    sprintf( szStateCode, "[%s][%li]", pszState, lCode );

    if( m_bSentPrevDiag && !m_pSock->SendResponse( usRCode, "\r\n", 2, FALSE ) )
        return( (m_bSentPrevDiag = FALSE) );
    if( !m_pSock->SendResponse( usRCode, szStateCode, strlen(szStateCode), FALSE ) )
        return( (m_bSentPrevDiag = FALSE) );
    if( !m_pSock->SendResponse( usRCode, (PVOID)pszText, strlen(pszText), FALSE ) )
        return( (m_bSentPrevDiag = FALSE) );

    return( (m_bSentPrevDiag = TRUE) );
}

BOOL CODBTPThread::SendDiagW( USHORT usRCode, const wchar_t* pszState, long lCode, const wchar_t* pszText )
{
    wchar_t szStateCode[SQL_SQLSTATE_SIZE+32];

    if( usRCode == ODBTP_ERROR )
    {
        if( pszState && *pszState == L'0' && *(pszState + 1) == L'8' )
            m_bGotODBCConnError = TRUE;
        if( m_bGotODBCConnError ) usRCode = ODBTP_DISCONNECT;
    }
    if( !pszState )
    {
        m_bSentPrevDiag = FALSE;
        return( m_pSock->SendResponse( usRCode ) );
    }
    swprintf( szStateCode, L"[%s][%li]", pszState, lCode );

    if( m_bSentPrevDiag && !m_pSock->SendResponseW( usRCode, L"\r\n", FALSE ) )
        return( (m_bSentPrevDiag = FALSE) );
    if( !m_pSock->SendResponseW( usRCode, szStateCode, FALSE ) )
        return( (m_bSentPrevDiag = FALSE) );
    if( !m_pSock->SendResponseW( usRCode, pszText, FALSE ) )
        return( (m_bSentPrevDiag = FALSE) );

    return( (m_bSentPrevDiag = TRUE) );
}

void CODBTPThread::SendDisconnectDiag( OdbHandle* pOdb )
{
    if( m_pCon->m_ulAttrUnicodeSQL )
        pOdb->DisplayDiagW( SendDisconnectDiagW );
    else
        pOdb->DisplayDiag( SendDisconnectDiag );
}

int CODBTPThread::SendDisconnectDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text )
{
    CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
    if( !pThread->SendDiag( ODBTP_DISCONNECT, State, Code, Text ) ) return( 0 );
    return( State ? -1 : 0 );
}

int CODBTPThread::SendDisconnectDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text )
{
    CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
    if( !pThread->SendDiagW( ODBTP_DISCONNECT, State, Code, Text ) ) return( 0 );
    return( State ? -1 : 0 );
}

void CODBTPThread::SendErrorDiag( OdbHandle* pOdb )
{
    if( m_pCon->m_ulAttrUnicodeSQL )
        pOdb->DisplayDiagW( SendErrorDiagW );
    else
        pOdb->DisplayDiag( SendErrorDiag );
}

int CODBTPThread::SendErrorDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text )
{
    CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
    if( !pThread->SendDiag( ODBTP_ERROR, State, Code, Text ) ) return( 0 );
    return( State ? -1 : 0 );
}

int CODBTPThread::SendErrorDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text )
{
    CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
    if( !pThread->SendDiagW( ODBTP_ERROR, State, Code, Text ) ) return( 0 );
    return( State ? -1 : 0 );
}

void CODBTPThread::SendMessageDiag( OdbHandle* pOdb )
{
    if( m_pCon->m_ulAttrUnicodeSQL )
        pOdb->DisplayDiagW( SendMessageDiagW );
    else
        pOdb->DisplayDiag( SendMessageDiag );
}

int CODBTPThread::SendMessageDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text )
{
    CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
    USHORT        usRCode;
    usRCode = pOdb->m_nHandleType == SQL_HANDLE_STMT ? ODBTP_QUERYOK : ODBTP_OK;
    if( !pThread->SendDiag( usRCode, State, Code, Text ) ) return( 0 );
    return( State ? -1 : 0 );
}

int CODBTPThread::SendMessageDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text )
{
    CODBTPThread* pThread = (CODBTPThread*)pOdb->GetUserData();
    USHORT        usRCode;
    usRCode = pOdb->m_nHandleType == SQL_HANDLE_STMT ? ODBTP_QUERYOK : ODBTP_OK;
    if( !pThread->SendDiagW( usRCode, State, Code, Text ) ) return( 0 );
    return( State ? -1 : 0 );
}

