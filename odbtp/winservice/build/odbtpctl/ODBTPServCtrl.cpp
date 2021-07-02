/* $Id: ODBTPServCtrl.cpp,v 1.11 2004/08/04 01:08:08 rtwitty Exp $ */
/*
    odbtpctl - ODBTP service controller

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
// ODBTPServCtrl.cpp: implementation of the CODBTPServCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ODBTPServCtrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CODBTPServCtrl::CODBTPServCtrl()
    : CNTServCtrl( "ODBTPServer", "ODBTP Server", "odbtpsrv.exe" )
{

}

CODBTPServCtrl::~CODBTPServCtrl()
{

}

BOOL CODBTPServCtrl::OnInstall()
{
    DWORD dwData;
    HKEY  hKey = NULL;
    PSTR  pszFile;
    char  szBuf[4096];

    wsprintf( szBuf,
              "SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters",
              m_szServiceName );

    if( ::RegCreateKey( HKEY_LOCAL_MACHINE, szBuf, &hKey ) != ERROR_SUCCESS )
        return( Error( "Unable to create %s registry key.", szBuf ) );


    ::GetModuleFileName( NULL, szBuf, sizeof(szBuf) );
    pszFile = strrchr( szBuf, '\\' ) + 1;

    strcpy( pszFile, "odbtpsrv.log" );

    ::RegSetValueEx( hKey,
                     "LogFile",
                     0,
                     REG_SZ,
                     (CONST BYTE*)szBuf,
                     strlen(szBuf) + 1 );

    dwData = 1;

    ::RegSetValueEx( hKey,
                     "EnableLog",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 0;

    ::RegSetValueEx( hKey,
                     "LogReadAndSent",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD));

    dwData = 0;

    ::RegSetValueEx( hKey,
                     "LogODBC",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD));

    dwData = 2799;

    ::RegSetValueEx( hKey,
                     "Port",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 32;

    ::RegSetValueEx( hKey,
                     "ListenBacklog",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 256;

    ::RegSetValueEx( hKey,
                     "MaxClientThreads",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 32;

    ::RegSetValueEx( hKey,
                     "MaxActiveClientThreads",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 60;

    ::RegSetValueEx( hKey,
                     "ClientThreadWaitTimeout",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 180;

    ::RegSetValueEx( hKey,
                     "ReadTimeout",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 4096;

    ::RegSetValueEx( hKey,
                     "TransBufferSize",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 2000000;

    ::RegSetValueEx( hKey,
                     "MaxRequestSize",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 32;

    ::RegSetValueEx( hKey,
                     "ConnPoolSize",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 300;

    ::RegSetValueEx( hKey,
                     "ConnPoolTimeout",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD));

    dwData = 24;

    ::RegSetValueEx( hKey,
                     "ReservConnPoolSize",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 3600;

    ::RegSetValueEx( hKey,
                     "ReservConnPoolTimeout",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD));

    dwData = 0;

    ::RegSetValueEx( hKey,
                     "BadConnCheck",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 4;

    ::RegSetValueEx( hKey,
                     "MaxQrysPerConn",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD));

    dwData = 0;

    ::RegSetValueEx( hKey,
                     "ConnectTimeout",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 0;

    ::RegSetValueEx( hKey,
                     "QueryTimeout",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 64;

    ::RegSetValueEx( hKey,
                     "FetchRowCount",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    dwData = 0;

    ::RegSetValueEx( hKey,
                     "EnableProcCache",
                     0,
                     REG_DWORD,
                     (CONST BYTE*)&dwData,
                     sizeof(DWORD) );

    ::RegCloseKey( hKey );

    return TRUE;
}

void CODBTPServCtrl::OnCommand( PCSTR pszCmd )
{
    if( stricmp( pszCmd, "help" ) )
        fprintf( stderr, "\"%s\" is an invalid command.\n\n", pszCmd );

    fprintf( stderr,
             "Syntax: odbtpctl [control_cmd]\n"
             "  Where control_cmd can be one of the following:\n\n"
             "    help - display this message\n"
             "    install - install service\n"
             "    uninstall - uinstall service\n"
             "    start - start the service\n"
             "    stop - stop the service\n"
             "    restart - restart the service\n"
             "    status - display status of service\n"
             "    version - display version info\n" );
}

void CODBTPServCtrl::OnError( PCSTR pszError )
{
    fprintf( stderr, "%s\n", pszError );
}

void CODBTPServCtrl::OnMessage( PCSTR pszMessage )
{
    fprintf( stderr, "%s\n", pszMessage );
}

