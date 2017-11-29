/* $Id: odbtp.h,v 1.39 2005/12/22 04:48:02 rtwitty Exp $ */
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
#ifndef _ODBTP_H_
#define _ODBTP_H_

/* The below line must be uncommented for 64-bit systems, such as Tru64. */
/* #define _C_LONG_64_ 1 */

#define ODBTP_LIB_VERSION "1.1.4"

#ifdef ODBTP_DLL /* Using odbtp.dll */
  #ifdef ODBTP_DLL_BUILD
    #define _odbdecl __declspec(dllexport)
  #else
    #define _odbdecl __declspec(dllimport)
  #endif
#else /* Not Using odbtp.dll */
    #define _odbdecl
#endif
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

/* ODB Data Definitions */
#define ODB_BINARY    (-2)
#define ODB_BIGINT    (-25)
#define ODB_UBIGINT   (-27)
#define ODB_BIT       (-7)
#define ODB_CHAR      1
#define ODB_DATE      91
#define ODB_DATETIME  93
#define ODB_DOUBLE    8
#define ODB_FLOAT     8
#define ODB_GUID      (-11)
#define ODB_INT       (-16)
#define ODB_UINT      (-18)
#define ODB_NUMERIC   2
#define ODB_REAL      7
#define ODB_SMALLINT  (-15)
#define ODB_USMALLINT (-17)
#define ODB_TIME      92
#define ODB_TINYINT   (-26)
#define ODB_UTINYINT  (-28)
#define ODB_WCHAR     (-8)

/* ODB Login Types */
#define ODB_LOGIN_NORMAL   0
#define ODB_LOGIN_RESERVED 1
#define ODB_LOGIN_SINGLE   2

/* ODB Numeric Attributes */
#define ODB_ATTR_DRIVER         0
#define ODB_ATTR_FETCHROWCOUNT  1
#define ODB_ATTR_TRANSACTIONS   2
#define ODB_ATTR_DESCRIBEPARAMS 3
#define ODB_ATTR_UNICODESQL     4
#define ODB_ATTR_FULLCOLINFO    5
#define ODB_ATTR_QUERYTIMEOUT   6
#define ODB_ATTR_OICLEVEL       7
#define ODB_ATTR_TXNCAPABLE     8
#define ODB_ATTR_MAPCHARTOWCHAR 9
#define ODB_ATTR_VARDATASIZE    10
#define ODB_ATTR_CACHEPROCS     11
#define ODB_ATTR_RIGHTTRIMTEXT  12

/* ODB String Attributes */
#define ODB_ATTR_STRING         0x10000
#define ODB_ATTR_DSN            (0|ODB_ATTR_STRING)
#define ODB_ATTR_DRIVERNAME     (1|ODB_ATTR_STRING)
#define ODB_ATTR_DRIVERVER      (2|ODB_ATTR_STRING)
#define ODB_ATTR_DRIVERODBCVER  (3|ODB_ATTR_STRING)
#define ODB_ATTR_DBMSNAME       (4|ODB_ATTR_STRING)
#define ODB_ATTR_DBMSVER        (5|ODB_ATTR_STRING)
#define ODB_ATTR_SERVERNAME     (6|ODB_ATTR_STRING)
#define ODB_ATTR_USERNAME       (7|ODB_ATTR_STRING)
#define ODB_ATTR_DATABASENAME   (8|ODB_ATTR_STRING)

/* ODB ODBC Driver types */
#define ODB_DRIVER_UNKNOWN 0
#define ODB_DRIVER_MSSQL   1
#define ODB_DRIVER_JET     2
#define ODB_DRIVER_FOXPRO  3
#define ODB_DRIVER_ORACLE  4
#define ODB_DRIVER_SYBASE  5
#define ODB_DRIVER_MYSQL   6
#define ODB_DRIVER_DB2     7

/* ODB Transaction Types */
#define ODB_TXN_NONE            0
#define ODB_TXN_READUNCOMMITTED 1
#define ODB_TXN_READCOMMITTED   2
#define ODB_TXN_REPEATABLEREAD  3
#define ODB_TXN_SERIALIZABLE    4
#define ODB_TXN_DEFAULT         255

/* ODB ODBC Interface Conformace Levels */
#define ODB_OIC_CORE   1
#define ODB_OIC_LEVEL1 2
#define ODB_OIC_LEVEL2 3

