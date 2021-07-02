/* $Id: old_odbtpcli.c,v 1.1 2004/04/14 16:41:59 rtwitty Exp $ */
/*
    odbtpcli - Example ODBTP client program

    Copyright (C) 2002  Robert E. Twitty <rtwitty@users.sourceforge.net>

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
    set.  ODBTPCLI requires a file containing 3 lines of the following
    format to be specified on the command line when running the program.

    Line 1: Hostname of ODBTP server
    Line 2: ODBC connect string
    Line 3: Query string to be executed

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

int main( int argc, char* argv[] )
{
    odbHANDLE hCon;
    FILE*     file;
    odbHANDLE hQry;
    odbCHAR   szDbConnect[256];
    odbCHAR   szServer[128];
    odbCHAR   szSQL[512];
    odbUSHORT usCol, usTotalCols;
    odbUSHORT usLoginType = ODB_LOGIN_NORMAL;

    odbWinsockStartup(); /* Only required for Win32 clients. */

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
    if( !fgets( szSQL, sizeof(szSQL), file ) ) {
        fprintf( stderr, "Unable to read file %s\n", argv[1] );
        fclose( file );
        return 1;
    }
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
    fprintf( stderr, "Response Code: %02X\n", odbGetResponseCode( hCon ) );
    fprintf( stderr, "Response Size: %u\n", odbGetResponseSize( hCon ) );
    fprintf( stderr, "%s\n\n", odbGetResponse( hCon ) );

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
            printf( "Column Info\n" );
            for( usCol = 1; usCol <= usTotalCols; usCol++ ) {
                printf( "%s.%s.%s.%s(%s) %s.%s %04X\n",
                        odbColCatalog( hQry, usCol ),
                        odbColSchema( hQry, usCol ),
                        odbColTable( hQry, usCol ),
                        odbColName( hQry, usCol ),
                        odbColSqlTypeName( hQry, usCol ),
                        odbColBaseTable( hQry, usCol ),
                        odbColBaseName( hQry, usCol ),
                        odbColFlags( hQry, usCol ) );
            }
            printf( "\n\n" );
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
    }
    if( !odbLogout( hCon, usLoginType == ODB_LOGIN_RESERVED ) ) {
        fprintf( stderr, "Logut Error: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    odbFree( hCon );

    odbWinsockCleanup(); /* Only required for Win32 clients. */

    return 0;
}

odbBOOL FetchRows( odbHANDLE hQry, odbUSHORT usTotalCols )
{
    odbBOOL       bOK;
    odbPVOID      pData;
    odbCHAR       sz[32];
    odbPTIMESTAMP pts;
    odbUSHORT     usCol;
    odbULONG      ulRow = 0;

    while( (bOK = odbFetchRow( hQry )) && !odbNoData( hQry ) ) {
        printf( "[ROW %i]\n", ++ulRow );
        for( usCol = 1; usCol <= usTotalCols; usCol++ ) {
            if( !(pData = odbColData( hQry, usCol )) ) {
                printf( "NULL " );
                continue;
            }
            switch( odbColDataType( hQry, usCol ) ) {
                case ODB_BIT:
                    printf( "%u ", (odbULONG)*((odbPBYTE)pData) ); break;

                case ODB_TINYINT:
                    printf( "%i ", (odbLONG)*((odbPBYTE)pData) ); break;
                case ODB_UTINYINT:
                    printf( "%u ", (odbULONG)*((odbPBYTE)pData) ); break;

                case ODB_SMALLINT:
                    printf( "%i ", (odbLONG)*((odbPSHORT)pData) ); break;
                case ODB_USMALLINT:
                    printf( "%u ", (odbULONG)*((odbPUSHORT)pData) ); break;

                case ODB_INT:
                    printf( "%i ", *((odbPLONG)pData) ); break;
                case ODB_UINT:
                    printf( "%u ", *((odbPULONG)pData) ); break;

                case ODB_BIGINT:
                    printf( "%s ", odbLongLongToStr( *((odbPLONGLONG)pData), &sz[31] ) ); break;
                case ODB_UBIGINT:
                    printf( "%s ", odbULongLongToStr( *((odbPULONGLONG)pData), &sz[31] ) ); break;

                case ODB_REAL:
                    printf( "%g ", *((odbPFLOAT)pData) ); break;

                case ODB_DOUBLE:
                    printf( "%lg ", *((odbPDOUBLE)pData) ); break;

                case ODB_DATETIME:
                    pts = (odbPTIMESTAMP)pData;
                    printf( "%02i/%02i/%4i-%02i:%02i:%02i.%i ",
                            pts->usMonth, pts->usDay, pts->sYear,
                            pts->usHour, pts->usMinute, pts->usSecond,
                            pts->ulFraction );
                            break;

                default:
                    printf( "%s ", pData ); break;
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
