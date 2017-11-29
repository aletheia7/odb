/* $Id: OdbSupport.h,v 1.25 2005/12/22 04:44:42 rtwitty Exp $ */
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
#ifndef _ODBSUPPORT_H
#define _ODBSUPPORT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\odb\odb.h"

#define MAX_CONNSTR_SIZE 512

/* ODB Data Types */
#define ODB_BINARY    SQL_C_BINARY
#define ODB_BIGINT    SQL_C_SBIGINT
#define ODB_UBIGINT   SQL_C_UBIGINT
#define ODB_BIT       SQL_C_BIT
#define ODB_CHAR      SQL_C_CHAR
#define ODB_DATE      SQL_C_TYPE_DATE
#define ODB_DATETIME  SQL_C_TYPE_TIMESTAMP
#define ODB_DOUBLE    SQL_C_DOUBLE
#define ODB_GUID      SQL_C_GUID
#define ODB_INT       SQL_C_SLONG
#define ODB_UINT      SQL_C_ULONG
#define ODB_NUMERIC   SQL_C_NUMERIC
#define ODB_REAL      SQL_C_FLOAT
#define ODB_SMALLINT  SQL_C_SSHORT
#define ODB_USMALLINT SQL_C_USHORT
#define ODB_TIME      SQL_C_TYPE_TIME
#define ODB_TINYINT   SQL_C_STINYINT
#define ODB_UTINYINT  SQL_C_UTINYINT
#define ODB_WCHAR     SQL_C_WCHAR

/* ODB Connection Types */
#define ODB_CON_NORMAL   0
#define ODB_CON_RESERVED 1
#define ODB_CON_SINGLE   2

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

class COdbtpQry;
class COdbProc;

class COdbtpCon : public OdbCon
{
public:
    COdbtpCon();
    virtual ~COdbtpCon();

    BOOL             m_bInUse;
    BOOL             m_bSentId;
    int              m_nMaxProcs;
    int              m_nPoolIndex;
    int              m_nTotalProcs;
    COdbProc**       m_ppProcs;
    COdbtpQry**      m_ppQrys;
    char             m_szId[MAX_CONNSTR_SIZE];
    unsigned __int64 m_u64ReleaseTime;
    USHORT           m_usType;

// Attributes
    ULONG m_ulAttrCacheProcs;
    ULONG m_ulAttrDescribeParams;
    ULONG m_ulAttrDriver;
    ULONG m_ulAttrFetchRowCount;
    ULONG m_ulAttrFullColInfo;
    ULONG m_ulAttrMapCharToWChar;
    ULONG m_ulAttrQueryTimeout;
    ULONG m_ulAttrRightTrimText;
    ULONG m_ulAttrTransactions;
    ULONG m_ulAttrUnicodeSQL;
    ULONG m_ulAttrVarDataSize;

// Global Data
    static BOOL    m_bBadConnCheck;
    static BOOL    m_bEnableProcCache;
    static int     m_nMaxQrys;
    static OdbEnv* m_pEnv;
    static ULONG   m_ulConnectTimeout;
    static ULONG   m_ulFetchRowCount;
    static ULONG   m_ulQueryTimeout;

    BOOL       AddProc( COdbProc* pProc );
    void       DeleteQrys();
    BOOL       DetachQrys();
    BOOL       Drop();
    void       FreeProcCache();
    BOOL       GetAttribute( COdbtpSock* pSock );
    int        GetPoolIndex(){ return m_nPoolIndex; }
    COdbProc*  GetProc( char* pszCatalog, char* pszSchema,
                        char* pszProcedure );
    COdbProc*  GetProcW( wchar_t* pszCatalog, wchar_t* pszSchema,
                         wchar_t* pszProcedure );
    COdbtpQry* GetQry( int nId );
    BOOL       Init( USHORT usType, int nPoolIndex );
    BOOL       IsGood();
    BOOL       PostConnectInit();
    BOOL       Release();
    BOOL       ResetAttributes();
    BOOL       SendId( COdbtpSock* pSock );
    BOOL       SetAttribute( COdbtpSock* pSock );
    BOOL       SetTimeoutForQrys( ULONG ulTimeout );

    virtual void DisplayDiag( OdbDiagHandler DiagHandler = NULL );
    virtual void DisplayDiagW( OdbDiagHandlerW DiagHandler = NULL );
};

class COdbData
{
public:
    COdbData();
    virtual ~COdbData();

    BOOL     m_bBound;
    OdbDATA* m_pData;
    wchar_t* m_pszNameW;
    short    m_sDecDigits;
    short    m_sDefaultOdbType;
    short    m_sNameLen;
    short    m_sNullable;
    short    m_sOdbType;
    short    m_sSqlType;
    char     m_szName[132];
    ULONG    m_ulColSize;

    BOOL Alloc( short sOdbType, long lLen, ULONG ulVarDataSize );

    operator OdbDATA*() { return( m_pData ); }
};

class COdbColData : public COdbData
{
public:
    COdbColData();
    virtual ~COdbColData();
};

class COdbParamData : public COdbData
{
public:
    COdbParamData();
    virtual ~COdbParamData();

    BOOL     m_bDescribed;
    short    m_sTypeLen;
    USHORT   m_usType;
    char     m_szType[132];
    wchar_t* m_pszTypeW;
};

class COdbtpQry : public OdbQry
{
public:
    COdbtpQry();
    virtual ~COdbtpQry();

