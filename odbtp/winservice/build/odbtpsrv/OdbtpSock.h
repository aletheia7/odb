/* $Id: OdbtpSock.h,v 1.10 2005/01/01 01:04:47 rtwitty Exp $ */
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

#if !defined(AFX_ODBTPSOCK_H__B3A4722A_5397_11D6_812E_0050DA0B930B__INCLUDED_)
#define AFX_ODBTPSOCK_H__B3A4722A_5397_11D6_812E_0050DA0B930B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\TcpSock\TcpSock.h"
#include "..\odb\odb.h"

#define ODBTP_VERSION "ODBTP/1.1"

/* ODBTP Connection Level Request Codes */
#define ODBTP_LOGIN        0x01
#define ODBTP_LOGOUT       0x02
#define ODBTP_GETCONNID    0x03
#define ODBTP_GETATTR      0x04
#define ODBTP_SETATTR      0x05
#define ODBTP_COMMIT       0x06
#define ODBTP_ROLLBACK     0x07
#define ODBTP_CANCELREQ    0x1F

/* ODBTP Query Level Request Codes */
#define ODBTP_EXECUTE      0x20
#define ODBTP_PREPARE      0x21
#define ODBTP_BINDCOL      0x22
#define ODBTP_BINDPARAM    0x23
#define ODBTP_GETPARAM     0x24
#define ODBTP_SETPARAM     0x25
#define ODBTP_FETCHROW     0x26
#define ODBTP_FETCHRESULT  0x27
#define ODBTP_GETROWCOUNT  0x28
#define ODBTP_GETCOLINFO   0x29
#define ODBTP_GETPARAMINFO 0x2A
#define ODBTP_GETCURSOR    0x2B
#define ODBTP_SETCURSOR    0x2C
#define ODBTP_SETCOL       0x2D
#define ODBTP_ROWOP        0x2E
#define ODBTP_PREPAREPROC  0x2F
#define ODBTP_DROP         0x3F

/* ODBTP Connection Level Response Codes */
#define ODBTP_OK           0x80
#define ODBTP_CONNECTID    0x81
#define ODBTP_ATTRIBUTE    0x82
#define ODBTP_CANCELRESP   0x9F

/* ODBTP Query Level Response Codes */
#define ODBTP_QUERYOK      0xA0
#define ODBTP_COLINFO      0xA1
#define ODBTP_ROWDATA      0xA2
#define ODBTP_ROWCOUNT     0xA3
#define ODBTP_PARAMINFO    0xA4
#define ODBTP_PARAMDATA    0xA5
#define ODBTP_NODATA       0xA6
#define ODBTP_CURSOR       0xA7
#define ODBTP_ROWSTATUS    0xA8
#define ODBTP_COLINFOEX    0xA9
#define ODBTP_PARAMINFOEX  0xAA

/* ODBTP Error Response Codes */
#define ODBTP_ERROR        0xE0
#define ODBTP_UNSUPPORTED  0xE1
#define ODBTP_INVALID      0xE2

/* ODBTP Disconnect Response Codes */
#define ODBTP_DISCONNECT   0xF0
#define ODBTP_VIOLATION    0xF1
#define ODBTP_UNAVAILABLE  0xF2
#define ODBTP_MAXCONNECT   0xF3
#define ODBTP_NODBCONNECT  0xF4
#define ODBTP_SYSTEMFAIL   0xF5

// Error Codes
#define ODBTPERR_MEMORY       1001
#define ODBTPERR_PROTOCOL     1002
#define ODBTPERR_REQUESTSIZE  1003
#define ODBTPERR_REQUEST      1004
#define ODBTPERR_CLOSED       1005

class COdbtpSock : public CTcpSock
{
public:
	COdbtpSock();
	virtual ~COdbtpSock();

private:
    PVOID m_pRequestBuf;
    PVOID m_pTransBuf;
    PBYTE m_pbyExtractData;
    PBYTE m_pbyTransData;
    PCSTR m_pszRemoteAddress;
    ULONG m_ulClientNumber;
    ULONG m_ulExtractSize;
    ULONG m_ulLastResponseCode;
    ULONG m_ulMaxRequestSize;
    ULONG m_ulRequestBufSize;
    ULONG m_ulRequestCode;
    ULONG m_ulRequestSize;
    ULONG m_ulResponseCode;
    ULONG m_ulResponseSize;
    ULONG m_ulTransBufSize;
    ULONG m_ulTransSize;

public:
    BOOL  ExtractRequest( PBYTE pbyData, ULONG ulLen );
    BOOL  ExtractRequest( PBYTE* ppbyData, ULONG ulLen );
    BOOL  ExtractRequest( PBYTE pbyData );
    BOOL  ExtractRequest( PULONG pulData );
    BOOL  ExtractRequest( PUSHORT pusData );
    BOOL  ExtractRequest( float* pfData );
    BOOL  ExtractRequest( double* pdData );
    BOOL  ExtractRequest( unsigned __int64* pu64Data );
    BOOL  ExtractRequest( SQL_TIMESTAMP_STRUCT* ptsData );
    BOOL  ExtractRequest( SQLGUID* pguidData );
    ULONG GetClientNumber(){ return m_ulClientNumber; }
    PCSTR GetErrorMessage();
    PBYTE GetExtractRequestPtr(){ return m_pbyExtractData; }
    ULONG GetExtractRequestSize(){ return m_ulExtractSize; }
    PVOID GetRequest(){ return m_pRequestBuf; }
    PCSTR GetRemoteAddress(){ return m_pszRemoteAddress; }
    ULONG GetRequestCode(){ return m_ulRequestCode; }
    ULONG GetRequestSize(){ return m_ulRequestSize; }
    ULONG GetResponseCode(){ return m_ulLastResponseCode; }
    ULONG GetResponseSize(){ return m_ulResponseSize; }
    void  HostToNetworkOrder( PBYTE pbyTo, PBYTE pbyFrom, ULONG ulLen );
    BOOL  Init( ULONG ulTransBufSize, ULONG ulMaxRequestSize, ULONG ulReadTimeout );
    void  NetworkToHostOrder( PBYTE pbyTo, PBYTE pbyFrom, ULONG ulLen );
    BOOL  ReadData( PBYTE pbyData, ULONG ulBytesRequired );
    BOOL  ReadRequest();
    BOOL  SendResponse( USHORT usCode, PVOID pData = NULL, ULONG ulLen = 0, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, BYTE byData, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, ULONG ulData, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, USHORT usData, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, float fData, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, double dData, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, unsigned __int64 u64Data, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, SQL_TIMESTAMP_STRUCT* ptsData, BOOL bFinal = TRUE );
    BOOL  SendResponse( USHORT usCode, SQLGUID* pguidData, BOOL bFinal = TRUE );
    BOOL  SendResponseText( USHORT usCode, PCSTR pszText = NULL );
    BOOL  SendResponseW( USHORT usCode, const wchar_t* pszData, BOOL bFinal = TRUE );
    void  SetClientNumber( ULONG ulNumber ){ m_ulClientNumber = ulNumber; }
};

#endif // !defined(AFX_ODBTPSOCK_H__B3A4722A_5397_11D6_812E_0050DA0B930B__INCLUDED_)
