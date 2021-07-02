/*

This program uses the following stored procedure to demonstrate how
to set and get parameter data.

CREATE PROCEDURE TwoTimes
    @RVal real,
    @FVal float,
    @RVal2 real = NULL OUTPUT,
    @FVal2 float = NULL OUTPUT
AS
    SET NOCOUNT ON

    SET @RVal2 = @RVal * 2.0
    SET @FVal2 = @FVal * 2.0
GO

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <odbtp.h>

int main( int argc, char* argv[] )
{
    odbDOUBLE dVal = 1555.2226;
    odbDOUBLE dVal2;
    odbFLOAT  fVal = 1555.2226f;
    odbFLOAT  fVal2;
    odbHANDLE hCon;
    odbHANDLE hQry;

    odbWinsockStartup(); /* Only required for Win32 clients. */

    if( !(hCon = odbAllocate(NULL)) ) {
        fprintf( stderr, "Unable to allocate\n" );
        return 1;
    }
    if( !odbLogin( hCon, "odbtpserver.somewhere.com", 2799, ODB_LOGIN_NORMAL,
                   "DRIVER={SQL Server};SERVER=myserver;UID=myuid;PWD=mypwd;DATABASE=OdbtpTest;" ) )
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
    if( !odbPrepareProc( hQry, "TwoTimes" ) ) {
        fprintf( stderr, "PrepareProc Failed: %s\n", odbGetErrorText( hQry ) );
        odbFree( hCon );
        return 1;
    }
    if( !odbSetParamFloat( hQry,
                           odbParamNum( hQry, "@RVal" ),
                           fVal, FALSE ) )
    {
        fprintf( stderr, "SetParam Failed: %s\n", odbGetErrorText( hQry ) );
        odbFree( hCon );
        return 1;
    }
    if( !odbSetParamDouble( hQry,
                            odbParamNum( hQry, "@FVal" ),
                            dVal, TRUE ) )
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
    if( !odbGetOutputParams( hQry ) ) {
        fprintf( stderr, "GetOutputParams Failed: %s\n", odbGetErrorText( hQry ) );
        odbFree( hCon );
        return 1;
    }
    fVal2 = odbParamDataFloat( hQry, odbParamNum( hQry, "@RVal2" ) );
    dVal2 = odbParamDataDouble( hQry, odbParamNum( hQry, "@FVal2" ) );

    printf( "SQL Server Calculation:\n" );
    printf( "    32-bit Precision = %12.6f\n",  fVal2 );
    printf( "    64-bit Precision = %12.6lf\n", dVal2 );

    fVal2 = fVal * 2.0f;
    dVal2 = dVal * 2.0;
    printf( "\nProgram Calculation:\n" );
    printf( "    32-bit Precision = %12.6f\n",  fVal2 );
    printf( "    64-bit Precision = %12.6lf\n", dVal2 );

    if( !odbLogout( hCon, FALSE ) ) {
        fprintf( stderr, "Logut Error: %s\n", odbGetErrorText( hCon ) );
        odbFree( hCon );
        return 1;
    }
    odbFree( hCon );

    odbWinsockCleanup(); /* Only required for Win32 clients. */

    return 0;
}
