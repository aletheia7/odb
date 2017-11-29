/* $Id: odbtp.c,v 1.45 2005/12/22 04:48:02 rtwitty Exp $ */
/*
    odbtp - ODBTP client library

    Copyright (C) 2002-2005 Robert E. Twitty <rtwitty@users.sourceforge.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef WIN32  /* Using WIN32 */
  #include <windows.h>
  #include <stdio.h>
  #include <time.h>
  #include <sys\stat.h>
  #include "w32sockutil.h"
  #define strcasecmp  stricmp
  #define strncasecmp strnicmp
#else /* Not Using WIN32 */
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <time.h>
  #include <sys/stat.h>
  #include "sockutil.h"
#endif

#ifdef ODBTP_DLL /* Using odbtp.dll */
  #define ODBTP_DLL_BUILD 1
#endif
#include "odbtp.h"
#include "odbtpbuild.h"

#ifdef WIN32  /* Using WIN32 */
    #define DEFAULT_INTERFACE_FILE "c:\\WINDOWS\\odbtp.conf"
#else /* Not Using WIN32 */
    #define DEFAULT_INTERFACE_FILE "/usr/local/share/odbtp.conf"
#endif


#define odbNetworkToHostOrder odbHostToNetworkOrder

#define IS_TRUE_VALUE(v) ((v) == 't' || (v) == 'T' || (v) == 'y' || (v) == 'Y')

#define HEX2DEC(c) ((c) >= '0' && (c) <= '9' ? (c) - '0' : \
                     ((c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : \
                       ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : 0)))


static odbTIMESTAMP tsNull = {0,0,0,0,0,0,0};
static odbGUID      guidNull = {0,0,0,{0,0,0,0,0,0,0,0}};

#ifdef ODBTP_DLL
BOOL WINAPI DllMain( HINSTANCE hDllInst, DWORD fdwReason, LPVOID lpvReserved )
{
    BOOL bResult = TRUE;

    // Dispatch this call based on the reason it was called.
    switch( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
            odbWinsockStartup();
            break;

        case DLL_PROCESS_DETACH:
            odbWinsockCleanup();
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        default:
            break;
    }
    return( bResult );
}
#endif

_odbdecl
odbHANDLE odbAllocate( odbHANDLE hCon )
{
    odbHCONNECTION h;
    odbHQUERY      hQry;
    odbPHANDLE     phQry = NULL;
    odbULONG       ul;

    if( !hCon ) return( odbAllocateConnection() );

    h = (odbHCONNECTION)hCon;
    if( h->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( NULL );
    }
    for( ul = 0; ul < h->ulMaxQrys; ul++ ) {
        if( *(h->phQrys + ul) ) continue;
        phQry = (h->phQrys + ul);
        break;
    }
    if( !phQry ) {
        odbSetError( hCon, ODBTPERR_MAXQUERYS );
        return( NULL );
    }
    if( !(hQry = (odbHQUERY)odbAllocateQuery()) ) {
        odbSetError( hCon, ODBTPERR_MEMORY );
        return( NULL );
    }
    hQry->Sock = h->Sock;
    hQry->hCon = hCon;
    hQry->ulId = ul;
    hQry->bUseBroadTypes = h->bUseBroadTypes;
    *phQry = (odbHANDLE)hQry;

    odbSetError( hCon, ODBTPERR_NONE );

    return( (odbHANDLE)hQry );
}

odbHANDLE odbAllocateConnection(void)
{
    odbHCONNECTION hCon;
    odbULONG       ul;

    if( !(hCon = (odbHCONNECTION)malloc( sizeof(odbHCONNECTION_s) )) )
        return( NULL );

    hCon->ulType = ODBTPHANDLE_CONNECTION;
    hCon->ulError = ODBTPERR_NONE;
    hCon->Sock = NULL;
    hCon->ulRequestCode = 0;
    hCon->ulResponseCode = 0;
    hCon->pResponseBuf = NULL;
    hCon->ulResponseBufSize = 256;
    hCon->ulResponseSize = 0;
    hCon->ulExtractSize = 0;
    hCon->UserData.pVal = NULL;
    hCon->phQrys = NULL;
    hCon->ulMaxQrys = 32;
    hCon->ulQryCursor = 0;
    hCon->bUseRowCache = FALSE;
    hCon->ulRowCacheSize = 0;
    hCon->bConvertAll = FALSE;
    hCon->bConvertDatetime = FALSE;
    hCon->bConvertGuid = FALSE;
    hCon->bUseBroadTypes = FALSE;
    hCon->DataTypes = NULL;
    hCon->ulTotalDataTypes = 0;
    hCon->szVersion[0] = 0;

    if( !(hCon->Sock = odbSockAllocate()) ) {
        odbFreeConnection( hCon );
        return( NULL );
    }
    if( !(hCon->pResponseBuf = (odbPVOID)malloc( hCon->ulResponseBufSize )) ) {
        odbFreeConnection( hCon );
        return( NULL );
    }
    if( !(hCon->phQrys = (odbPHANDLE)malloc( hCon->ulMaxQrys * sizeof(odbHQUERY) )) ) {
        odbFreeConnection( hCon );
        return( NULL );
    }
    for( ul = 0; ul < hCon->ulMaxQrys; ul++ ) *(hCon->phQrys + ul) = NULL;

    return( (odbHANDLE)hCon );
}

odbHANDLE odbAllocateQuery(void)
{
    odbHQUERY hQry;
    odbULONG  ulBytes;

    if( !(hQry = (odbHQUERY)malloc( sizeof(odbHQUERY_s) )) )
        return( NULL );

    hQry->ulType = ODBTPHANDLE_QUERY;
    hQry->ulError = ODBTPERR_NONE;
    hQry->Sock = NULL;
    hQry->ulRequestCode = 0;
    hQry->ulResponseCode = 0;
    hQry->pResponseBuf = NULL;
    hQry->ulResponseBufSize = 0;
    hQry->ulResponseSize = 0;
    hQry->ulExtractSize = 0;
    hQry->UserData.pVal = NULL;
    hQry->hCon = NULL;
    hQry->ulId = 0xFFFFFFFF;
    hQry->pNormalBuf = NULL;
    hQry->ulNormalBufSize = 256;
    hQry->ulNormalSize = 0;
    hQry->pColInfoBuf = NULL;
    hQry->ulColInfoBufSize = 1024;
    hQry->ulColInfoSize = 0;
    hQry->Cols = NULL;
    hQry->usMaxCols = 32;
    hQry->usTotalCols = 0;
    hQry->pParamInfoBuf = NULL;
    hQry->ulParamInfoBufSize = 1024;
    hQry->ulParamInfoSize = 0;
    hQry->pParamDataBuf = NULL;
    hQry->ulParamDataBufSize = 512;
    hQry->ulParamDataSize = 0;
    hQry->Params = NULL;
    hQry->usMaxParams = 16;
    hQry->usTotalParams = 0;
    hQry->usTotalParams = 0;
    hQry->pRowDataBuf = NULL;
    hQry->ulRowDataBufSize = 0x10000;
    hQry->ulRowDataSize = 0;
    hQry->Rows = NULL;
    hQry->lMaxRows = 0;
    hQry->lTotalRows = 0;
    hQry->lRowCursor = 0;
    hQry->ulRowExtractSize = 0;
    hQry->bGotRow = FALSE;
    hQry->lRowCount = 0;
    hQry->pbySavedRowDataEndByte = NULL;
    hQry->bNoData = TRUE;
    hQry->bMoreResults = FALSE;
    hQry->usCursorType = ODB_CURSOR_FORWARD;
    hQry->usCursorConcur = ODB_CONCUR_DEFAULT;
    hQry->bBookmarksOn = FALSE;

    if( !(hQry->pNormalBuf = (odbPVOID)malloc( hQry->ulNormalBufSize )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    if( !(hQry->pColInfoBuf = (odbPVOID)malloc( hQry->ulColInfoBufSize )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    ulBytes = hQry->usMaxCols * sizeof(odbCOLUMN_s);
    if( !(hQry->Cols = (odbCOLUMN)malloc( ulBytes )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    if( !(hQry->pParamInfoBuf = (odbPVOID)malloc( hQry->ulParamInfoBufSize )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    if( !(hQry->pParamDataBuf = (odbPVOID)malloc( hQry->ulParamDataBufSize )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    ulBytes = hQry->usMaxParams * sizeof(odbPARAMETER_s);
    if( !(hQry->Params = (odbPARAMETER)malloc( ulBytes )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    if( !(hQry->pRowDataBuf = (odbPVOID)malloc( hQry->ulRowDataBufSize )) ) {
        odbFreeQuery( hQry );
        return( NULL );
    }
    return( (odbHANDLE)hQry );
}

_odbdecl
odbBOOL odbBindCol( odbHANDLE hQry, odbUSHORT usCol, odbSHORT sDataType,
                    odbLONG lDataLen, odbBOOL bFinal )
{
    odbHQUERY h = (odbHQUERY)hQry;
    odbCOLUMN Col;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usCol == 0 || usCol > h->usTotalCols )
        return( odbSetError( hQry, ODBTPERR_COLNUMBER ) );

    if( h->lTotalRows != 0 )
        return( odbSetError( hQry, ODBTPERR_FETCHEDROWS ) );

    Col = (h->Cols + usCol - 1);
    if( sDataType == 0 ) sDataType = Col->sDefaultDataType;

    if( h->ulRequestCode != ODBTP_BINDCOL &&
        !odbSendRequestByte( hQry, ODBTP_BINDCOL, (odbBYTE)h->ulId, FALSE ) )
    {
        return( FALSE );
    }
    if( !odbSendRequestShort( hQry, ODBTP_BINDCOL, usCol, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDCOL, (odbUSHORT)sDataType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestLong( hQry, ODBTP_BINDCOL, (odbULONG)lDataLen, bFinal ) )
        return( FALSE );

    Col->sDataType = sDataType;

    return( bFinal ? odbReadResponse( hQry ) : TRUE );
}

_odbdecl
odbBOOL odbBindInOutParam( odbHANDLE hQry,
                           odbUSHORT usParam, odbSHORT sDataType,
                           odbLONG lDataLen, odbBOOL bFinal )
{
    return( odbBindParam( hQry, usParam, ODB_PARAM_INOUT, sDataType, lDataLen, bFinal ) );
}

_odbdecl
odbBOOL odbBindInputParam( odbHANDLE hQry,
                           odbUSHORT usParam, odbSHORT sDataType,
                           odbLONG lDataLen, odbBOOL bFinal )
{
    return( odbBindParam( hQry, usParam, ODB_PARAM_INPUT, sDataType, lDataLen, bFinal ) );
}

_odbdecl
odbBOOL odbBindOutputParam( odbHANDLE hQry,
                            odbUSHORT usParam, odbSHORT sDataType,
                            odbLONG lDataLen, odbBOOL bFinal )
{
    return( odbBindParam( hQry, usParam, ODB_PARAM_OUTPUT, sDataType, lDataLen, bFinal ) );
}

_odbdecl
odbBOOL odbBindParam( odbHANDLE hQry, odbUSHORT usParam,
                      odbUSHORT usType, odbSHORT sDataType,
                      odbLONG lDataLen, odbBOOL bFinal )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usParam == 0 || usParam > h->usTotalParams )
        return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );

    Param = (h->Params + usParam - 1);
    if( sDataType == 0 ) sDataType = Param->sDefaultDataType;

    if( h->ulRequestCode != ODBTP_BINDPARAM &&
        !odbSendRequestByte( hQry, ODBTP_BINDPARAM, (odbBYTE)h->ulId, FALSE ) )
    {
        return( FALSE );
    }
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, usParam, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, usType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, (odbUSHORT)sDataType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestLong( hQry, ODBTP_BINDPARAM, (odbULONG)lDataLen, bFinal ) )
        return( FALSE );

    Param->bBound = TRUE;
    Param->usType = usType;
    Param->sDataType = sDataType;

    return( bFinal ? odbReadResponse( hQry ) : TRUE );
}

_odbdecl
odbBOOL odbBindParamEx( odbHANDLE hQry, odbUSHORT usParam,
                        odbUSHORT usType, odbSHORT sDataType,
                        odbLONG lDataLen, odbSHORT sSqlType,
                        odbULONG ulColSize, odbSHORT sDecDigits,
                        odbBOOL bFinal )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usParam == 0 || usParam > h->usTotalParams )
        return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );

    Param = (h->Params + usParam - 1);

    if( h->ulRequestCode != ODBTP_BINDPARAM &&
        !odbSendRequestByte( hQry, ODBTP_BINDPARAM, (odbBYTE)h->ulId, FALSE ) )
    {
        return( FALSE );
    }
    usType |= ODB_PARAM_DESCRIBED;

    if( sDataType == 0 ) sDataType = odbGetOdbDataType( sSqlType );

    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, usParam, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, usType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, (odbUSHORT)sDataType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestLong( hQry, ODBTP_BINDPARAM, (odbULONG)lDataLen, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, (odbUSHORT)sSqlType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestLong( hQry, ODBTP_BINDPARAM, ulColSize, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_BINDPARAM, (odbUSHORT)sDecDigits, bFinal ) )
        return( FALSE );

    Param->bBound = TRUE;
    Param->usType = usType;
    Param->sDataType = sDataType;
    Param->sSqlType = sSqlType;
    Param->ulColSize = ulColSize;
    Param->sDecDigits = sDecDigits;

    return( bFinal ? odbReadResponse( hQry ) : TRUE );
}

odbCOLUMN odbCol( odbHANDLE hQry, odbUSHORT usCol, odbBOOL bValid )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( NULL );
    }
    if( usCol == 0 || usCol > h->usTotalCols ) {
        odbSetError( hQry, ODBTPERR_COLNUMBER );
        return( NULL );
    }
    if( bValid && !h->bGotRow ) {
        odbSetError( hQry, ODBTPERR_FETCHROW );
        return( NULL );
    }
    odbSetError( hQry, ODBTPERR_NONE );
    return( (h->Cols + usCol - 1) );
}

_odbdecl
odbLONG odbColActualLen( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, TRUE )) ) return( -1 );
    return( Col->lActualLen );
}

_odbdecl
odbPCSTR odbColBaseName( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    return( Col->pszBaseName );
}

_odbdecl
odbPCSTR odbColBaseTable( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    return( Col->pszBaseTable );
}

_odbdecl
odbPCSTR odbColCatalog( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    return( Col->pszCatalog );
}

_odbdecl
odbPVOID odbColData( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;

    if( !(Col = odbCol( hQry, usCol, TRUE )) ) return( NULL );

    if( Col->lDataLen < 0 ) return( NULL );

    switch( Col->sDataType ) {
        case ODB_BIT:
        case ODB_TINYINT:
        case ODB_UTINYINT:  return( &Col->Data.byVal );
        case ODB_SMALLINT:
        case ODB_USMALLINT: return( &Col->Data.usVal );
        case ODB_INT:
        case ODB_UINT:      return( &Col->Data.ulVal );
        case ODB_BIGINT:
        case ODB_UBIGINT:   return( &Col->Data.ullVal );
        case ODB_REAL:      return( &Col->Data.fVal );
        case ODB_DOUBLE:    return( &Col->Data.dVal );
        case ODB_DATETIME:  return( &Col->Data.tsVal );
        case ODB_GUID:      return( &Col->Data.guidVal );
    }
    return( Col->Data.pVal );
}

_odbdecl
odbBYTE odbColDataByte( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPBYTE pbyData = (odbPBYTE)odbColData( hQry, usCol );
    if( !pbyData ) return( 0 );
    return( *pbyData );
}

_odbdecl
odbDOUBLE odbColDataDouble( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPDOUBLE pdData = (odbPDOUBLE)odbColData( hQry, usCol );
    if( !pdData ) return( 0.0 );
    return( *pdData );
}

_odbdecl
odbFLOAT  odbColDataFloat( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPFLOAT pfData = (odbPFLOAT)odbColData( hQry, usCol );
    if( !pfData ) return( 0.0 );
    return( *pfData );
}

_odbdecl
odbPGUID odbColDataGuid( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPGUID pguidData = (odbPGUID)odbColData( hQry, usCol );
    if( !pguidData ) return( &guidNull );
    return( pguidData );
}

_odbdecl
odbLONG odbColDataLen( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, TRUE )) ) return( -1 );
    return( Col->lDataLen );
}

_odbdecl
odbULONG odbColDataLong( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPULONG pulData = (odbPULONG)odbColData( hQry, usCol );
    if( !pulData ) return( 0 );
    return( *pulData );
}

_odbdecl
odbULONGLONG odbColDataLongLong( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPULONGLONG pullData = (odbPULONGLONG)odbColData( hQry, usCol );
    if( !pullData ) return( 0 );
    return( *pullData );
}

_odbdecl
odbUSHORT odbColDataShort( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPUSHORT pusData = (odbPUSHORT)odbColData( hQry, usCol );
    if( !pusData ) return( 0 );
    return( *pusData );
}

_odbdecl
odbPSTR odbColDataText( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPSTR pszData = (odbPSTR)odbColData( hQry, usCol );
    if( !pszData ) return( "" );
    return( pszData );
}

_odbdecl
odbPTIMESTAMP odbColDataTimestamp( odbHANDLE hQry, odbUSHORT usCol )
{
    odbPTIMESTAMP ptsData = (odbPTIMESTAMP)odbColData( hQry, usCol );
    if( !ptsData ) return( &tsNull );
    return( ptsData );
}

_odbdecl
odbSHORT odbColDataType( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( 0 );
    return( Col->sDataType );
}

_odbdecl
odbSHORT odbColDecDigits( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( 0 );
    return( Col->sDecDigits );
}

_odbdecl
odbSHORT odbColDefaultDataType( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( 0 );
    return( Col->sDefaultDataType );
}

_odbdecl
odbULONG odbColFlags( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( 0 );
    return( Col->ulFlags );
}

_odbdecl
odbPCSTR odbColName( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    return( Col->pszName );
}

_odbdecl
odbUSHORT odbColNum( odbHANDLE hQry, odbPCSTR pszName )
{
    odbHQUERY h = (odbHQUERY)hQry;
    odbUSHORT usCol;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0 );
    }
    for( usCol = 0; usCol < h->usTotalCols; usCol++ )
        if( !strcasecmp( (h->Cols + usCol)->pszName, pszName ) )
            return( usCol + 1 );

    odbSetError( hQry, ODBTPERR_COLNAME );
    return( 0 );
}

_odbdecl
odbPCSTR odbColSchema( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    return( Col->pszSchema );
}

_odbdecl
odbULONG odbColSize( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( 0 );
    return( Col->ulColSize );
}

_odbdecl
odbSHORT odbColSqlType( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( 0 );
    return( Col->sSqlType );
}