/* ODB Col Info Flags */
#define ODB_COLINFO_NOTNULL    0x0001
#define ODB_COLINFO_UNSIGNED   0x0002
#define ODB_COLINFO_AUTONUMBER 0x0004
#define ODB_COLINFO_PRIMARYKEY 0x0008
#define ODB_COLINFO_HIDDEN     0x0010

/* ODB Param Types */
#define ODB_PARAM_NONE      0x0000
#define ODB_PARAM_INPUT     0x0001
#define ODB_PARAM_OUTPUT    0x0002
#define ODB_PARAM_INOUT     (ODB_PARAM_INPUT | ODB_PARAM_OUTPUT)
#define ODB_PARAM_RETURNVAL (0x0004 | ODB_PARAM_OUTPUT)
#define ODB_PARAM_RESULTCOL 0x0008
#define ODB_PARAM_DESCRIBED 0x8000

/* ODB Length Indicators */
#define ODB_NULL       (-1)
#define ODB_DEFAULT    (-5)
#define ODB_IGNORE     (-6)
#define ODB_TRUNCATION 0xE0000001
#define ODB_SENTSYNC   0xF000000F

/* ODB Cursor Types */
#define ODB_CURSOR_FORWARD 0
#define ODB_CURSOR_STATIC  1
#define ODB_CURSOR_KEYSET  2
#define ODB_CURSOR_DYNAMIC 3

/* ODB Cursor Concurrency Types */
#define ODB_CONCUR_DEFAULT  0
#define ODB_CONCUR_READONLY 1
#define ODB_CONCUR_LOCK     2
#define ODB_CONCUR_ROWVER   3
#define ODB_CONCUR_VALUES   4

/* ODB Fetch Types */
#define ODB_FETCH_DEFAULT  0
#define ODB_FETCH_FIRST    1
#define ODB_FETCH_LAST     2
#define ODB_FETCH_NEXT     3
#define ODB_FETCH_PREV     4
#define ODB_FETCH_ABS      5
#define ODB_FETCH_REL      6
#define ODB_FETCH_BOOKMARK 7
#define ODB_FETCH_REREAD   8

/* ODB Row Operations */
#define ODB_ROW_GETSTATUS 0
#define ODB_ROW_REFRESH   1
#define ODB_ROW_UPDATE    2
#define ODB_ROW_DELETE    3
#define ODB_ROW_ADD       4
#define ODB_ROW_LOCK      5
#define ODB_ROW_UNLOCK    6
#define ODB_ROW_BOOKMARK  7

/* ODB Row Status Values */
#define ODB_ROWSTAT_ERROR   0
#define ODB_ROWSTAT_SUCCESS 1
#define ODB_ROWSTAT_UPDATED 2
#define ODB_ROWSTAT_DELETED 3
#define ODB_ROWSTAT_ADDED   4
#define ODB_ROWSTAT_NOROW   5
#define ODB_ROWSTAT_UNKNOWN 0xFFFFFFFF

/* Error Codes */
#define ODBTPERR_NONE         0
#define ODBTPERR_MEMORY       1
#define ODBTPERR_HANDLE       2
#define ODBTPERR_CONNECT      3
#define ODBTPERR_READ         4
#define ODBTPERR_SEND         5
#define ODBTPERR_TIMEOUTCONN  6
#define ODBTPERR_TIMEOUTREAD  7
#define ODBTPERR_TIMEOUTSEND  8
#define ODBTPERR_CONNECTED    9
#define ODBTPERR_PROTOCOL     10
#define ODBTPERR_RESPONSE     11
#define ODBTPERR_MAXQUERYS    12
#define ODBTPERR_COLNUMBER    13
#define ODBTPERR_COLNAME      14
#define ODBTPERR_FETCHROW     15
#define ODBTPERR_NOTPREPPROC  16
#define ODBTPERR_NOPARAMINFO  17
#define ODBTPERR_PARAMNUMBER  18
#define ODBTPERR_PARAMNAME    19
#define ODBTPERR_PARAMBIND    20
#define ODBTPERR_PARAMGET     21
#define ODBTPERR_ATTRTYPE     22
#define ODBTPERR_GETQUERY     23
#define ODBTPERR_INTERFFILE   24
#define ODBTPERR_INTERFSYN    25
#define ODBTPERR_INTERFTYPE   26
#define ODBTPERR_CONNSTRLEN   27
#define ODBTPERR_NOSEEKCURSOR 28
#define ODBTPERR_SEEKROWPOS   29
#define ODBTPERR_DETACHED     30
#define ODBTPERR_GETTYPEINFO  31
#define ODBTPERR_LOADTYPES    32
#define ODBTPERR_NOREQUEST    33
#define ODBTPERR_FETCHEDROWS  34
#define ODBTPERR_DISCONNECTED 35
#define ODBTPERR_HOSTRESOLVE  36
#define ODBTPERR_SERVER       100

