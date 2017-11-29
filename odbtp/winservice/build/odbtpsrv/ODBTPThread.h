/* $Id: ODBTPThread.h,v 1.8 2004/06/02 20:12:21 rtwitty Exp $ */
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
// ODBTPThread.h: interface for the CODBTPThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBTPTHREAD_H__803AD892_4D62_11D6_812D_0050DA0B930B__INCLUDED_)
#define AFX_ODBTPTHREAD_H__803AD892_4D62_11D6_812D_0050DA0B930B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CODBTPThread
{
public:
	CODBTPThread( CODBTPService* pService, COdbtpSock* pSock );
	virtual ~CODBTPThread();

public:
    COdbtpSock* m_pSock;

private:
    BOOL           m_bGotODBCConnError;
    BOOL           m_bSentPrevDiag;
    BOOL           m_bRanChild;
    COdbtpCon*     m_pCon;
    CODBTPService* m_pService;

public:
    BOOL DoBINDCOL( COdbtpQry* pQry );
    BOOL DoBINDPARAM( COdbtpQry* pQry );
    BOOL DoCANCELREQ();
    BOOL DoCOMMIT();
    BOOL DoDROP( COdbtpQry* pQry );
    BOOL DoEXECUTE( COdbtpQry* pQry );
    BOOL DoFETCHRESULT( COdbtpQry* pQry );
    BOOL DoFETCHROW( COdbtpQry* pQry );
    BOOL DoGETATTR();
    BOOL DoGETCOLINFO( COdbtpQry* pQry );
    BOOL DoGETCONNID();
    BOOL DoGETCURSOR( COdbtpQry* pQry );
    BOOL DoGETPARAM( COdbtpQry* pQry );
    BOOL DoGETPARAMINFO( COdbtpQry* pQry );
    BOOL DoGETROWCOUNT( COdbtpQry* pQry );
    BOOL DoLOGIN( PSTR pszDBConnect = NULL );
    BOOL DoLOGOUT();
    BOOL DoPREPARE( COdbtpQry* pQry );
    BOOL DoPREPAREPROC( COdbtpQry* pQry );
    BOOL DoQueryRequest();
    BOOL DoQueryRequestError( COdbtpQry* pQry );
    BOOL DoROLLBACK();
    BOOL DoROWOP( COdbtpQry* pQry );
    BOOL DoSETATTR();
    BOOL DoSETCOL( COdbtpQry* pQry );
    BOOL DoSETCURSOR( COdbtpQry* pQry );
    BOOL DoSETPARAM( COdbtpQry* pQry );
    BOOL Log( PCSTR pszText );
    BOOL LogError( PCSTR pszError );
    BOOL LogRequest();
    BOOL LogResponse();
    BOOL LogSockError();
    void Run( PSTR pszDBConnect = NULL );
    BOOL SendDiag( USHORT usRCode, PCSTR pszState, long lCode, PCSTR pszText );
    BOOL SendDiagW( USHORT usRCode, const wchar_t* pszState, long lCode, const wchar_t* pszText );
    void SendDisconnectDiag( OdbHandle* pOdb );
    void SendErrorDiag( OdbHandle* pOdb );
    void SendMessageDiag( OdbHandle* pOdb );

// Odb Diagnostic message handlers
    static int SendDisconnectDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text );
    static int SendDisconnectDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text );
    static int SendErrorDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text );
    static int SendErrorDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text );
    static int SendMessageDiag( OdbHandle* pOdb, const char* State, SQLINTEGER Code, const char* Text );
    static int SendMessageDiagW( OdbHandle* pOdb, const wchar_t* State, SQLINTEGER Code, const wchar_t* Text );
};

#endif // !defined(AFX_ODBTPTHREAD_H__803AD892_4D62_11D6_812D_0050DA0B930B__INCLUDED_)