_odbdecl
odbPCSTR odbColSqlTypeName( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    odbSHORT  sDD;
    odbULONG  ulSize;

    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    ulSize = Col->ulColSize;
    sDD = Col->sDecDigits;

    if( ((odbHQUERY)hQry)->bUseBroadTypes ) {
        switch( Col->sSqlType ) {
            case SQL_BIT:       return( "bit" );
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INT:       return( "int" );
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE:    return( "real" );
            case SQL_DECIMAL:   if( sDD == 4 && (ulSize == 10 || ulSize == 19) )
                                    return( "money" );
                                return( "real" );
            case SQL_BIGINT:
            case SQL_NUMERIC:   return( "numeric" );
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
            case SQL_DATETIME:  return( "datetime" );
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_NCHAR:
            case SQL_NVARCHAR:  return( "char" );
            case SQL_TEXT:
            case SQL_NTEXT:     return( "text" );
            case SQL_BINARY:
            case SQL_VARBINARY: return( "blob" );
            case SQL_IMAGE:     return( "image" );
        }
    }
    else if( *Col->pszSqlType ) {
        return( Col->pszSqlType );
    }
    else {
        switch( Col->sSqlType ) {
            case SQL_BIT:       return( "bit" );
            case SQL_TINYINT:   return( "tinyint" );
            case SQL_SMALLINT:  return( "smallint" );
            case SQL_INT:       return( "int" );
            case SQL_BIGINT:    return( "bigint" );
            case SQL_REAL:      return( "real" );
            case SQL_FLOAT:     return( "float" );
            case SQL_DOUBLE:    return( "double" );
            case SQL_DECIMAL:   if( sDD == 4 ) {
                                    if( ulSize == 10 ) return( "smallmoney" );
                                    if( ulSize == 19 ) return( "money" );
                                }
                                return( "decimal" );
            case SQL_NUMERIC:   if( sDD == 4 && ulSize == 19 )
                                    return( "currency" );
                                return( "numeric" );
            case SQL_DATE:
            case SQL_TIME:
            case SQL_TIMESTAMP:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
            case SQL_DATETIME:  if( ulSize <= 16 ) return( "smalldatetime" );
                                return( "datetime" );
            case SQL_CHAR:      return( "char" );
            case SQL_VARCHAR:   return( "varchar" );
            case SQL_TEXT:      return( "text" );
            case SQL_NCHAR:     return( "nchar" );
            case SQL_NVARCHAR:  return( "nvarchar" );
            case SQL_NTEXT:     return( "ntext" );
            case SQL_BINARY:    return( "binary" );
            case SQL_VARBINARY: return( "varbinary" );
            case SQL_IMAGE:     return( "image" );
            case SQL_GUID:      return( "uniqueidentifier" );
            case SQL_VARIANT:   return( "sql_variant" );
        }
    }
    return( "unknown" );
}

_odbdecl
odbPCSTR odbColTable( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, FALSE )) ) return( NULL );
    return( Col->pszTable );
}

_odbdecl
odbBOOL odbColTruncated( odbHANDLE hQry, odbUSHORT usCol )
{
    odbCOLUMN Col;
    if( !(Col = odbCol( hQry, usCol, TRUE )) ) return( FALSE );
    return( Col->bTruncated );
}

_odbdecl
odbPVOID odbColUserData( odbHANDLE hQry, odbUSHORT usCol )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( NULL );
    }
    if( usCol == 0 || usCol > h->usTotalCols ) {
        odbSetError( hQry, ODBTPERR_COLNUMBER );
        return( NULL );
    }
    odbSetError( hQry, ODBTPERR_NONE );
    return( (h->Cols + usCol - 1)->pUserData );
}

_odbdecl
odbBOOL odbCommit( odbHANDLE hCon )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !odbSendRequest( hCon, ODBTP_COMMIT, NULL, 0, TRUE ) )
        return( FALSE );

    return( odbReadResponse( hCon ) );
}

odbBOOL odbConnect( odbHANDLE hCon, odbPCSTR pszServer, odbUSHORT usPort )
{
    odbBOOL        bLookupFailed;
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    odbPSTR        pszVersion;
    odbSOCKET      Sock;

    if( h->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    Sock = h->Sock;
    if( Sock->sock != INVALID_SOCKET )
        return( odbSetError( hCon, ODBTPERR_CONNECTED ) );
    if( !odbSockConnect( Sock, pszServer, usPort, &bLookupFailed ) ) {
        if( bLookupFailed )
            return( odbSetError( hCon, ODBTPERR_HOSTRESOLVE ) );
        else if( Sock->bTimeout )
            return( odbSetError( hCon, ODBTPERR_TIMEOUTCONN ) );
        return( odbSetError( hCon, ODBTPERR_CONNECT ) );
    }
    if( !odbReadResponse( hCon ) ) return( FALSE );

    if( (pszVersion = strrchr( (odbPCSTR)h->pResponseBuf, ']' )) )
        strncpy( h->szVersion, pszVersion + 1, 15 );
    else
        strncpy( h->szVersion, (odbPCSTR)h->pResponseBuf, 15 );
    h->szVersion[15] = 0;

    return( TRUE );
}

_odbdecl
odbBOOL odbConvertAll( odbHANDLE hCon, odbBOOL bConvert )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    ((odbHCONNECTION)hCon)->bConvertAll = bConvert;

    return( odbSetError( hCon, ODBTPERR_NONE ) );
}

_odbdecl
odbBOOL odbConvertDatetime( odbHANDLE hCon, odbBOOL bConvert )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    ((odbHCONNECTION)hCon)->bConvertDatetime = bConvert;

    return( odbSetError( hCon, ODBTPERR_NONE ) );
}

_odbdecl
odbBOOL odbConvertGuid( odbHANDLE hCon, odbBOOL bConvert )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    ((odbHCONNECTION)hCon)->bConvertGuid = bConvert;

    return( odbSetError( hCon, ODBTPERR_NONE ) );
}

_odbdecl
void odbCTimeToTimestamp( odbPTIMESTAMP ptsTime, odbLONG lTime )
{
    time_t     tTime = lTime;
    struct tm* ptmTime;

    ptmTime = localtime( &tTime );

    ptsTime->sYear = ptmTime->tm_year + 1900;
    ptsTime->usMonth = ptmTime->tm_mon + 1;
    ptsTime->usDay = ptmTime->tm_mday;
    ptsTime->usHour = ptmTime->tm_hour;
    ptsTime->usMinute = ptmTime->tm_min;
    ptsTime->usSecond = ptmTime->tm_sec;
    ptsTime->ulFraction = 0;
}

_odbdecl
odbBOOL odbDescribeSqlType( odbHANDLE hCon,
                            odbPCSTR pszSqlTypeName, odbPSHORT psSqlType,
                            odbPULONG pulColSize, odbPSHORT psDecDigits )
{
    *psSqlType = 0;
    *pulColSize = 0;
    *psDecDigits = 0;

    if( hCon ) {
        odbDATATYPE    DataType;
        odbHCONNECTION h = (odbHCONNECTION)hCon;

        if( h->ulType != ODBTPHANDLE_CONNECTION )
            return( odbSetError( hCon, ODBTPERR_HANDLE ) );

        if( (DataType = h->DataTypes) ) {
            odbULONG ul;

            for( ul = 0; ul < h->ulTotalDataTypes; ul++, DataType++ ) {
                if( !strcasecmp( pszSqlTypeName, DataType->szName ) ) {
                    *psSqlType = DataType->sSqlType;
                    *pulColSize = DataType->ulColSize;
                    *psDecDigits = DataType->sDecDigits;
                    return( TRUE );
                }
            }
            return( FALSE );
        }
    }
    if( !strcasecmp( pszSqlTypeName, "bigint" ) ) {
        *psSqlType = SQL_BIGINT;
        *pulColSize = 19;
    }
    else if( !strcasecmp( pszSqlTypeName, "binary" ) ) {
        *psSqlType = SQL_BINARY;
        *pulColSize = 255;
    }
    else if( !strcasecmp( pszSqlTypeName, "bit" ) ) {
        *psSqlType = SQL_BIT;
        *pulColSize = 1;
    }
    else if( !strcasecmp( pszSqlTypeName, "char" ) ) {
        *psSqlType = SQL_CHAR;
        *pulColSize = 255;
    }
    else if( !strcasecmp( pszSqlTypeName, "currency" ) ) {
        *psSqlType = SQL_NUMERIC;
        *pulColSize = 19;
        *psDecDigits = 4;
    }
    else if( !strcasecmp( pszSqlTypeName, "datetime" ) ) {
        *psSqlType = SQL_DATETIME;
        *pulColSize = 23;
        *psDecDigits = 3;
    }
    else if( !strcasecmp( pszSqlTypeName, "decimal" ) ) {
        *psSqlType = SQL_DECIMAL;
        *pulColSize = 28;
    }
    else if( !strcasecmp( pszSqlTypeName, "double" ) ) {
        *psSqlType = SQL_DOUBLE;
        *pulColSize = 53;
    }
    else if( !strcasecmp( pszSqlTypeName, "float" ) ) {
        *psSqlType = SQL_FLOAT;
        *pulColSize = 53;
    }
    else if( !strcasecmp( pszSqlTypeName, "image" ) ) {
        *psSqlType = SQL_IMAGE;
        *pulColSize = 2147483647;
    }
    else if( !strcasecmp( pszSqlTypeName, "int" ) ) {
        *psSqlType = SQL_INT;
        *pulColSize = 10;
    }
    else if( !strcasecmp( pszSqlTypeName, "money" ) ) {
        *psSqlType = SQL_DECIMAL;
        *pulColSize = 19;
        *psDecDigits = 4;
    }
    else if( !strcasecmp( pszSqlTypeName, "nchar" ) ) {
        *psSqlType = SQL_NCHAR;
        *pulColSize = 255;
    }
    else if( !strcasecmp( pszSqlTypeName, "ntext" ) ) {
        *psSqlType = SQL_NTEXT;
        *pulColSize = 1073741823;
    }
    else if( !strcasecmp( pszSqlTypeName, "numeric" ) ) {
        *psSqlType = SQL_NUMERIC;
        *pulColSize = 28;
    }
    else if( !strcasecmp( pszSqlTypeName, "nvarchar" ) ) {
        *psSqlType = SQL_NVARCHAR;
        *pulColSize = 255;
    }
    else if( !strcasecmp( pszSqlTypeName, "real" ) ) {
        *psSqlType = SQL_REAL;
        *pulColSize = 24;
    }
    else if( !strcasecmp( pszSqlTypeName, "smalldatetime" ) ) {
        *psSqlType = SQL_DATETIME;
        *pulColSize = 16;
    }
    else if( !strcasecmp( pszSqlTypeName, "smallint" ) ) {
        *psSqlType = SQL_SMALLINT;
        *pulColSize = 5;
    }
    else if( !strcasecmp( pszSqlTypeName, "smallmoney" ) ) {
        *psSqlType = SQL_DECIMAL;
        *pulColSize = 10;
        *psDecDigits = 4;
    }
    else if( !strcasecmp( pszSqlTypeName, "sql_variant" ) ) {
        *psSqlType = SQL_VARIANT;
        *pulColSize = 255;
    }
    else if( !strcasecmp( pszSqlTypeName, "text" ) ) {
        *psSqlType = SQL_TEXT;
        *pulColSize = 2147483647;
    }
    else if( !strcasecmp( pszSqlTypeName, "tinyint" ) ) {
        *psSqlType = SQL_TINYINT;
        *pulColSize = 3;
    }
    else if( !strcasecmp( pszSqlTypeName, "uniqueidentifier" ) ||
             !strcasecmp( pszSqlTypeName, "guid" ) )
    {
        *psSqlType = SQL_GUID;
        *pulColSize = 36;
    }
    else if( !strcasecmp( pszSqlTypeName, "varbinary" ) ) {
        *psSqlType = SQL_VARBINARY;
        *pulColSize = 255;
    }
    else if( !strcasecmp( pszSqlTypeName, "varchar" ) ) {
        *psSqlType = SQL_VARCHAR;
        *pulColSize = 255;
    }
    else {
        return( FALSE );
    }
    return( TRUE );
}

_odbdecl
odbBOOL odbDetachQry( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    if( h->hCon ) {
        if( !odbDropQry( hQry ) ) return( FALSE );
        *(((odbHCONNECTION)h->hCon)->phQrys + h->ulId) = NULL;
        h->hCon = NULL;
        h->Sock = NULL;
        h->ulId = 0xFFFFFFFF;
    }
    return( odbSetError( hQry, ODBTPERR_NONE ) );
}

odbBOOL odbDisconnect( odbHANDLE hCon )
{
    odbHODBTP hOdb = (odbHODBTP)hCon;

    if( hOdb->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    odbSockClose( hOdb->Sock );
    return( odbSetError( hCon, ODBTPERR_NONE ) );
}

_odbdecl
odbBOOL odbDoRowOperation( odbHANDLE hQry, odbUSHORT usRowOp )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    if( !odbSendRequestByte( hQry, ODBTP_ROWOP, (odbBYTE)h->ulId, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_ROWOP, usRowOp, TRUE ) )
        return( FALSE );

    return( odbReadResponse( hQry ) );
}

_odbdecl
odbBOOL odbDropQry( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    if( !odbSendRequestByte( hQry, ODBTP_DROP, (odbBYTE)h->ulId, TRUE ) )
        return( FALSE );

    return( odbReadResponse( hQry ) );
}

odbBOOL odbExecute( odbHANDLE hQry, odbPCSTR pszSQL )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    h->bNoData = TRUE;
    h->bMoreResults = FALSE;
    h->ulRowExtractSize = 0;

    if( !odbSendRequestByte( hQry, ODBTP_EXECUTE, (odbBYTE)h->ulId, FALSE ) )
        return( FALSE );

    if( !pszSQL || *pszSQL == 0 ) {
        if( !odbSendRequest( hQry, ODBTP_EXECUTE, NULL, 0, TRUE ) )
            return( FALSE );
    }
    else {
        h->usTotalParams = 0;
        if( !odbSendRequestText( hQry, ODBTP_EXECUTE, pszSQL, TRUE ) )
            return( FALSE );
    }
    return( odbProcessResult( hQry ) );
}