/* SQL Data Types */
#define SQL_BIT            (-7)
#define SQL_TINYINT        (-6)
#define SQL_SMALLINT       5
#define SQL_INTEGER        4
#define SQL_INT            4
#define SQL_BIGINT         (-5)
#define SQL_NUMERIC        2
#define SQL_REAL           7
#define SQL_FLOAT          6
#define SQL_DOUBLE         8
#define SQL_DECIMAL        3
#define SQL_DATE           9
#define SQL_TIME           10
#define SQL_TIMESTAMP      11
#define SQL_TYPE_DATE      91
#define SQL_TYPE_TIME      92
#define SQL_TYPE_TIMESTAMP 93
#define SQL_DATETIME       93
#define SQL_CHAR           1
#define SQL_VARCHAR        12
#define SQL_LONGVARCHAR    (-1)
#define SQL_TEXT           (-1)
#define SQL_WCHAR          (-8)
#define SQL_NCHAR          (-8)
#define SQL_WVARCHAR       (-9)
#define SQL_NVARCHAR       (-9)
#define SQL_WLONGVARCHAR   (-10)
#define SQL_NTEXT          (-10)
#define SQL_BINARY         (-2)
#define SQL_VARBINARY      (-3)
#define SQL_LONGVARBINARY  (-4)
#define SQL_IMAGE          (-4)
#define SQL_GUID           (-11)
#define SQL_VARIANT        (-150)

/* Miscellaneous Definitions */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Odbtp Lib Data Types */
typedef int            odbBOOL;
typedef char           odbCHAR;
typedef unsigned char  odbBYTE;
typedef short          odbSHORT;
typedef unsigned short odbUSHORT;
#ifndef _C_LONG_64_
typedef long           odbLONG;
typedef unsigned long  odbULONG;
#else
typedef int            odbLONG;
typedef unsigned int   odbULONG;
#endif
typedef float          odbFLOAT;
typedef double         odbDOUBLE;

#ifdef WIN32  /* Using WIN32 */
typedef __int64            odbLONGLONG;
typedef unsigned __int64   odbULONGLONG;
#else /* Not Using WIN32 */
#ifndef _C_LONG_64_
typedef long long          odbLONGLONG;
typedef unsigned long long odbULONGLONG;
#else
typedef long               odbLONGLONG;
typedef unsigned long      odbULONGLONG;
#endif
#endif

typedef struct
{
    odbSHORT  sYear;
    odbUSHORT usMonth;
    odbUSHORT usDay;
    odbUSHORT usHour;
    odbUSHORT usMinute;
    odbUSHORT usSecond;
    odbULONG  ulFraction;
}
odbTIMESTAMP;

typedef struct
{
    odbULONG  ulData1;
    odbUSHORT usData2;
    odbUSHORT usData3;
    odbBYTE   byData4[8];
}
odbGUID;

typedef odbBOOL*       odbPBOOL;
typedef odbCHAR*       odbPSTR;
typedef odbBYTE*       odbPBYTE;
typedef const odbCHAR* odbPCSTR;
typedef odbSHORT*      odbPSHORT;
typedef odbUSHORT*     odbPUSHORT;
typedef odbLONG*       odbPLONG;
typedef odbULONG*      odbPULONG;
typedef odbLONGLONG*   odbPLONGLONG;
typedef odbULONGLONG*  odbPULONGLONG;
typedef odbFLOAT*      odbPFLOAT;
typedef odbDOUBLE*     odbPDOUBLE;
typedef odbTIMESTAMP*  odbPTIMESTAMP;
typedef odbGUID*       odbPGUID;
typedef void*          odbPVOID;

typedef struct
{
    void* x;
}
odbHANDLE_s;