    BOOL           m_bBookmarksOn;
    BOOL           m_bBoundCols;
    BOOL           m_bBoundParams;
    BOOL           m_bCheckColBinds;
    BOOL           m_bGotResult;
    BOOL           m_bGotRow;
    BOOL           m_bPreparedProc;
    BOOL           m_bSentInfo;
    long           m_lAffectedRows;
    COdbColData*   m_pColData;
    COdbtpCon*     m_pCon;
    COdbParamData* m_pParamData;
    short          m_sMaxCols;
    short          m_sMaxParams;
    short          m_sTotalCols;
    short          m_sTotalParams;
    ULONG          m_ulFetchRowCount;
    USHORT         m_usCursorConcur;
    USHORT         m_usCursorType;
    OdbVARBOOKMARK m_Bookmark;
    OdbVARBOOKMARK m_RowBookmark;

    BOOL   Attach( COdbtpCon* pCon );
    BOOL   BindColData( USHORT usCol, short sOdbType, long lLen = 0 );
    BOOL   BindColData( COdbtpSock* pSock );
    BOOL   BindParamData( COdbtpSock* pSock );
    BOOL   CanGetRow(){ return m_sTotalCols != 0; }
    void   ClearError();
    BOOL   Detach();
    BOOL   DoRowOperation( COdbtpSock* pSock );
    BOOL   GetColInfo();
    BOOL   GetColInfoW();
    USHORT GetCursorType(){ return m_usCursorType; }
    ULONG  GetFetchRowCount(){ return m_ulFetchRowCount; }
    BOOL   GetNextResult();
    short  GetOdbDataType( short sSqlDataType );
    short  GetOdbParamType( short sSqlParamType );
    BOOL   GetParamData( COdbtpSock* pSock );
    BOOL   GetParamInfo();
    BOOL   GetResultInfo();
    BOOL   GetRow( USHORT usFetchType, long lFetchParam );
    BOOL   GotResult(){ return m_bGotResult; }
    BOOL   GotRow(){ return m_bGotRow; }
    BOOL   PrepareProc( COdbtpSock* pSock, PSTR pszCatalog,
                        PSTR pszSchema, PSTR pszProcedure );
    BOOL   PrepareProcW( COdbtpSock* pSock, wchar_t* pszCatalog,
                         wchar_t* pszSchema, wchar_t* pszProcedure );
    BOOL   SendAffectedRowCount( COdbtpSock* pSock );
    BOOL   SendColInfo( COdbtpSock* pSock );
    BOOL   SendColInfoW( COdbtpSock* pSock );
    BOOL   SendColInfoEx( COdbtpSock* pSock, short sCol );
    BOOL   SendColInfoExW( COdbtpSock* pSock, short sCol );
    BOOL   SendCursor( COdbtpSock* pSock );
    BOOL   SendParamInfo( COdbtpSock* pSock );
    BOOL   SendParamInfoEx( COdbtpSock* pSock );
    BOOL   SendResultInfo( COdbtpSock* pSock, OdbDiagHandler dhSendInfo );
    BOOL   SendResultInfoW( COdbtpSock* pSock, OdbDiagHandlerW dhSendInfo );
    BOOL   SendRow( COdbtpSock* pSock, BOOL bFinal = TRUE );
    BOOL   SendWCharData( COdbtpSock* pSock, USHORT usCode, OdbWCHAR* pData );
    BOOL   SetColData( COdbtpSock* pSock );
    BOOL   SetColInfoFlags( short sCol, PULONG pulFlags );
    BOOL   SetCursor( COdbtpSock* pSock );
    BOOL   SetParamData( COdbtpSock* pSock );
    BOOL   SqlExec( PSTR pszSQL = NULL );
    BOOL   SqlPrep( PSTR pszSQL );
    BOOL   UnBindColData();
    BOOL   UnBindParamData();

    virtual void DisplayDiag( OdbDiagHandler DiagHandler = NULL );
    virtual void DisplayDiagW( OdbDiagHandlerW DiagHandler = NULL );
};

typedef struct
{
    short    m_sDecDigits;
    short    m_sDefaultOdbType;
    short    m_sNameLen;
    short    m_sNullable;
    short    m_sSqlType;
    short    m_sTypeLen;
    wchar_t* m_pszNameW;
    wchar_t* m_pszTypeW;
    char     m_szName[132];
    char     m_szType[132];
    ULONG    m_ulColSize;
    USHORT   m_usType;
}
SProcParam;

class COdbProc
{
public:
    COdbProc();
    virtual ~COdbProc();

    BOOL        m_bCached;
    BOOL        m_bUnicode;
    int         m_nMaxParams;
    int         m_nTotalParams;
    SProcParam* m_pParams;
    wchar_t*    m_pszCatalogW;
    wchar_t*    m_pszProcedureW;
    wchar_t*    m_pszSchemaW;
    char        m_szCatalog[164];
    char        m_szSchema[164];
    char        m_szProcedure[164];

    BOOL        Init( char* pszCatalog, char* pszSchema,
                      char* pszProcedure );
    BOOL        InitW( wchar_t* pszCatalog, wchar_t* pszSchema,
                       wchar_t* pszProcedure );
    SProcParam* NewParam();
};

class COdbtpConPool
{
public:
    COdbtpConPool();
    virtual ~COdbtpConPool();

    USHORT        m_usType;
    int           m_nTotalCons;
    COdbtpCon**   m_ppCons;
    CThreadAccess m_taPool;

    void       CheckTimeouts( ULONG ulTimeout );
    COdbtpCon* Connect( COdbtpSock* pSock, PSTR pszDBConnect );
    BOOL       Disconnect( COdbtpCon* pCon, BOOL bDrop );
    void       Free();
    COdbtpCon* GetCon( PCSTR pszId );
    BOOL       Init( int nTotalCons, USHORT usType );
    COdbtpCon* ReInit( COdbtpCon* pCon );
};

#endif
