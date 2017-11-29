/* $Id: odbtpbuild.h,v 1.21 2005/12/22 04:48:02 rtwitty Exp $ */
/*
    odbtp - ODBTP client library

    Copyright (C) 2002-2004 Robert E. Twitty <rtwitty@users.sourceforge.net>

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
#ifndef _ODBTPBUILD_H_
#define _ODBTPBUILD_H_

// Odbtp Handle Types
#define ODBTPHANDLE_CONNECTION 1
#define ODBTPHANDLE_QUERY      2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    SOCKET   sock;
    odbULONG ulConnectTimeout;
    odbULONG ulReadTimeout;
    odbULONG ulSendTimeout;
    odbBOOL  bTimeout;
    odbPVOID pTransBuf;
    odbULONG ulTransBufSize;
    odbULONG ulTransSize;
    odbPBYTE pbyTransData;
    odbLONG  lError;
}
odbSOCKET_s;

typedef odbSOCKET_s* odbSOCKET;

typedef struct
{
    odbPSTR  pszName;
    odbSHORT sNameLen;
    odbSHORT sSqlType;
    odbSHORT sDefaultDataType;
    odbSHORT sDataType;
    odbULONG ulColSize;
    odbSHORT sDecDigits;
    odbSHORT sNullable;
    odbPSTR  pszSqlType;
    odbPSTR  pszTable;
    odbPSTR  pszSchema;
    odbPSTR  pszCatalog;
    odbPSTR  pszBaseName;
    odbPSTR  pszBaseTable;
    odbULONG ulFlags;
    odbCHAR  szSqlType[48];
    odbLONG  lDataLen;
    odbPBYTE pbySavedRowDataByte;
    odbULONG ulSavedRowDataByte;
    odbBOOL  bTruncated;
    odbLONG  lActualLen;
    union
    {
        odbBYTE      byVal;
        odbUSHORT    usVal;
        odbULONG     ulVal;
        odbULONGLONG ullVal;
        odbFLOAT     fVal;
        odbDOUBLE    dVal;
        odbTIMESTAMP tsVal;
        odbGUID      guidVal;
        odbPVOID     pVal;
    }
    Data;
    odbPVOID pUserData;
}
odbCOLUMN_s;

typedef odbCOLUMN_s* odbCOLUMN;

typedef struct
{
    odbUSHORT usType;
    odbSHORT  sSqlType;
    odbSHORT  sDefaultDataType;
    odbSHORT  sDataType;
    odbULONG  ulColSize;
    odbSHORT  sDecDigits;
    odbSHORT  sNullable;
    odbPSTR   pszName;
    odbPSTR   pszSqlType;
    odbLONG   lDataLen;
    odbBOOL   bBound;
    odbBOOL   bGot;
    odbBOOL   bTruncated;
    odbLONG   lActualLen;
    union
    {
        odbBYTE      byVal;
        odbUSHORT    usVal;
        odbULONG     ulVal;
        odbULONGLONG ullVal;
        odbFLOAT     fVal;
        odbDOUBLE    dVal;
        odbTIMESTAMP tsVal;
        odbGUID      guidVal;
        odbPVOID     pVal;
    }
    Data;
    odbPVOID pUserData;
}
odbPARAMETER_s;

typedef odbPARAMETER_s* odbPARAMETER;

typedef struct
{
    odbPBYTE pbyData;
    odbULONG ulSize;
}
odbROW_s;

typedef odbROW_s* odbROW;

typedef struct
{
    odbCHAR  szName[48];
    odbULONG ulColSize;
    odbSHORT sDecDigits;
    odbSHORT sSqlType;
}
odbDATATYPE_s;

typedef odbDATATYPE_s* odbDATATYPE;

typedef struct
{
    odbULONG  ulType;
    odbULONG  ulError;
    odbSOCKET Sock;
    odbULONG  ulRequestCode;
    odbULONG  ulResponseCode;
    odbPVOID  pResponseBuf;
    odbULONG  ulResponseBufSize;
    odbULONG  ulResponseSize;
    odbPBYTE  pbyExtractData;
    odbULONG  ulExtractSize;
    union
    {
        odbULONG ulVal;
        odbPVOID pVal;
    }
    UserData;
}
odbHODBTP_s;

typedef odbHODBTP_s* odbHODBTP;

typedef struct
{
    odbULONG  ulType;
    odbULONG  ulError;
    odbSOCKET Sock;
    odbULONG  ulRequestCode;
    odbULONG  ulResponseCode;
    odbPVOID  pResponseBuf;
    odbULONG  ulResponseBufSize;
    odbULONG  ulResponseSize;
    odbPBYTE  pbyExtractData;
    odbULONG  ulExtractSize;
    union
    {
        odbULONG ulVal;
        odbPVOID pVal;
    }
    UserData;
    odbHANDLE*  phQrys;
    odbULONG    ulMaxQrys;
    odbULONG    ulQryCursor;
    odbBOOL     bUseRowCache;
    odbULONG    ulRowCacheSize;
    odbBOOL     bConvertAll;
    odbBOOL     bConvertDatetime;
    odbBOOL     bConvertGuid;
    odbBOOL     bUseBroadTypes;
    odbDATATYPE DataTypes;
    odbULONG    ulTotalDataTypes;
    odbCHAR     szVersion[16];
}
odbHCONNECTION_s;

typedef odbHCONNECTION_s* odbHCONNECTION;

typedef struct
{
    odbULONG  ulType;
    odbULONG  ulError;
    odbSOCKET Sock;
    odbULONG  ulRequestCode;
    odbULONG  ulResponseCode;
    odbPVOID  pResponseBuf;
    odbULONG  ulResponseBufSize;
    odbULONG  ulResponseSize;
    odbPBYTE  pbyExtractData;
    odbULONG  ulExtractSize;
    union
    {
        odbULONG ulVal;
        odbPVOID pVal;
    }
    UserData;
    odbHANDLE    hCon;
    odbULONG     ulId;
    odbPVOID     pNormalBuf;
    odbULONG     ulNormalBufSize;
    odbULONG     ulNormalSize;
    odbPVOID     pColInfoBuf;
    odbULONG     ulColInfoBufSize;
    odbULONG     ulColInfoSize;
    odbCOLUMN    Cols;
    odbUSHORT    usMaxCols;
    odbUSHORT    usTotalCols;
    odbPVOID     pParamInfoBuf;
    odbULONG     ulParamInfoBufSize;
    odbULONG     ulParamInfoSize;
    odbPVOID     pParamDataBuf;
    odbULONG     ulParamDataBufSize;
    odbULONG     ulParamDataSize;
    odbPARAMETER Params;
    odbUSHORT    usMaxParams;
    odbUSHORT    usTotalParams;
    odbBOOL      bPreparedProc;
    odbPVOID     pRowDataBuf;
    odbULONG     ulRowDataBufSize;
    odbULONG     ulRowDataSize;
    odbROW       Rows;
    odbLONG      lMaxRows;
    odbLONG      lTotalRows;
    odbLONG      lRowCursor;
    odbPBYTE     pbyRowExtractData;
    odbULONG     ulRowExtractSize;
    odbPBYTE     pbySavedRowDataEndByte;
    odbULONG     ulSavedRowDataEndByte;
    odbBOOL      bGotRow;
    odbLONG      lRowCount;
    odbBOOL      bNoData;
    odbBOOL      bMoreResults;
    odbUSHORT    usCursorType;
    odbUSHORT    usCursorConcur;
    odbBOOL      bBookmarksOn;
    odbBOOL      bUseBroadTypes;
}
odbHQUERY_s;

typedef odbHQUERY_s* odbHQUERY;

typedef struct
{
    odbPCSTR pszType;
    odbPCSTR pszOdbtpHost;
    odbULONG ulOdbtpPort;
    odbPCSTR pszDBConnect;
    odbPCSTR pszDriver;
    odbPCSTR pszServer;
    odbPCSTR pszDatabase;
    odbPCSTR pszUsername;
    odbPCSTR pszPassword;
    odbPCSTR pszLoginType;
    odbBOOL  bUseRowCache;
    odbULONG ulRowCacheSize;
    odbBOOL  bFullColInfo;
    odbBOOL  bLoadDataTypes;
    odbBOOL  bConvertAll;
    odbBOOL  bConvertDatetime;
    odbBOOL  bConvertGuid;
    odbBOOL  bUnicodeSQL;
    odbBOOL  bRightTrimText;
    odbULONG ulVarDataSize;
    odbBOOL  bUseBroadTypes;
    odbULONG ulConnectTimeout;
    odbULONG ulReadTimeout;
}
odbINTERFACE;

typedef odbINTERFACE* odbPINTERFACE;

odbHANDLE    odbAllocateConnection(void);
odbHANDLE    odbAllocateQuery(void);
odbCOLUMN    odbCol( odbHANDLE hQry, odbUSHORT usCol, odbBOOL bValid );
odbBOOL      odbConnect( odbHANDLE hCon, odbPCSTR pszServer, odbUSHORT usPort );
odbBOOL      odbDisconnect( odbHANDLE hCon );
odbBOOL      odbExtract( odbHANDLE hOdb, odbPVOID pData, odbULONG ulLen );
odbBOOL      odbExtractByte( odbHANDLE hOdb, odbPBYTE pbyData );
odbBOOL      odbExtractDouble( odbHANDLE hOdb, odbPDOUBLE pdData );
odbBOOL      odbExtractFloat( odbHANDLE hOdb, odbPFLOAT pfData );
odbBOOL      odbExtractGuid( odbHANDLE hOdb, odbPGUID pguidData );
odbBOOL      odbExtractLong( odbHANDLE hOdb, odbPULONG pulData );
odbBOOL      odbExtractLongLong( odbHANDLE hOdb, odbPULONGLONG pullData );
odbBOOL      odbExtractPointer( odbHANDLE hOdb, odbPVOID* ppData,
                                odbULONG ulLen );
odbBOOL      odbExtractShort( odbHANDLE hOdb, odbPUSHORT pusData );
odbBOOL      odbExtractTimestamp( odbHANDLE hOdb, odbPTIMESTAMP ptsData );
void         odbFreeConnection( odbHCONNECTION hCon );
void         odbFreeQuery( odbHQUERY hQry );
odbBOOL      odbGetColInfo( odbHANDLE hQry );
odbBOOL      odbGetCursor( odbHANDLE hQry );
odbPCSTR     odbGetDataTypeName( odbHANDLE hCon, odbSHORT sSqlType,
                                 odbULONG ulColSize, odbSHORT sDecDigits );
odbBOOL      odbGetInterface( odbPINTERFACE pInterface,
                              odbPSTR pszInterfaces, odbPCSTR pszName );
odbSHORT     odbGetOdbDataType( odbSHORT sSqlType );
odbBOOL      odbGetParamInfo( odbHANDLE hQry );
void         odbHostToNetworkOrder( odbPBYTE pbyTo, odbPBYTE pbyFrom, odbULONG ulLen );
void         odbInitExtract( odbHANDLE hOdb, odbPVOID pData, odbULONG ulSize );
void         odbInitResponseBuf( odbHANDLE hOdb );
odbPARAMETER odbParam( odbHANDLE hQry, odbUSHORT usParam, odbBOOL bValid );
odbBOOL      odbProcessColInfo( odbHANDLE hQry );
odbBOOL      odbProcessCursor( odbHANDLE hQry );
odbBOOL      odbProcessParamInfo( odbHANDLE hQry );
odbBOOL      odbProcessResult( odbHANDLE hQry );
odbBOOL      odbProcessRowData( odbHANDLE hQry );
odbULONG     odbReadDataLong( odbPBYTE pbyData );
odbPSTR      odbReadInterfaceFile( odbHANDLE hOdb, odbPCSTR pszFile );
odbBOOL      odbReadResponse( odbHANDLE hOdb );
void         odbRestoreRowDataBytes( odbHANDLE hQry );
void         odbSaveResponseBuf( odbHANDLE hOdb );
odbBOOL      odbSendRequest( odbHANDLE hOdb, odbUSHORT usCode,
                             odbPVOID pData, odbULONG ulLen,
                             odbBOOL bFinal );
odbBOOL      odbSendRequestByte( odbHANDLE hOdb, odbUSHORT usCode,
                                 odbBYTE byData, odbBOOL bFinal );
odbBOOL      odbSendRequestDouble( odbHANDLE hOdb, odbUSHORT usCode,
                                   odbDOUBLE dData, odbBOOL bFinal );
odbBOOL      odbSendRequestFloat( odbHANDLE hOdb, odbUSHORT usCode,
                                  odbFLOAT fData, odbBOOL bFinal );
odbBOOL      odbSendRequestGuid( odbHANDLE hOdb, odbUSHORT usCode,
                                 odbPGUID pguidData, odbBOOL bFinal );
odbBOOL      odbSendRequestLong( odbHANDLE hOdb, odbUSHORT usCode,
                                 odbULONG ulData, odbBOOL bFinal );
odbBOOL      odbSendRequestLongLong( odbHANDLE hOdb, odbUSHORT usCode,
                                     odbULONGLONG ullData, odbBOOL bFinal );
odbBOOL      odbSendRequestShort( odbHANDLE hOdb, odbUSHORT usCode,
                                  odbUSHORT usData, odbBOOL bFinal );
odbBOOL      odbSendRequestText( odbHANDLE hOdb, odbUSHORT usCode,
                                 odbPCSTR pszData, odbBOOL bFinal );
odbBOOL      odbSendRequestTimestamp( odbHANDLE hOdb, odbUSHORT usCode,
                                      odbPTIMESTAMP ptsData, odbBOOL bFinal );
odbBOOL      odbSetDBConnectStr( odbPSTR pszDBConnect, odbULONG ulLen,
                                 odbPINTERFACE pInterface );
odbSOCKET    odbSockAllocate(void);
void         odbSockClose( odbSOCKET Sock );
odbBOOL      odbSockConnect( odbSOCKET Sock, odbPCSTR pszServer,
                             odbUSHORT usPort, odbPBOOL pbLookupFailed );
void         odbSockFree( odbSOCKET Sock );
odbLONG      odbSockRead( odbSOCKET Sock, void* pData, odbLONG lLen );
odbBOOL      odbSockReadData( odbSOCKET Sock, odbPBYTE pbyData,
                              odbULONG ulBytesRequired );
odbLONG      odbSockSend( odbSOCKET Sock, void* pData, odbLONG lLen );

#ifdef __cplusplus
}
#endif

#endif