odbBOOL odbExtract( odbHANDLE hOdb, odbPVOID pData, odbULONG ulLen )
{
    odbHODBTP h = (odbHODBTP)hOdb;

    if( ulLen != 0 ) {
        if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
            return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

        memcpy( pData, h->pbyExtractData, ulLen );
        h->pbyExtractData += ulLen;
        h->ulExtractSize -= ulLen;
    }
    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractByte( odbHANDLE hOdb, odbPBYTE pbyData )
{
    odbHODBTP h = (odbHODBTP)hOdb;

    if( h->ulExtractSize == 0 || h->ulExtractSize < 1 )
        return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

    *pbyData = *h->pbyExtractData;
    h->pbyExtractData += 1;
    h->ulExtractSize -= 1;

    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractDouble( odbHANDLE hOdb, odbPDOUBLE pdData )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    odbULONG  ulLen = sizeof(odbDOUBLE);

    if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
        return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

    odbNetworkToHostOrder( (odbPBYTE)pdData, h->pbyExtractData, ulLen );
    h->pbyExtractData += ulLen;
    h->ulExtractSize -= ulLen;

    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractFloat( odbHANDLE hOdb, odbPFLOAT pfData )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    odbULONG  ulLen = sizeof(odbFLOAT);

    if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
        return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

    odbNetworkToHostOrder( (odbPBYTE)pfData, h->pbyExtractData, ulLen );
    h->pbyExtractData += ulLen;
    h->ulExtractSize -= ulLen;

    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractGuid( odbHANDLE hOdb, odbPGUID pguidData )
{
    if( !odbExtractLong( hOdb, (odbPULONG)&pguidData->ulData1 ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&pguidData->usData2 ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&pguidData->usData3 ) ) return( FALSE );
    return( odbExtract( hOdb, (odbPVOID)pguidData->byData4, 8 ) );
}

odbBOOL odbExtractLong( odbHANDLE hOdb, odbPULONG pulData )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    odbULONG  ul;
    odbULONG  ulLen = sizeof(odbULONG);

    if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
        return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

    memcpy( &ul, h->pbyExtractData, ulLen );
    *pulData = ntohl( ul );
    h->pbyExtractData += ulLen;
    h->ulExtractSize -= ulLen;

    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL      odbExtractLongLong( odbHANDLE hOdb, odbPULONGLONG pullData )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    odbULONG  ulLen = sizeof(odbULONGLONG);

    if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
        return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

    odbNetworkToHostOrder( (odbPBYTE)pullData, h->pbyExtractData, ulLen );
    h->pbyExtractData += ulLen;
    h->ulExtractSize -= ulLen;

    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractPointer( odbHANDLE hOdb, odbPVOID* ppData,
                           odbULONG ulLen )
{
    odbHODBTP h = (odbHODBTP)hOdb;

    if( ulLen != 0 ) {
        if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
            return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

        *ppData = h->pbyExtractData;
        h->pbyExtractData += ulLen;
        h->ulExtractSize -= ulLen;
    }
    else {
        *ppData = h->pbyExtractData;
    }
    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractShort( odbHANDLE hOdb, odbPUSHORT pusData )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    odbULONG  ulLen = sizeof(odbUSHORT);
    odbUSHORT us;

    if( h->ulExtractSize == 0 || h->ulExtractSize < ulLen )
        return( odbSetError( hOdb, ODBTPERR_RESPONSE ) );

    memcpy( &us, h->pbyExtractData, ulLen );
    *pusData = ntohs( us );
    h->pbyExtractData += ulLen;
    h->ulExtractSize -= ulLen;

    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbExtractTimestamp( odbHANDLE hOdb, odbPTIMESTAMP ptsData )
{
    if( !odbExtractShort( hOdb, (odbPUSHORT)&ptsData->sYear ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&ptsData->usMonth ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&ptsData->usDay ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&ptsData->usHour ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&ptsData->usMinute ) ) return( FALSE );
    if( !odbExtractShort( hOdb, (odbPUSHORT)&ptsData->usSecond ) ) return( FALSE );
    return( odbExtractLong( hOdb, (odbPULONG)&ptsData->ulFraction ) );
}

_odbdecl
odbBOOL odbFetchNextResult( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    h->bNoData = TRUE;
    h->bMoreResults = FALSE;
    h->ulRowExtractSize = 0;

    if( !odbSendRequestByte( hQry, ODBTP_FETCHRESULT, (odbBYTE)h->ulId, TRUE ) )
        return( FALSE );

    return( odbProcessResult( hQry ) );
}

_odbdecl
odbBOOL odbFetchRow( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    odbRestoreRowDataBytes( hQry );

    if( h->lRowCursor ) {
        odbROW Row;

        if( h->lRowCursor > h->lTotalRows ) {
            h->bNoData = TRUE;
            h->bGotRow = FALSE;
            return( odbSetError( hQry, ODBTPERR_NONE ) );
        }
        Row = h->Rows + h->lRowCursor - 1;
        odbInitExtract( hQry, Row->pbyData, Row->ulSize );
        h->lRowCursor++;
    }
    else if( h->ulRowExtractSize == 0 ) {
        odbPBYTE pbyRowsSentSync;

        h->bNoData = TRUE;
        h->bGotRow = FALSE;

        if( !odbSendRequestByte( hQry, ODBTP_FETCHROW, (odbBYTE)h->ulId, TRUE ) )
            return( FALSE );

        if( !odbReadResponse( hQry ) ) return( FALSE );

        if( h->ulResponseCode != ODBTP_ROWDATA ) return( TRUE );

        pbyRowsSentSync = h->pbyExtractData + h->ulExtractSize - 8;
        if( odbReadDataLong( pbyRowsSentSync ) == ODB_SENTSYNC ) {
            *pbyRowsSentSync = 0;
            h->ulExtractSize -= 8;
        }
    }
    else {
        odbInitExtract( hQry, h->pbyRowExtractData, h->ulRowExtractSize );
    }
    return( odbProcessRowData( hQry ) );
}

_odbdecl
odbBOOL odbFetchRowEx( odbHANDLE hQry, odbUSHORT usFetchType,
                       odbLONG lFetchParam )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    odbRestoreRowDataBytes( hQry );

    if( h->lRowCursor ) {
        odbROW  Row;
        odbLONG lRow;

        switch( usFetchType ) {
            case ODB_FETCH_FIRST:
                lRow = 1;
                break;

            case ODB_FETCH_LAST:
                lRow = h->lTotalRows;
                break;

            case ODB_FETCH_PREV:
                lFetchParam = -1;

            case ODB_FETCH_REL:
                if( h->bGotRow || h->lRowCursor == 1 )
                    lRow = h->lRowCursor + lFetchParam - 1;
                else
                    lRow = h->lRowCursor + lFetchParam;
                break;

            case ODB_FETCH_ABS:
                lRow = lFetchParam;
                break;

            default:
                lRow = h->lRowCursor;
        }
        if( lRow < 1 || lRow > h->lTotalRows ) {
            h->bNoData = TRUE;
            h->bGotRow = FALSE;
            h->lRowCursor = lRow > 0 ? h->lTotalRows + 1 : 1;
            return( odbSetError( hQry, ODBTPERR_NONE ) );
        }
        Row = h->Rows + lRow - 1;
        odbInitExtract( hQry, Row->pbyData, Row->ulSize );
        h->lRowCursor = lRow + 1;
    }
    else {
        odbPBYTE pbyRowsSentSync;

        h->bNoData = TRUE;
        h->bGotRow = FALSE;
        h->ulRowExtractSize = 0;

        if( !odbSendRequestByte( hQry, ODBTP_FETCHROW, (odbBYTE)h->ulId, FALSE ) )
            return( FALSE );
        if( !odbSendRequestShort( hQry, ODBTP_FETCHROW, usFetchType, FALSE ) )
            return( FALSE );
        if( !odbSendRequestLong( hQry, ODBTP_FETCHROW, (odbULONG)lFetchParam, TRUE ) )
            return( FALSE );

        if( !odbReadResponse( hQry ) ) return( FALSE );

        if( h->ulResponseCode != ODBTP_ROWDATA ) return( TRUE );

        pbyRowsSentSync = h->pbyExtractData + h->ulExtractSize - 8;
        if( odbReadDataLong( pbyRowsSentSync ) == ODB_SENTSYNC ) {
            *pbyRowsSentSync = 0;
            h->ulExtractSize -= 8;
        }
    }
    return( odbProcessRowData( hQry ) );
}

_odbdecl
odbBOOL odbFetchRowsIntoCache( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;
    odbLONG   lLen;
    odbLONG   lRow;
    odbPBYTE  pbyRowData;
    odbROW    Row;
    odbULONG  ulRowDataSize;
    odbULONG  ulSize;
    odbULONG  ulTotalSize = 0;
    odbUSHORT usCol;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    h->bGotRow = FALSE;
    h->lRowCursor = 1;
    h->lTotalRows = 0;

    if( !odbSendRequestByte( hQry, ODBTP_FETCHROW, (odbBYTE)h->ulId, TRUE ) )
        return( FALSE );

    if( !odbReadResponse( hQry ) ) return( FALSE );

    if( h->ulResponseCode != ODBTP_ROWDATA ) return( TRUE );

    pbyRowData = (odbPBYTE)h->pRowDataBuf;
    ulRowDataSize = h->ulRowDataSize - 8;
    if( odbReadDataLong( pbyRowData + ulRowDataSize ) != ODB_SENTSYNC )
        return( odbSetError( hQry, ODBTPERR_RESPONSE ) );
    h->lTotalRows = (odbLONG)odbReadDataLong( pbyRowData + ulRowDataSize + 4 );
    *(pbyRowData + ulRowDataSize) = 0;

    if( h->lTotalRows > h->lMaxRows ) {
        odbULONG ulBytes;

        if( h->Rows ) free( h->Rows );
        if( (h->lMaxRows = h->lTotalRows) < 256 ) h->lMaxRows = 256;
        ulBytes = h->lMaxRows * sizeof(odbROW_s);
        if( !(h->Rows = (odbROW)malloc( ulBytes )) )
            return( odbSetError( hQry, ODBTPERR_MEMORY ) );
    }
    for( lRow = 0; lRow < h->lTotalRows; lRow++ ) {
        Row = h->Rows + lRow;
        Row->pbyData = pbyRowData;
        Row->ulSize = 0;

        for( usCol = 0; usCol < h->usTotalCols; usCol++ ) {
            if( (lLen = (odbLONG)odbReadDataLong( pbyRowData )) < 0 ) {
                if( lLen == ODB_TRUNCATION ) {
                    pbyRowData += 2 * sizeof(odbLONG);
                    Row->ulSize += 2 * sizeof(odbLONG);
                    lLen = (odbLONG)odbReadDataLong( pbyRowData );
                }
                else {
                    lLen = 0;
                }
            }
            ulSize = (odbULONG)lLen + sizeof(odbLONG);
            pbyRowData += ulSize;
            Row->ulSize += ulSize;
        }
        ulTotalSize += Row->ulSize;
    }
    if( ulTotalSize != ulRowDataSize )
        return( odbSetError( hQry, ODBTPERR_RESPONSE ) );
    return( TRUE );
}

_odbdecl
odbBOOL odbFinalizeRequest( odbHANDLE hOdb )
{
    odbUSHORT usCode = (odbUSHORT)((odbHODBTP)hOdb)->ulRequestCode;

    if( usCode == 0 ) return( odbSetError( hOdb, ODBTPERR_NOREQUEST ) );
    if( !odbSendRequest( hOdb, usCode, NULL, 0, TRUE ) ) return( FALSE );
    return( odbReadResponse( hOdb ) );
}

_odbdecl
void odbFree( odbHANDLE hOdb )
{
    if( !hOdb ) return;

    if( ((odbHODBTP)hOdb)->ulType == ODBTPHANDLE_CONNECTION )
        odbFreeConnection( (odbHCONNECTION)hOdb );
    else
        odbFreeQuery( (odbHQUERY)hOdb );
}

void odbFreeConnection( odbHCONNECTION hCon )
{
    odbULONG ul;

    if( !hCon ) return;

    if( hCon->phQrys ) {
        for( ul = 0; ul < hCon->ulMaxQrys; ul++ )
            odbFreeQuery( (odbHQUERY)*(hCon->phQrys + ul) );
        free( hCon->phQrys );
    }
    if( hCon->DataTypes ) free( hCon->DataTypes );
    if( hCon->pResponseBuf ) free( hCon->pResponseBuf );
    odbSockFree( hCon->Sock );
    free( hCon );
}

void odbFreeQuery( odbHQUERY hQry )
{
    if( !hQry ) return;

    if( hQry->hCon && hQry->ulId != 0xFFFFFFFF )
        *(((odbHCONNECTION)hQry->hCon)->phQrys + hQry->ulId) = NULL;

    if( hQry->Rows ) free( hQry->Rows );
    if( hQry->pRowDataBuf ) free( hQry->pRowDataBuf );
    if( hQry->Params ) free( hQry->Params );
    if( hQry->pParamDataBuf ) free( hQry->pParamDataBuf );
    if( hQry->pParamInfoBuf ) free( hQry->pParamInfoBuf );
    if( hQry->Cols ) free( hQry->Cols );
    if( hQry->pColInfoBuf ) free( hQry->pColInfoBuf );
    if( hQry->pNormalBuf ) free( hQry->pNormalBuf );
    free( hQry );
}

_odbdecl
odbBOOL odbGetAttrLong( odbHANDLE hCon, odbLONG lAttr, odbPULONG pulVal )
{
    odbUSHORT usType = 0;

    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !odbSendRequestLong( hCon, ODBTP_GETATTR, lAttr, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hCon, ODBTP_GETATTR, usType, TRUE ) )
        return( FALSE );
    if( !odbReadResponse( hCon ) ) return( FALSE );

    if( !odbExtractLong( hCon, pulVal ) ) return( FALSE );

    return( TRUE );
}

_odbdecl
odbBOOL odbGetAttrText( odbHANDLE hCon, odbLONG lAttr,
                        odbPSTR pszVal, odbULONG ulValLen )
{
    odbPSTR   pszData;
    odbULONG  ulLen;
    odbUSHORT usType = 1;

    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !odbSendRequestLong( hCon, ODBTP_GETATTR, lAttr, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hCon, ODBTP_GETATTR, usType, TRUE ) )
        return( FALSE );
    if( !odbReadResponse( hCon ) ) return( FALSE );

    if( !odbExtractLong( hCon, &ulLen ) ) return( FALSE );
    if( !odbExtractPointer( hCon, (odbPVOID*)&pszData, ulLen ) ) return( FALSE );

    ulLen = ulLen < ulValLen ? ulLen : ulValLen - 1;
    if( ulLen ) strncpy( pszVal, pszData, ulLen );
    pszVal[ulLen] = 0;

    return( TRUE );
}

odbBOOL odbGetColInfo( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( !odbSendRequestByte( hQry, ODBTP_GETCOLINFO, (odbBYTE)h->ulId, TRUE ) )
        return( FALSE );

    if( !odbReadResponse( hQry ) ) return( FALSE );

    return( odbProcessColInfo( hQry ) );
}

_odbdecl
odbHANDLE odbGetConnection( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( NULL );
    }
    return( h->hCon );
}

_odbdecl
odbPCSTR odbGetConnectionId( odbHANDLE hCon )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( NULL );
    }
    if( !odbSendRequest( hCon, ODBTP_GETCONNID, NULL, 0, TRUE ) )
        return( FALSE );
    if( !odbReadResponse( hCon ) ) return( NULL );
    return( (odbPCSTR)odbGetResponse( hCon ) );
}

odbBOOL odbGetCursor( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( !odbSendRequestByte( hQry, ODBTP_GETCURSOR, (odbBYTE)h->ulId, TRUE ) )
        return( FALSE );

    if( !odbReadResponse( hQry ) ) return( FALSE );

    return( odbProcessCursor( hQry ) );
}

odbPCSTR odbGetDataTypeName( odbHANDLE hCon, odbSHORT sSqlType,
                             odbULONG ulColSize, odbSHORT sDecDigits )
{
    odbDATATYPE    DataType;
    odbHCONNECTION h = (odbHCONNECTION)hCon;

    if( (DataType = h->DataTypes) ) {
        odbDATATYPE FirstDataType = NULL;
        odbULONG    ul;

        for( ul = 0; ul < h->ulTotalDataTypes; ul++, DataType++ ) {
            if( DataType->sSqlType != sSqlType ) continue;
            if( !FirstDataType ) FirstDataType = DataType;

            if( DataType->ulColSize == ulColSize &&
                DataType->sDecDigits == sDecDigits )
            {
                return( DataType->szName );
            }
        }
        if( FirstDataType ) return( FirstDataType->szName );
    }
    return( "" );
}

_odbdecl
odbULONG odbGetError( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->ulError );
}

_odbdecl
odbPCSTR odbGetErrorText( odbHANDLE hOdb )
{
    switch( ((odbHODBTP)hOdb)->ulError ) {
        case ODBTPERR_NONE:         return( "None" );
        case ODBTPERR_MEMORY:       return( "Unable to allocate memory" );
        case ODBTPERR_HANDLE:       return( "Invalid handle" );
        case ODBTPERR_CONNECT:      return( "Unable to connect to server" );
        case ODBTPERR_READ:         return( "Unable to read from server" );
        case ODBTPERR_SEND:         return( "Unable to send to server" );
        case ODBTPERR_TIMEOUTCONN:  return( "Connection time out" );
        case ODBTPERR_TIMEOUTREAD:  return( "Read time out" );
        case ODBTPERR_TIMEOUTSEND:  return( "Send time out" );
        case ODBTPERR_CONNECTED:    return( "Already connected" );
        case ODBTPERR_PROTOCOL:     return( "Protocol violation" );
        case ODBTPERR_RESPONSE:     return( "Invalid Response" );
        case ODBTPERR_MAXQUERYS:    return( "Max querys allocated" );
        case ODBTPERR_COLNUMBER:    return( "Bad column number" );
        case ODBTPERR_COLNAME:      return( "Bad column name" );
        case ODBTPERR_FETCHROW:     return( "Did not fetch row" );
        case ODBTPERR_NOTPREPPROC:  return( "Not prepared procedure" );
        case ODBTPERR_NOPARAMINFO:  return( "Parameter info not available" );
        case ODBTPERR_PARAMNUMBER:  return( "Bad parameter number" );
        case ODBTPERR_PARAMNAME:    return( "Bad parameter name" );
        case ODBTPERR_PARAMBIND:    return( "Parameter not bound" );
        case ODBTPERR_PARAMGET:     return( "Did not get parameter" );
        case ODBTPERR_ATTRTYPE:     return( "Invalid attribute response type" );
        case ODBTPERR_GETQUERY:     return( "Unable to get query" );
        case ODBTPERR_INTERFFILE:   return( "Unable to load interface file" );
        case ODBTPERR_INTERFSYN:    return( "Interface syntax error" );
        case ODBTPERR_INTERFTYPE:   return( "Unknown interface type" );
        case ODBTPERR_CONNSTRLEN:   return( "DB Connect string to long" );
        case ODBTPERR_NOSEEKCURSOR: return( "Seek not allowed when using cursor" );
        case ODBTPERR_SEEKROWPOS:   return( "Invalid seek row position" );
        case ODBTPERR_DETACHED:     return( "Detached object" );
        case ODBTPERR_GETTYPEINFO:  return( "Unable to execute SQLGetTypeInfo" );
        case ODBTPERR_LOADTYPES:    return( "Unable to load data types" );
        case ODBTPERR_NOREQUEST:    return( "No request pending" );
        case ODBTPERR_FETCHEDROWS:  return( "Not allowed after fetched rows" );
        case ODBTPERR_DISCONNECTED: return( "Disconnected from server" );
        case ODBTPERR_HOSTRESOLVE:  return( "Unable to resolve server hostname" );
        case ODBTPERR_SERVER:       return( odbGetResponse( hOdb ) );
    }
    return( "Unknown" );
}

_odbdecl
odbHANDLE odbGetFirstQuery( odbHANDLE hCon )
{
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    odbULONG       ul;

    if( h->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( NULL );
    }
    for( ul = 0; ul < h->ulMaxQrys; ul++ ) {
        if( *(h->phQrys + ul) ) {
            h->ulQryCursor = ul;
            return( *(h->phQrys + ul) );
        }
    }
    return( NULL );
}

odbBOOL odbGetInterface( odbPINTERFACE pInterface,
                         odbPSTR pszInterfaces, odbPCSTR pszName )
{
    odbBOOL bCheckItem = FALSE;
    odbBOOL bGotInterface = FALSE;
    odbPSTR psz = pszInterfaces;
    odbPSTR pszInterface;
    odbPSTR pszItem;
    odbPSTR pszLine;
    odbPSTR pszValue;
    odbPSTR pszX;
    odbPSTR pszY;

    while( *psz ) {
        for( pszLine = psz; *psz && *psz != '\n'; psz++ );
        pszX = psz;
        if( *psz ) psz++;
        for( ; pszX > pszLine && (odbBYTE)*(pszX - 1) <= ' '; pszX-- );
        *pszX = 0;
        for( ; *pszLine && (odbBYTE)*pszLine <= ' '; pszLine++ );

        if( *pszLine == 0 || *pszLine == ';' || *pszLine == '#' ) continue;

        if( *pszLine == '[' ) {
            if( bGotInterface ) return( TRUE );

            bCheckItem = FALSE;
            pszInterface = ++pszLine;
            for( ; *pszLine && *pszLine != ']'; pszLine++ );
            if( *pszLine == 0 ) return( FALSE );
            *pszLine = 0;

            if( !strcasecmp( pszInterface, "global" ) ||
                (bGotInterface = !strcasecmp( pszInterface, pszName )) )
            {
                bCheckItem = TRUE;
            }
        }
        else if( bCheckItem ) {
            pszItem = pszLine;
            for( pszX = pszLine; *pszX && *pszX != '='; pszX++ );
            if( *pszX != '=' ) return( FALSE );

            pszY = (pszX++);
            for( pszY--; pszY >= pszItem && (odbBYTE)*pszY <= ' '; pszY-- );
            *(pszY + 1) = 0;

            for( ; *pszX && (odbBYTE)*pszX <= ' '; pszX++ );
            pszValue = pszX;

            if( !strcasecmp( pszItem, "type" ) )
                pInterface->pszType = pszValue;
            else if( !strcasecmp( pszItem, "odbtp host" ) )
                pInterface->pszOdbtpHost = pszValue;
            else if( !strcasecmp( pszItem, "odbtp port" ) )
                pInterface->ulOdbtpPort = atol( pszValue );
            else if( !strcasecmp( pszItem, "driver" ) )
                pInterface->pszDriver = pszValue;
            else if( !strcasecmp( pszItem, "server" ) )
                pInterface->pszServer = pszValue;
            else if( !strcasecmp( pszItem, "database" ) )
                pInterface->pszDatabase = pszValue;
            else if( !strcasecmp( pszItem, "data connect string" ) )
                pInterface->pszDBConnect = pszValue;
            else if( !strcasecmp( pszItem, "login type" ) )
                pInterface->pszLoginType = pszValue;
            else if( !strcasecmp( pszItem, "use row cache" ) )
                pInterface->bUseRowCache = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "row cache size" ) )
                pInterface->ulRowCacheSize = atol( pszValue );
            else if( !strcasecmp( pszItem, "full col info" ) )
                pInterface->bFullColInfo = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "load data types" ) )
                pInterface->bLoadDataTypes = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "convert all" ) )
                pInterface->bConvertAll = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "convert datetime" ) )
                pInterface->bConvertDatetime = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "convert guid" ) )
                pInterface->bConvertGuid = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "unicode sql" ) )
                pInterface->bUnicodeSQL = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "right trim text" ) )
                pInterface->bRightTrimText = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "var data size" ) )
                pInterface->ulVarDataSize = atol( pszValue );
            else if( !strcasecmp( pszItem, "use broad types" ) )
                pInterface->bUseBroadTypes = IS_TRUE_VALUE( *pszValue );
            else if( !strcasecmp( pszItem, "connect timeout" ) )
                pInterface->ulConnectTimeout = atoi( pszValue );
            else if( !strcasecmp( pszItem, "read timeout" ) )
                pInterface->ulReadTimeout = atoi( pszValue );
        }
    }
    return( TRUE );
}