typedef odbHANDLE_s* odbHANDLE;
typedef odbHANDLE*   odbPHANDLE;

/* Odbtp Lib Functions */
_odbdecl
odbHANDLE     odbAllocate( odbHANDLE hCon );
_odbdecl
odbBOOL       odbBindCol( odbHANDLE hQry, odbUSHORT usCol, odbSHORT sDataType,
                          odbLONG lDataLen, odbBOOL bFinal );
_odbdecl
odbBOOL       odbBindInOutParam( odbHANDLE hQry,
                                 odbUSHORT usParam, odbSHORT sDataType,
                                 odbLONG lDataLen, odbBOOL bFinal );
_odbdecl
odbBOOL       odbBindInputParam( odbHANDLE hQry,
                                 odbUSHORT usParam, odbSHORT sDataType,
                                 odbLONG lDataLen, odbBOOL bFinal );
_odbdecl
odbBOOL       odbBindOutputParam( odbHANDLE hQry,
                                  odbUSHORT usParam, odbSHORT sDataType,
                                  odbLONG lDataLen, odbBOOL bFinal );
_odbdecl
odbBOOL       odbBindParam( odbHANDLE hQry, odbUSHORT usParam,
                            odbUSHORT usType, odbSHORT sDataType,
                            odbLONG lDataLen, odbBOOL bFinal );
_odbdecl
odbBOOL       odbBindParamEx( odbHANDLE hQry, odbUSHORT usParam,
                              odbUSHORT usType, odbSHORT sDataType,
                              odbLONG lDataLen, odbSHORT sSqlType,
                              odbULONG ulColSize, odbSHORT sDecDigits,
                              odbBOOL bFinal );
