/* $Id: odbtpsrv.cpp,v 1.5 2004/06/02 20:12:21 rtwitty Exp $ */
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
// odbtpsrv.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "OdbtpSock.h"
#include "OdbSupport.h"
#include "ODBTPService.h"

static PCSTR g_pszProcType;
static char  g_szExceptionLog[MAX_PATH];

LONG WINAPI UnhandledExceptionTrap( LPEXCEPTION_POINTERS ExceptionInfo )
{
    static FILE* fexcept = NULL;

    if( !fexcept )
    {
        if( (fexcept = fopen( g_szExceptionLog, "a" )) )
        {
            SYSTEMTIME st;

            ::GetLocalTime( &st );
            fprintf( fexcept, "%02i/%02i/%i %02i:%02i:%02i (%s)\n",
                     st.wMonth, st.wDay, st.wYear,
                     st.wHour, st.wMinute, st.wSecond,
                     g_pszProcType );

            fclose( fexcept );
            fexcept = NULL;
        }
    }
    return( EXCEPTION_EXECUTE_HANDLER );
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Create the service object
    CODBTPService Service;

    ::GetModuleFileName( NULL, g_szExceptionLog, sizeof(g_szExceptionLog) );
    strcpy( strrchr( g_szExceptionLog, '\\' ) + 1, "exception.log" );
    ::SetUnhandledExceptionFilter( UnhandledExceptionTrap );

    if( lpCmdLine )
    {
        if( !strcmp( lpCmdLine, "child" ) )
        {
            g_pszProcType = "child";
            Service.ChildRun();
            return 0;
        }
        else if( !strcmp( lpCmdLine, "server" ) )
        {
            g_pszProcType = "server";
            Service.ServerRun();
            return 0;
        }
    }
    g_pszProcType = "service";

    Service.StartService();

    // When we get here, the service has been stopped
    return( Service.m_Status.dwWin32ExitCode );
}