_odbdecl
odbHANDLE odbGetNextQuery( odbHANDLE hCon )
{
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    odbULONG       ul;

    if( h->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( NULL );
    }
    for( ul = h->ulQryCursor + 1; ul < h->ulMaxQrys; ul++ ) {
        if( *(h->phQrys + ul) ) {
            h->ulQryCursor = ul;
            return( *(h->phQrys + ul) );
        }
    }
    return( NULL );
}

odbSHORT odbGetOdbDataType( odbSHORT sSqlType )
{
    switch( sSqlType ) {
        case SQL_BIT:            return( ODB_BIT );
        case SQL_TINYINT:        return( ODB_UTINYINT );
        case SQL_SMALLINT:       return( ODB_SMALLINT );
        case SQL_INTEGER:        return( ODB_INT );
        case SQL_BIGINT:         return( ODB_BIGINT );
        case SQL_REAL:           return( ODB_REAL );
        case SQL_FLOAT:
        case SQL_DOUBLE:         return( ODB_DOUBLE );
        case SQL_TYPE_TIMESTAMP: return( ODB_DATETIME );
        case SQL_GUID:           return( ODB_GUID );
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:  return( ODB_BINARY );
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:   return( ODB_WCHAR );
    }
    return( sSqlType ? ODB_CHAR : 0 );
}

_odbdecl
odbBOOL odbGetOutputParams( odbHANDLE hQry )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;
    odbUSHORT    usParam;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( h->usTotalParams == 0 )
        return( odbSetError( hQry, ODBTPERR_NONE ) );

    h->ulRequestCode = 0;

    for( usParam = 1; usParam <= h->usTotalParams; usParam++ ) {
        Param = (h->Params + usParam - 1);
        if( !Param->bBound || !(Param->usType & ODB_PARAM_OUTPUT) ) {
            continue;
        }
        if( h->ulRequestCode != ODBTP_GETPARAM &&
            !odbSendRequestByte( hQry, ODBTP_GETPARAM, (odbBYTE)h->ulId, FALSE ) )
        {
            return( FALSE );
        }
        if( !odbSendRequestShort( hQry, ODBTP_GETPARAM, usParam, FALSE ) )
            return( FALSE );
    }
    if( h->ulRequestCode != ODBTP_GETPARAM )
        return( odbSetError( hQry, ODBTPERR_NONE ) );
    if( !odbSendRequest( hQry, ODBTP_GETPARAM, NULL, 0, TRUE ) )
        return( FALSE );
    if( !odbReadResponse( hQry ) )
        return( FALSE );

    for( usParam = 0; usParam < h->usTotalParams; usParam++ )
        (h->Params + usParam)->bGot = FALSE;

    do {
        if( !odbExtractShort( hQry, &usParam ) )
            return( FALSE );
        if( usParam == 0 || usParam > h->usTotalParams )
            return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );

        Param = (h->Params + usParam - 1);
        if( !odbExtractLong( hQry, (odbPULONG)&Param->lDataLen ) )
            return( FALSE );

        *(h->pbyExtractData - sizeof(odbULONG) - sizeof(odbUSHORT)) = 0;

        if( Param->lDataLen == ODB_TRUNCATION ) {
            Param->bTruncated = TRUE;
            if( !odbExtractLong( hQry, (odbPULONG)&Param->lActualLen ) )
                return( FALSE );
            if( !odbExtractLong( hQry, (odbPULONG)&Param->lDataLen ) )
                return( FALSE );
        }
        else {
            Param->bTruncated = FALSE;
            Param->lActualLen = Param->lDataLen;
        }
        if( Param->lDataLen < 0 ) {
            Param->bGot = TRUE;
            continue;
        }
        switch( Param->sDataType ) {
            case ODB_BIT:
            case ODB_TINYINT:
            case ODB_UTINYINT:
                if( !odbExtractByte( hQry, &Param->Data.byVal ) )
                    return( FALSE );
                break;

            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( !odbExtractShort( hQry, &Param->Data.usVal ) )
                    return( FALSE );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( !odbExtractLong( hQry, &Param->Data.ulVal ) )
                    return( FALSE );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( !odbExtractLongLong( hQry, &Param->Data.ullVal ) )
                    return( FALSE );
                break;

            case ODB_REAL:
                if( !odbExtractFloat( hQry, &Param->Data.fVal ) )
                    return( FALSE );
                break;

            case ODB_DOUBLE:
                if( !odbExtractDouble( hQry, &Param->Data.dVal ) )
                    return( FALSE );
                break;

            case ODB_DATETIME:
                if( !odbExtractTimestamp( hQry, &Param->Data.tsVal ) )
                    return( FALSE );
                break;

            case ODB_GUID:
                if( !odbExtractGuid( hQry, &Param->Data.guidVal ) )
                    return( FALSE );
                break;

            default:
                if( !odbExtractPointer( hQry, &Param->Data.pVal, Param->lDataLen ) )
                    return( FALSE );
        }
        Param->bGot = TRUE;
    }
    while( h->ulExtractSize != 0 );

    return( TRUE );
}

_odbdecl
odbBOOL odbGetParam( odbHANDLE hQry, odbUSHORT usParam, odbBOOL bFinal )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usParam == 0 || usParam > h->usTotalParams )
        return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );

    Param = (h->Params + usParam - 1);

    if( !Param->bBound )
        return( odbSetError( hQry, ODBTPERR_PARAMBIND ) );

    if( h->ulRequestCode != ODBTP_GETPARAM &&
        !odbSendRequestByte( hQry, ODBTP_GETPARAM, (odbBYTE)h->ulId, FALSE ) )
    {
        return( FALSE );
    }
    if( !odbSendRequestShort( hQry, ODBTP_GETPARAM, usParam, bFinal ) )
        return( FALSE );

    if( !bFinal ) return( TRUE );
    if( !odbReadResponse( hQry ) ) return( FALSE );

    for( usParam = 0; usParam < h->usTotalParams; usParam++ )
        (h->Params + usParam)->bGot = FALSE;

    do {
        if( !odbExtractShort( hQry, &usParam ) )
            return( FALSE );
        if( usParam == 0 || usParam > h->usTotalParams )
            return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );

        Param = (h->Params + usParam - 1);
        if( !odbExtractLong( hQry, (odbPULONG)&Param->lDataLen ) )
            return( FALSE );

        *(h->pbyExtractData - sizeof(odbULONG) - sizeof(odbUSHORT)) = 0;

        if( Param->lDataLen == ODB_TRUNCATION ) {
            Param->bTruncated = TRUE;
            if( !odbExtractLong( hQry, (odbPULONG)&Param->lActualLen ) )
                return( FALSE );
            if( !odbExtractLong( hQry, (odbPULONG)&Param->lDataLen ) )
                return( FALSE );
        }
        else {
            Param->bTruncated = FALSE;
            Param->lActualLen = Param->lDataLen;
        }
        if( Param->lDataLen < 0 ) {
            Param->bGot = TRUE;
            continue;
        }
        switch( Param->sDataType ) {
            case ODB_BIT:
            case ODB_TINYINT:
            case ODB_UTINYINT:
                if( !odbExtractByte( hQry, &Param->Data.byVal ) )
                    return( FALSE );
                break;

            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( !odbExtractShort( hQry, &Param->Data.usVal ) )
                    return( FALSE );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( !odbExtractLong( hQry, &Param->Data.ulVal ) )
                    return( FALSE );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( !odbExtractLongLong( hQry, &Param->Data.ullVal ) )
                    return( FALSE );
                break;

            case ODB_REAL:
                if( !odbExtractFloat( hQry, &Param->Data.fVal ) )
                    return( FALSE );
                break;

            case ODB_DOUBLE:
                if( !odbExtractDouble( hQry, &Param->Data.dVal ) )
                    return( FALSE );
                break;

            case ODB_DATETIME:
                if( !odbExtractTimestamp( hQry, &Param->Data.tsVal ) )
                    return( FALSE );
                break;

            case ODB_GUID:
                if( !odbExtractGuid( hQry, &Param->Data.guidVal ) )
                    return( FALSE );
                break;

            default:
                if( !odbExtractPointer( hQry, &Param->Data.pVal, Param->lDataLen ) )
                    return( FALSE );
        }
        Param->bGot = TRUE;
    }
    while( h->ulExtractSize != 0 );

    return( TRUE );
}

odbBOOL odbGetParamInfo( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( !odbSendRequestByte( hQry, ODBTP_GETPARAMINFO, (odbBYTE)h->ulId, TRUE ) )
        return( FALSE );

    if( !odbReadResponse( hQry ) ) return( FALSE );

    return( odbProcessParamInfo( hQry ) );
}

_odbdecl
odbHANDLE odbGetQuery( odbHANDLE hCon, odbULONG ulQryId )
{
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    odbHQUERY      hQry;
    odbPHANDLE     phQry;

    if( h->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( NULL );
    }
    if( ulQryId >= h->ulMaxQrys ) {
        odbSetError( hCon, ODBTPERR_MAXQUERYS );
        return( NULL );
    }
    if( *(phQry = (h->phQrys + ulQryId)) ) {
        odbSetError( hCon, ODBTPERR_NONE );
        return( *phQry );
    }
    if( !(hQry = (odbHQUERY)odbAllocateQuery()) ) {
        odbSetError( hCon, ODBTPERR_MEMORY );
        return( NULL );
    }
    hQry->Sock = h->Sock;
    hQry->hCon = hCon;
    hQry->ulId = ulQryId;
    hQry->bUseBroadTypes = h->bUseBroadTypes;
    *phQry = (odbHANDLE)hQry;

    if( !odbGetColInfo( *phQry ) || !odbGetParamInfo( *phQry ) ||
        !odbGetCursor( *phQry ) )
    {
        odbSetError( hCon, ODBTPERR_GETQUERY );
        odbFree( *phQry );
        return( NULL );
    }
    return( (odbHANDLE)hQry );
}

_odbdecl
odbULONG odbGetQueryId( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0xFFFFFFFF );
    }
    return( h->ulId );
}

_odbdecl
odbPVOID odbGetResponse( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->pResponseBuf );
}

_odbdecl
odbULONG odbGetResponseCode( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->ulResponseCode );
}

_odbdecl
odbULONG odbGetResponseSize( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->ulResponseSize );
}

_odbdecl
odbULONG odbGetRowCacheSize( odbHANDLE hCon )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( 0xFFFFFFFF );
    }
    return( ((odbHCONNECTION)hCon)->ulRowCacheSize );
}

_odbdecl
odbLONG odbGetRowCount( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( -1 );
    }
    if( h->ulResponseCode != ODBTP_ROWCOUNT ) {
        if( !odbSendRequestByte( hQry, ODBTP_GETROWCOUNT, (odbBYTE)h->ulId, TRUE ) )
            return( -1 );
        if( !odbReadResponse( hQry ) )
            return( -1 );
        if( !odbExtractLong( hQry, (odbPULONG)&h->lRowCount ) )
            return( -1 );
    }
    return( h->lRowCount );
}

_odbdecl
odbULONG odbGetRowStatus( odbHANDLE hQry )
{
    odbULONG ulStatus;

    if( !odbDoRowOperation( hQry, ODB_ROW_GETSTATUS ) )
        return( ODB_ROWSTAT_UNKNOWN );
    if( !odbExtractLong( hQry, &ulStatus ) )
        return( ODB_ROWSTAT_UNKNOWN );

    return( ulStatus );
}

_odbdecl
odbLONG odbGetSockError( odbHANDLE hOdb )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    return( h->Sock ? h->Sock->lError : 0 );
}

_odbdecl
odbPCSTR odbGetSockErrorText( odbHANDLE hOdb )
{
    return( get_sock_error_text( odbGetSockError( hOdb ) ) );
}

_odbdecl
odbUSHORT odbGetTotalCols( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0 );
    }
    return( h->usTotalCols );
}

_odbdecl
odbUSHORT odbGetTotalParams( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0 );
    }
    return( h->usTotalParams );
}

_odbdecl
odbLONG odbGetTotalRows( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0 );
    }
    return( h->lTotalRows );
}

_odbdecl
odbPVOID odbGetUserData( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->UserData.pVal );
}

_odbdecl
odbULONG odbGetUserDataLong( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->UserData.ulVal );
}

_odbdecl
odbPCSTR odbGetVersion( odbHANDLE hCon )
{
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    if( h->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( NULL );
    }
    odbSetError( hCon, ODBTPERR_NONE );
    return( h->szVersion );
}

_odbdecl
odbBOOL odbGotParam( odbHANDLE hQry, odbUSHORT usParam )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ||
        usParam == 0 || usParam > h->usTotalParams )
    {
        return( FALSE );
    }
    return( (h->Params + usParam - 1)->bGot );
}

_odbdecl
odbPSTR odbGuidToStr( odbPSTR pszStr, odbPGUID pguidVal )
{
    int      n;
    odbPSTR  psz = pszStr;
    odbPCSTR pszHex = "0123456789ABCDEF";

    for( n = 28; n >= 0; n -= 4 )
        *(psz++) = *(pszHex + ((pguidVal->ulData1 >> n) & 0xF));
    *(psz++) = '-';

    for( n = 12; n >= 0; n -= 4 )
        *(psz++) = *(pszHex + ((pguidVal->usData2 >> n) & 0xF));
    *(psz++) = '-';

    for( n = 12; n >= 0; n -= 4 )
        *(psz++) = *(pszHex + ((pguidVal->usData3 >> n) & 0xF));
    *(psz++) = '-';

    *(psz++) = *(pszHex + ((pguidVal->byData4[0] >> 4) & 0xF));
    *(psz++) = *(pszHex + (pguidVal->byData4[0] & 0xF));
    *(psz++) = *(pszHex + ((pguidVal->byData4[1] >> 4) & 0xF));
    *(psz++) = *(pszHex + (pguidVal->byData4[1] & 0xF));
    *(psz++) = '-';

    for( n = 2; n < 8; n++ ) {
        *(psz++) = *(pszHex + ((pguidVal->byData4[n] >> 4) & 0xF));
        *(psz++) = *(pszHex + (pguidVal->byData4[n] & 0xF));
    }
    *psz = 0;

    return( pszStr );
}

void odbHostToNetworkOrder( odbPBYTE pbyTo, odbPBYTE pbyFrom, odbULONG ulLen )
{
    odbULONG n;
    short    s = 1;
    char*    sp = (char*)&s;

    if( *sp == 1 ) {
        pbyFrom += ulLen - 1;
        for( n = 0; n < ulLen; n++ ) *(pbyTo++) = *(pbyFrom--);
    }
    else {
        for( n = 0; n < ulLen; n++ ) *(pbyTo++) = *(pbyFrom++);
    }
}

void odbInitExtract( odbHANDLE hOdb, odbPVOID pData, odbULONG ulSize )
{
    ((odbHODBTP)hOdb)->pbyExtractData = (odbPBYTE)pData;
    ((odbHODBTP)hOdb)->ulExtractSize = ulSize;
}

void odbInitResponseBuf( odbHANDLE hOdb )
{
    if( !odbIsConnection( hOdb ) ) {
        odbHQUERY h = (odbHQUERY)hOdb;

        switch( h->ulResponseCode ) {
            case ODBTP_COLINFO:
            case ODBTP_COLINFOEX:
                h->pResponseBuf = h->pColInfoBuf;
                h->ulResponseBufSize = h->ulColInfoBufSize;
                break;

            case ODBTP_PARAMINFO:
            case ODBTP_PARAMINFOEX:
                h->pResponseBuf = h->pParamInfoBuf;
                h->ulResponseBufSize = h->ulParamInfoBufSize;
                break;

            case ODBTP_PARAMDATA:
                h->pResponseBuf = h->pParamDataBuf;
                h->ulResponseBufSize = h->ulParamDataBufSize;
                break;

            case ODBTP_ROWDATA:
                h->pResponseBuf = h->pRowDataBuf;
                h->ulResponseBufSize = h->ulRowDataBufSize;
                h->ulRowExtractSize = 0;
                break;

            default:
                h->pResponseBuf = h->pNormalBuf;
                h->ulResponseBufSize = h->ulNormalBufSize;
        }
    }
}

_odbdecl
odbBOOL odbIsConnected( odbHANDLE hOdb )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    return( h->Sock && h->Sock->sock != INVALID_SOCKET );
}

_odbdecl
odbBOOL odbIsConnection( odbHANDLE hOdb )
{
    return( ((odbHODBTP)hOdb)->ulType == ODBTPHANDLE_CONNECTION );
}

_odbdecl
odbBOOL odbIsTextAttr( odbLONG lAttr )
{
    return( (lAttr & ODB_ATTR_STRING) ? TRUE : FALSE );
}

_odbdecl
odbBOOL odbIsUsingRowCache( odbHANDLE hCon )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION ) {
        odbSetError( hCon, ODBTPERR_HANDLE );
        return( 0xFFFFFFFF );
    }
    return( ((odbHCONNECTION)hCon)->bUseRowCache );
}

