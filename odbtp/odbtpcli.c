/* $Id: odbtpcli.c,v 1.17 2005/01/01 00:48:48 rtwitty Exp $ */
/*
    odbtpcli - Example ODBTP client program

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
/*
    ODBTPCLI is a general purpose ODBTP client that demonstrates how
    to execute a query and fetch rows if the query generates a result
    set.  ODBTPCLI requires a file containing 3 or more lines of the
    following format to be specified on the command line when running
    the program.

    Line 1: Hostname of ODBTP server
    Line 2: ODBC connect string
    Line 3: Line 1 of SQL query
     .
     .
     .
    Line N: Line N - 2 of SQL query

    Example file contents:

    odbtp.somewhere.com
    DRIVER={SQL Server};SERVER=sqlsrvr;UID=myuid;PWD=mypwd;DATABASE=master;
    EXEC sp_who
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "odbtp.h"

odbBOOL FetchRows( odbHANDLE hQry, odbUSHORT usTotalCols );
void    RightTrim( odbPSTR pszText );
void    PrintTextData( odbPCSTR pszText, odbLONG lLen );
void    PrintBinaryData( odbPBYTE pbyData, odbLONG lLen );

int main( int argc, char* argv[] )
{
    odbHANDLE hCon;
    FILE*     file;
    odbHANDLE hQry;
    odbLONG   l;
    odbPSTR   psz;
    odbCHAR   szDbConnect[256];
    odbCHAR   szServer[128];
    odbCHAR   szSQL[0x20000];
    odbUSHORT usCol, usTotalCols;
    odbUSHORT usLoginType = ODB_LOGIN_NORMAL;

#ifndef ODBTP_DLL
    odbWinsockStartup(); /* Only required for Win32 clients. */
