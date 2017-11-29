/* $Id: odb.h,v 1.6 2004/06/02 20:12:21 rtwitty Exp $ */
/*
    Odb - ODBC class library

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
#ifndef _ODB_H
#define _ODB_H

#ifndef ODB_BUILD
  #include <sql.h>
  #include <sqlext.h>
  #include <odbcss.h>
  #include "odbtypes.h"
#endif

#define ODB_ERR_MEMORY   (-9501)
#define ODB_ERR_COLNAME  (-9502)
#define ODB_ERR_UNALLOC  (-9503)
#define ODB_ERR_VALUE    (-9504)
#define ODB_ERR_APICALL  (-9505)

class OdbSrc;
class OdbHandle;
class OdbEnv;
class OdbCon;
class OdbQry;

typedef int (*OdbDiagHandler)( OdbHandle* pOdb, const char* szState,
                               SQLINTEGER nCode, const char* szText );
typedef int (*OdbDiagHandlerW)( OdbHandle* pOdb, const wchar_t* szState,
                                SQLINTEGER nCode, const wchar_t* szText );

/////////////////////////////////////////////////////////////////////////////
class OdbSrc
{
public:
    char szAppend[256];
    char szDATABASE[64];
    char szDBQ[256];
    char szDEFAULTDIR[128];
    char szDRIVER[64];
    char szDSN[64];
    char szFIL[32];
    char szFILEDSN[256];
    char szPWD[32];
    char szSAVEFILE[128];
    char szSERVER[48];
    char szUID[48];

public:
    OdbSrc();
    virtual ~OdbSrc();

    void Clear();
    void GetAppend( char* pszAppend );
    BOOL GetConnectString( char* pszConnect, int nBufLen );
    void GetDATABASE( char* pszDATABASE );
    void GetDBQ( char* pszDBQ );
    void GetDEFAULTDIR( char* pszDEFAULTDIR );
    void GetDRIVER( char* pszDRIVER );
    void GetDSN( char* pszDSN );
    void GetFIL( char* pszFIL );
    void GetFILEDSN( char* pszFILEDSN );
    void GetPWD( char* pszPWD );
    void GetSAVEFILE( char* pszSAVEFILE );
    void GetSERVER( char* pszSERVER );
    void GetUID( char* pszUID );
    void SetAppend( const char* pszAppend );
    void SetDATABASE( const char* pszDATABASE );
    void SetDBQ( const char* pszDBQ );
    void SetDEFAULTDIR( const char* pszDEFAULTDIR );
    void SetDRIVER( const char* pszDRIVER );
    void SetDSN( const char* pszDSN );
    void SetFIL( const char* pszFIL );
    void SetFILEDSN( const char* pszFILEDSN );
    void SetPWD( const char* pszPWD );
    void SetSAVEFILE( const char* pszSAVEFILE );
    void SetSERVER( const char* pszSERVER );
    void SetUID( const char* pszUID );
};

/////////////////////////////////////////////////////////////////////////////
class OdbHandle
{
public:
    OdbDiagHandler  m_dhErr;
    OdbDiagHandlerW m_dhErrW;
    OdbDiagHandler  m_dhMsg;
    OdbDiagHandlerW m_dhMsgW;

    SQLHANDLE      m_hODBC;
    SQLSMALLINT    m_nHandleType;
    SQLRETURN      m_nLastResult;
    OdbHandle*     m_pOdbParent;
    void*          m_pUserData;

public:
    OdbHandle( SQLSMALLINT nHandleType );
    virtual ~OdbHandle();

    BOOL  Allocate( OdbHandle* pOdbParent );
    BOOL  Allocate( OdbHandle& OdbParent );
    BOOL  Allocated();
    BOOL  Error();
    BOOL  Error( char* szErrorState, SQLINTEGER nErrorCode = 0 );
    BOOL  GetAttr( SQLINTEGER nAttr, char* pValue, SQLINTEGER nValLen, SQLINTEGER* pnActLen = NULL );
    BOOL  GetAttr( SQLINTEGER nAttr, SQLPOINTER* pValue );
    BOOL  GetAttr( SQLINTEGER nAttr, SQLINTEGER* pValue );
    BOOL  GetAttr( SQLINTEGER nAttr, SQLUINTEGER* pValue );
    void* GetUserData();
    BOOL  Info();
    BOOL  Info( char* szInfoState, SQLINTEGER nInfoCode = 0 );
    BOOL  NeedData();
    BOOL  NoData();
    BOOL  ProcessLastResult();
    BOOL  ProcessLastResultW();
    BOOL  SetAttr( SQLINTEGER nAttr, char* pValue, SQLINTEGER nValLen = SQL_NTS );
    BOOL  SetAttr( SQLINTEGER nAttr, SQLPOINTER Value );
    BOOL  SetAttr( SQLINTEGER nAttr, SQLINTEGER Value );
    BOOL  SetAttr( SQLINTEGER nAttr, SQLUINTEGER Value );
    void  SetErrDiagHandler( OdbDiagHandler DiagHandler );
    void  SetErrDiagHandlerW( OdbDiagHandlerW DiagHandler );
    void  SetMsgDiagHandler( OdbDiagHandler DiagHandler );
    void  SetMsgDiagHandlerW( OdbDiagHandlerW DiagHandler );
    void  SetUserData( void* pUserData );
    BOOL  StillExecuting();

    virtual void DisplayDiag( OdbDiagHandler DiagHandler = NULL );
    virtual void DisplayDiagW( OdbDiagHandlerW DiagHandler = NULL );
    virtual BOOL Free();
    virtual BOOL OnAllocate();

    static int DefaultDiagHandler( OdbHandle* pOdb, const char* szState,
                                   SQLINTEGER nCode, const char* szText );
    static int DefaultDiagHandlerW( OdbHandle* pOdb, const wchar_t* szState,
                                    SQLINTEGER nCode, const wchar_t* szText );
};

/////////////////////////////////////////////////////////////////////////////
class OdbEnv : public OdbHandle
{
public:
    OdbEnv( BOOL bAllocate = FALSE );
    virtual ~OdbEnv();

    BOOL Commit();
    BOOL GetSources( char* pszName, SQLSMALLINT nName, char* pszDriver, SQLSMALLINT nDriver, BOOL bFirst = FALSE );
    BOOL GetDrivers( char* pszName, SQLSMALLINT nName, char* pszAttrs, SQLSMALLINT nAttrs, BOOL bFirst = FALSE );
    BOOL OnAllocate();
    BOOL Rollback();
};

/////////////////////////////////////////////////////////////////////////////
class OdbCon : public OdbHandle
{
private:
    BOOL        m_bConnected;
    SQLUINTEGER m_nQueryTimeout;

public:
    OdbCon( OdbEnv* pEnv = NULL );
    OdbCon( OdbEnv& Env );
    virtual ~OdbCon();

    BOOL Commit();
    BOOL Connect( char* pszDSN, char* pszUID, char* pszPWD );
    BOOL Connect( char* pszInConnect,
                  char* pszOutConnect = NULL, SQLSMALLINT nOut = 0,
                  SQLSMALLINT nDriverCompletion = SQL_DRIVER_NOPROMPT,
                  HWND hWnd = NULL );
    BOOL Connect( OdbSrc* pSrc,
                  char* pszOutConnect = NULL, SQLSMALLINT nOut = 0,
                  SQLSMALLINT nDriverCompletion = SQL_DRIVER_NOPROMPT,
                  HWND hWnd = NULL );
    BOOL Connect( OdbSrc& pSrc,
                  char* pszOutConnect = NULL, SQLSMALLINT nOut = 0,
                  SQLSMALLINT nDriverCompletion = SQL_DRIVER_NOPROMPT,
                  HWND hWnd = NULL );
    BOOL Connected();
    BOOL DisableTrans();
    BOOL Disconnect();
    BOOL EnableAsyncMode( BOOL bEnable = TRUE );
    BOOL EnableManualCommit( BOOL bEnable = TRUE );
    BOOL GetInfo( SQLUSMALLINT nInfo, char* pValue, SQLSMALLINT nValLen, SQLSMALLINT* pnActLen = NULL );
    BOOL GetInfo( SQLUSMALLINT nInfo, SQLUSMALLINT* pValue );
    BOOL GetInfo( SQLUSMALLINT nInfo, SQLUINTEGER* pValue );
    BOOL Rollback();
    BOOL SetConnectTimeout( SQLUINTEGER nSeconds = 20 );
    BOOL SetQueryTimeout( SQLUINTEGER nSeconds = 0 );
    BOOL UseReadCommittedTrans();
    BOOL UseReadUncommittedTrans();
    BOOL UseRepeatableReadTrans();
    BOOL UseSerializableTrans();

    virtual BOOL Free();

    SQLUINTEGER GetQueryTimeout(){return m_nQueryTimeout;}
};

/////////////////////////////////////////////////////////////////////////////
class OdbQry : public OdbHandle
{
public:
    enum
    {
        odbConcurReadOnly = SQL_CONCUR_READ_ONLY,
        odbConcurLock = SQL_CONCUR_LOCK,
        odbConcurRowver = SQL_CONCUR_ROWVER,
        odbConcurValues = SQL_CONCUR_VALUES
    }
    odb_cursor_concurrencies;

public:
    SQLUSMALLINT  m_nRowStatus;
    SQLUSMALLINT* m_pnRowStatus;
    SQLUSMALLINT* m_pnRowOp;

public:
    OdbQry( OdbCon* pCon = NULL );
    OdbQry( OdbCon& Con );
    virtual ~OdbQry();

    BOOL AddRow();
    BOOL BindCol( SQLUSMALLINT nCol, SQLSMALLINT nCType, SQLPOINTER pBuffer,
                  SQLINTEGER nBufLen, SQLINTEGER* pnLenOrInd );
    BOOL BindCol( SQLUSMALLINT nCol, OdbDATA& Data, BOOL bAutoAlloc = TRUE );
    BOOL BindCol( const char* pszColName, OdbDATA& Data, BOOL bAutoAlloc = TRUE );
    BOOL BindInOutParam( SQLUSMALLINT nParam, OdbDATA& Data, BOOL bAutoAlloc = TRUE );
    BOOL BindInOutParam( SQLUSMALLINT nParam, OdbDATA& Data, SQLSMALLINT nDataType,
                         SQLUINTEGER nColSize = 0, SQLSMALLINT nDecDigits = 0 );
    BOOL BindInputParam( SQLUSMALLINT nParam, OdbDATA& Data, BOOL bAutoAlloc = TRUE );
    BOOL BindInputParam( SQLUSMALLINT nParam, OdbDATA& Data, SQLSMALLINT nDataType,
                         SQLUINTEGER nColSize = 0, SQLSMALLINT nDecDigits = 0 );
    BOOL BindOutputParam( SQLUSMALLINT nParam, OdbDATA& Data, BOOL bAutoAlloc = TRUE );
    BOOL BindOutputParam( SQLUSMALLINT nParam, OdbDATA& Data, SQLSMALLINT nDataType,
                          SQLUINTEGER nColSize = 0, SQLSMALLINT nDecDigits = 0 );
    BOOL BindParam( SQLUSMALLINT nParam, SQLSMALLINT nType, SQLSMALLINT nCType,
                    SQLSMALLINT nDataType, SQLUINTEGER nColSize, SQLSMALLINT nDecDigits,
                    SQLPOINTER pBuffer, SQLINTEGER nBufLen, SQLINTEGER* pnLenOrInd );
    BOOL BindParam( SQLUSMALLINT nParam, SQLSMALLINT nType,
                    OdbDATA& Data, BOOL bAutoAlloc = TRUE );
    BOOL Cancel();
    BOOL CloseCursor();
    BOOL DefineRowset( SQLUINTEGER nSize, SQLUINTEGER nStructSize );
    BOOL DeleteRow( int nRow = 1 );
    BOOL DumpResults();
    BOOL EnableAsyncMode( BOOL bEnable = TRUE );
    BOOL EnableBookmarks( BOOL bEnable = TRUE );
    BOOL Execute( char* pszSQL = NULL );
    BOOL ExecuteW( wchar_t* pszSQL = NULL );
    BOOL Fetch();
    BOOL FetchAbs( SQLINTEGER nRow );
    BOOL FetchFirst();
    BOOL FetchLast();
    BOOL FetchNext();
    BOOL FetchPrev();
    BOOL FetchRel( SQLINTEGER nRowOffset );
    BOOL FetchViaBookmark( SQLINTEGER nBookmarkRowOffset );
    BOOL GetAffectedRowCount( SQLINTEGER* pnRows );
    BOOL GetBookmarkSize( SQLUINTEGER* pnSize );
    BOOL GetColAttr( SQLSMALLINT nCol, SQLUSMALLINT nField,
                     SQLINTEGER* pValue );
    BOOL GetColAttr( SQLSMALLINT nCol, SQLUSMALLINT nField,
                     char* pValue, SQLSMALLINT nValLen,
                     SQLSMALLINT* pnActLen = NULL );
    BOOL GetColAttrW( SQLSMALLINT nCol, SQLUSMALLINT nField,
                      wchar_t* pValue, SQLSMALLINT nValLen,
                      SQLSMALLINT* pnActLen = NULL );
    BOOL GetColInfo( SQLSMALLINT nCol, char* pszName, SQLSMALLINT nNameLen,
                     SQLSMALLINT* pnDataType = NULL, SQLUINTEGER* pnColSize = NULL,
                     SQLSMALLINT* pnDecDigits = NULL,
                     SQLSMALLINT* pnNullable = NULL );
    BOOL GetColInfoW( SQLSMALLINT nCol, wchar_t* pszName, SQLSMALLINT nNameLen,
                      SQLSMALLINT* pnDataType = NULL, SQLUINTEGER* pnColSize = NULL,
                      SQLSMALLINT* pnDecDigits = NULL,
                      SQLSMALLINT* pnNullable = NULL );
    BOOL GetColNumber( const char* pszColName, SQLSMALLINT* pnCol );
    BOOL GetColNumberW( const wchar_t* pszColName, SQLSMALLINT* pnCol );
    BOOL GetColSize( SQLSMALLINT nCol, SQLUINTEGER* pnSize );
    BOOL GetColSize( const char* pszColName, SQLUINTEGER* pnSize );
    BOOL GetColSizeW( const wchar_t* pszColName, SQLUINTEGER* pnSize );
    BOOL GetColumns( char* pszTable );
    BOOL GetCursorRowCount( SQLINTEGER* pnRows );
    BOOL GetData( SQLUSMALLINT nCol, SQLSMALLINT nCType, SQLPOINTER pBuffer, SQLINTEGER nBufLen, SQLINTEGER* pnLenOrInd );
    BOOL GetData( SQLUSMALLINT nCol, OdbDATA& Data );
    BOOL GetParamInfo( SQLUSMALLINT nParam, SQLSMALLINT* pnDataType,
                       SQLUINTEGER* pnColSize, SQLSMALLINT* pnDecDigits,
                       SQLSMALLINT* pnNullable );
    BOOL GetNumParams( SQLSMALLINT* pnParams );
    BOOL GetNumResultCols( SQLSMALLINT* pnCols );
    BOOL GetParamData( SQLPOINTER* ppData );
    BOOL GetTables();
    BOOL GetTypeInfo( SQLSMALLINT nType = SQL_ALL_TYPES );
    void IgnoreRow( int nRow );
    BOOL LockRow( int nRow = 1 );
    BOOL MoreResults();
    BOOL OnAllocate();
    BOOL PositionRow( int nRow = 1 );
    BOOL Prepare( char* pszSQL );
    BOOL PrepareW( wchar_t* pszSQL );
    void ProcessRow( int nRow );
    BOOL PutData( SQLPOINTER pData, SQLINTEGER nLenOrInd );
    BOOL PutData( OdbDATA& Data );
    BOOL RefreshRow( int nRow = 1 );
    BOOL RowAdded( int nRow = 1 );
    BOOL RowDeleted( int nRow = 1 );
    BOOL RowError( int nRow = 1 );
    BOOL RowInfo( int nRow = 1 );
    BOOL RowNone( int nRow = 1 );
    BOOL RowSuccess( int nRow = 1 );
    BOOL RowUpdated( int nRow = 1 );
    BOOL SetBookmark( SQLPOINTER pBookmark );
    BOOL SetBookmark( OdbVARBOOKMARK& Bookmark );
    BOOL SetTimeout( SQLUINTEGER nSeconds = 0 );
    BOOL UnBindCols();
    BOOL UnBindParams();
    BOOL UnlockRow( int nRow = 1 );
    BOOL UpdateRow( int nRow = 1 );
    BOOL UseDynamicCursor( SQLUINTEGER nConcurrency = odbConcurRowver );
    BOOL UseForwardCursor( SQLUINTEGER nConcurrency = odbConcurReadOnly );
    BOOL UseKeysetCursor( SQLUINTEGER nConcurrency = odbConcurRowver );
    BOOL UseStaticCursor( SQLUINTEGER nConcurrency = odbConcurReadOnly );

    virtual BOOL Free();

private:
    BOOL      ExecApiCall( SQLCHAR* pszApiCall );
    BOOL      ExecApiCallW( SQLWCHAR* pszApiCall );
    SQLCHAR*  GetApiParam( SQLCHAR** ppszParams, BOOL bNumeric = FALSE );
    SQLWCHAR* GetApiParamW( SQLWCHAR** ppszParams, BOOL bNumeric = FALSE );
};

#endif