_odbdecl
odbBOOL odbLoadDataTypes( odbHANDLE hCon )
{
    odbBOOL        bOK;
    odbDATATYPE    DataType;
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    odbHANDLE      hQry;
    odbULONG       ul = 0;
    odbULONG       ulMax = 48;

    if( h->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( h->DataTypes ) return( odbSetError( hCon, ODBTPERR_NONE ) );

    if( !(hQry = odbAllocate( hCon )) ) return( FALSE );

    if( !odbExecute( hQry, "||SQLGetTypeInfo" ) ) {
        odbFree( hQry );
        return( odbSetError( hCon, ODBTPERR_GETTYPEINFO ) );
    }
    if( odbGetTotalCols( hQry ) < 14 ) {
        odbFree( hQry );
        return( odbSetError( hCon, ODBTPERR_LOADTYPES ) );
    }
    h->DataTypes = (odbDATATYPE)malloc( sizeof(odbDATATYPE_s) * ulMax );
    if( !h->DataTypes ) {
        odbFree( hQry );
        return( odbSetError( hCon, ODBTPERR_MEMORY ) );
    }
    while( (bOK = odbFetchRow( hQry )) && !odbNoData( hQry ) ) {
        if( ul == ulMax ) {
            ulMax += 8;
            h->DataTypes =
              (odbDATATYPE)realloc( h->DataTypes, sizeof(odbDATATYPE_s) * ulMax );

            if( !h->DataTypes ) {
                odbFree( hQry );
                return( odbSetError( hCon, ODBTPERR_MEMORY ) );
            }
        }
        DataType = h->DataTypes + (ul++);
        strncpy( DataType->szName, odbColDataText( hQry, 1 ), 47 );
        DataType->szName[47] = 0;
        DataType->sSqlType = (odbSHORT)odbColDataShort( hQry, 2 );
        DataType->ulColSize = odbColDataLong( hQry, 3 );
        DataType->sDecDigits = (odbSHORT)odbColDataShort( hQry, 14 );
    }
    odbFree( hQry );

    if( !bOK ) {
        free( h->DataTypes );
        h->DataTypes = NULL;
        return( odbSetError( hCon, ODBTPERR_LOADTYPES ) );
    }
    h->ulTotalDataTypes = ul;

    return( odbSetError( hCon, ODBTPERR_NONE ) );
}

_odbdecl
odbBOOL odbLogin( odbHANDLE hCon, odbPCSTR pszServer, odbUSHORT usPort,
                  odbUSHORT usType, odbPCSTR pszDBConnect )
{
    odbUSHORT usLen = strlen( pszDBConnect );

    if( usPort == 0 ) usPort = 2799;

    if( !odbConnect( hCon, pszServer, usPort ) ) return( FALSE );

    if( !odbSendRequestShort( hCon, ODBTP_LOGIN, usType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hCon, ODBTP_LOGIN, usLen, FALSE ) )
        return( FALSE );
    if( !odbSendRequest( hCon, ODBTP_LOGIN, (odbPVOID)pszDBConnect, usLen, TRUE ) )
        return( FALSE );

    if( !odbReadResponse( hCon ) ) return( FALSE );

    return( TRUE );
}

_odbdecl
odbBOOL odbLoginInterface( odbHANDLE hCon, odbPCSTR pszFile,
                           odbPCSTR pszInterface, odbPCSTR pszUsername,
                           odbPCSTR pszPassword, odbPCSTR pszDatabase,
                           odbUSHORT usType )
{
    odbHCONNECTION h = (odbHCONNECTION)hCon;
    odbINTERFACE   Interface;
    odbPSTR        pszInterfaces;
    odbCHAR        szDBConnect[512];

    if( h->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !(pszInterfaces = odbReadInterfaceFile( hCon, pszFile )) )
        return( FALSE );

    if( !pszInterface ) pszInterface = "";
    if( !pszUsername ) pszUsername = "";
    if( !pszPassword ) pszPassword = "";
    if( !pszDatabase ) pszDatabase = "";

    Interface.pszType = "dsn";
    Interface.pszOdbtpHost = NULL;
    Interface.ulOdbtpPort = 0;
    Interface.pszDriver = NULL;
    Interface.pszServer = NULL;
    Interface.pszUsername = pszUsername;
    Interface.pszPassword = pszPassword;
    Interface.pszDatabase = pszDatabase;
    Interface.pszDBConnect = NULL;
    Interface.pszLoginType = NULL;
    Interface.bUseRowCache = FALSE;
    Interface.ulRowCacheSize = 0;
    Interface.bFullColInfo = FALSE;
    Interface.bLoadDataTypes = FALSE;
    Interface.bConvertAll = FALSE;
    Interface.bConvertDatetime = FALSE;
    Interface.bConvertGuid = FALSE;
    Interface.bUnicodeSQL = FALSE;
    Interface.bRightTrimText = FALSE;
    Interface.ulVarDataSize = 0;
    Interface.bUseBroadTypes = FALSE;
    Interface.ulConnectTimeout = (odbULONG)-1;
    Interface.ulReadTimeout = (odbULONG)-1;

    if( !odbGetInterface( &Interface, pszInterfaces, pszInterface ) ) {
        free( pszInterfaces );
        return( odbSetError( hCon, ODBTPERR_INTERFSYN ) );
    }
    h->bConvertAll = Interface.bConvertAll;
    h->bConvertDatetime = Interface.bConvertDatetime;
    h->bConvertGuid = Interface.bConvertGuid;
    h->bUseBroadTypes = Interface.bUseBroadTypes;

    if( Interface.ulConnectTimeout != (odbULONG)-1 )
        odbSetConnectTimeout( hCon, Interface.ulConnectTimeout );
    if( Interface.ulReadTimeout != (odbULONG)-1 )
        odbSetReadTimeout( hCon, Interface.ulReadTimeout );

    if( *pszInterface == 0 ) pszInterface = "localhost";

    if( !Interface.pszOdbtpHost ) Interface.pszOdbtpHost = pszInterface;

    if( !strcasecmp( Interface.pszType, "mssql" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "SQL Server";
        if( !Interface.pszServer ) {
            if( !strcasecmp( pszInterface, "localhost" ) )
                Interface.pszServer = "(local)";
            else
                Interface.pszServer = pszInterface;
        }
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};SERVER=%s;UID=%u;PWD=%p;DATABASE=%d;";
    }
    else if( !strcasecmp( Interface.pszType, "sybase" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Sybase ASE ODBC Driver";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};SRVR=%s;UID=%u;PWD=%p;DATABASE=%d;";
    }
    else if( !strcasecmp( Interface.pszType, "oracle" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Oracle ODBC Driver";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};DBQ=%d;UID=%u;PWD=%p;";
    }
    else if( !strcasecmp( Interface.pszType, "access" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Microsoft Access Driver (*.mdb)";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};DBQ=%d;UID=%u;PWD=%p;";
    }
    else if( !strcasecmp( Interface.pszType, "foxpro" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Microsoft Visual FoxPro Driver";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};SOURCETYPE=DBF;SOURCEDB=%d;EXCLUSIVE=NO;";
    }
    else if( !strcasecmp( Interface.pszType, "foxpro_dbc" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Microsoft Visual FoxPro Driver";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};SOURCETYPE=DBC;SOURCEDB=%d;EXCLUSIVE=NO;";
    }
    else if( !strcasecmp( Interface.pszType, "db2" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "IBM DB2 ODBC Driver";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};HOSTNAME=%s;PORT=50000;PROTOCOL=TCPIP;UID=%u;PWD=%p;DATABASE=%d;";
    }
    else if( !strcasecmp( Interface.pszType, "text" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Microsoft Text Driver (*.txt; *.csv)";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};DBQ=%d;EXTENSIONS=asc,csv,tab,txt;";
    }
    else if( !strcasecmp( Interface.pszType, "excel" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "Microsoft Excel Driver (*.xls)";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};DRIVERID=790;DBQ=%d;";
    }
    else if( !strcasecmp( Interface.pszType, "mysql" ) ) {
        if( !Interface.pszDriver )
            Interface.pszDriver = "MySQL ODBC 3.51 Driver";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect =
                "DRIVER={%r};SERVER=%s;OPTION=3;UID=%u;PWD=%p;DATABASE=%d;";
    }
    else if( !strcasecmp( Interface.pszType, "dsn" ) ||
             !strcasecmp( Interface.pszType, "general" ) )
    {
        if( !Interface.pszDriver ) Interface.pszDriver = "";
        if( !Interface.pszServer ) Interface.pszServer = pszInterface;
        if( !Interface.pszDBConnect )
            Interface.pszDBConnect = "DSN=%d;UID=%u;PWD=%p;";
    }
    else {
        free( pszInterfaces );
        return( odbSetError( hCon, ODBTPERR_INTERFTYPE ) );
    }
    if( !odbSetDBConnectStr( szDBConnect, sizeof(szDBConnect), &Interface ) ) {
        free( pszInterfaces );
        return( odbSetError( hCon, ODBTPERR_CONNSTRLEN ) );
    }
    if( Interface.pszLoginType && *Interface.pszLoginType ) {
        if( !strcasecmp( Interface.pszLoginType, "single" ) )
            usType = ODB_LOGIN_SINGLE;
        else if( !strcasecmp( Interface.pszLoginType, "reserved" ) )
            usType = ODB_LOGIN_RESERVED;
        else
            usType = ODB_LOGIN_NORMAL;
    }
    if( !odbLogin( hCon, Interface.pszOdbtpHost,
                   (odbUSHORT)Interface.ulOdbtpPort,
                   usType, szDBConnect ) )
    {
        free( pszInterfaces );
        return( FALSE );
    }
    free( pszInterfaces );

    if( Interface.bUseRowCache &&
        !odbUseRowCache( hCon, TRUE, Interface.ulRowCacheSize ) )
    {
        return( FALSE );
    }
    if( Interface.bUnicodeSQL &&
        !odbSetAttrLong( hCon, ODB_ATTR_UNICODESQL, 1 ) )
    {
        return( FALSE );
    }
    if( Interface.bFullColInfo &&
        !odbSetAttrLong( hCon, ODB_ATTR_FULLCOLINFO, 1 ) )
    {
        return( FALSE );
    }
    if( Interface.bRightTrimText &&
        !odbSetAttrLong( hCon, ODB_ATTR_RIGHTTRIMTEXT, 1 ) )
    {
        return( FALSE );
    }
    if( Interface.ulVarDataSize &&
        !odbSetAttrLong( hCon, ODB_ATTR_VARDATASIZE, Interface.ulVarDataSize ) )
    {
        return( FALSE );
    }
    if( Interface.bLoadDataTypes && !odbLoadDataTypes( hCon ) )
        return( FALSE );

    return( TRUE );
}

_odbdecl
odbBOOL odbLogout( odbHANDLE hCon, odbBOOL bDisconnectDb )
{
    odbBYTE  byDisconnectDb = bDisconnectDb ? 1 : 0;
    odbULONG ulError;

    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );
    if( !odbSendRequestByte( hCon, ODBTP_LOGOUT, byDisconnectDb, TRUE ) )
        return( FALSE );

    odbReadResponse( hCon );
    if( odbGetResponseCode( hCon ) == ODBTP_DISCONNECT )
        ulError = ODBTPERR_NONE;
    else
        ulError = odbGetError( hCon );
    odbDisconnect( hCon );

    return( odbSetError( hCon, ulError ) );
}

_odbdecl
odbPSTR odbLongLongToStr( odbLONGLONG llVal, odbPSTR pszStrEnd )
{
    odbBOOL bIsNegative = FALSE;

    if( llVal < 0 ) {
        bIsNegative = TRUE;
        if( (odbULONGLONG)llVal != 0x8000000000000000 ) llVal = -llVal;
    }
    pszStrEnd = odbULongLongToStr( (odbULONGLONG)llVal, pszStrEnd );
    if( bIsNegative ) *(--pszStrEnd) = '-';
    return( pszStrEnd );
}

_odbdecl
odbBOOL odbNoData( odbHANDLE hQry )
{
    odbHQUERY h = (odbHQUERY)hQry;
    if( h->ulType != ODBTPHANDLE_QUERY ) return( TRUE );
    return( h->bNoData );
}

odbPARAMETER odbParam( odbHANDLE hQry, odbUSHORT usParam, odbBOOL bValid )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( NULL );
    }
    if( usParam == 0 || usParam > h->usTotalParams ) {
        odbSetError( hQry, ODBTPERR_PARAMNUMBER );
        return( NULL );
    }
    Param = (h->Params + usParam - 1);

    if( bValid && !Param->bGot ) {
        odbSetError( hQry, ODBTPERR_PARAMGET );
        return( NULL );
    }
    odbSetError( hQry, ODBTPERR_NONE );
    return( Param );
}

_odbdecl
odbLONG odbParamActualLen( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, TRUE )) ) return( -1 );
    return( Param->lActualLen );
}

_odbdecl
odbPVOID odbParamData( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;

    if( !(Param = odbParam( hQry, usParam, TRUE )) ) return( NULL );

    if( Param->lDataLen < 0 ) return( NULL );

    switch( Param->sDataType ) {
        case ODB_BIT:
        case ODB_TINYINT:
        case ODB_UTINYINT:  return( &Param->Data.byVal );
        case ODB_SMALLINT:
        case ODB_USMALLINT: return( &Param->Data.usVal );
        case ODB_INT:
        case ODB_UINT:      return( &Param->Data.ulVal );
        case ODB_BIGINT:
        case ODB_UBIGINT:   return( &Param->Data.ullVal );
        case ODB_REAL:      return( &Param->Data.fVal );
        case ODB_DOUBLE:    return( &Param->Data.dVal );
        case ODB_DATETIME:  return( &Param->Data.tsVal );
        case ODB_GUID:      return( &Param->Data.guidVal );
    }
    return( Param->Data.pVal );
}

_odbdecl
odbBYTE odbParamDataByte( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPBYTE pbyData = (odbPBYTE)odbParamData( hQry, usParam );
    if( !pbyData ) return( 0 );
    return( *pbyData );
}

_odbdecl
odbDOUBLE odbParamDataDouble( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPDOUBLE pdData = (odbPDOUBLE)odbParamData( hQry, usParam );
    if( !pdData ) return( 0.0 );
    return( *pdData );
}

_odbdecl
odbFLOAT  odbParamDataFloat( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPFLOAT pfData = (odbPFLOAT)odbParamData( hQry, usParam );
    if( !pfData ) return( 0.0 );
    return( *pfData );
}

_odbdecl
odbPGUID odbParamDataGuid( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPGUID pguidData = (odbPGUID)odbParamData( hQry, usParam );
    if( !pguidData ) return( &guidNull );
    return( pguidData );
}

_odbdecl
odbLONG odbParamDataLen( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, TRUE )) ) return( -1 );
    return( Param->lDataLen );
}

_odbdecl
odbULONG odbParamDataLong( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPULONG pulData = (odbPULONG)odbParamData( hQry, usParam );
    if( !pulData ) return( 0 );
    return( *pulData );
}

_odbdecl
odbULONGLONG odbParamDataLongLong( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPULONGLONG pullData = (odbPULONGLONG)odbParamData( hQry, usParam );
    if( !pullData ) return( 0 );
    return( *pullData );
}

_odbdecl
odbUSHORT odbParamDataShort( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPUSHORT pusData = (odbPUSHORT)odbParamData( hQry, usParam );
    if( !pusData ) return( 0 );
    return( *pusData );
}

_odbdecl
odbPSTR odbParamDataText( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPSTR pszData = (odbPSTR)odbParamData( hQry, usParam );
    if( !pszData ) return( "" );
    return( pszData );
}

_odbdecl
odbPTIMESTAMP odbParamDataTimestamp( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPTIMESTAMP ptsData = (odbPTIMESTAMP)odbParamData( hQry, usParam );
    if( !ptsData ) return( &tsNull );
    return( ptsData );
}

_odbdecl
odbSHORT odbParamDataType( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, FALSE )) ) return( 0 );
    return( Param->sDataType );
}

_odbdecl
odbSHORT odbParamDecDigits( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, FALSE )) ) return( 0 );
    return( Param->sDecDigits );
}

_odbdecl
odbSHORT odbParamDefaultDataType( odbHANDLE hQry, odbUSHORT usParam )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0 );
    }
    if( usParam == 0 || usParam > h->usTotalParams ) {
        odbSetError( hQry, ODBTPERR_PARAMNUMBER );
        return( 0 );
    }
    return( (h->Params + usParam - 1)->sDefaultDataType );
}

_odbdecl
odbPCSTR odbParamName( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, FALSE )) ) return( NULL );
    if( !Param->pszName ) {
        odbSetError( hQry, ODBTPERR_NOPARAMINFO );
        return( NULL );
    }
    return( Param->pszName );
}

_odbdecl
odbUSHORT odbParamNum( odbHANDLE hQry, odbPCSTR pszName )
{
    odbHQUERY h = (odbHQUERY)hQry;
    odbUSHORT usParam;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( 0 );
    }
    if( !h->bPreparedProc ) {
        odbSetError( hQry, ODBTPERR_NOTPREPPROC );
        return( 0 );
    }
    for( usParam = 0; usParam < h->usTotalParams; usParam++ )
        if( !strcasecmp( (h->Params + usParam)->pszName, pszName ) )
            return( usParam + 1 );

    odbSetError( hQry, ODBTPERR_PARAMNAME );
    return( 0 );
}

_odbdecl
odbULONG odbParamSize( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, FALSE )) ) return( 0 );
    return( Param->ulColSize );
}

_odbdecl
odbSHORT odbParamSqlType( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, FALSE )) ) return( 0 );
    if( !Param->sSqlType ) {
        odbSetError( hQry, ODBTPERR_NOPARAMINFO );
        return( 0 );
    }
    return( Param->sSqlType );
}

_odbdecl
odbPCSTR odbParamSqlTypeName( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, FALSE )) ) return( NULL );
    if( !Param->pszSqlType ) {
        odbSetError( hQry, ODBTPERR_NOPARAMINFO );
        return( NULL );
    }
    return( Param->pszSqlType );
}

_odbdecl
odbBOOL odbParamTruncated( odbHANDLE hQry, odbUSHORT usParam )
{
    odbPARAMETER Param;
    if( !(Param = odbParam( hQry, usParam, TRUE )) ) return( FALSE );
    return( Param->bTruncated );
}

_odbdecl
odbUSHORT odbParamType( odbHANDLE hQry, odbUSHORT usParam )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( ODB_PARAM_NONE );
    }
    if( usParam == 0 || usParam > h->usTotalParams ) {
        odbSetError( hQry, ODBTPERR_PARAMNUMBER );
        return( ODB_PARAM_NONE );
    }
    odbSetError( hQry, ODBTPERR_NONE );
    return( (h->Params + usParam - 1)->usType );
}

_odbdecl
odbPVOID odbParamUserData( odbHANDLE hQry, odbUSHORT usParam )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY ) {
        odbSetError( hQry, ODBTPERR_HANDLE );
        return( NULL );
    }
    if( usParam == 0 || usParam > h->usTotalParams ) {
        odbSetError( hQry, ODBTPERR_PARAMNUMBER );
        return( NULL );
    }
    odbSetError( hQry, ODBTPERR_NONE );
    return( (h->Params + usParam - 1)->pUserData );
}