#endif

    if( argc < 2 ) {
        fprintf( stderr, "Syntax: odbtpcli connect_file\n" );
        return 1;
    }
    if( !(file = fopen( argv[1], "r" )) ) {
        fprintf( stderr, "Unable to open %s\n", argv[1] );
        return 1;
    }
    if( !fgets( szServer, sizeof(szServer), file ) ) {
        fprintf( stderr, "Unable to read file %s\n", argv[1] );
        fclose( file );
        return 1;
    }
    if( !fgets( szDbConnect, sizeof(szDbConnect), file ) ) {
        fprintf( stderr, "Unable to read file %s\n", argv[1] );
        fclose( file );
        return 1;
    }
    psz = szSQL;
    l = sizeof(szSQL);

    for( fgets( psz, l, file ); !feof( file ); fgets( psz, l, file ) ) {
        l -= strlen(psz);
        psz += strlen(psz);
    }
    *psz = 0;

    fclose( file );
    RightTrim( szServer );
    RightTrim( szDbConnect );
    RightTrim( szSQL );

    if( !(hCon = odbAllocate(NULL)) ) {
        fprintf( stderr, "Unable to allocate\n" );
        return 1;
    }
    if( !odbLogin( hCon, szServer, 2799, usLoginType, szDbConnect ) ) {
        fprintf( stderr, "Login Failed: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    fprintf( stderr, "Version: %s\n\n", odbGetVersion( hCon ) );

    if( !odbSetAttrLong( hCon, ODB_ATTR_FULLCOLINFO, 1 ) ) {
        fprintf( stderr, "Set Attribute failed: %s\n", odbGetErrorText( hCon ) );
        odbLogout( hCon, TRUE );
        odbFree( hCon );
        return 1;
    }
    if( !(hQry = odbAllocate( hCon )) ) {
        fprintf( stderr, "Qry Alloc Failed: %s\n", odbGetErrorText( hCon ) );
        odbLogout( hCon, TRUE );
        odbFree( hCon );
        return 1;
    }
    if( !odbExecute( hQry, szSQL ) ) {
        fprintf( stderr, "Execute Failed: %s\n", odbGetErrorText( hQry ) );
        odbLogout( hCon, TRUE );
        odbFree( hCon );
        return 1;
    }
    while( !odbNoData( hQry ) ) {
        if( (usTotalCols = odbGetTotalCols( hQry )) > 0 ) {
            for( usCol = 1, l = 1; usCol <= usTotalCols; usCol++ ) {
                if( usCol > 1 ) printf( "," );
                psz = (odbPSTR)odbColName( hQry, usCol );
                if( *psz )
                    PrintTextData( psz, -1 );
                else
                    printf( "\"computed%d\"", l++ );
            }
            printf( "\n" );
            if( !FetchRows( hQry, usTotalCols ) ) {
                fprintf( stderr, "FetchRows Failed: %s\n", odbGetErrorText( hQry ) );
                odbLogout( hCon, TRUE );
                odbFree( hCon );
                return 1;
            }
        }
        else if( odbGetResponseCode( hQry ) == ODBTP_ROWCOUNT ) {
            printf( "%i rows affected.\n", odbGetRowCount( hQry ) );
        }
        else {
            printf( "MESSAGE: %s\n", odbGetResponse( hQry ) );
        }
        if( !odbFetchNextResult( hQry ) ) {
            fprintf( stderr, "FetchNextResult Failed: %s\n", odbGetErrorText( hQry ) );
            odbLogout( hCon, TRUE );
            odbFree( hCon );
            return 1;
        }
        else if( !odbNoData( hQry ) ) {
            printf( "<<<NEXT RESULT>>>\n" );
        }
    }
    if( !odbLogout( hCon, usLoginType == ODB_LOGIN_RESERVED ) ) {
        fprintf( stderr, "Logut Error: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    odbFree( hCon );

#ifndef ODBTP_DLL
    odbWinsockCleanup(); /* Only required for Win32 clients. */
#endif

    return 0;
}

odbBOOL FetchRows( odbHANDLE hQry, odbUSHORT usTotalCols )
{
    odbBOOL       bOK;
    odbPVOID      pData;
    odbCHAR       sz[48];
    odbUSHORT     usCol;

    while( (bOK = odbFetchRow( hQry )) && !odbNoData( hQry ) ) {
        for( usCol = 1; usCol <= usTotalCols; usCol++ ) {
            if( odbColTruncated( hQry, usCol ) ) {
                fprintf( stderr,
                         "Warning: Col %u was truncated. Actual size is %i.\n",
                         usCol, odbColActualLen( hQry, usCol ) );
            }
            if( usCol > 1 ) printf( "," );
            if( !(pData = odbColData( hQry, usCol )) ) {
                printf( "NULL" );
                continue;
            }
            switch( odbColDataType( hQry, usCol ) ) {
                case ODB_BIT:
                    printf( "%u", (odbULONG)*((odbPBYTE)pData) ); break;

                case ODB_TINYINT:
                    printf( "%i", (odbLONG)*((odbPBYTE)pData) ); break;
                case ODB_UTINYINT:
                    printf( "%u", (odbULONG)*((odbPBYTE)pData) ); break;

                case ODB_SMALLINT:
                    printf( "%i", (odbLONG)*((odbPSHORT)pData) ); break;
                case ODB_USMALLINT:
                    printf( "%u", (odbULONG)*((odbPUSHORT)pData) ); break;

                case ODB_INT:
                    printf( "%i", *((odbPLONG)pData) ); break;
                case ODB_UINT:
                    printf( "%u", *((odbPULONG)pData) ); break;

                case ODB_BIGINT:
                    printf( "%s", odbLongLongToStr( *((odbPLONGLONG)pData), &sz[31] ) ); break;
                case ODB_UBIGINT:
                    printf( "%s", odbULongLongToStr( *((odbPULONGLONG)pData), &sz[31] ) ); break;

                case ODB_REAL:
                    printf( "%g", *((odbPFLOAT)pData) ); break;

                case ODB_DOUBLE:
                    printf( "%lg", *((odbPDOUBLE)pData) ); break;

                case ODB_DATETIME:
                    printf( "%s", odbTimestampToStr( sz, (odbPTIMESTAMP)pData, TRUE ) ); break;

                case ODB_GUID:
                    printf( "%s", odbGuidToStr( sz, (odbPGUID)pData ) ); break;

                case ODB_CHAR:
                case ODB_WCHAR:
                    PrintTextData( (odbPCSTR)pData,
                                   odbColDataLen( hQry, usCol ) );
                    break;

                default:
                    PrintBinaryData( (odbPBYTE)pData,
                                     odbColDataLen( hQry, usCol ) );
            }
        }
        printf( "\n" );
    }
    return( bOK );
}

void RightTrim( odbPSTR pszText )
{
    odbPSTR psz = pszText + strlen(pszText) - 1;
    for( ; *psz <= ' ' &&  psz >= pszText; psz-- ) *psz = 0;
}

void PrintTextData( odbPCSTR pszText, odbLONG lLen )
{
    if( lLen < 0 ) lLen = strlen(pszText);

    putc( '\"', stdout );
    for( ; lLen > 0; lLen--, pszText++ ) {
        putc( *pszText, stdout );
        if( *pszText == '\"' ) putc( '\"', stdout );
    }
    putc( '\"', stdout );
}

void PrintBinaryData( odbPBYTE pbyData, odbLONG lLen )
{
    odbPCSTR pszHex = "0123456789ABCDEF";

    if( lLen <= 0 ) return;

    putc( '0', stdout );
    putc( 'x', stdout );

    for( ; lLen > 0; lLen--, pbyData++ ) {
        putc( *(pszHex + ((*pbyData >> 4) & 0x0F)), stdout );
        putc( *(pszHex + (*pbyData & 0x0F)), stdout );
    }
}