_odbdecl
odbLONG       odbColActualLen( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPCSTR      odbColBaseName( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPCSTR      odbColBaseTable( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPCSTR      odbColCatalog( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPVOID      odbColData( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbBYTE       odbColDataByte( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbDOUBLE     odbColDataDouble( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbFLOAT      odbColDataFloat( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPGUID      odbColDataGuid( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbLONG       odbColDataLen( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbULONG      odbColDataLong( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbULONGLONG  odbColDataLongLong( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbUSHORT     odbColDataShort( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPSTR       odbColDataText( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPTIMESTAMP odbColDataTimestamp( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbSHORT      odbColDataType( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbSHORT      odbColDecDigits( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbSHORT      odbColDefaultDataType( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbULONG      odbColFlags( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPCSTR      odbColName( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbUSHORT     odbColNum( odbHANDLE hQry, odbPCSTR pszName );
_odbdecl
odbPCSTR      odbColSchema( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbULONG      odbColSize( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbSHORT      odbColSqlType( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPCSTR      odbColSqlTypeName( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPCSTR      odbColTable( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbBOOL       odbColTruncated( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbPVOID      odbColUserData( odbHANDLE hQry, odbUSHORT usCol );
_odbdecl
odbBOOL       odbCommit( odbHANDLE hCon );
_odbdecl
odbBOOL       odbConvertAll( odbHANDLE hCon, odbBOOL bConvert );
_odbdecl
odbBOOL       odbConvertDatetime( odbHANDLE hCon, odbBOOL bConvert );
_odbdecl
odbBOOL       odbConvertGuid( odbHANDLE hCon, odbBOOL bConvert );
_odbdecl
void          odbCTimeToTimestamp( odbPTIMESTAMP ptsTime, odbLONG lTime );
_odbdecl
odbBOOL       odbDescribeSqlType( odbHANDLE hCon,
                                  odbPCSTR pszSqlTypeName, odbPSHORT psSqlType,
                                  odbPULONG pulColSize, odbPSHORT psDecDigits );
_odbdecl
odbBOOL       odbDetachQry( odbHANDLE hQry );
_odbdecl
odbBOOL       odbDoRowOperation( odbHANDLE hQry, odbUSHORT usRowOp );
_odbdecl
odbBOOL       odbDropQry( odbHANDLE hQry );
_odbdecl
odbBOOL       odbExecute( odbHANDLE hQry, odbPCSTR pszSQL );
_odbdecl
odbBOOL       odbFetchNextResult( odbHANDLE hQry );
_odbdecl
odbBOOL       odbFetchRow( odbHANDLE hQry );
_odbdecl
odbBOOL       odbFetchRowEx( odbHANDLE hQry, odbUSHORT usFetchType,
                             odbLONG lFetchParam );
_odbdecl
odbBOOL       odbFetchRowsIntoCache( odbHANDLE hQry );
_odbdecl
odbBOOL       odbFinalizeRequest( odbHANDLE hOdb );
_odbdecl
void          odbFree( odbHANDLE hOdb );
_odbdecl
odbBOOL       odbGetAttrLong( odbHANDLE hCon, odbLONG lAttr,
                              odbPULONG pulVal );
_odbdecl
odbBOOL       odbGetAttrText( odbHANDLE hCon, odbLONG lAttr,
                              odbPSTR pszVal, odbULONG ulValLen );
_odbdecl
odbHANDLE     odbGetConnection( odbHANDLE hQry );
_odbdecl
odbPCSTR      odbGetConnectionId( odbHANDLE hCon );
_odbdecl
odbULONG      odbGetError( odbHANDLE hOdb );
_odbdecl
odbPCSTR      odbGetErrorText( odbHANDLE hOdb );
_odbdecl
odbHANDLE     odbGetFirstQuery( odbHANDLE hCon );
_odbdecl
odbHANDLE     odbGetNextQuery( odbHANDLE hCon );
_odbdecl
odbBOOL       odbGetOutputParams( odbHANDLE hQry );
_odbdecl
odbBOOL       odbGetParam( odbHANDLE hQry, odbUSHORT usParam, odbBOOL bFinal );
_odbdecl
odbHANDLE     odbGetQuery( odbHANDLE hCon, odbULONG ulQryId );
_odbdecl
odbULONG      odbGetQueryId( odbHANDLE hQry );
_odbdecl
odbPVOID      odbGetResponse( odbHANDLE hOdb );
_odbdecl
odbULONG      odbGetResponseCode( odbHANDLE hOdb );
_odbdecl
odbULONG      odbGetResponseSize( odbHANDLE hOdb );
_odbdecl
odbULONG      odbGetRowCacheSize( odbHANDLE hCon );
_odbdecl
odbLONG       odbGetRowCount( odbHANDLE hQry );
_odbdecl
odbULONG      odbGetRowStatus( odbHANDLE hQry );
_odbdecl
odbLONG       odbGetSockError( odbHANDLE hOdb );
_odbdecl
odbPCSTR      odbGetSockErrorText( odbHANDLE hOdb );
_odbdecl
odbUSHORT     odbGetTotalCols( odbHANDLE hQry );
_odbdecl
odbUSHORT     odbGetTotalParams( odbHANDLE hQry );
_odbdecl
odbLONG       odbGetTotalRows( odbHANDLE hQry );
_odbdecl
odbPVOID      odbGetUserData( odbHANDLE hOdb );
_odbdecl
odbULONG      odbGetUserDataLong( odbHANDLE hOdb );
_odbdecl
odbPCSTR      odbGetVersion( odbHANDLE hCon );
_odbdecl
odbBOOL       odbGotParam( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPSTR       odbGuidToStr( odbPSTR pszStr, odbPGUID pguidVal );
_odbdecl
odbBOOL       odbIsConnected( odbHANDLE hOdb );
_odbdecl
odbBOOL       odbIsConnection( odbHANDLE hOdb );
_odbdecl
odbBOOL       odbIsTextAttr( odbLONG lAttr );
_odbdecl
odbBOOL       odbIsUsingRowCache( odbHANDLE hCon );
_odbdecl
odbBOOL       odbLoadDataTypes( odbHANDLE hCon );
_odbdecl
odbBOOL       odbLogin( odbHANDLE hCon, odbPCSTR pszServer, odbUSHORT usPort,
                        odbUSHORT usType, odbPCSTR pszDBConnect );
_odbdecl
odbBOOL      odbLoginInterface( odbHANDLE hCon, odbPCSTR pszFile,
                                odbPCSTR pszInterface, odbPCSTR pszUsername,
                                odbPCSTR pszPassword, odbPCSTR pszDatabase,
                                odbUSHORT usType );
_odbdecl
odbBOOL       odbLogout( odbHANDLE hCon, odbBOOL bDisconnectDb );
_odbdecl
odbPSTR       odbLongLongToStr( odbLONGLONG llVal, odbPSTR pszStrEnd );
_odbdecl
odbBOOL       odbNoData( odbHANDLE hQry );
_odbdecl
odbLONG       odbParamActualLen( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPVOID      odbParamData( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbBYTE       odbParamDataByte( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbDOUBLE     odbParamDataDouble( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbFLOAT      odbParamDataFloat( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPGUID      odbParamDataGuid( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbLONG       odbParamDataLen( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbULONG      odbParamDataLong( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbULONGLONG  odbParamDataLongLong( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbUSHORT     odbParamDataShort( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPSTR       odbParamDataText( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPTIMESTAMP odbParamDataTimestamp( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbSHORT      odbParamDataType( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbSHORT      odbParamDecDigits( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbSHORT      odbParamDefaultDataType( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPCSTR      odbParamName( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbUSHORT     odbParamNum( odbHANDLE hQry, odbPCSTR pszName );
_odbdecl
odbULONG      odbParamSize( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbSHORT      odbParamSqlType( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPCSTR      odbParamSqlTypeName( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbBOOL       odbParamTruncated( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbUSHORT     odbParamType( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbPVOID      odbParamUserData( odbHANDLE hQry, odbUSHORT usParam );
_odbdecl
odbBOOL       odbPrepare( odbHANDLE hQry, odbPCSTR pszSQL );
_odbdecl
odbBOOL       odbPrepareProc( odbHANDLE hQry, odbPCSTR pszProcedure );
_odbdecl
odbBOOL       odbRollback( odbHANDLE hCon );
_odbdecl
odbBOOL       odbSeekRow( odbHANDLE hQry, odbLONG lRow );
_odbdecl
odbBOOL       odbSetAttrLong( odbHANDLE hCon, odbLONG lAttr,
                              odbULONG ulVal );
_odbdecl
odbBOOL       odbSetAttrText( odbHANDLE hCon, odbLONG lAttr,
                              odbPCSTR pszVal );
_odbdecl
odbBOOL       odbSetCol( odbHANDLE hQry, odbUSHORT usCol,
                         odbPVOID pData, odbLONG lDataLen,
                         odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColByte( odbHANDLE hQry, odbUSHORT usCol,
                             odbBYTE byData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColDouble( odbHANDLE hQry, odbUSHORT usCol,
                               odbDOUBLE dData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColFloat( odbHANDLE hQry, odbUSHORT usCol,
                              odbFLOAT fData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColGuid( odbHANDLE hQry, odbUSHORT usCol,
                             odbPGUID pguidData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColIgnore( odbHANDLE hQry, odbUSHORT usCol,
                               odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColLong( odbHANDLE hQry, odbUSHORT usCol,
                             odbULONG ulData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColLongLong( odbHANDLE hQry, odbUSHORT usCol,
                                 odbULONGLONG ullData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColNull( odbHANDLE hQry, odbUSHORT usCol,
                             odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColShort( odbHANDLE hQry, odbUSHORT usCol,
                              odbUSHORT usData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColText( odbHANDLE hQry, odbUSHORT usCol,
                             odbPCSTR pszData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColTimestamp( odbHANDLE hQry, odbUSHORT usCol,
                                  odbPTIMESTAMP ptsData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetColUserData( odbHANDLE hQry, odbUSHORT usCol,
                                 odbPVOID pVal );
_odbdecl
odbULONG      odbSetConnectTimeout( odbHANDLE hOdb, odbULONG ulTimeout );
_odbdecl
odbBOOL       odbSetCursor( odbHANDLE hQry, odbUSHORT usType,
                            odbUSHORT usConcurrency,
                            odbBOOL bEnableBookmarks );
_odbdecl
odbBOOL       odbSetError( odbHANDLE hOdb, odbULONG ulError );
_odbdecl
odbBOOL       odbSetParam( odbHANDLE hQry, odbUSHORT usParam,
                           odbPVOID pData, odbLONG lDataLen,
                           odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamByte( odbHANDLE hQry, odbUSHORT usParam,
                               odbBYTE byData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamDefault( odbHANDLE hQry, odbUSHORT usParam,
                                  odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamDouble( odbHANDLE hQry, odbUSHORT usParam,
                                 odbDOUBLE dData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamFloat( odbHANDLE hQry, odbUSHORT usParam,
                                odbFLOAT fData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamGuid( odbHANDLE hQry, odbUSHORT usParam,
                               odbPGUID pguidData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamLong( odbHANDLE hQry, odbUSHORT usParam,
                               odbULONG ulData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamLongLong( odbHANDLE hQry, odbUSHORT usParam,
                                   odbULONGLONG ullData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamNull( odbHANDLE hQry, odbUSHORT usParam,
                               odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamShort( odbHANDLE hQry, odbUSHORT usParam,
                                odbUSHORT usData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamText( odbHANDLE hQry, odbUSHORT usParam,
                               odbPCSTR pszData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamTimestamp( odbHANDLE hQry, odbUSHORT usParam,
                                    odbPTIMESTAMP ptsData, odbBOOL bFinal );
_odbdecl
odbBOOL       odbSetParamUserData( odbHANDLE hQry, odbUSHORT usParam,
                                   odbPVOID pVal );
_odbdecl
odbULONG      odbSetReadTimeout( odbHANDLE hOdb, odbULONG ulTimeout );
_odbdecl
odbULONG      odbSetSendTimeout( odbHANDLE hOdb, odbULONG ulTimeout );
_odbdecl
void          odbSetUserData( odbHANDLE hOdb, odbPVOID pVal );
_odbdecl
void          odbSetUserDataLong( odbHANDLE hOdb, odbULONG ulVal );
_odbdecl
odbPGUID      odbStrToGuid( odbPGUID pguidVal, odbPCSTR pszStr );
_odbdecl
odbLONGLONG   odbStrToLongLong( odbPCSTR pszStr );
_odbdecl
odbPTIMESTAMP odbStrToTimestamp( odbPTIMESTAMP ptsTime, odbPCSTR pszStr );
_odbdecl
odbULONGLONG  odbStrToULongLong( odbPCSTR pszStr );
_odbdecl
odbLONG       odbTimestampToCTime( odbPTIMESTAMP ptsTime );
_odbdecl
odbPSTR       odbTimestampToStr( odbPSTR pszStr, odbPTIMESTAMP ptsTime,
                                 odbBOOL bIncludeFraction );
_odbdecl
odbPSTR       odbULongLongToStr( odbULONGLONG ullVal, odbPSTR pszStrEnd );
_odbdecl
odbBOOL       odbUseRowCache( odbHANDLE hCon, odbBOOL bUse, odbULONG ulSize );
_odbdecl
void          odbWinsockCleanup(void);
_odbdecl
odbBOOL       odbWinsockStartup(void);

#define odbFetchRowAbs( hQry, lRow ) odbFetchRowEx( (hQry), ODB_FETCH_ABS, (lRow) )
#define odbFetchRowBookmark( hQry, lRowOffset ) odbFetchRowEx( (hQry), ODB_FETCH_BOOKMARK, (lRowOffset) )
#define odbFetchRowFirst( hQry ) odbFetchRowEx( (hQry), ODB_FETCH_FIRST, 0 )
#define odbFetchRowLast( hQry ) odbFetchRowEx( (hQry), ODB_FETCH_LAST, 0 )
#define odbFetchRowNext( hQry ) odbFetchRowEx( (hQry), ODB_FETCH_NEXT, 0 )
#define odbFetchRowPrev( hQry ) odbFetchRowEx( (hQry), ODB_FETCH_PREV, 0 )
#define odbFetchRowRel( hQry, lRowOffset ) odbFetchRowEx( (hQry), ODB_FETCH_REL, (lRowOffset) )
#define odbFetchRowReread( hQry ) odbFetchRowEx( (hQry), ODB_FETCH_REREAD, 0 )

#define odbRowAdd( hQry ) odbDoRowOperation( (hQry), ODB_ROW_ADD )
#define odbRowBookmark( hQry ) odbDoRowOperation( (hQry), ODB_ROW_BOOKMARK )
#define odbRowDelete( hQry ) odbDoRowOperation( (hQry), ODB_ROW_DELETE )
#define odbRowLock( hQry ) odbDoRowOperation( (hQry), ODB_ROW_LOCK )
#define odbRowRefresh( hQry ) odbDoRowOperation( (hQry), ODB_ROW_REFRESH )
#define odbRowUnlock( hQry ) odbDoRowOperation( (hQry), ODB_ROW_UNLOCK )
#define odbRowUpdate( hQry ) odbDoRowOperation( (hQry), ODB_ROW_UPDATE )

#ifdef __cplusplus
}
#endif

#endif