_odbdecl
odbBOOL odbPrepare( odbHANDLE hQry, odbPCSTR pszSQL )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    h->bNoData = TRUE;
    h->bMoreResults = FALSE;
    h->usTotalCols = 0;
    h->lRowCount = 0;
    h->usTotalParams = 0;
    h->lTotalRows = 0;
    h->lRowCursor = 0;

    if( !odbSendRequestByte( hQry, ODBTP_PREPARE, (odbBYTE)h->ulId, FALSE ) )
        return( FALSE );

    if( !pszSQL || *pszSQL == 0 ) {
        if( !odbSendRequest( hQry, ODBTP_PREPARE, NULL, 0, TRUE ) )
            return( FALSE );
    }
    else {
        if( !odbSendRequestText( hQry, ODBTP_PREPARE, pszSQL, TRUE ) )
            return( FALSE );
    }
    if( !odbReadResponse( hQry ) ) return( FALSE );

    if( h->ulResponseCode == ODBTP_PARAMINFO )
        return( odbProcessParamInfo( hQry ) );

    return( TRUE );
}

_odbdecl
odbBOOL odbPrepareProc( odbHANDLE hQry, odbPCSTR pszProcedure )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    h->bNoData = TRUE;
    h->bMoreResults = FALSE;
    h->usTotalCols = 0;
    h->lRowCount = 0;
    h->usTotalParams = 0;
    h->lTotalRows = 0;
    h->lRowCursor = 0;

    if( !odbSendRequestByte( hQry, ODBTP_PREPAREPROC, (odbBYTE)h->ulId, FALSE ) )
        return( FALSE );

    if( !pszProcedure || *pszProcedure == 0 ) {
        if( !odbSendRequest( hQry, ODBTP_PREPAREPROC, NULL, 0, TRUE ) )
            return( FALSE );
    }
    else {
        if( !odbSendRequestText( hQry, ODBTP_PREPAREPROC, pszProcedure, TRUE ) )
            return( FALSE );
    }
    if( !odbReadResponse( hQry ) ) return( FALSE );

    if( h->ulResponseCode == ODBTP_PARAMINFOEX )
        return( odbProcessParamInfo( hQry ) );

    return( TRUE );
}

odbBOOL odbProcessColInfo( odbHANDLE hQry )
{
    odbCOLUMN     Col;
    odbHQUERY     h = (odbHQUERY)hQry;
    odbHANDLE     hCon = odbGetConnection( hQry );
    odbUSHORT     usCol;

    h->bGotRow = FALSE;
    h->ulRowExtractSize = 0;

    if( !odbExtractShort( hQry, &h->usTotalCols ) )
        return( FALSE );

    if( h->usTotalCols > h->usMaxCols ) {
        odbULONG ulBytes;

        free( h->Cols );
        h->usMaxCols = h->usTotalCols;
        ulBytes = h->usMaxCols * sizeof(odbCOLUMN_s);
        if( !(h->Cols = (odbCOLUMN)malloc( ulBytes )) )
            return( odbSetError( hQry, ODBTPERR_MEMORY ) );
    }
    for( usCol = 0; usCol < h->usTotalCols; usCol++ ) {
        Col = (h->Cols + usCol);

        if( !odbExtractShort( hQry, (odbPUSHORT)&Col->sNameLen ) )
            return( FALSE );
        if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszName, Col->sNameLen ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Col->sSqlType ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Col->sDefaultDataType ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Col->sDataType ) )
            return( FALSE );
        if( !odbExtractLong( hQry, &Col->ulColSize ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Col->sDecDigits ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Col->sNullable ) )
            return( FALSE );
        *(Col->pszName + Col->sNameLen) = 0;

        if( h->ulResponseCode == ODBTP_COLINFOEX ) {
            odbUSHORT usLen;

            if( !odbExtractShort( hQry, &usLen ) )  return( FALSE );
            if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszSqlType, usLen ) )
                return( FALSE );

            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
            if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszTable, usLen ) )
                return( FALSE );

            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
            if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszSchema, usLen ) )
                return( FALSE );

            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
            if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszCatalog, usLen ) )
                return( FALSE );

            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
            if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszBaseName, usLen ) )
                return( FALSE );

            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
            if( !odbExtractPointer( hQry, (odbPVOID*)&Col->pszBaseTable, usLen ) )
                return( FALSE );

            if( !odbExtractLong( hQry, &Col->ulFlags ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbULONG)) = 0;
        }
        else {
            Col->pszSqlType = "";
            Col->pszTable = "";
            Col->pszSchema = "";
            Col->pszCatalog = "";
            Col->pszBaseName = "";
            Col->pszBaseTable = "";
            Col->ulFlags = Col->sNullable == 0 ? ODB_COLINFO_NOTNULL : 0;
        }
        Col->lDataLen = ODB_NULL;
        Col->pbySavedRowDataByte = NULL;
        Col->bTruncated = FALSE;
        Col->lActualLen = ODB_NULL;
        Col->pUserData = NULL;

        if( *Col->pszSqlType == 0 ) {
            strcpy( Col->szSqlType,
                    odbGetDataTypeName( hCon, Col->sSqlType,
                                        Col->ulColSize, Col->sDecDigits ) );
            Col->pszSqlType = Col->szSqlType;
        }
    }
    return( TRUE );
}

odbBOOL odbProcessCursor( odbHANDLE hQry )
{
    odbBYTE   byBookmarksOn;
    odbHQUERY h = (odbHQUERY)hQry;

    if( !odbExtractShort( hQry, &h->usCursorType ) ||
        !odbExtractShort( hQry, &h->usCursorConcur ) ||
        !odbExtractByte( hQry, &byBookmarksOn ) )
    {
        return( FALSE );
    }
    h->bBookmarksOn = byBookmarksOn ? TRUE : FALSE;

    return( TRUE );
}

odbBOOL odbProcessParamInfo( odbHANDLE hQry )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;
    odbUSHORT    usLen;
    odbUSHORT    usParam;

    if( !odbExtractShort( hQry, &h->usTotalParams ) ) return( FALSE );

    h->bPreparedProc = h->ulResponseCode == ODBTP_PARAMINFOEX ? TRUE : FALSE;

    if( h->usTotalParams > h->usMaxParams ) {
        odbULONG ulBytes;

        free( h->Params );
        h->usMaxParams = h->usTotalParams;
        ulBytes = h->usMaxParams * sizeof(odbPARAMETER_s);
        if( !(h->Params = (odbPARAMETER)malloc( ulBytes )) )
            return( odbSetError( hQry, ODBTPERR_MEMORY ) );
    }
    for( usParam = 0; usParam < h->usTotalParams; usParam++ ) {
        Param = (h->Params + usParam);

        if( !odbExtractShort( hQry, &Param->usType ) )
            return( FALSE );
        if( h->bPreparedProc ) *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
        if( !odbExtractShort( hQry, (odbPUSHORT)&Param->sSqlType ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Param->sDefaultDataType ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Param->sDataType ) )
            return( FALSE );
        if( !odbExtractLong( hQry, &Param->ulColSize ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Param->sDecDigits ) )
            return( FALSE );
        if( !odbExtractShort( hQry, (odbPUSHORT)&Param->sNullable ) )
            return( FALSE );

        if( h->bPreparedProc ) {
            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            if( !odbExtractPointer( hQry, (odbPVOID*)&Param->pszName, usLen ) )
                return( FALSE );

            if( !odbExtractShort( hQry, &usLen ) ) return( FALSE );
            *(h->pbyExtractData - sizeof(odbUSHORT)) = 0;
            if( !odbExtractPointer( hQry, (odbPVOID*)&Param->pszSqlType, usLen ) )
                return( FALSE );
        }
        else {
            Param->pszName = "";
            Param->pszSqlType = "";
        }
        if( (Param->usType & ODB_PARAM_INOUT) && Param->sDataType != 0 )
            Param->bBound = TRUE;
        else
            Param->bBound = FALSE;

        Param->bGot = FALSE;
        Param->bTruncated = FALSE;
        Param->lActualLen = ODB_NULL;
        Param->pUserData = NULL;
    }
    return( TRUE );
}

odbBOOL odbProcessResult( odbHANDLE hQry )
{
    odbHQUERY      h = (odbHQUERY)hQry;
    odbHCONNECTION hCon = (odbHCONNECTION)h->hCon;

    h->bNoData = TRUE;
    h->bMoreResults = FALSE;
    h->usTotalCols = 0;
    h->lRowCount = 0;
    h->lTotalRows = 0;
    h->lRowCursor = 0;
    h->pbySavedRowDataEndByte = NULL;

    if( !odbReadResponse( hQry ) ) return( FALSE );

    if( h->ulResponseCode == ODBTP_NODATA ) return( TRUE );

    h->bNoData = FALSE;
    h->bMoreResults = TRUE;
    if( h->ulResponseCode == ODBTP_ROWCOUNT )
        return( odbExtractLong( hQry, (odbPULONG)&h->lRowCount ) );

    if( h->ulResponseCode != ODBTP_COLINFO &&
        h->ulResponseCode != ODBTP_COLINFOEX )
    {
        return( TRUE );
    }
    if( !odbProcessColInfo( hQry ) ) return( FALSE );

    if( hCon->bConvertAll || hCon->bConvertDatetime || hCon->bConvertGuid ) {
        odbBOOL   bBoundCol = FALSE;
        odbCOLUMN Col;
        odbUSHORT usCol;

        for( usCol = 1; usCol <= h->usTotalCols; usCol++ ) {
            Col = (h->Cols + usCol - 1);

            if( Col->sDataType == ODB_CHAR || Col->sDataType == ODB_WCHAR )
                continue;

            if( !hCon->bConvertAll ) {
                if( Col->sDataType == ODB_DATETIME ) {
                    if( !hCon->bConvertDatetime ) continue;
                }
                else if( Col->sDataType == ODB_GUID ) {
                    if( !hCon->bConvertGuid ) continue;
                }
                else {
                    continue;
                }
            }
            if( !odbBindCol( hQry, usCol, ODB_CHAR, 0, FALSE ) )
                return( FALSE );
            bBoundCol = TRUE;
        }
        if( bBoundCol ) {
            if( !odbSendRequest( hQry, ODBTP_BINDCOL, NULL, 0, TRUE ) ||
                !odbReadResponse( hQry ) )
            {
                return( FALSE );
            }
        }
    }
    if( hCon->bUseRowCache && h->usCursorType == ODB_CURSOR_FORWARD )
        return( odbFetchRowsIntoCache( hQry ) );

    return( TRUE );
}

odbBOOL odbProcessRowData( odbHANDLE hQry )
{
    odbCOLUMN Col;
    odbHQUERY h = (odbHQUERY)hQry;
    odbUSHORT usCol;

    for( usCol = 0; usCol < h->usTotalCols; usCol++ ) {
        Col = (h->Cols + usCol);

        if( !odbExtractLong( hQry, (odbPULONG)&Col->lDataLen ) )
            return( FALSE );

        Col->pbySavedRowDataByte = h->pbyExtractData - sizeof(odbULONG);
        Col->ulSavedRowDataByte = *Col->pbySavedRowDataByte;
        *Col->pbySavedRowDataByte = 0;

        if( Col->lDataLen == ODB_TRUNCATION ) {
            Col->bTruncated = TRUE;
            if( !odbExtractLong( hQry, (odbPULONG)&Col->lActualLen ) )
                return( FALSE );
            if( !odbExtractLong( hQry, (odbPULONG)&Col->lDataLen ) )
                return( FALSE );
        }
        else {
            Col->bTruncated = FALSE;
            Col->lActualLen = Col->lDataLen;
        }
        if( Col->lDataLen < 0 ) continue;

        switch( Col->sDataType ) {
            case ODB_BIT:
            case ODB_TINYINT:
            case ODB_UTINYINT:
                if( !odbExtractByte( hQry, &Col->Data.byVal ) )
                    return( FALSE );
                break;

            case ODB_SMALLINT:
            case ODB_USMALLINT:
                if( !odbExtractShort( hQry, &Col->Data.usVal ) )
                    return( FALSE );
                break;

            case ODB_INT:
            case ODB_UINT:
                if( !odbExtractLong( hQry, &Col->Data.ulVal ) )
                    return( FALSE );
                break;

            case ODB_BIGINT:
            case ODB_UBIGINT:
                if( !odbExtractLongLong( hQry, &Col->Data.ullVal ) )
                    return( FALSE );
                break;

            case ODB_REAL:
                if( !odbExtractFloat( hQry, &Col->Data.fVal ) )
                    return( FALSE );
                break;

            case ODB_DOUBLE:
                if( !odbExtractDouble( hQry, &Col->Data.dVal ) )
                    return( FALSE );
                break;

            case ODB_DATETIME:
                if( !odbExtractTimestamp( hQry, &Col->Data.tsVal ) )
                    return( FALSE );
                break;

            case ODB_GUID:
                if( !odbExtractGuid( hQry, &Col->Data.guidVal ) )
                    return( FALSE );
                break;

            default:
                if( !odbExtractPointer( hQry, &Col->Data.pVal, Col->lDataLen ) )
                    return( FALSE );
        }
    }
    h->pbySavedRowDataEndByte = h->pbyExtractData;
    h->ulSavedRowDataEndByte = *h->pbySavedRowDataEndByte;
    *h->pbySavedRowDataEndByte = 0;
    h->bNoData = FALSE;
    h->bGotRow = TRUE;
    if( !h->lRowCursor && h->usCursorType == ODB_CURSOR_FORWARD )
        h->lTotalRows++;
    h->pbyRowExtractData = h->pbyExtractData;
    h->ulRowExtractSize = h->ulExtractSize;
    return( TRUE );
}

odbULONG odbReadDataLong( odbPBYTE pbyData )
{
    odbULONG ul;
    memcpy( &ul, pbyData, sizeof(odbULONG) );
    return( ntohl( ul ) );
}

odbPSTR odbReadInterfaceFile( odbHANDLE hOdb, odbPCSTR pszFile )
{
    FILE*       file;
    odbPSTR     pszInterfaces;
    struct stat st;

    if( !pszFile || *pszFile == 0 ) pszFile = DEFAULT_INTERFACE_FILE;

    if( stat( pszFile, &st ) || !(file = fopen( pszFile, "rb" )) ) {
        odbSetError( hOdb, ODBTPERR_INTERFFILE );
        return( NULL );
    }
    if( !(pszInterfaces = (char*)malloc( st.st_size + 1 )) ) {
        fclose( file );
        odbSetError( hOdb, ODBTPERR_MEMORY );
        return( NULL );
    }
    fread( pszInterfaces, st.st_size, 1, file );
    *(pszInterfaces + st.st_size) = 0;
    fclose( file );
    odbSetError( hOdb, ODBTPERR_NONE );

    return( pszInterfaces );
}

odbBOOL odbReadResponse( odbHANDLE hOdb )
{
    odbBYTE   byCode;
    odbBYTE   byFlag;
    odbHODBTP h = (odbHODBTP)hOdb;
    odbSOCKET Sock = h->Sock;
    odbPBYTE  pbyResponseData;
    odbULONG  ulSize;
    odbUSHORT usSize;

    if( !Sock ) return( odbSetError( hOdb, ODBTPERR_DETACHED ) );

    if( Sock->sock == INVALID_SOCKET )
        return( odbSetError( hOdb, ODBTPERR_DISCONNECTED ) );

    byFlag = 0;
    h->ulResponseCode = 0;
    h->ulExtractSize = 0;
    Sock->ulTransSize = 0;
    pbyResponseData = (odbPBYTE)h->pResponseBuf;

    while( byFlag == 0 ) {
        if( !odbSockReadData( Sock, &byCode, 1 ) )
            return( odbSetError( hOdb, Sock->bTimeout ? ODBTPERR_TIMEOUTREAD :
                                                        ODBTPERR_READ ) );
        if( !odbSockReadData( Sock, &byFlag, 1 ) )
            return( odbSetError( hOdb, Sock->bTimeout ? ODBTPERR_TIMEOUTREAD :
                                                        ODBTPERR_READ ) );
        if( !odbSockReadData( Sock, (odbPBYTE)&usSize, 2 ) )
            return( odbSetError( hOdb, Sock->bTimeout ? ODBTPERR_TIMEOUTREAD :
                                                        ODBTPERR_READ ) );

        ulSize = (odbULONG)ntohs( usSize );

        if( byCode == 0 || (byFlag & 0xFF) > 0x01 )
            return( odbSetError( hOdb, ODBTPERR_PROTOCOL ) );

        if( h->ulResponseCode != (odbULONG)byCode ) {
            h->ulResponseCode = byCode;
            odbInitResponseBuf( hOdb );
            h->ulResponseSize = 0;
            pbyResponseData = (odbPBYTE)h->pResponseBuf;
        }
        if( ulSize > 0 ) {
            h->ulResponseSize += ulSize;

            if( h->ulResponseSize >= h->ulResponseBufSize ) {
                h->ulResponseBufSize =
                  h->ulResponseSize + (h->ulResponseSize / 2);
                h->pResponseBuf =
                  (odbPVOID)realloc( h->pResponseBuf, h->ulResponseBufSize );
                if( !h->pResponseBuf )
                    return( odbSetError( hOdb, ODBTPERR_MEMORY ) );
                pbyResponseData = (odbPBYTE)h->pResponseBuf;
                pbyResponseData += h->ulResponseSize - ulSize;
            }
            if( !odbSockReadData( Sock, pbyResponseData, ulSize ) )
                return( odbSetError( hOdb, Sock->bTimeout ? ODBTPERR_TIMEOUTREAD :
                                                            ODBTPERR_READ ) );
            pbyResponseData += ulSize;
        }
    }
    odbSaveResponseBuf( hOdb );
    h->pbyExtractData = (odbPBYTE)h->pResponseBuf;
    h->ulExtractSize = h->ulResponseSize;
    *pbyResponseData = 0;

    if( byCode >= ODBTP_ERROR ) {
        if( byCode >= ODBTP_DISCONNECT ) odbSockClose( Sock );
        return( odbSetError( hOdb, ODBTPERR_SERVER ) );
    }
    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

void odbRestoreRowDataBytes( odbHANDLE hQry )
{
    odbCOLUMN Col;
    odbHQUERY h = (odbHQUERY)hQry;
    odbUSHORT usCol;

    if( h->pbySavedRowDataEndByte ) {
        *h->pbySavedRowDataEndByte = (odbBYTE)h->ulSavedRowDataEndByte;
        h->pbySavedRowDataEndByte = NULL;
    }
    for( usCol = 0; usCol < h->usTotalCols; usCol++ ) {
        Col = (h->Cols + usCol);

        if( Col->pbySavedRowDataByte ) {
            *Col->pbySavedRowDataByte = (odbBYTE)Col->ulSavedRowDataByte;
            Col->pbySavedRowDataByte = NULL;
        }
    }
}

_odbdecl
odbBOOL odbRollback( odbHANDLE hCon )
{
    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !odbSendRequest( hCon, ODBTP_ROLLBACK, NULL, 0, TRUE ) )
        return( FALSE );

    return( odbReadResponse( hCon ) );
}

