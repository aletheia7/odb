/* $Id: OdbtpSock.cpp,v 1.6 2005/01/01 01:04:47 rtwitty Exp $ */
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
// OdbtpSock.cpp: implementation of the COdbtpSock class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "..\TcpSock\TcpSock.h"
#include "OdbtpSock.h"
#include "UTF8Support.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COdbtpSock::COdbtpSock()
{
    m_pTransBuf = NULL;
    m_pRequestBuf = NULL;
    m_ulClientNumber = 0;
}

COdbtpSock::~COdbtpSock()
{
    if( m_pRequestBuf ) free( m_pRequestBuf );
    if( m_pTransBuf ) free( m_pTransBuf );
}

BOOL COdbtpSock::ExtractRequest( PBYTE pbyData, ULONG ulLen )
{
    if( ulLen > 0 )
    {
        if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
            return( SetError( ODBTPERR_REQUEST ) );

        memcpy( pbyData, m_pbyExtractData, ulLen );
        m_pbyExtractData += ulLen;
        m_ulExtractSize -= ulLen;
    }
    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( PBYTE* ppbyData, ULONG ulLen )
{
    if( ulLen > 0 )
    {
        if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
            return( SetError( ODBTPERR_REQUEST ) );

        *ppbyData = m_pbyExtractData;
        m_pbyExtractData += ulLen;
        m_ulExtractSize -= ulLen;
    }
    else
    {
        *ppbyData = m_pbyExtractData;
    }
    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( PBYTE pbyData )
{
    if( m_ulExtractSize == 0 || m_ulExtractSize < 1 )
        return( SetError( ODBTPERR_REQUEST ) );

    *pbyData = *m_pbyExtractData;
    m_pbyExtractData += 1;
    m_ulExtractSize -= 1;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( PULONG pulData )
{
    ULONG ul;
    ULONG ulLen = sizeof(ULONG);

    if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
        return( SetError( ODBTPERR_REQUEST ) );

    memcpy( &ul, m_pbyExtractData, ulLen );
    *pulData = ntohl( ul );
    m_pbyExtractData += ulLen;
    m_ulExtractSize -= ulLen;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( PUSHORT pusData )
{
    ULONG  ulLen = sizeof(USHORT);
    USHORT us;

    if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
        return( SetError( ODBTPERR_REQUEST ) );

    memcpy( &us, m_pbyExtractData, ulLen );
    *pusData = ntohs( us );
    m_pbyExtractData += ulLen;
    m_ulExtractSize -= ulLen;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( float* pfData )
{
    ULONG ulLen = sizeof(float);

    if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
        return( SetError( ODBTPERR_REQUEST ) );

    NetworkToHostOrder( (PBYTE)pfData, m_pbyExtractData, ulLen );
    m_pbyExtractData += ulLen;
    m_ulExtractSize -= ulLen;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( double* pdData )
{
    ULONG ulLen = sizeof(double);

    if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
        return( SetError( ODBTPERR_REQUEST ) );

    NetworkToHostOrder( (PBYTE)pdData, m_pbyExtractData, ulLen );
    m_pbyExtractData += ulLen;
    m_ulExtractSize -= ulLen;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( unsigned __int64* pu64Data )
{
    ULONG ulLen = sizeof(unsigned __int64);

    if( m_ulExtractSize == 0 || m_ulExtractSize < ulLen )
        return( SetError( ODBTPERR_REQUEST ) );

    NetworkToHostOrder( (PBYTE)pu64Data, m_pbyExtractData, ulLen );
    m_pbyExtractData += ulLen;
    m_ulExtractSize -= ulLen;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ExtractRequest( SQL_TIMESTAMP_STRUCT* ptsData )
{
    if( !ExtractRequest( (PUSHORT)&ptsData->year ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&ptsData->month ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&ptsData->day ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&ptsData->hour ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&ptsData->minute ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&ptsData->second ) ) return( FALSE );
    return( ExtractRequest( (PULONG)&ptsData->fraction ) );
}

BOOL COdbtpSock::ExtractRequest( SQLGUID* pguidData )
{
    if( !ExtractRequest( (PULONG)&pguidData->Data1 ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&pguidData->Data2 ) ) return( FALSE );
    if( !ExtractRequest( (PUSHORT)&pguidData->Data3 ) ) return( FALSE );
    return( ExtractRequest( (PBYTE)pguidData->Data4, 8 ) );
}

PCSTR COdbtpSock::GetErrorMessage()
{
    switch( GetError() )
    {
        case ODBTPERR_MEMORY:       return( "Unable to allocate memory" );
        case ODBTPERR_PROTOCOL:     return( "Protocol Violation" );
        case ODBTPERR_REQUESTSIZE:  return( "Request size too large" );
        case ODBTPERR_REQUEST:      return( "Invalid Request" );
        case ODBTPERR_CLOSED  :     return( "Connection closed" );
    }
    return( CTcpSock::GetErrorMessage() );
}

void COdbtpSock::HostToNetworkOrder( PBYTE pbyTo, PBYTE pbyFrom, ULONG ulLen )
{
    pbyFrom += ulLen - 1;
    for( ULONG n = 0; n < ulLen; n++ ) *(pbyTo++) = *(pbyFrom--);
}

BOOL COdbtpSock::Init( ULONG ulTransBufSize, ULONG ulMaxRequestSize, ULONG ulReadTimeout )
{
    if( !(m_pszRemoteAddress = GetPeerAddress()) ) return( FALSE );

    m_ulTransSize = 0;
    m_ulTransBufSize = ulTransBufSize;
    if( !(m_pTransBuf = (PVOID)malloc( m_ulTransBufSize )) )
        return( SetError( ODBTPERR_MEMORY ) );

    m_ulRequestSize = 0;
    m_ulMaxRequestSize = ulMaxRequestSize;
    if( m_ulMaxRequestSize > 0x00020000 )
        m_ulRequestBufSize = 0x00020000;
    else
        m_ulRequestBufSize = m_ulMaxRequestSize;
    if( !(m_pRequestBuf = (PVOID)malloc( m_ulRequestBufSize )) )
    {
        free( m_pTransBuf );
        m_pTransBuf = NULL;
        return( SetError( ODBTPERR_MEMORY ) );
    }
    SetReadTimeout( ulReadTimeout );

    m_ulExtractSize = 0;
    m_ulRequestCode = 0;
    m_ulRequestSize = 0;
    m_ulResponseCode = 0;
    m_ulResponseSize = 0;
    m_ulLastResponseCode = 0;

    return( SetError( TCPERR_NONE ) );
}

void COdbtpSock::NetworkToHostOrder( PBYTE pbyTo, PBYTE pbyFrom, ULONG ulLen )
{
    pbyFrom += ulLen - 1;
    for( ULONG n = 0; n < ulLen; n++ ) *(pbyTo++) = *(pbyFrom--);
}

BOOL COdbtpSock::ReadData( PBYTE pbyData, ULONG ulBytesRequired )
{
    int   nRead;
    ULONG ul;

    for( ul = 0; ul < ulBytesRequired; ul++ )
    {
        if( m_ulTransSize == 0 )
        {
            if( (nRead = Read( m_pTransBuf, m_ulTransBufSize )) <= 0 )
            {
                if( nRead == 0 ) SetError( ODBTPERR_CLOSED );
                return( FALSE );
            }
            m_ulTransSize = (ULONG)nRead;
            m_pbyTransData = (PBYTE)m_pTransBuf;
        }
        *(pbyData++) = *(m_pbyTransData++);
        m_ulTransSize--;
    }
    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::ReadRequest()
{
    BYTE   byCode;
    BYTE   byFlag;
    PBYTE  pbyRequestData;
    ULONG  ulSize;
    USHORT usSize;

    byFlag = 0;
    m_ulRequestCode = 0;
    m_ulExtractSize = 0;
    m_ulTransSize = 0;

    while( byFlag == 0 )
    {
        if( !ReadData( &byCode, 1 ) ) return( FALSE );
        if( !ReadData( &byFlag, 1 ) ) return( FALSE );
        if( !ReadData( (PBYTE)&usSize, 2 ) ) return( FALSE );
        ulSize = (ULONG)ntohs( usSize );

        if( byCode == 0 || (byFlag & 0xFF) > 0x01 )
            return( SetError( ODBTPERR_PROTOCOL ) );

        if( m_ulRequestCode != (ULONG)byCode )
        {
            m_ulRequestCode = byCode;
            pbyRequestData = (PBYTE)m_pRequestBuf;
            m_ulRequestSize = 0;
        }
        if( ulSize > 0 )
        {
            m_ulRequestSize += ulSize;

            if( m_ulRequestSize >= m_ulRequestBufSize )
            {
                if( m_ulRequestSize > m_ulMaxRequestSize )
                    return( SetError( ODBTPERR_REQUESTSIZE ) );
                m_ulRequestBufSize =
                  m_ulRequestSize + 2048 - (m_ulRequestSize % 1024);
                if( m_ulRequestBufSize > m_ulMaxRequestSize )
                    m_ulRequestBufSize = m_ulMaxRequestSize;
                m_pRequestBuf = (PVOID)realloc( m_pRequestBuf, m_ulRequestBufSize );
                if( !m_pRequestBuf ) return( SetError( ODBTPERR_MEMORY ) );
                pbyRequestData = (PBYTE)m_pRequestBuf;
                pbyRequestData += m_ulRequestSize - ulSize;
            }
            if( !ReadData( pbyRequestData, ulSize ) ) return( FALSE );
            pbyRequestData += ulSize;
        }
    }
    *pbyRequestData = 0;
    m_ulExtractSize = m_ulRequestSize;
    m_pbyExtractData = (PBYTE)m_pRequestBuf;

    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, PVOID pData, ULONG ulLen, BOOL bFinal )
{
    PBYTE  pby = (PBYTE)pData;
    PBYTE  pbyTrans = (PBYTE)m_pTransBuf;
    ULONG  ul;
    USHORT usSize;

    if( m_ulResponseCode != (ULONG)usCode )
    {
        m_ulResponseCode = usCode;
        m_ulLastResponseCode = usCode;
        *pbyTrans = (BYTE)usCode;
        *(pbyTrans + 1) = 0;
        m_ulTransSize = 4;
        m_ulResponseSize = 0;
    }
    for( ul = 0; ul < ulLen; ul++, pby++, m_ulTransSize++ )
    {
        if( m_ulTransSize == m_ulTransBufSize )
        {
            usSize = htons( (USHORT)(m_ulTransSize - 4) );
            memcpy( pbyTrans + 2, &usSize, 2 );
            if( Send( m_pTransBuf, m_ulTransSize ) != (int)m_ulTransSize )
                return( FALSE );
            m_ulResponseSize += (m_ulTransSize - 4);
            m_ulTransSize = 4;
        }
        *(pbyTrans + m_ulTransSize) = *pby;
    }
    if( bFinal )
    {
        *(pbyTrans + 1) = 0x01;
        usSize = htons( (USHORT)(m_ulTransSize - 4) );
        memcpy( pbyTrans + 2, &usSize, 2 );
        if( Send( m_pTransBuf, m_ulTransSize ) != (int)m_ulTransSize )
            return( FALSE );

        m_ulResponseSize += (m_ulTransSize - 4);
        m_ulResponseCode = 0;
    }
    return( SetError( TCPERR_NONE ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, BYTE byData, BOOL bFinal )
{
    return( SendResponse( usCode, &byData, 1, bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, ULONG ulData, BOOL bFinal )
{
    ulData = htonl( ulData );
    return( SendResponse( usCode, &ulData, sizeof(ULONG), bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, USHORT usData, BOOL bFinal )
{
    usData = htons( usData );
    return( SendResponse( usCode, &usData, sizeof(USHORT), bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, float fData, BOOL bFinal )
{
    float f;
    HostToNetworkOrder( (PBYTE)&f, (PBYTE)&fData, sizeof(float) );
    return( SendResponse( usCode, &f, sizeof(float), bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, double dData, BOOL bFinal )
{
    double d;
    HostToNetworkOrder( (PBYTE)&d, (PBYTE)&dData, sizeof(double) );
    return( SendResponse( usCode, &d, sizeof(double), bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, unsigned __int64 u64Data, BOOL bFinal )
{
    unsigned __int64 u64;
    HostToNetworkOrder( (PBYTE)&u64, (PBYTE)&u64Data, sizeof(unsigned __int64) );
    return( SendResponse( usCode, &u64, sizeof(unsigned __int64), bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, SQL_TIMESTAMP_STRUCT* ptsData, BOOL bFinal )
{
    if( !SendResponse( usCode, (USHORT)ptsData->year, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)ptsData->month, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)ptsData->day, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)ptsData->hour, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)ptsData->minute, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)ptsData->second, FALSE ) ) return( FALSE );
    return( SendResponse( usCode, (ULONG)ptsData->fraction, bFinal ) );
}

BOOL COdbtpSock::SendResponse( USHORT usCode, SQLGUID* pguidData, BOOL bFinal )
{
    if( !SendResponse( usCode, (ULONG)pguidData->Data1, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)pguidData->Data2, FALSE ) ) return( FALSE );
    if( !SendResponse( usCode, (USHORT)pguidData->Data3, FALSE ) ) return( FALSE );
    return( SendResponse( usCode, (PVOID)pguidData->Data4, 8, bFinal ) );
}

BOOL COdbtpSock::SendResponseText( USHORT usCode, PCSTR pszText )
{
    PCSTR pszStateCode;

    if( !pszText )
    {
        switch( usCode )
        {
            case ODBTP_OK:          pszText = "OK"; break;
            case ODBTP_ERROR:       pszText = "Error"; break;
            case ODBTP_UNSUPPORTED: pszText = "Unsupported"; break;
            case ODBTP_INVALID:     pszText = "Invalid"; break;
            case ODBTP_DISCONNECT:  pszText = "Disconnected"; break;
            case ODBTP_VIOLATION:   pszText = "Violation"; break;
            case ODBTP_UNAVAILABLE: pszText = "Unavailable"; break;
            case ODBTP_MAXCONNECT:  pszText = "Too many connections"; break;
            case ODBTP_NODBCONNECT: pszText = "DB connection not available"; break;
            case ODBTP_SYSTEMFAIL:  pszText = "System Failure"; break;

            default: pszText = "";
        }
    }
    if( (usCode & ODBTP_DISCONNECT) == ODBTP_DISCONNECT )
        pszStateCode = "[ODBTP][2]";
    else if( (usCode & ODBTP_ERROR) == ODBTP_ERROR )
        pszStateCode = "[ODBTP][1]";
    else
        pszStateCode = "[ODBTP][0]";

    if( !SendResponse( usCode, (PVOID)pszStateCode, strlen(pszStateCode), FALSE ) )
        return( FALSE );

    return( SendResponse( usCode, (PVOID)pszText, strlen(pszText), TRUE ) );
}

BOOL COdbtpSock::SendResponseW( USHORT usCode, const wchar_t* pszData, BOOL bFinal )
{
    int  b;
    char sz[264];

    for( b = 0; *pszData; pszData++ )
    {
        if( b >= 256 )
        {
            if( !SendResponse( usCode, sz, b, FALSE ) ) return( FALSE );
            b = 0;
        }
        b += UTF8Encode( &sz[b], *pszData );
    }
    if( b != 0 ) return( SendResponse( usCode, sz, b, FALSE ) );
    return( SendResponse( usCode, NULL, 0, bFinal ) );
}

