/*
    INITINTS initializes a table that was created as follows.

    CREATE TABLE dbo.TheInts (
        Id int IDENTITY (1, 1) NOT NULL ,
        TheTinyInt tinyint NOT NULL ,
        TheSmallInt smallint NOT NULL ,
        TheInt int NOT NULL ,
        TheBigInt bigint NOT NULL ,
        CONSTRAINT PKCL_TheInts_Id PRIMARY KEY  CLUSTERED (
            Id
        )
    )

    This program demonstrates how to prepare a query and pass parameter
    values prior to its execution.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <odbtp.h>

int main( int argc, char* argv[] )
{
    odbBYTE      byVal;
    odbHANDLE    hCon;
    odbHANDLE    hQry;
    int          i, s;
    odbULONG     ulVal;
    odbULONGLONG ullVal;
    odbUSHORT    usVal;

    odbWinsockStartup(); /* Only required for Win32 clients. */

    if( !(hCon = odbAllocate(NULL)) ) {
        fprintf( stderr, "Unable to allocate\n" );
        return 1;
    }
    if( !odbLogin( hCon, "odbtp.somewhere.com", 2799, ODB_LOGIN_NORMAL,
                   "DRIVER={SQL Server};SERVER=myserver;UID=myuid;PWD=mypwd;DATABASE=theintsdb;" ) )
    {
        fprintf( stderr, "Login Failed: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    if( !(hQry = odbAllocate( hCon )) ) {
        fprintf( stderr, "Qry Alloc Failed: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    if( !odbExecute( hQry, "TRUNCATE TABLE TheInts" ) ) {
        fprintf( stderr, "Execute Failed: %s\n", odbGetErrorText( hQry ) );
        odbFree( hCon );
        return 1;
    }
    if( !odbPrepare( hQry,
                     "INSERT INTO "
                     "TheInts( TheTinyInt, TheSmallInt, TheInt, TheBigInt ) "
                     "VALUES( ?, ?, ?, ? )" ) )
    {
        fprintf( stderr, "Prepare Failed: %s\n", odbGetErrorText( hQry ) );
        odbFree( hCon );
        return 1;
    }
    if( !odbBindInputParam( hQry, 1, 0, 0, FALSE ) ||
        !odbBindInputParam( hQry, 2, 0, 0, FALSE ) ||
        !odbBindInputParam( hQry, 3, 0, 0, FALSE ) ||
        !odbBindInputParam( hQry, 4, 0, 0, TRUE ) )
    {
        fprintf( stderr, "BindParam Failed: %s\n", odbGetErrorText( hQry ) );
        odbFree( hCon );
        return 1;
    }
    for( i = 0; i < 128; i++ ) {
        if( i < 64 ) {
            byVal = (s = i % 8) ? 1 << s : 1;
            usVal = (s = i % 16) ? 1 << s : 1;
            ulVal = (s = i % 32) ? 1 << s : 1;
            ullVal = (s = i % 64) ? (odbULONGLONG)1 << s : 1;
        }
        else {
            byVal = i % 8 ? (byVal << 1) + 1: 1;
            usVal = i % 16 ? (usVal << 1) + 1 : 1;
            ulVal = i % 32 ? (ulVal << 1) + 1 : 1;
            ullVal = i % 64 ? (ullVal << 1) + 1 : 1;
        }
        if( !odbSetParamByte( hQry, 1, byVal, FALSE ) ||
            !odbSetParamShort( hQry, 2, usVal, FALSE ) ||
            !odbSetParamLong( hQry, 3, ulVal, FALSE ) ||
            !odbSetParamLongLong( hQry, 4, ullVal, TRUE ) )
        {
            fprintf( stderr, "SetParam Failed: %s\n", odbGetErrorText( hQry ) );
            odbFree( hCon );
            return 1;
        }
        if( !odbExecute( hQry, NULL ) ) {
            fprintf( stderr, "Execute Failed: %s\n", odbGetErrorText( hQry ) );
            odbFree( hCon );
            return 1;
        }
    }
    if( !odbLogout( hCon, FALSE ) ) {
        fprintf( stderr, "Logut Error: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    odbFree( hCon );

    odbWinsockCleanup(); /* Only required for Win32 clients. */

    return 0;
}