void odbSaveResponseBuf( odbHANDLE hOdb )
{
    if( !odbIsConnection( hOdb ) ) {
         odbHQUERY h = (odbHQUERY)hOdb;

        switch( h->ulResponseCode ) {
            case ODBTP_COLINFO:
            case ODBTP_COLINFOEX:
                h->pColInfoBuf = h->pResponseBuf;
                h->ulColInfoBufSize = h->ulResponseBufSize;
                h->ulColInfoSize = h->ulResponseSize;
                break;

            case ODBTP_PARAMINFO:
            case ODBTP_PARAMINFOEX:
                h->pParamInfoBuf = h->pResponseBuf;
                h->ulParamInfoBufSize = h->ulResponseBufSize;
                h->ulParamInfoSize = h->ulResponseSize;
                break;

            case ODBTP_PARAMDATA:
                h->pParamDataBuf = h->pResponseBuf;
                h->ulParamDataBufSize = h->ulResponseBufSize;
                h->ulParamDataSize = h->ulResponseSize;
                break;

            case ODBTP_ROWDATA:
                h->pRowDataBuf = h->pResponseBuf;
                h->ulRowDataBufSize = h->ulResponseBufSize;
                h->ulRowDataSize = h->ulResponseSize;
                h->pbyRowExtractData = (odbPBYTE)h->pResponseBuf;
                h->ulRowExtractSize = h->ulResponseSize;
                break;

            default:
                h->pNormalBuf = h->pResponseBuf;
                h->ulNormalBufSize = h->ulResponseBufSize;
                h->ulNormalSize = h->ulResponseSize;
        }
    }
}

_odbdecl
odbBOOL odbSeekRow( odbHANDLE hQry, odbLONG lRow )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    if( !h->lRowCursor ) {
        if( h->usCursorType != ODB_CURSOR_FORWARD )
            return( odbSetError( hQry, ODBTPERR_NOSEEKCURSOR ) );

        lRow--;

        while( h->lTotalRows < lRow ) {
            if( !odbFetchRow( hQry ) ) return( FALSE );
            if( odbNoData( hQry ) ) break;
        }
        if( lRow != h->lTotalRows )
            return( odbSetError( hQry, ODBTPERR_SEEKROWPOS ) );
    }
    else {
        if( lRow < 1 || lRow > h->lTotalRows )
            return( odbSetError( hQry, ODBTPERR_SEEKROWPOS ) );

        h->lRowCursor = lRow;
    }
    return( odbSetError( hQry, ODBTPERR_NONE ) );
}

odbBOOL odbSendRequest( odbHANDLE hOdb, odbUSHORT usCode,
                        odbPVOID pData, odbULONG ulLen,
                        odbBOOL bFinal )
{
    odbHODBTP h = (odbHODBTP)hOdb;
    odbSOCKET Sock = h->Sock;
    odbPBYTE  pby = (odbPBYTE)pData;
    odbPBYTE  pbyTrans;
    odbULONG  ul;
    odbUSHORT usSize;

    if( !Sock ) return( odbSetError( hOdb, ODBTPERR_DETACHED ) );

    if( Sock->sock == INVALID_SOCKET )
        return( odbSetError( hOdb, ODBTPERR_DISCONNECTED ) );

    pbyTrans = (odbPBYTE)Sock->pTransBuf;

    if( h->ulRequestCode != (odbULONG)usCode || h->ulError != ODBTPERR_NONE ) {
        h->ulRequestCode = usCode;
        *pbyTrans = (odbBYTE)usCode;
        *(pbyTrans + 1) = 0;
        Sock->ulTransSize = 4;
    }
    for( ul = 0; ul < ulLen; ul++, pby++, Sock->ulTransSize++ ) {
        if( Sock->ulTransSize == Sock->ulTransBufSize ) {
            usSize = htons( (odbUSHORT)(Sock->ulTransSize - 4) );
            memcpy( pbyTrans + 2, &usSize, 2 );
            if( odbSockSend( Sock, Sock->pTransBuf, Sock->ulTransSize ) !=
                (odbLONG)Sock->ulTransSize )
            {
                odbSockClose( Sock );
                return( odbSetError( hOdb, Sock->bTimeout ? ODBTPERR_TIMEOUTSEND :
                                                            ODBTPERR_SEND ) );
            }
            Sock->ulTransSize = 4;
        }
        *(pbyTrans + Sock->ulTransSize) = *pby;
    }
    if( bFinal ) {
        *(pbyTrans + 1) = 0x01;
        usSize = htons( (odbUSHORT)(Sock->ulTransSize - 4) );
        memcpy( pbyTrans + 2, &usSize, 2 );
        if( odbSockSend( Sock, Sock->pTransBuf, Sock->ulTransSize ) !=
            (odbLONG)Sock->ulTransSize )
        {
            odbSockClose( Sock );
            return( odbSetError( hOdb, Sock->bTimeout ? ODBTPERR_TIMEOUTSEND :
                                                        ODBTPERR_SEND ) );
        }
        h->ulRequestCode = 0;
        h->ulResponseCode = 0;
        h->ulResponseSize = 0;
    }
    return( odbSetError( hOdb, ODBTPERR_NONE ) );
}

odbBOOL odbSendRequestByte( odbHANDLE hOdb, odbUSHORT usCode,
                            odbBYTE byData, odbBOOL bFinal )
{
    return( odbSendRequest( hOdb, usCode, &byData, 1, bFinal ) );
}

odbBOOL odbSendRequestDouble( odbHANDLE hOdb, odbUSHORT usCode,
                              odbDOUBLE dData, odbBOOL bFinal )
{
    odbDOUBLE d;
    odbHostToNetworkOrder( (odbPBYTE)&d, (odbPBYTE)&dData, sizeof(odbDOUBLE) );
    return( odbSendRequest( hOdb, usCode, &d, sizeof(odbDOUBLE), bFinal ) );
}

odbBOOL odbSendRequestFloat( odbHANDLE hOdb, odbUSHORT usCode,
                             odbFLOAT fData, odbBOOL bFinal )
{
    odbFLOAT f;
    odbHostToNetworkOrder( (odbPBYTE)&f, (odbPBYTE)&fData, sizeof(odbFLOAT) );
    return( odbSendRequest( hOdb, usCode, &f, sizeof(odbFLOAT), bFinal ) );
}

odbBOOL odbSendRequestGuid( odbHANDLE hOdb, odbUSHORT usCode,
                            odbPGUID pguidData, odbBOOL bFinal )
{
    if( !odbSendRequestLong( hOdb, usCode, (odbULONG)pguidData->ulData1, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)pguidData->usData2, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)pguidData->usData3, FALSE ) )
        return( FALSE );
    return( odbSendRequest( hOdb, usCode, (odbPVOID)pguidData->byData4, 8, bFinal ) );
}

odbBOOL odbSendRequestLong( odbHANDLE hOdb, odbUSHORT usCode,
                            odbULONG ulData, odbBOOL bFinal )
{
    ulData = htonl( ulData );
    return( odbSendRequest( hOdb, usCode, &ulData, sizeof(odbULONG), bFinal ) );
}

odbBOOL odbSendRequestLongLong( odbHANDLE hOdb, odbUSHORT usCode,
                                odbULONGLONG ullData, odbBOOL bFinal )
{
    odbULONGLONG ull;
    odbHostToNetworkOrder( (odbPBYTE)&ull, (odbPBYTE)&ullData, sizeof(odbULONGLONG) );
    return( odbSendRequest( hOdb, usCode, &ull, sizeof(odbULONGLONG), bFinal ) );
}

odbBOOL odbSendRequestShort( odbHANDLE hOdb, odbUSHORT usCode,
                             odbUSHORT usData, odbBOOL bFinal )
{
    usData = htons( usData );
    return( odbSendRequest( hOdb, usCode, &usData, sizeof(odbUSHORT), bFinal ) );
}

odbBOOL odbSendRequestText( odbHANDLE hOdb, odbUSHORT usCode,
                            odbPCSTR pszData, odbBOOL bFinal )
{
    if( !pszData )
        return( odbSendRequest( hOdb, usCode, NULL, 0, bFinal ) );
    return( odbSendRequest( hOdb, usCode, (odbPVOID)pszData,
                            strlen(pszData), bFinal ) );
}

odbBOOL odbSendRequestTimestamp( odbHANDLE hOdb, odbUSHORT usCode,
                                 odbPTIMESTAMP ptsData, odbBOOL bFinal )
{
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)ptsData->sYear, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)ptsData->usMonth, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)ptsData->usDay, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)ptsData->usHour, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)ptsData->usMinute, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hOdb, usCode, (odbUSHORT)ptsData->usSecond, FALSE ) )
        return( FALSE );
    return( odbSendRequestLong( hOdb, usCode, (odbULONG)ptsData->ulFraction, bFinal ) );
}

_odbdecl
odbBOOL odbSetAttrLong( odbHANDLE hCon, odbLONG lAttr, odbULONG ulVal )
{
    odbUSHORT usType = 0;

    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !odbSendRequestLong( hCon, ODBTP_SETATTR, lAttr, FALSE ) ||
        !odbSendRequestShort( hCon, ODBTP_SETATTR, usType, FALSE ) ||
        !odbSendRequestLong( hCon, ODBTP_SETATTR, ulVal, TRUE ) )
    {
        return( FALSE );
    }
    return( odbReadResponse( hCon ) );
}

_odbdecl
odbBOOL odbSetAttrText( odbHANDLE hCon, odbLONG lAttr, odbPCSTR pszVal )
{
    odbULONG  ulLen = strlen(pszVal);
    odbUSHORT usType = 1;

    if( ((odbHODBTP)hCon)->ulType != ODBTPHANDLE_CONNECTION )
        return( odbSetError( hCon, ODBTPERR_HANDLE ) );

    if( !odbSendRequestLong( hCon, ODBTP_SETATTR, lAttr, FALSE ) ||
        !odbSendRequestShort( hCon, ODBTP_SETATTR, usType, FALSE ) ||
        !odbSendRequestLong( hCon, ODBTP_SETATTR, ulLen, FALSE ) ||
        !odbSendRequest( hCon, ODBTP_SETATTR, (odbPVOID)pszVal, ulLen, TRUE ) )
    {
        return( FALSE );
    }
    return( odbReadResponse( hCon ) );
}

_odbdecl
odbBOOL odbSetCol( odbHANDLE hQry, odbUSHORT usCol,
                   odbPVOID pData, odbLONG lDataLen,
                   odbBOOL bFinal )
{
    odbHQUERY h = (odbHQUERY)hQry;
    odbCOLUMN Col;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usCol == 0 || usCol > h->usTotalCols )
        return( odbSetError( hQry, ODBTPERR_COLNUMBER ) );

    Col = (h->Cols + usCol - 1);

    if( h->ulRequestCode != ODBTP_SETCOL &&
        !odbSendRequestByte( hQry, ODBTP_SETCOL, (odbBYTE)h->ulId, FALSE ) )
    {
        return( FALSE );
    }
    if( !odbSendRequestShort( hQry, ODBTP_SETCOL, usCol, FALSE ) )
        return( FALSE );
    if( !odbSendRequestLong( hQry, ODBTP_SETCOL, (odbULONG)lDataLen, FALSE ) )
        return( FALSE );

    if( lDataLen <= 0 ) {
        if( !bFinal ) return( TRUE );
        pData = NULL;
        lDataLen = 0;
    }
    if( !odbSendRequest( hQry, ODBTP_SETCOL, pData, lDataLen, bFinal ) )
        return( FALSE );

    return( bFinal ? odbReadResponse( hQry ) : TRUE );
}

_odbdecl
odbBOOL odbSetColByte( odbHANDLE hQry, odbUSHORT usCol,
                       odbBYTE byData, odbBOOL bFinal )
{
    return( odbSetCol( hQry, usCol, &byData, sizeof(odbBYTE), bFinal ) );
}

_odbdecl
odbBOOL odbSetColDouble( odbHANDLE hQry, odbUSHORT usCol,
                         odbDOUBLE dData, odbBOOL bFinal )
{
    odbDOUBLE d;
    odbHostToNetworkOrder( (odbPBYTE)&d, (odbPBYTE)&dData, sizeof(odbDOUBLE) );
    return( odbSetCol( hQry, usCol, &d, sizeof(odbDOUBLE), bFinal ) );
}

_odbdecl
odbBOOL odbSetColFloat( odbHANDLE hQry, odbUSHORT usCol,
                        odbFLOAT fData, odbBOOL bFinal )
{
    odbFLOAT f;
    odbHostToNetworkOrder( (odbPBYTE)&f, (odbPBYTE)&fData, sizeof(odbFLOAT) );
    return( odbSetCol( hQry, usCol, &f, sizeof(odbFLOAT), bFinal ) );
}

_odbdecl
odbBOOL odbSetColGuid( odbHANDLE hQry, odbUSHORT usCol,
                       odbPGUID pguidData, odbBOOL bFinal )
{
    odbGUID guid;

    guid.ulData1 = htonl( pguidData->ulData1 );
    guid.usData2 = htons( pguidData->usData2 );
    guid.usData3 = htons( pguidData->usData3 );
    memcpy( guid.byData4, pguidData->byData4, 8 );
    return( odbSetCol( hQry, usCol, &guid, sizeof(odbGUID), bFinal ) );
}

_odbdecl
odbBOOL odbSetColIgnore( odbHANDLE hQry, odbUSHORT usCol,
                         odbBOOL bFinal )
{
    return( odbSetCol( hQry, usCol, NULL, ODB_IGNORE, bFinal ) );
}

_odbdecl
odbBOOL odbSetColLong( odbHANDLE hQry, odbUSHORT usCol,
                       odbULONG ulData, odbBOOL bFinal )
{
    ulData = htonl( ulData );
    return( odbSetCol( hQry, usCol, &ulData, sizeof(odbULONG), bFinal ) );
}

_odbdecl
odbBOOL odbSetColLongLong( odbHANDLE hQry, odbUSHORT usCol,
                           odbULONGLONG ullData, odbBOOL bFinal )
{
    odbULONGLONG ull;
    odbHostToNetworkOrder( (odbPBYTE)&ull, (odbPBYTE)&ullData, sizeof(odbULONGLONG) );
    return( odbSetCol( hQry, usCol, &ull, sizeof(odbULONGLONG), bFinal ) );
}

_odbdecl
odbBOOL odbSetColNull( odbHANDLE hQry, odbUSHORT usCol,
                       odbBOOL bFinal )
{
    return( odbSetCol( hQry, usCol, NULL, ODB_NULL, bFinal ) );
}

_odbdecl
odbBOOL odbSetColShort( odbHANDLE hQry, odbUSHORT usCol,
                        odbUSHORT usData, odbBOOL bFinal )
{
    usData = htons( usData );
    return( odbSetCol( hQry, usCol, &usData, sizeof(odbUSHORT), bFinal ) );
}

_odbdecl
odbBOOL odbSetColText( odbHANDLE hQry, odbUSHORT usCol,
                       odbPCSTR pszData, odbBOOL bFinal )
{
    return( odbSetCol( hQry, usCol, (odbPVOID)pszData, strlen(pszData), bFinal ) );
}

_odbdecl
odbBOOL odbSetColTimestamp( odbHANDLE hQry, odbUSHORT usCol,
                            odbPTIMESTAMP ptsData, odbBOOL bFinal )
{
    odbTIMESTAMP ts;

    ts.sYear = htons( ptsData->sYear );
    ts.usMonth = htons( ptsData->usMonth );
    ts.usDay = htons( ptsData->usDay );
    ts.usHour = htons( ptsData->usHour );
    ts.usMinute = htons( ptsData->usMinute );
    ts.usSecond = htons( ptsData->usSecond );
    ts.ulFraction = htonl( ptsData->ulFraction );
    return( odbSetCol( hQry, usCol, &ts, sizeof(odbTIMESTAMP), bFinal ) );
}

_odbdecl
odbBOOL odbSetColUserData( odbHANDLE hQry, odbUSHORT usCol,
                             odbPVOID pVal )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usCol == 0 || usCol > h->usTotalCols )
        return( odbSetError( hQry, ODBTPERR_COLNUMBER ) );
    (h->Cols + usCol - 1)->pUserData = pVal;
    return( odbSetError( hQry, ODBTPERR_NONE ) );
}

_odbdecl
odbULONG odbSetConnectTimeout( odbHANDLE hOdb, odbULONG ulTimeout )
{
    odbULONG ulOldTimeout = ((odbHODBTP)hOdb)->Sock->ulConnectTimeout;
    ((odbHODBTP)hOdb)->Sock->ulConnectTimeout = ulTimeout;
    return( ulOldTimeout );
}

_odbdecl
odbBOOL odbSetCursor( odbHANDLE hQry, odbUSHORT usType,
                      odbUSHORT usConcurrency, odbBOOL bEnableBookmarks )
{
    odbHQUERY h = (odbHQUERY)hQry;
    odbBYTE   byEnableBMs = bEnableBookmarks ? 1 : 0;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );

    h->bNoData = TRUE;
    h->bMoreResults = FALSE;
    h->usTotalCols = 0;
    h->lRowCount = 0;
    h->lTotalRows = 0;
    h->lRowCursor = 0;
    h->pbySavedRowDataEndByte = NULL;

    if( !odbSendRequestByte( hQry, ODBTP_SETCURSOR, (odbBYTE)h->ulId, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_SETCURSOR, usType, FALSE ) )
        return( FALSE );
    if( !odbSendRequestShort( hQry, ODBTP_SETCURSOR, usConcurrency, FALSE ) )
        return( FALSE );
    if( !odbSendRequestByte( hQry, ODBTP_SETCURSOR, byEnableBMs, TRUE ) )
        return( FALSE );

    if( !odbReadResponse( hQry ) ) return( FALSE );

    return( odbProcessCursor( hQry ) );
}

odbBOOL odbSetDBConnectStr( odbPSTR pszDBConnect, odbULONG ulLen,
                            odbPINTERFACE pInterface )
{
    odbPCSTR psz;
    odbPCSTR pszFrom = pInterface->pszDBConnect;
    odbPSTR  pszTo = pszDBConnect;
    odbULONG n;

    if( (ulLen--) == 0 ) return( FALSE );

    for( n = 0; *pszFrom; pszFrom++ ) {
        if( n == ulLen ) return( FALSE );

        if( *pszFrom == '%' ) {
            switch( *(pszFrom + 1) ) {
                case 'r': psz = pInterface->pszDriver; break;
                case 's': psz = pInterface->pszServer; break;
                case 'd': psz = pInterface->pszDatabase; break;
                case 'u': psz = pInterface->pszUsername; break;
                case 'p': psz = pInterface->pszPassword; break;
                case '%': psz = "%"; break;
                default:  psz = NULL;
            }
            if( psz ) {
                for( ; *psz; psz++ ) {
                    if( n == ulLen ) return( FALSE );
                    *(pszTo + (n++)) = *psz;
                }
                pszFrom++;
                continue;
            }
        }
        *(pszTo + (n++)) = *pszFrom;
    }
    *(pszTo + n) = 0;
    return( TRUE );
}

_odbdecl
odbBOOL odbSetError( odbHANDLE hOdb, odbULONG ulError )
{
    return( (((odbHODBTP)hOdb)->ulError = ulError) == 0 ? TRUE : FALSE );
}

_odbdecl
odbBOOL odbSetParam( odbHANDLE hQry, odbUSHORT usParam,
                     odbPVOID pData, odbLONG lDataLen,
                     odbBOOL bFinal )
{
    odbHQUERY    h = (odbHQUERY)hQry;
    odbPARAMETER Param;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usParam == 0 || usParam > h->usTotalParams )
        return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );

    Param = (h->Params + usParam - 1);

    if( !Param->bBound )
        return( odbSetError( hQry, ODBTPERR_PARAMBIND ) );

    if( h->ulRequestCode != ODBTP_SETPARAM &&
        !odbSendRequestByte( hQry, ODBTP_SETPARAM, (odbBYTE)h->ulId, FALSE ) )
    {
        return( FALSE );
    }
    if( !odbSendRequestShort( hQry, ODBTP_SETPARAM, usParam, FALSE ) )
        return( FALSE );
    if( !odbSendRequestLong( hQry, ODBTP_SETPARAM, (odbULONG)lDataLen, FALSE ) )
        return( FALSE );

    if( lDataLen <= 0 ) {
        if( !bFinal ) return( TRUE );
        pData = NULL;
        lDataLen = 0;
    }
    if( !odbSendRequest( hQry, ODBTP_SETPARAM, pData, lDataLen, bFinal ) )
        return( FALSE );

    return( bFinal ? odbReadResponse( hQry ) : TRUE );
}

_odbdecl
odbBOOL odbSetParamByte( odbHANDLE hQry, odbUSHORT usParam,
                         odbBYTE byData, odbBOOL bFinal )
{
    return( odbSetParam( hQry, usParam, &byData, sizeof(odbBYTE), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamDefault( odbHANDLE hQry, odbUSHORT usParam,
                            odbBOOL bFinal )
{
    return( odbSetParam( hQry, usParam, NULL, ODB_DEFAULT, bFinal ) );
}

_odbdecl
odbBOOL odbSetParamDouble( odbHANDLE hQry, odbUSHORT usParam,
                           odbDOUBLE dData, odbBOOL bFinal )
{
    odbDOUBLE d;
    odbHostToNetworkOrder( (odbPBYTE)&d, (odbPBYTE)&dData, sizeof(odbDOUBLE) );
    return( odbSetParam( hQry, usParam, &d, sizeof(odbDOUBLE), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamFloat( odbHANDLE hQry, odbUSHORT usParam,
                          odbFLOAT fData, odbBOOL bFinal )
{
    odbFLOAT f;
    odbHostToNetworkOrder( (odbPBYTE)&f, (odbPBYTE)&fData, sizeof(odbFLOAT) );
    return( odbSetParam( hQry, usParam, &f, sizeof(odbFLOAT), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamGuid( odbHANDLE hQry, odbUSHORT usParam,
                         odbPGUID pguidData, odbBOOL bFinal )
{
    odbGUID guid;

    guid.ulData1 = htonl( pguidData->ulData1 );
    guid.usData2 = htons( pguidData->usData2 );
    guid.usData3 = htons( pguidData->usData3 );
    memcpy( guid.byData4, pguidData->byData4, 8 );
    return( odbSetParam( hQry, usParam, &guid, sizeof(odbGUID), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamLong( odbHANDLE hQry, odbUSHORT usParam,
                         odbULONG ulData, odbBOOL bFinal )
{
    ulData = htonl( ulData );
    return( odbSetParam( hQry, usParam, &ulData, sizeof(odbULONG), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamLongLong( odbHANDLE hQry, odbUSHORT usParam,
                             odbULONGLONG ullData, odbBOOL bFinal )
{
    odbULONGLONG ull;
    odbHostToNetworkOrder( (odbPBYTE)&ull, (odbPBYTE)&ullData, sizeof(odbULONGLONG) );
    return( odbSetParam( hQry, usParam, &ull, sizeof(odbULONGLONG), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamNull( odbHANDLE hQry, odbUSHORT usParam,
                         odbBOOL bFinal )
{
    return( odbSetParam( hQry, usParam, NULL, ODB_NULL, bFinal ) );
}

_odbdecl
odbBOOL odbSetParamShort( odbHANDLE hQry, odbUSHORT usParam,
                          odbUSHORT usData, odbBOOL bFinal )
{
    usData = htons( usData );
    return( odbSetParam( hQry, usParam, &usData, sizeof(odbUSHORT), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamText( odbHANDLE hQry, odbUSHORT usParam,
                         odbPCSTR pszData, odbBOOL bFinal )
{
    return( odbSetParam( hQry, usParam, (odbPVOID)pszData, strlen(pszData), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamTimestamp( odbHANDLE hQry, odbUSHORT usParam,
                              odbPTIMESTAMP ptsData, odbBOOL bFinal )
{
    odbTIMESTAMP ts;

    ts.sYear = htons( ptsData->sYear );
    ts.usMonth = htons( ptsData->usMonth );
    ts.usDay = htons( ptsData->usDay );
    ts.usHour = htons( ptsData->usHour );
    ts.usMinute = htons( ptsData->usMinute );
    ts.usSecond = htons( ptsData->usSecond );
    ts.ulFraction = htonl( ptsData->ulFraction );
    return( odbSetParam( hQry, usParam, &ts, sizeof(odbTIMESTAMP), bFinal ) );
}

_odbdecl
odbBOOL odbSetParamUserData( odbHANDLE hQry, odbUSHORT usParam,
                             odbPVOID pVal )
{
    odbHQUERY h = (odbHQUERY)hQry;

    if( h->ulType != ODBTPHANDLE_QUERY )
        return( odbSetError( hQry, ODBTPERR_HANDLE ) );
    if( usParam == 0 || usParam > h->usTotalParams )
        return( odbSetError( hQry, ODBTPERR_PARAMNUMBER ) );
    (h->Params + usParam - 1)->pUserData = pVal;
    return( odbSetError( hQry, ODBTPERR_NONE ) );
}

_odbdecl
odbULONG odbSetReadTimeout( odbHANDLE hOdb, odbULONG ulTimeout )
{
    odbULONG ulOldTimeout = ((odbHODBTP)hOdb)->Sock->ulReadTimeout;
    ((odbHODBTP)hOdb)->Sock->ulReadTimeout = ulTimeout;
    return( ulOldTimeout );
}

_odbdecl
odbULONG odbSetSendTimeout( odbHANDLE hOdb, odbULONG ulTimeout )
{
    odbULONG ulOldTimeout = ((odbHODBTP)hOdb)->Sock->ulSendTimeout;
    ((odbHODBTP)hOdb)->Sock->ulSendTimeout = ulTimeout;
    return( ulOldTimeout );
}

_odbdecl
void odbSetUserData( odbHANDLE hOdb, odbPVOID pVal )
{
    ((odbHODBTP)hOdb)->UserData.pVal = pVal;
}

_odbdecl
void odbSetUserDataLong( odbHANDLE hOdb, odbULONG ulVal )
{
    ((odbHODBTP)hOdb)->UserData.ulVal = ulVal;
}

odbSOCKET odbSockAllocate(void)
{
    odbSOCKET Sock;

    if( !(Sock = (odbSOCKET)malloc( sizeof(odbSOCKET_s) )) )
        return( NULL );

    Sock->sock = INVALID_SOCKET;
    Sock->ulConnectTimeout = 30;
    Sock->ulReadTimeout = 600;
    Sock->ulSendTimeout = 60;
    Sock->ulTransBufSize = 4096;
    Sock->ulTransSize = 0;
    Sock->lError = 0;

    if( !(Sock->pTransBuf = (odbPVOID)malloc( Sock->ulTransBufSize )) ) {
        free( Sock );
        return( NULL );
    }
    return( Sock );
}

void odbSockClose( odbSOCKET Sock )
{
    if( Sock->sock != INVALID_SOCKET ) {
        sock_close( Sock->sock );
        Sock->sock = INVALID_SOCKET;
    }
}

odbBOOL odbSockConnect( odbSOCKET Sock, odbPCSTR pszServer,
                        odbUSHORT usPort, odbPBOOL pbLookupFailed )
{
    int  nError;
    int  nTimeout = Sock->ulConnectTimeout;
    int* pnTimeout = nTimeout > 0 ? &nTimeout : NULL;

    Sock->bTimeout = FALSE;
    Sock->sock = sock_connect( pszServer, usPort, pnTimeout,
                               &nError, pbLookupFailed );
    Sock->lError = nError;
    if( pnTimeout && *pnTimeout < 0 ) Sock->bTimeout = TRUE;
    return( Sock->sock != INVALID_SOCKET ? TRUE : FALSE );
}

void odbSockFree( odbSOCKET Sock )
{
    if( Sock->sock != INVALID_SOCKET ) sock_close( Sock->sock );
    if( Sock->pTransBuf ) free( Sock->pTransBuf );
    free( Sock );
}

odbLONG odbSockRead( odbSOCKET Sock, odbPVOID pData, odbLONG lLen )
{
    odbLONG lRead;
    int     nError;
    int     nTimeout = Sock->ulReadTimeout;
    int*    pnTimeout = nTimeout > 0 ? &nTimeout : NULL;

    Sock->bTimeout = FALSE;
    lRead = sock_read( Sock->sock, (char*)pData, lLen, pnTimeout, &nError );
    Sock->lError = nError;
    if( lRead == SOCKET_ERROR ) odbSockClose( Sock );
    if( pnTimeout && *pnTimeout < 0 ) Sock->bTimeout = TRUE;
    return( lRead );
}

odbBOOL odbSockReadData( odbSOCKET Sock, odbPBYTE pbyData,
                         odbULONG ulBytesRequired )
{
    odbLONG  lRead;
    odbULONG ul;

    for( ul = 0; ul < ulBytesRequired; ul++ ) {
        if( Sock->ulTransSize == 0 ) {
            if( (lRead = odbSockRead( Sock, Sock->pTransBuf, Sock->ulTransBufSize )) <= 0 ) {
                return( FALSE );
            }
            Sock->ulTransSize = lRead;
            Sock->pbyTransData = (odbPBYTE)Sock->pTransBuf;
        }
        *(pbyData++) = *(Sock->pbyTransData++);
        Sock->ulTransSize--;
    }
    return( TRUE );
}

odbLONG odbSockSend( odbSOCKET Sock, odbPVOID pData, odbLONG lLen )
{
    odbLONG lSend;
    int     nError;
    int     nTimeout = Sock->ulSendTimeout;
    int*    pnTimeout = nTimeout > 0 ? &nTimeout : NULL;

    Sock->bTimeout = FALSE;
    if( lLen == 0 ) return( 0 );
    lSend = sock_send( Sock->sock, (char*)pData, lLen, pnTimeout, &nError );
    Sock->lError = nError;
    if( pnTimeout && *pnTimeout < 0 ) Sock->bTimeout = TRUE;
    return( lSend );
}

_odbdecl
odbPGUID odbStrToGuid( odbPGUID pguidVal, odbPCSTR pszStr )
{
    int i, n;

    memset( (char*)pguidVal, 0, sizeof(odbGUID) );

    for( n = 0; n < 8; n++, pszStr++ )
        pguidVal->ulData1 = (pguidVal->ulData1 << 4) + HEX2DEC(*pszStr);
    if( *(pszStr++) != '-' ) return( pguidVal );

    for( n = 0; n < 4; n++, pszStr++ )
        pguidVal->usData2 = (pguidVal->usData2 << 4) + HEX2DEC(*pszStr);
    if( *(pszStr++) != '-' ) return( pguidVal );

    for( n = 0; n < 4; n++, pszStr++ )
        pguidVal->usData3 = (pguidVal->usData3 << 4) + HEX2DEC(*pszStr);
    if( *(pszStr++) != '-' ) return( pguidVal );

    for( n = 0; n < 4; n++, pszStr++ ) {
        i = n / 2;
        pguidVal->byData4[i] = (pguidVal->byData4[i] << 4) + HEX2DEC(*pszStr);
    }
    if( *(pszStr++) != '-' ) return( pguidVal );

    for( ; n < 16; n++, pszStr++ ) {
        i = n / 2;
        pguidVal->byData4[i] = (pguidVal->byData4[i] << 4) + HEX2DEC(*pszStr);
    }
    return( pguidVal );
}

_odbdecl
odbLONGLONG odbStrToLongLong( odbPCSTR pszStr )
{
    odbBOOL     bIsNegative = FALSE;
    odbLONGLONG llVal = 0;

    for( ; *pszStr == ' '; pszStr++ );
    if( *pszStr == '-' ) {
        bIsNegative = TRUE;
        pszStr++;
    }
    for( ; *pszStr >= '0' && *pszStr <= '9'; pszStr++ )
        llVal = (llVal * 10) + (*pszStr - 48);

    if( bIsNegative ) llVal = -llVal;

    return( llVal );
}

_odbdecl
odbPTIMESTAMP odbStrToTimestamp( odbPTIMESTAMP ptsTime, odbPCSTR pszStr )
{
    odbPCSTR psz = pszStr;

    memcpy( ptsTime, &tsNull, sizeof(odbTIMESTAMP) );

    for( ; *psz && *psz != '-'; psz++ )
        ptsTime->sYear = (ptsTime->sYear * 10) + *psz - 48;
    for( ; *psz && *psz != '-'; psz++ )
        ptsTime->usMonth = (ptsTime->usMonth * 10) + *psz - 48;
    for( ; *psz && *psz != ' '; psz++ )
        ptsTime->usDay = (ptsTime->usDay * 10) + *psz - 48;

    for( ; *psz && *psz != ':'; psz++ )
        ptsTime->usHour = (ptsTime->usHour * 10) + *psz - 48;
    for( ; *psz && *psz != ':'; psz++ )
        ptsTime->usMinute = (ptsTime->usMinute * 10) + *psz - 48;
    for( ; *psz && *psz != '.'; psz++ )
        ptsTime->usSecond = (ptsTime->usSecond * 10) + *psz - 48;

    for( ; *psz; psz++ )
        ptsTime->ulFraction = (ptsTime->ulFraction * 10) + *psz - 48;
    ptsTime->ulFraction *= 1000;

    return( ptsTime );
}

_odbdecl
odbULONGLONG odbStrToULongLong( odbPCSTR pszStr )
{
    odbULONGLONG ullVal = 0;

    for( ; *pszStr == ' '; pszStr++ );
    if( *pszStr == '-' ) return( (odbULONGLONG)odbStrToLongLong( pszStr ) );
    for( ; *pszStr >= '0' && *pszStr <= '9'; pszStr++ )
        ullVal = (ullVal * 10) + (*pszStr - 48);

    return( ullVal );
}

_odbdecl
odbLONG odbTimestampToCTime( odbPTIMESTAMP ptsTime )
{
    struct tm tmTime;

    tmTime.tm_year = ptsTime->sYear - 1900;
    tmTime.tm_mon = ptsTime->usMonth - 1;
    tmTime.tm_mday = ptsTime->usDay;
    tmTime.tm_hour = ptsTime->usHour;
    tmTime.tm_min = ptsTime->usMinute;
    tmTime.tm_sec = ptsTime->usSecond;
    tmTime.tm_isdst = -1;

    return( mktime( &tmTime ) );
}

_odbdecl
odbPSTR odbTimestampToStr( odbPSTR pszStr, odbPTIMESTAMP ptsTime,
                           odbBOOL bIncludeFraction )
{
    odbPSTR  psz = pszStr;

    *(psz++) = (char)((ptsTime->sYear / 1000) + 48);
    *(psz++) = (char)(((ptsTime->sYear % 1000) / 100) + 48);
    *(psz++) = (char)(((ptsTime->sYear % 100) / 10) + 48);
    *(psz++) = (char)((ptsTime->sYear % 10) + 48);
    *(psz++) = '-';
    *(psz++) = (char)((ptsTime->usMonth / 10) + 48);
    *(psz++) = (char)((ptsTime->usMonth % 10) + 48);
    *(psz++) = '-';
    *(psz++) = (char)((ptsTime->usDay / 10) + 48);
    *(psz++) = (char)((ptsTime->usDay % 10) + 48);
    *(psz++) = ' ';
    *(psz++) = (char)((ptsTime->usHour / 10) + 48);
    *(psz++) = (char)((ptsTime->usHour % 10) + 48);
    *(psz++) = ':';
    *(psz++) = (char)((ptsTime->usMinute / 10) + 48);
    *(psz++) = (char)((ptsTime->usMinute % 10) + 48);
    *(psz++) = ':';
    *(psz++) = (char)((ptsTime->usSecond / 10) + 48);
    *(psz++) = (char)((ptsTime->usSecond % 10) + 48);

    if( bIncludeFraction ) {
        odbULONG ulMillisecond = ptsTime->ulFraction / 1000000;
        *(psz++) = '.';
        *(psz++) = (char)((ulMillisecond / 100) + 48);
        *(psz++) = (char)(((ulMillisecond % 100) / 10) + 48);
        *(psz++) = (char)((ulMillisecond % 10) + 48);
    }
    *psz = 0;

    return( pszStr );
}

_odbdecl
odbPSTR odbULongLongToStr( odbULONGLONG ullVal, odbPSTR pszStrEnd )
{
    *pszStrEnd = 0;

    do {
        *(--pszStrEnd) = (odbCHAR)((ullVal % 10) + 48);
    }
    while( (ullVal = ullVal / 10) );

    return( pszStrEnd );
}

_odbdecl
odbBOOL odbUseRowCache( odbHANDLE hCon, odbBOOL bUse, odbULONG ulSize )
{
    if( !odbSetAttrLong( hCon, ODB_ATTR_FETCHROWCOUNT, ulSize ) )
        return( FALSE );
    ((odbHCONNECTION)hCon)->bUseRowCache = bUse;
    ((odbHCONNECTION)hCon)->ulRowCacheSize = ulSize;
    return( TRUE );
}

#ifdef WIN32  /* Using WIN32 */
_odbdecl
void odbWinsockCleanup(void)
{
    sock_uninit();
}

_odbdecl
odbBOOL odbWinsockStartup(void)
{
    return( !sock_init( NULL ) );
}
#else /* Not Using WIN32 */
_odbdecl
void odbWinsockCleanup(void)
{
}

_odbdecl
odbBOOL odbWinsockStartup(void)
{
    return( TRUE );
}
#endif

