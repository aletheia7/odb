/*
   +----------------------------------------------------------------------+
   | This library is free software; you can redistribute it and/or        |
   | modify it under the terms of the GNU Lesser General Public           |
   | License as published by the Free Software Foundation; either         |
   | version 2.1 of the License, or (at your option) any later version.   |
   |                                                                      |
   | This library is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    |
   | Lesser General Public License for more details.                      |
   |                                                                      |
   | You should have received a copy of the GNU Lesser General Public     |
   | License in the file LICENSE along with this library;                 |
   | if not, write to the                                                 |
   |                                                                      |
   |   Free Software Foundation, Inc.,                                    |
   |   59 Temple Place, Suite 330,                                        |
   |   Boston, MA  02111-1307  USA                                        |
   +----------------------------------------------------------------------+
   | Author: Robert E. Twitty <rtwitty@users.sourceforge.net>             |
   +----------------------------------------------------------------------+
*/
/* $Id: php_odbtp.c,v 1.8 2005/12/22 04:54:00 rtwitty Exp $ */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if HAVE_ODBTP

#include <odbtp.h>
#include <time.h>

#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_odbtp.h"

/* #define ODBTP_MSSQL 1 */

#define PHP_ODB_IS_HEX(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F'))

#define PHP_ODB_LERROR_STRING 1
#define PHP_ODB_LERROR_STATE  2
#define PHP_ODB_LERROR_CODE   3

#define PHP_ODB_OPTION_CONVERT_ALL      1
#define PHP_ODB_OPTION_CONVERT_DATETIME 2
#define PHP_ODB_OPTION_CONVERT_GUID     3
#define PHP_ODB_OPTION_DONT_POOL_DBC    4

#define PHP_ODB_QUERY_EXECUTE      1
#define PHP_ODB_QUERY_PREPARE      2
#define PHP_ODB_QUERY_PREPARE_PROC 3
#define PHP_ODB_QUERY_SELECT_DB    4

#define PHP_ODB_INFO_ID             1
#define PHP_ODB_INFO_VERSION        2
#define PHP_ODB_INFO_ROW_CACHE_SIZE 3
#define PHP_ODB_INFO_TOTALCOLS      4
#define PHP_ODB_INFO_TOTALPARAMS    5
#define PHP_ODB_INFO_TOTALROWS      6
#define PHP_ODB_INFO_ROWCOUNT       7
#define PHP_ODB_INFO_ROWSTATUS      8
#define PHP_ODB_INFO_DETACHED       9
#define PHP_ODB_INFO_ALL            10
#define PHP_ODB_INFO_NAME           11
#define PHP_ODB_INFO_BINDTYPE       12
#define PHP_ODB_INFO_TYPENAME       13
#define PHP_ODB_INFO_LENGTH         14
#define PHP_ODB_INFO_TABLE          15
#define PHP_ODB_INFO_SCHEMA         16
#define PHP_ODB_INFO_CATALOG        17
#define PHP_ODB_INFO_FLAGS          18
#define PHP_ODB_INFO_BASENAME       19
#define PHP_ODB_INFO_NUMBER         20
#define PHP_ODB_INFO_PARAMTYPE      21

#define PHP_ODB_FETCH_NONE   0
#define PHP_ODB_FETCH_ROW    1
#define PHP_ODB_FETCH_ASSOC  2
#define PHP_ODB_FETCH_ARRAY  3
#define PHP_ODB_FETCH_OBJECT 4

#define PHP_ODB_SEEK_DATA   1
#define PHP_ODB_SEEK_FIELD  2
#define PHP_ODB_SEEK_RESULT 3

#if ODBTP_MSSQL /* mssql specific defines */
#define MSSQL_FETCH 0x10000000
#define MSSQL_ASSOC (MSSQL_FETCH | 1)
#define MSSQL_NUM   (MSSQL_FETCH | 2)
#define MSSQL_BOTH  (MSSQL_ASSOC | MSSQL_NUM)
#endif

typedef struct
{
    odbHANDLE hOdb;
}
odbtp_handle;

typedef struct
{
    odbHANDLE hQry;
    zend_bool attached_cols;
    zend_bool attached_params;
    long      link;
    long      col_pos;
}
odbtp_query;

typedef struct
{
    odbHANDLE    hCon;
    odbtp_query* default_qry;
    char*        last_error;
    zend_bool    dont_pool_dbc;
}
odbtp_connection;

static char odbHexTable[] = "0123456789ABCDEF";

/* True global resources - no need for thread safety here */
static int le_connection;
#define le_connection_name "ODBTP Connection"
static int le_query;
#define le_query_name "ODBTP Query"

ZEND_DECLARE_MODULE_GLOBALS(odbtp)

#ifdef ZEND_BEGIN_ARG_INFO
ZEND_BEGIN_ARG_INFO(a3_arg_force_ref, 0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();
#else
static unsigned char a3_arg_force_ref[] = { 3, BYREF_NONE, BYREF_NONE, BYREF_FORCE };
#endif

/* {{{ odbtp_functions[]
 *
 * Every user visible function must have an entry in odbtp_functions[].
 */
zend_function_entry odbtp_functions[] = {
	PHP_FE(odbtp_connect,          NULL)
	PHP_FE(odbtp_rconnect,         NULL)
	PHP_FE(odbtp_sconnect,         NULL)
	PHP_FE(odbtp_version,          NULL)
	PHP_FE(odbtp_close,            NULL)
	PHP_FE(odbtp_connect_id,       NULL)
	PHP_FE(odbtp_load_data_types,  NULL)
	PHP_FE(odbtp_use_row_cache,    NULL)
	PHP_FE(odbtp_row_cache_size,   NULL)
	PHP_FE(odbtp_convert_all,      NULL)
	PHP_FE(odbtp_convert_datetime, NULL)
	PHP_FE(odbtp_convert_guid,     NULL)
	PHP_FE(odbtp_dont_pool_dbc,    NULL)
	PHP_FE(odbtp_get_attr,         NULL)
	PHP_FE(odbtp_set_attr,         NULL)
	PHP_FE(odbtp_commit,           NULL)
	PHP_FE(odbtp_rollback,         NULL)
	PHP_FE(odbtp_get_message,      NULL)
	PHP_FE(odbtp_get_error,        NULL)
	PHP_FE(odbtp_last_error,       NULL)
	PHP_FE(odbtp_last_error_state, NULL)
	PHP_FE(odbtp_last_error_code,  NULL)
	PHP_FE(odbtp_allocate_query,   NULL)
	PHP_FE(odbtp_query_id,         NULL)
	PHP_FE(odbtp_get_query,        NULL)
	PHP_FE(odbtp_set_cursor,       NULL)
	PHP_FE(odbtp_query,            NULL)
	PHP_FE(odbtp_prepare,          NULL)
	PHP_FE(odbtp_prepare_proc,     NULL)
	PHP_FE(odbtp_execute,          NULL)
	PHP_FE(odbtp_num_params,       NULL)
	PHP_FE(odbtp_attach_param,     a3_arg_force_ref)
	PHP_FE(odbtp_type_param,       NULL)
	PHP_FE(odbtp_param_name,       NULL)
	PHP_FE(odbtp_param_bindtype,   NULL)
	PHP_FE(odbtp_param_type,       NULL)
	PHP_FE(odbtp_param_length,     NULL)
	PHP_FE(odbtp_param_number,     NULL)
	PHP_FE(odbtp_input,            NULL)
	PHP_FE(odbtp_output,           NULL)
	PHP_FE(odbtp_inout,            NULL)
	PHP_FE(odbtp_set,              NULL)
	PHP_FE(odbtp_get,              NULL)
	PHP_FE(odbtp_num_fields,       NULL)
	PHP_FE(odbtp_bind_field,       NULL)
	PHP_FE(odbtp_attach_field,     a3_arg_force_ref)
	PHP_FE(odbtp_fetch_field,      NULL)
	PHP_FE(odbtp_field_name,       NULL)
	PHP_FE(odbtp_field_bindtype,   NULL)
	PHP_FE(odbtp_field_type,       NULL)
	PHP_FE(odbtp_field_length,     NULL)
	PHP_FE(odbtp_field_table,      NULL)
	PHP_FE(odbtp_field_schema,     NULL)
	PHP_FE(odbtp_field_catalog,    NULL)
	PHP_FE(odbtp_field_flags,      NULL)
	PHP_FE(odbtp_field_basename,   NULL)
	PHP_FE(odbtp_num_rows,         NULL)
	PHP_FE(odbtp_fetch,            NULL)
	PHP_FE(odbtp_fetch_row,        NULL)
	PHP_FE(odbtp_fetch_assoc,      NULL)
	PHP_FE(odbtp_fetch_array,      NULL)
	PHP_FE(odbtp_fetch_object,     NULL)
	PHP_FE(odbtp_field,            NULL)
	PHP_FE(odbtp_field_ignore,     NULL)
	PHP_FE(odbtp_row_refresh,      NULL)
	PHP_FE(odbtp_row_update,       NULL)
	PHP_FE(odbtp_row_delete,       NULL)
	PHP_FE(odbtp_row_add,          NULL)
	PHP_FE(odbtp_row_lock,         NULL)
	PHP_FE(odbtp_row_unlock,       NULL)
	PHP_FE(odbtp_row_bookmark,     NULL)
	PHP_FE(odbtp_row_status,       NULL)
	PHP_FE(odbtp_affected_rows,    NULL)
	PHP_FE(odbtp_fetch_output,     NULL)
	PHP_FE(odbtp_next_result,      NULL)
	PHP_FE(odbtp_free_query,       NULL)
	PHP_FE(odbtp_new_datetime,     NULL)
	PHP_FE(odbtp_datetime2ctime,   NULL)
	PHP_FE(odbtp_ctime2datetime,   NULL)
	PHP_FE(odbtp_detach,           NULL)
	PHP_FE(odbtp_is_detached,      NULL)
	PHP_FE(odbtp_data_seek,        NULL)
	PHP_FE(odbtp_field_seek,       NULL)
	PHP_FE(odbtp_result,           NULL)
	PHP_FE(odbtp_fetch_batch,      NULL)
	PHP_FE(odbtp_select_db,        NULL)
	PHP_FE(odbtp_bind,             a3_arg_force_ref)
	PHP_FE(odbtp_guid_string,      NULL)
	PHP_FE(odbtp_flags,            NULL)
	PHP_FE(odbtp_void_func,        NULL)

/* These aliases are included to facilitate porting from mssql extension */

    PHP_FALIAS(odbtp_pconnect,             odbtp_connect,        NULL)
    PHP_FALIAS(odbtp_get_last_message,     odbtp_last_error,     NULL)
    PHP_FALIAS(odbtp_init,                 odbtp_prepare_proc,   NULL)
    PHP_FALIAS(odbtp_rows_affected,        odbtp_affected_rows,  NULL)
    PHP_FALIAS(odbtp_free_result,          odbtp_free_query,     NULL)
    PHP_FALIAS(odbtp_free_statement,       odbtp_free_query,     NULL)
    PHP_FALIAS(odbtp_min_error_severity,   odbtp_void_func,      NULL)
    PHP_FALIAS(odbtp_min_message_severity, odbtp_void_func,      NULL)

#if ODBTP_MSSQL /* Include support for mssql extension */

    PHP_FALIAS(mssql_connect,              odbtp_connect,          NULL)
    PHP_FALIAS(mssql_close,                odbtp_close,            NULL)
    PHP_FALIAS(mssql_get_last_message,     odbtp_last_error,       NULL)
    PHP_FALIAS(mssql_query,                odbtp_query,            NULL)
    PHP_FALIAS(mssql_init,                 odbtp_prepare_proc,     NULL)
    PHP_FALIAS(mssql_bind,                 odbtp_bind,             a3_arg_force_ref)
    PHP_FALIAS(mssql_execute,              odbtp_execute,          NULL)
    PHP_FALIAS(mssql_num_fields,           odbtp_num_fields,       NULL)
    PHP_FALIAS(mssql_fetch_field,          odbtp_fetch_field,      NULL)
    PHP_FALIAS(mssql_field_name,           odbtp_field_name,       NULL)
    PHP_FALIAS(mssql_field_type,           odbtp_field_type,       NULL)
    PHP_FALIAS(mssql_field_length,         odbtp_field_length,     NULL)
    PHP_FALIAS(mssql_num_rows,             odbtp_num_rows,         NULL)
    PHP_FALIAS(mssql_fetch_row,            odbtp_fetch_row,        NULL)
    PHP_FALIAS(mssql_fetch_assoc,          odbtp_fetch_assoc,      NULL)
    PHP_FALIAS(mssql_fetch_array,          odbtp_fetch_array,      NULL)
    PHP_FALIAS(mssql_fetch_object,         odbtp_fetch_object,     NULL)
    PHP_FALIAS(mssql_affected_rows,        odbtp_affected_rows,    NULL)
    PHP_FALIAS(mssql_next_result,          odbtp_next_result,      NULL)
    PHP_FALIAS(mssql_data_seek,            odbtp_data_seek,        NULL)
    PHP_FALIAS(mssql_field_seek,           odbtp_field_seek,       NULL)
    PHP_FALIAS(mssql_result,               odbtp_result,           NULL)
    PHP_FALIAS(mssql_fetch_batch,          odbtp_fetch_batch,      NULL)
    PHP_FALIAS(mssql_select_db,            odbtp_select_db,        NULL)
    PHP_FALIAS(mssql_guid_string,          odbtp_guid_string,      NULL)
    PHP_FALIAS(mssql_pconnect,             odbtp_connect,          NULL)
    PHP_FALIAS(mssql_rows_affected,        odbtp_affected_rows,    NULL)
    PHP_FALIAS(mssql_free_result,          odbtp_free_query,       NULL)
    PHP_FALIAS(mssql_free_statement,       odbtp_free_query,       NULL)
    PHP_FALIAS(mssql_min_error_severity,   odbtp_void_func,        NULL)
    PHP_FALIAS(mssql_min_message_severity, odbtp_void_func,        NULL)

#endif

	{NULL, NULL, NULL}	/* Must be the last line in odbtp_functions[] */
};
/* }}} */

/* {{{ odbtp_module_entry
 */
zend_module_entry odbtp_module_entry = {
	STANDARD_MODULE_HEADER,
	"odbtp",
	odbtp_functions,
	PHP_MINIT(odbtp),
	PHP_MSHUTDOWN(odbtp),
    PHP_RINIT(odbtp),
    PHP_RSHUTDOWN(odbtp),
	PHP_MINFO(odbtp),
    NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_ODBTP
ZEND_GET_MODULE(odbtp)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("odbtp.interface_file", "/usr/local/share/odbtp.conf", PHP_INI_ALL, OnUpdateString, interface_file, zend_odbtp_globals, odbtp_globals)
    STD_PHP_INI_ENTRY("odbtp.datetime_format", "object", PHP_INI_ALL, OnUpdateString, datetime_format, zend_odbtp_globals, odbtp_globals)
    STD_PHP_INI_ENTRY("odbtp.guid_format", "string", PHP_INI_ALL, OnUpdateString, guid_format, zend_odbtp_globals, odbtp_globals)
    STD_PHP_INI_BOOLEAN("odbtp.detach_default_queries", "0", PHP_INI_ALL, OnUpdateBool, detach_default_queries, zend_odbtp_globals, odbtp_globals)
    STD_PHP_INI_BOOLEAN("odbtp.truncation_errors", "1", PHP_INI_ALL, OnUpdateBool, truncation_errors, zend_odbtp_globals, odbtp_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_odbtp_init_globals
 */
static void php_odbtp_init_globals(zend_odbtp_globals *odbtp_globals)
{
	odbtp_globals->default_port = 2799;
	odbtp_globals->interface_file = NULL;
	odbtp_globals->datetime_format = NULL;
	odbtp_globals->guid_format = NULL;
	odbtp_globals->detach_default_queries = FALSE;
	odbtp_globals->truncation_errors = TRUE;
}
/* }}} */

/* {{{ php_odbtp_set_default_link
 */
static void php_odbtp_set_default_link(int id TSRMLS_DC)
{
    if( ODBTP_G(default_link) != -1 )
        zend_list_delete(ODBTP_G(default_link));
    ODBTP_G(default_link) = id;
    zend_list_addref(id);
}
/* }}} */

/* {{{ free_odbtp_connection
 */
static void free_odbtp_connection(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	odbtp_connection* con = (odbtp_connection*)rsrc->ptr;
	odbHANDLE         hCon = con->hCon;

    if( hCon ) {
        odbHANDLE hQry;
        for( hQry = odbGetFirstQuery( hCon ); hQry; hQry = odbGetNextQuery( hCon ) )
            ((odbtp_query*)odbGetUserData( hQry ))->hQry = NULL;
        if( odbIsConnected( hCon ) ) odbLogout( hCon, con->dont_pool_dbc );
	    odbFree( hCon );
    }
    if( con->last_error ) efree( con->last_error );
    efree( con );
}
/* }}} */

/* {{{ odbtp_unattach_cols
 */
static void odbtp_unattach_cols(odbtp_query* qry TSRMLS_DC)
{
	odbHANDLE hQry;

    if( qry->attached_cols && (hQry = qry->hQry) ) {
        odbUSHORT usCol;
        odbUSHORT usTotalCols = odbGetTotalCols( hQry );
        zval*     zVar;

        for( usCol = 1; usCol <- usTotalCols; usCol++ ) {
            if( !(zVar = (zval*)odbColUserData( hQry, usCol )) ) continue;
            zval_ptr_dtor( &zVar );
            odbSetColUserData( hQry, usCol, NULL );
        }
        qry->attached_cols = FALSE;
    }
}
/* }}} */

/* {{{ odbtp_unattach_params
 */
static void odbtp_unattach_params(odbtp_query* qry TSRMLS_DC)
{
	odbHANDLE hQry;

    if( qry->attached_params && (hQry = qry->hQry) ) {
        odbUSHORT usParam;
        odbUSHORT usTotalParams = odbGetTotalParams( hQry );
        zval*     zVar;

        for( usParam = 1; usParam <- usTotalParams; usParam++ ) {
            if( !(zVar = (zval*)odbParamUserData( hQry, usParam )) ) continue;
            zval_ptr_dtor( &zVar );
            odbSetParamUserData( hQry, usParam, NULL );
        }
        qry->attached_params = FALSE;
    }
}
/* }}} */

/* {{{ free_odbtp_query
 */
static void free_odbtp_query(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
    odbtp_query* qry = (odbtp_query*)rsrc->ptr;
	odbHANDLE    hQry = qry->hQry;

    if( hQry ) {
        odbHANDLE hCon = odbGetConnection( hQry );
        if( hCon ) {
            odbtp_connection* con = (odbtp_connection*)odbGetUserData( hCon );
            if( qry == con->default_qry ) con->default_qry = NULL;
        }
        odbtp_unattach_cols(qry TSRMLS_CC);
        odbtp_unattach_params(qry TSRMLS_CC);
        odbFree( hQry );
    }
    efree( qry );
}
/* }}} */

/* {{{ set_odbtp_last_error
 */
static void set_odbtp_last_error(odbHANDLE hOdb, const char* message TSRMLS_DC)
{
    if( ODBTP_G(last_error) ) efree( ODBTP_G(last_error) );
    ODBTP_G(last_error) = estrdup( message );

    if( hOdb ) {
        odbHANDLE hCon;

        hCon = odbIsConnection( hOdb ) ? hOdb : odbGetConnection( hOdb );

        if( hCon ) {
            odbtp_connection* con = (odbtp_connection*)odbGetUserData( hCon );
            if( con ) {
                if( con->last_error ) efree( con->last_error );
                con->last_error = estrdup( message );
            }
        }
    }
}
/* }}} */

/* {{{ php_odbtp_error
 */
static void php_odbtp_error(odbHANDLE hOdb, const char* message TSRMLS_DC)
{
    char sz[256];

    if( *message != '[' ) {
        sprintf( sz, "[ODBTPERR][0]%s", message );
        message = sz;
    }
    php_error( E_WARNING, "%s", message );
    set_odbtp_last_error(hOdb, message TSRMLS_CC);
}
/* }}} */

/* {{{ php_odbtp_invalid
 */
static void php_odbtp_invalid(odbHANDLE hOdb, const char* message TSRMLS_DC)
{
    char sz[256];

    sprintf( sz, "[ODBTPINV][0]%s", message );
    php_error( E_WARNING, "%s", sz );
    set_odbtp_last_error(hOdb, sz TSRMLS_CC);
}
/* }}} */

/* {{{ process_odbtp_error
 */
static void process_odbtp_error(odbHANDLE hOdb TSRMLS_DC)
{
    const char* psz;
    char        sz[256];

    switch( odbGetError( hOdb ) ) {
        case ODBTPERR_CONNECT:
        case ODBTPERR_READ:
        case ODBTPERR_SEND:
            sprintf( sz, "%s (%s)",
                     odbGetErrorText( hOdb ), odbGetSockErrorText( hOdb ) );
            psz = sz;
            break;

        default:
            psz = odbGetErrorText( hOdb );
    }
    php_odbtp_error(hOdb, psz TSRMLS_CC);
}
/* }}} */

/* {{{ process_odbtp_invalid
 */
static void process_odbtp_invalid(const char* resource_name TSRMLS_DC)
{
    char sz[128];
    sprintf( sz, "Invalid %s resource", resource_name );
    php_odbtp_invalid(NULL, sz TSRMLS_CC);
}
/* }}} */

/* {{{ process_odbtp_truncation
 */
static void process_odbtp_truncation(odbHANDLE hQry, odbUSHORT usItem, odbBOOL bField TSRMLS_DC)
{
    char sz[80];

    if( bField ) {
        sprintf( sz, "Field %d was truncated. Actual size is %d",
                 usItem - 1, odbColActualLen( hQry, usItem ) );
    }
    else {
        sprintf( sz, "Parameter %d was truncated. Actual size is %d",
                 usItem, odbParamActualLen( hQry, usItem ) );
    }
    php_odbtp_error(hQry, sz TSRMLS_CC);
}
/* }}} */

/* {{{ odbtp_read_zval_boolean
 */
static zend_bool odbtp_read_zval_boolean(zval* zVal TSRMLS_DC)
{
    if( Z_TYPE_P(zVal) != IS_BOOL ) {
        char* psz;

        switch( Z_TYPE_P(zVal) ) {
            case IS_LONG:
                return( Z_LVAL_P(zVal) != 0 ? TRUE : FALSE );

            case IS_DOUBLE:
                return( Z_DVAL_P(zVal) != 0.0 ? TRUE : FALSE );

            case IS_STRING:
                for( psz = Z_STRVAL_P(zVal); *psz; psz++ )
                    if( (odbBYTE)*psz > ' ' && *psz != '0' && *psz != '-' )
                        break;

                switch( *psz ) {
                    case 't':
                    case 'T':
                    case 'y':
                    case 'Y': return( TRUE );

                    default: if( *psz >= '1' && *psz <= '9' ) return( TRUE );
                }

            default:
                return( FALSE );
        }
    }
    return( Z_BVAL_P(zVal) );
}
/* }}} */

/* {{{ odbtp_read_zval_double
 */
static double odbtp_read_zval_double(zval* zVal TSRMLS_DC)
{
    if( Z_TYPE_P(zVal) != IS_DOUBLE ) {
        switch( Z_TYPE_P(zVal) ) {
            case IS_BOOL:
                return( Z_BVAL_P(zVal) ? 1.0 : 0.0 );

            case IS_LONG:
                return( (double)Z_LVAL_P(zVal) );

            case IS_STRING:
                return( atof( Z_STRVAL_P(zVal) ) );

            default:
                return( 0.0 );
        }
    }
    return( Z_DVAL_P(zVal) );
}
/* }}} */

/* {{{ odbtp_read_zval_long
 */
static long odbtp_read_zval_long(zval* zVal TSRMLS_DC)
{
    if( Z_TYPE_P(zVal) != IS_LONG ) {
        switch( Z_TYPE_P(zVal) ) {
            case IS_BOOL:
                return( Z_BVAL_P(zVal) ? 1 : 0 );

            case IS_DOUBLE:
                return( (long)Z_DVAL_P(zVal) );

            case IS_STRING:
                return( atol( Z_STRVAL_P(zVal) ) );

            default:
                return( 0 );
        }
    }
    return( Z_LVAL_P(zVal) );
}
/* }}} */

/* {{{ odbtp_read_zval_string
 */
static char* odbtp_read_zval_string(zval* zVal, char* pszBuf, long* plLen TSRMLS_DC)
{
    if( Z_TYPE_P(zVal) != IS_STRING ) {
        switch( Z_TYPE_P(zVal) ) {
            case IS_BOOL:
                strcpy( pszBuf, (Z_BVAL_P(zVal) ? "1" : "0") );
                break;

            case IS_LONG:
                sprintf( pszBuf, "%li", Z_LVAL_P(zVal) );
                break;

            case IS_DOUBLE:
                sprintf( pszBuf, "%lg", Z_DVAL_P(zVal) );
                break;

            default:
                strcpy( pszBuf, "?" );
        }
        if( plLen ) *plLen = strlen( pszBuf );
        return( pszBuf );
    }
    if( plLen ) *plLen = Z_STRLEN_P(zVal);
    return( Z_STRVAL_P(zVal) );
}
/* }}} */

/* {{{ odbtp_timestamp_to_object
 */
static odbBOOL odbtp_timestamp_to_object(zval* zTime, odbPTIMESTAMP ptsTime TSRMLS_DC)
{
    if( object_init( zTime ) == FAILURE ) return( FALSE );

    add_property_long( zTime, "year", ptsTime->sYear );
    add_property_long( zTime, "month", ptsTime->usMonth );
    add_property_long( zTime, "day", ptsTime->usDay );
    add_property_long( zTime, "hour", ptsTime->usHour );
    add_property_long( zTime, "minute", ptsTime->usMinute );
    add_property_long( zTime, "second", ptsTime->usSecond );
    add_property_long( zTime, "fraction", ptsTime->ulFraction );

    return( TRUE );
}
/* }}} */

/* {{{ odbtp_guid_to_object
 */
static odbBOOL odbtp_guid_to_object(zval* zGuid, odbPGUID pguidVal TSRMLS_DC)
{
    if( object_init( zGuid ) == FAILURE ) return( FALSE );

    add_property_long( zGuid, "data1", pguidVal->ulData1 );
    add_property_long( zGuid, "data2", pguidVal->usData2 );
    add_property_long( zGuid, "data3", pguidVal->usData3 );
    add_property_stringl( zGuid, "data4", pguidVal->byData4, 8, 1 );

    return( TRUE );
}
/* }}} */

/* {{{ object_to_odbtp_timestamp
 */
static odbBOOL object_to_odbtp_timestamp(odbPTIMESTAMP ptsTime, zval* zTime TSRMLS_DC)
{
    HashTable* htProps;
    zval**     pzValue;

    memset( (char*)ptsTime, 0, sizeof(odbTIMESTAMP) );

    htProps = Z_OBJPROP_P(zTime);

    if( zend_hash_find( htProps, "year", 5, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->sYear = (odbSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "month", 6, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->usMonth = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "day", 4, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->usDay = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "hour", 5, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->usHour = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "minute", 7, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->usMinute = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "second", 7, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->usSecond = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "fraction", 9, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    ptsTime->ulFraction = odbtp_read_zval_long( *pzValue TSRMLS_CC );

    return( TRUE );
}
/* }}} */

/* {{{ object_to_odbtp_guid
 */
static odbBOOL object_to_odbtp_guid(odbPGUID pguidVal, zval* zGuid TSRMLS_DC)
{
    HashTable* htProps;
    long       l;
    char*      psz;
    zval**     pzValue;
    char       szData4[32];

    memset( (char*)pguidVal, 0, sizeof(odbGUID) );

    htProps = Z_OBJPROP_P(zGuid);

    if( zend_hash_find( htProps, "data1", 6, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    pguidVal->ulData1 = (odbULONG)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "data2", 6, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    pguidVal->usData2 = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "data3", 6, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    pguidVal->usData3 = (odbUSHORT)odbtp_read_zval_long( *pzValue TSRMLS_CC );

    if( zend_hash_find( htProps, "data4", 6, (void**)&pzValue ) != SUCCESS )
        return( FALSE );
    psz = odbtp_read_zval_string( *pzValue, szData4, &l TSRMLS_CC );
    memcpy( pguidVal->byData4, psz, l <= 8 ? l : 8 );

    return( TRUE );
}
/* }}} */

/* {{{ odbtp_set_zval_to_timestamp
 */
static odbBOOL odbtp_set_zval_to_timestamp(zval* zTime, odbPTIMESTAMP ptsTime TSRMLS_DC)
{
    char* pszFormat = ODBTP_G(datetime_format);
    char  sz[32];

    if( !strcasecmp( pszFormat, "iso" ) ) {
        odbTimestampToStr( sz, ptsTime, FALSE );
    }
    else if( !strcasecmp( pszFormat, "ctime" ) ) {
        ZVAL_LONG( zTime, odbTimestampToCTime( ptsTime ) );
        return( TRUE );
    }
    else if( !strncasecmp( pszFormat, "mdyhm", 5 ) ) {
        odbSHORT  sYear = ptsTime->sYear;
        odbUSHORT usDay = ptsTime->usDay;
        odbUSHORT usHour = ptsTime->usHour % 12;
        time_t    tTime;
        odbPSTR   pszTime;

        /* Force year and day to ctime friendly values */
        ptsTime->sYear = 1999;
        ptsTime->usDay = 1;

        /* Get month abbreviation from ctime function */
        tTime = odbTimestampToCTime( ptsTime );
        if( (pszTime = ctime( &tTime )) )
            strncpy( sz,  pszTime + 4, 4 );
        else
            strcpy( sz, "??? " );

        ptsTime->sYear = sYear;
        ptsTime->usDay = usDay;

        if( !strcasecmp( pszFormat + 5, "sf" ) ) {
            sprintf( &sz[4], "%2i %i %02i:%02i:%02i:%03i%s",
                     ptsTime->usDay, ptsTime->sYear,
                     (usHour ? usHour : 12), ptsTime->usMinute,
                     ptsTime->usSecond, ptsTime->ulFraction / 1000000,
                     (ptsTime->usHour < 12 ? "AM" : "PM") );
        }
        else if( tolower( *(pszFormat + 5) ) == 's' ) {
            sprintf( &sz[4], "%2i %i %02i:%02i:%02i%s",
                     ptsTime->usDay, ptsTime->sYear,
                     (usHour ? usHour : 12), ptsTime->usMinute,
                     ptsTime->usSecond, (ptsTime->usHour < 12 ? "AM" : "PM") );
        }
        else {
            sprintf( &sz[4], "%2i %i %02i:%02i%s",
                     ptsTime->usDay, ptsTime->sYear,
                     (usHour ? usHour : 12), ptsTime->usMinute,
                     (ptsTime->usHour < 12 ? "AM" : "PM") );
        }
    }
    else {
        return( odbtp_timestamp_to_object(zTime, ptsTime TSRMLS_CC) );
    }
    ZVAL_STRING( zTime, sz, 1 );
    return( TRUE );
}
/* }}} */

/* {{{ odbtp_set_zval_to_guid
 */
static odbBOOL odbtp_set_zval_to_guid(zval* zGuid, odbPGUID pguidVal TSRMLS_DC)
{
    char* pszFormat = ODBTP_G(guid_format);

    if( !strcasecmp( pszFormat, "string" ) ) {
        char sz[40];

        odbGuidToStr( sz, pguidVal );
        ZVAL_STRING( zGuid, sz, 1 );
        return( TRUE );
    }
    return( odbtp_guid_to_object(zGuid, pguidVal TSRMLS_CC) );
}
/* }}} */

/* {{{ odbtp_get_col_num
 */
static odbUSHORT odbtp_get_col_num(odbHANDLE hQry, zval* zField TSRMLS_DC)
{
    odbUSHORT usCol;

    if( Z_TYPE_P(zField) == IS_LONG ) {
        usCol = (odbUSHORT)Z_LVAL_P(zField) + 1;

        if( usCol == 0 || usCol > odbGetTotalCols( hQry ) ) {
            php_odbtp_invalid(hQry, "Invalid field index" TSRMLS_CC);
            return( 0 );
        }
    }
    else {
        char  szBuf[64];
        char* pszField = odbtp_read_zval_string( zField, szBuf, NULL TSRMLS_CC );

        if( !(usCol = odbColNum( hQry, pszField )) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            return( 0 );
        }
    }
    return( usCol );
}
/* }}} */

/* {{{ odbtp_get_param_num
 */
static odbUSHORT odbtp_get_param_num(odbHANDLE hQry, zval* zParam TSRMLS_DC)
{
    odbUSHORT usParam;

    if( Z_TYPE_P(zParam) == IS_LONG ) {
        usParam = (odbUSHORT)Z_LVAL_P(zParam);

        if( usParam == 0 || usParam > odbGetTotalParams( hQry ) ) {
            php_odbtp_invalid(hQry, "Invalid param index" TSRMLS_CC);
            return( 0 );
        }
    }
    else {
        char  szBuf[64];
        char* pszParam = odbtp_read_zval_string( zParam, szBuf, NULL TSRMLS_CC );

        if( !(usParam = odbParamNum( hQry, pszParam )) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            return( 0 );
        }
    }
    return( usParam );
}
/* }}} */

/* {{{ php_odbtp_get_last_error
 */
static void php_odbtp_get_last_error(INTERNAL_FUNCTION_PARAMETERS, long lPart)
{
    char* psz;
    zval* rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rCon ) == FAILURE )
        return;

    if( rCon ) {
        odbtp_connection* con;
        ZEND_FETCH_RESOURCE( con, odbtp_connection*, &rCon, -1, le_connection_name, le_connection );
        psz = con->last_error;
    }
    else {
        psz = ODBTP_G(last_error);
    }
    switch( lPart ) {
        case PHP_ODB_LERROR_STRING:
            if( psz ) {
                RETURN_STRING( psz, 1 );
            }
            RETURN_EMPTY_STRING();

        case PHP_ODB_LERROR_STATE:
            if( psz && *psz == '[' ) {
                char* pszX;
                for( pszX = (++psz); *pszX && *pszX != ']'; pszX++ );
                RETURN_STRINGL( psz, (long)(pszX - psz), 1 );
            }
            RETURN_EMPTY_STRING();

        case PHP_ODB_LERROR_CODE:
            if( psz && *psz == '[' && (psz = strchr( psz + 1, '[' )) ) {
                long lCode = 0;

                for( psz++; *psz && *psz != ']'; psz++ ) {
                    if( *psz < '0' || *psz > '9' ) break;
                    lCode = (lCode * 10) + (long)*psz - 48;
                }
                if( *psz != ']' ) lCode = 0;
                RETURN_LONG( lCode );
            }
            RETURN_LONG( 0 );
    }
}
/* }}} */

/* {{{ odbtp_set_param
 */
static zend_bool odbtp_set_param(odbHANDLE hQry, odbUSHORT usParam, zval* zData, odbBOOL bFinal TSRMLS_DC)
{
    odbBOOL      bOK;
    odbBYTE      byData;
    odbDOUBLE    dData;
    odbFLOAT     fData;
    odbGUID      guidData;
    long         lDataLen;
    odbPVOID     pData;
    odbSHORT     sDataType;
    char         szBuf[64];
    odbTIMESTAMP tsData;
    odbULONG     ulData;
    odbULONGLONG ullData;
    odbUSHORT    usData;

    if( !(sDataType = odbParamDataType( hQry, usParam )) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        return( FALSE );
    }
    if( zData == NULL ) {
        if( !odbSetParamDefault( hQry, usParam, bFinal ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            return( FALSE );
        }
        return( TRUE );
    }
    if( Z_TYPE_P(zData) == IS_NULL ) {
        if( !odbSetParamNull( hQry, usParam, bFinal ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            return( FALSE );
        }
        return( TRUE );
    }
    switch( sDataType ) {
        case ODB_BIT:
            byData = odbtp_read_zval_boolean( zData TSRMLS_CC );
            bOK = odbSetParamByte( hQry, usParam, byData, bFinal );
            break;

        case ODB_TINYINT:
            byData = (odbCHAR)odbtp_read_zval_long( zData TSRMLS_CC );
            bOK = odbSetParamByte( hQry, usParam, byData, bFinal );
            break;

        case ODB_UTINYINT:
            byData = (odbBYTE)odbtp_read_zval_long( zData TSRMLS_CC );
            bOK = odbSetParamByte( hQry, usParam, byData, bFinal );
            break;

        case ODB_SMALLINT:
            usData = (odbSHORT)odbtp_read_zval_long( zData TSRMLS_CC );
            bOK = odbSetParamShort( hQry, usParam, usData, bFinal );
            break;

        case ODB_USMALLINT:
            usData = (odbUSHORT)odbtp_read_zval_long( zData TSRMLS_CC );
            bOK = odbSetParamShort( hQry, usParam, usData, bFinal );
            break;

        case ODB_INT:
        case ODB_UINT:
            ulData = odbtp_read_zval_long( zData TSRMLS_CC );
            bOK = odbSetParamLong( hQry, usParam, ulData, bFinal );
            break;

        case ODB_BIGINT:
            ullData = odbStrToLongLong( odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC ) );
            bOK = odbSetParamLongLong( hQry, usParam, ullData, bFinal );
            break;

        case ODB_UBIGINT:
            ullData = odbStrToULongLong( odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC ) );
            bOK = odbSetParamLongLong( hQry, usParam, ullData, bFinal );
            break;

        case ODB_REAL:
            fData = (float)odbtp_read_zval_double( zData TSRMLS_CC );
            bOK = odbSetParamFloat( hQry, usParam, fData, bFinal );
            break;

        case ODB_DOUBLE:
            dData = odbtp_read_zval_double( zData TSRMLS_CC );
            bOK = odbSetParamDouble( hQry, usParam, dData, bFinal );
            break;

        case ODB_DATETIME:
            if( Z_TYPE_P(zData) == IS_OBJECT ) {
                object_to_odbtp_timestamp(&tsData, zData TSRMLS_CC);
            }
            else if( Z_TYPE_P(zData) == IS_STRING ) {
                pData = odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC );;
                odbStrToTimestamp( &tsData, (odbPCSTR)pData );
            }
            else {
                ulData = odbtp_read_zval_long( zData TSRMLS_CC );;
                odbCTimeToTimestamp( &tsData, (odbLONG)ulData );
            }
            bOK = odbSetParamTimestamp( hQry, usParam, &tsData, bFinal );
            break;

        case ODB_GUID:
            if( Z_TYPE_P(zData) == IS_OBJECT ) {
                object_to_odbtp_guid(&guidData, zData TSRMLS_CC);
            }
            else {
                pData = odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC );;
                odbStrToGuid( &guidData, (odbPCSTR)pData );
            }
            bOK = odbSetParamGuid( hQry, usParam, &guidData, bFinal );
            break;

        default:
            pData = odbtp_read_zval_string( zData, szBuf, &lDataLen TSRMLS_CC );
            bOK = odbSetParam( hQry, usParam, pData, lDataLen, bFinal );
            break;
    }
    if( !bOK ) {
        process_odbtp_error(hQry TSRMLS_CC);
        return( FALSE );
    }
    return( TRUE );
}
/* }}} */

/* {{{ odbtp_get_param_value
 */
static void odbtp_get_param_value(zval* zValue, odbHANDLE hQry, odbUSHORT usParam TSRMLS_DC)
{
    odbULONG lDataLen;
    odbPVOID pData;
    char*    pszData;
    char     sz[24];

    if( !(pData = odbParamData( hQry, usParam )) ) {
        ZVAL_NULL( zValue );
        return;
    }
    switch( odbParamDataType( hQry, usParam ) ) {
        case ODB_BIT:
            ZVAL_BOOL( zValue, *((odbPBYTE)pData) != 0 ); break;

        case ODB_TINYINT:
            ZVAL_LONG( zValue, *((odbPSTR)pData) ); break;
        case ODB_UTINYINT:
            ZVAL_LONG( zValue, *((odbPBYTE)pData) ); break;

        case ODB_SMALLINT:
            ZVAL_LONG( zValue, *((odbPSHORT)pData) ); break;
        case ODB_USMALLINT:
            ZVAL_LONG( zValue, *((odbPUSHORT)pData) ); break;

        case ODB_INT:
            ZVAL_LONG( zValue, *((odbPLONG)pData) ); break;
        case ODB_UINT:
            ZVAL_LONG( zValue, *((odbPULONG)pData) ); break;

        case ODB_BIGINT:
            pszData = odbLongLongToStr( *((odbPLONGLONG)pData), &sz[23] );
            ZVAL_STRING( zValue, pszData, 1 ); break;
        case ODB_UBIGINT:
            pszData = odbULongLongToStr( *((odbPULONGLONG)pData), &sz[23] );
            ZVAL_STRING( zValue, pszData, 1 ); break;

        case ODB_REAL:
            ZVAL_DOUBLE( zValue, *((odbPFLOAT)pData) ); break;
        case ODB_DOUBLE:
            ZVAL_DOUBLE( zValue, *((odbPDOUBLE)pData) ); break;

        case ODB_DATETIME:
            odbtp_set_zval_to_timestamp(zValue, (odbPTIMESTAMP)pData TSRMLS_CC);
            break;

        case ODB_GUID:
            odbtp_set_zval_to_guid(zValue, (odbPGUID)pData TSRMLS_CC);
            break;

        default:
            if( ODBTP_G(truncation_errors) &&
                odbParamTruncated( hQry, usParam ) )
            {
                process_odbtp_truncation(hQry, usParam, FALSE TSRMLS_CC);
            }
            lDataLen = odbParamDataLen( hQry, usParam );
            ZVAL_STRINGL( zValue, (char*)pData, lDataLen, 1 ); break;
    }
}
/* }}} */

/* {{{ odbtp_get_field_value
 */
static void odbtp_get_field_value(zval* zValue, odbHANDLE hQry, odbUSHORT usCol TSRMLS_DC)
{
    odbLONG  lDataLen;
    odbPVOID pData;
    char*    pszData;
    char     sz[24];

    if( !(pData = odbColData( hQry, usCol )) ) {
        ZVAL_NULL( zValue );
        return;
    }
    switch( odbColDataType( hQry, usCol ) ) {
        case ODB_BIT:
            ZVAL_BOOL( zValue, *((odbPBYTE)pData) != 0 ); break;

        case ODB_TINYINT:
            ZVAL_LONG( zValue, *((odbPSTR)pData) ); break;
        case ODB_UTINYINT:
            ZVAL_LONG( zValue, *((odbPBYTE)pData) ); break;

        case ODB_SMALLINT:
            ZVAL_LONG( zValue, *((odbPSHORT)pData) ); break;
        case ODB_USMALLINT:
            ZVAL_LONG( zValue, *((odbPUSHORT)pData) ); break;

        case ODB_INT:
            ZVAL_LONG( zValue, *((odbPLONG)pData) ); break;
        case ODB_UINT:
            ZVAL_LONG( zValue, *((odbPULONG)pData) ); break;

        case ODB_BIGINT:
            pszData = odbLongLongToStr( *((odbPLONGLONG)pData), &sz[23] );
            ZVAL_STRING( zValue, pszData, 1 ); break;
        case ODB_UBIGINT:
            pszData = odbULongLongToStr( *((odbPULONGLONG)pData), &sz[23] );
            ZVAL_STRING( zValue, pszData, 1 ); break;

        case ODB_REAL:
            ZVAL_DOUBLE( zValue, *((odbPFLOAT)pData) ); break;
        case ODB_DOUBLE:
            ZVAL_DOUBLE( zValue, *((odbPDOUBLE)pData) ); break;

        case ODB_DATETIME:
            odbtp_set_zval_to_timestamp(zValue, (odbPTIMESTAMP)pData TSRMLS_CC);
            break;

        case ODB_GUID:
            odbtp_set_zval_to_guid(zValue, (odbPGUID)pData TSRMLS_CC);
            break;

        default:
            if( ODBTP_G(truncation_errors) &&
                odbColTruncated( hQry, usCol ) )
            {
                process_odbtp_truncation(hQry, usCol, TRUE TSRMLS_CC);
            }
            lDataLen = odbColDataLen( hQry, usCol );
            ZVAL_STRINGL( zValue, (char*)pData, lDataLen, 1 );
    }
}
/* }}} */

/* {{{ odbtp_set_input_params
 */
static zend_bool odbtp_set_input_params(odbtp_query* qry TSRMLS_DC)
{
    if( qry->attached_params ) {
        odbBOOL   bSetParam = FALSE;
        odbHANDLE hQry = qry->hQry;
        odbUSHORT usParam;
        odbUSHORT usTotalParams;
        zval*     zVar;

        usTotalParams = odbGetTotalParams( hQry );

        for( usParam = 1; usParam <= usTotalParams; usParam++ ) {
            if( (zVar = odbParamUserData( hQry, usParam )) &&
                (odbParamType( hQry, usParam ) & ODB_PARAM_INPUT) )
            {
                if( !odbtp_set_param(hQry, usParam, zVar, FALSE TSRMLS_CC) )
                    return( FALSE );
                bSetParam = TRUE;
            }
        }
        if( bSetParam && !odbFinalizeRequest( hQry ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            return( FALSE );
        }
    }
    return( TRUE );
}
/* }}} */

/* {{{ odbtp_get_output_params
 */
static zend_bool odbtp_get_output_params(odbtp_query* qry TSRMLS_DC)
{
    if( qry->attached_params ) {
        odbHANDLE hQry = qry->hQry;
        odbUSHORT usParam;
        odbUSHORT usTotalParams;
        zval*     zVar;

        if( !odbGetOutputParams( hQry ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            return( FALSE );
        }
        usTotalParams = odbGetTotalParams( hQry );

        for( usParam = 1; usParam <= usTotalParams; usParam++ ) {
            if( (zVar = odbParamUserData( hQry, usParam )) &&
                (odbParamType( hQry, usParam ) & ODB_PARAM_OUTPUT) &&
                odbGotParam( hQry, usParam ) )
            {
                zval_dtor( zVar );
                odbtp_get_param_value(zVar, hQry, usParam TSRMLS_CC);
            }
        }
    }
    return( TRUE );
}
/* }}} */

/* {{{ odbtp_get_attached_col_values
 */
static void odbtp_get_attached_col_values(odbtp_query* qry TSRMLS_DC)
{
    if( qry->attached_cols ) {
        odbHANDLE hQry = qry->hQry;
        odbUSHORT usCol;
        odbUSHORT usTotalCols;
        zval*     zVar;

        usTotalCols = odbGetTotalCols( hQry );

        for( usCol = 1; usCol <= usTotalCols; usCol++ ) {
            if( (zVar = odbColUserData( hQry, usCol )) ) {
                zval_dtor( zVar );
                odbtp_get_field_value(zVar, hQry, usCol TSRMLS_CC);
            }
        }
    }
}
/* }}} */

/* {{{ php_odbtp_do_connect
 */
static void php_odbtp_do_connect(INTERNAL_FUNCTION_PARAMETERS, odbUSHORT usType)
{
    odbBOOL           bSuccess;
    odbBOOL           bIsAllHex = TRUE;
    odbtp_connection* con;
    odbHANDLE         hCon;
    int               iConnInfoLen = 0;
    int               iDatabaseLen = 0;
    int               iHostInfoLen;
    int               iPasswordLen = 0;
    char*             psz;
    char*             pszConnInfo = NULL;
    char*             pszDatabase = NULL;
    char*             pszHost;
    char*             pszHostInfo;
    char*             pszPassword = NULL;
    odbUSHORT         usPort = ODBTP_G(default_port);

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sss",
                              &pszHostInfo, &iHostInfoLen,
                              &pszConnInfo, &iConnInfoLen,
                              &pszPassword, &iPasswordLen,
                              &pszDatabase, &iDatabaseLen ) == FAILURE )
        return;

    if( !(hCon = odbAllocate( NULL )) ) {
        php_odbtp_error(NULL, "Unable to allocate memory" TSRMLS_CC);
        RETURN_FALSE;
    }
    for( psz = pszHostInfo; *psz && *psz != ':'; psz++ );

    if( *psz ) {
        char* pszVal;

        pszHost = emalloc( strlen(pszHostInfo) + 1 );
        strcpy( pszHost, pszHostInfo );
        psz = pszHost + (psz - pszHostInfo);
        *(psz++) = 0;
        for( pszVal = psz; *psz && *psz != ':'; psz++ );
        if( *psz ) *(psz++) = 0;
        if( *pszVal ) usPort = (odbUSHORT)atoi( pszVal );
        if( *psz ) {
            for( pszVal = psz; *psz && *psz != ':'; psz++ );
            if( *psz ) *(psz++) = 0;
            if( *pszVal ) odbSetConnectTimeout( hCon, atoi( pszVal ) );
            if( *psz ) {
                for( pszVal = psz; *psz && *psz != ':'; psz++ );
                if( *psz ) *(psz++) = 0;
                if( *pszVal ) odbSetReadTimeout( hCon, atoi( pszVal ) );
            }
        }
    }
    else {
        pszHost = pszHostInfo;
    }
    if( !pszConnInfo ) pszConnInfo = "";

    /* check if received an ODBC connect string, connection id or username */
    for( psz = pszConnInfo; *psz && *psz != '=' && *psz != ';'; psz++ )
        if( bIsAllHex && !PHP_ODB_IS_HEX(*psz) ) bIsAllHex = FALSE;

    if( *psz || /* ODBC connect string */
        (bIsAllHex && iConnInfoLen == 32 && !pszPassword) /* connection id */
      )
    {
        bSuccess = odbLogin( hCon, pszHost, usPort, usType, pszConnInfo );
    }
    else /* username, so assume host is an interface name */ {
        bSuccess = odbLoginInterface( hCon, ODBTP_G(interface_file), pszHost,
                                      pszConnInfo, pszPassword,
                                      pszDatabase, usType );
    }
    if( pszHost != pszHostInfo ) efree( pszHost );

    if( !bSuccess ) {
        process_odbtp_error(hCon TSRMLS_CC);
        odbFree( hCon );
        RETURN_FALSE;
    }
    con = (odbtp_connection*)emalloc( sizeof(odbtp_connection) );
    con->hCon = hCon;
    con->default_qry = NULL;
    con->last_error = NULL;
    con->dont_pool_dbc = FALSE;
    odbSetUserData( hCon, con );

    ZEND_REGISTER_RESOURCE(return_value, con, le_connection);
    php_odbtp_set_default_link(Z_RESVAL_P(return_value) TSRMLS_CC);
}
/* }}} */

/* {{{ php_odbtp_set_option
 */
static void php_odbtp_set_option(INTERNAL_FUNCTION_PARAMETERS, odbLONG lOption)
{
    zend_bool         bSet = TRUE;
    odbtp_connection* con;
    long              id = -1;
    zval**            prCon = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|rb",
                              &rCon,
                              &bSet ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    switch( lOption ) {
        case PHP_ODB_OPTION_CONVERT_ALL:
            if( !odbConvertAll( con->hCon, bSet ) ) {
                process_odbtp_error(con->hCon TSRMLS_CC);
                RETURN_FALSE;
            }
            break;

        case PHP_ODB_OPTION_CONVERT_DATETIME:
            if( !odbConvertDatetime( con->hCon, bSet ) ) {
                process_odbtp_error(con->hCon TSRMLS_CC);
                RETURN_FALSE;
            }
            break;

        case PHP_ODB_OPTION_CONVERT_GUID:
            if( !odbConvertGuid( con->hCon, bSet ) ) {
                process_odbtp_error(con->hCon TSRMLS_CC);
                RETURN_FALSE;
            }
            break;

        case PHP_ODB_OPTION_DONT_POOL_DBC:
            con->dont_pool_dbc = bSet;
            break;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ php_odbtp_get_response
 */
static void php_odbtp_get_response(INTERNAL_FUNCTION_PARAMETERS, odbBOOL bGetError)
{
    odbHANDLE     hOdb;
    long          id = -1;
    odbtp_handle* odb;
    zval*         rOdb = NULL;
    zval**        prOdb = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rOdb ) == FAILURE )
        return;

    if( rOdb )
        prOdb = &rOdb;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE2( odb, odbtp_handle*, prOdb, id, "ODBTP Handle", le_connection, le_query );

    if( !(hOdb = odb->hOdb) ) RETURN_EMPTY_STRING();

    if( bGetError ) {
        if( odbGetError( hOdb ) != ODBTPERR_NONE ) {
            RETURN_STRING( (char*)odbGetErrorText( hOdb ), 1 );
        }
    }
    else {
        odbULONG response_code = odbGetResponseCode( hOdb );
        odbULONG response_len = odbGetResponseSize( hOdb );

        if( (response_code == ODBTP_OK || response_code == ODBTP_QUERYOK) &&
            response_len > 0 )
        {
            RETURN_STRINGL( (char*)odbGetResponse( hOdb ), response_len, 1 );
        }
    }
    RETURN_EMPTY_STRING();
}
/* }}} */

/* {{{ php_odbtp_obtain_query
 */
static void php_odbtp_obtain_query(INTERNAL_FUNCTION_PARAMETERS, odbBOOL bAllocate)
{
    odbtp_connection* con;
    odbHANDLE         hQry;
    long              id = -1;
    odbLONG           lQryId = 0;
    zval**            prCon = NULL;
    odbtp_query*      qry;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|rl",
                              &rCon,
                              &lQryId ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( bAllocate )
        hQry = odbAllocate( con->hCon );
    else
        hQry = odbGetQuery( con->hCon, lQryId );

    if( !hQry ) {
        process_odbtp_error(con->hCon TSRMLS_CC);
        RETURN_FALSE;
    }
    qry = (odbtp_query*)emalloc( sizeof(odbtp_query) );
    qry->hQry = hQry;
    qry->attached_cols = FALSE;
    qry->attached_params = FALSE;
    odbSetUserData( hQry, qry );
    ZEND_REGISTER_RESOURCE(return_value, qry, le_query);
    qry->link = Z_RESVAL_P(return_value);
    if( !con->default_qry ) con->default_qry = qry;
}
/* }}} */

/* {{{ php_odbtp_connect_info
 */
static void php_odbtp_connect_info(INTERNAL_FUNCTION_PARAMETERS, odbLONG lType)
{
    odbtp_connection* con;
    odbHANDLE         hCon;
    long              id = -1;
    odbLONG           lInfo = -1;
    zval**            prCon = NULL;
    odbPSTR           pszInfo = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rCon ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !(hCon = con->hCon) ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    switch( lType ) {
        case PHP_ODB_INFO_ID:
            pszInfo = (odbPSTR)odbGetConnectionId( hCon );
            break;

        case PHP_ODB_INFO_VERSION:
            pszInfo = (odbPSTR)odbGetVersion( hCon );
            break;

        case PHP_ODB_INFO_ROW_CACHE_SIZE:
            if( odbIsUsingRowCache( hCon ) )
                lInfo = (odbLONG)odbGetRowCacheSize( hCon );
            else
                lInfo = -1;
            break;
    }
    if( odbGetError( hCon ) != ODBTPERR_NONE ) {
        process_odbtp_error(hCon TSRMLS_CC);
        RETURN_FALSE;
    }
    if( pszInfo ) {
        RETURN_STRING( pszInfo, 1 );
    }
    RETURN_LONG( lInfo );
}
/* }}} */

/* {{{ php_odbtp_query_info
 */
static void php_odbtp_query_info(INTERNAL_FUNCTION_PARAMETERS, odbLONG lType)
{
    odbtp_connection* con;
    odbLONG           lInfo;
    odbHANDLE         hQry;
    long              id = -1;
    odbtp_handle*     odb;
    odbtp_query*      qry;
    zval**            prOdb = NULL;
    zval*             rOdb = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rOdb ) == FAILURE )
        return;

    if( rOdb )
        prOdb = &rOdb;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE2( odb, odbtp_handle*, prOdb, id, "ODBTP Handle", le_connection, le_query );

    if( !odb->hOdb ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( odbIsConnection( odb->hOdb ) ) {
        con = (odbtp_connection*)odb;
        if( !(qry = con->default_qry) ) {
            RETURN_FALSE;
        }
        else {
            hQry = qry->hQry;
        }
    }
    else {
        qry = (odbtp_query*)odb;
        hQry = qry->hQry;
    }
    switch( lType ) {
        case PHP_ODB_INFO_ID:
            lInfo = odbGetQueryId( hQry ); break;
        case PHP_ODB_INFO_TOTALCOLS:
            lInfo = odbGetTotalCols( hQry ); break;
        case PHP_ODB_INFO_TOTALPARAMS:
            lInfo = odbGetTotalParams( hQry ); break;
        case PHP_ODB_INFO_TOTALROWS:
            lInfo = odbGetTotalRows( hQry ); break;
        case PHP_ODB_INFO_ROWCOUNT:
            lInfo = odbGetRowCount( hQry ); break;
        case PHP_ODB_INFO_ROWSTATUS:
            lInfo = odbGetRowStatus( hQry ); break;
        case PHP_ODB_INFO_DETACHED:
            if( odbIsConnected( hQry ) ) {
                RETURN_FALSE;
            }
            RETURN_TRUE;
        default:
            RETURN_LONG( 0 );
    }
    if( odbGetError( hQry ) != ODBTPERR_NONE ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_LONG( lInfo );
}
/* }}} */

/* {{{ php_odbtp_do_query
 */
static void php_odbtp_do_query(INTERNAL_FUNCTION_PARAMETERS, odbLONG lType)
{
    odbBOOL           bOK;
    odbtp_connection* con;
    odbHANDLE         hQry;
    long              id = -1;
    int               iQueryLen = 0;
    odbtp_handle*     odb;
    char*             pszQuery = "";
    char*             pszUseDB;
    odbtp_query*      qry;
    zval*             rOdb = NULL;
    zval**            prOdb = NULL;
    zval*             zDummy;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|rz",
                              &pszQuery, &iQueryLen,
                              &rOdb,
                              &zDummy ) == FAILURE )
        return;

    if( rOdb )
        prOdb = &rOdb;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE2( odb, odbtp_handle*, prOdb, id, "ODBTP Handle", le_connection, le_query );

    if( !odb->hOdb ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( odbIsConnection( odb->hOdb ) ) {
        con = (odbtp_connection*)odb;
        if( !(qry = con->default_qry) || ODBTP_G(detach_default_queries) ) {
            if( qry ) {
                odbDetachQry( qry->hQry );
                con->default_qry = NULL;
            }
            if( !(hQry = odbAllocate( con->hCon )) ) {
                process_odbtp_error(con->hCon TSRMLS_CC);
                RETURN_FALSE;
            }
            qry = (odbtp_query*)emalloc( sizeof(odbtp_query) );
            qry->hQry = hQry;
            qry->attached_cols = FALSE;
            qry->attached_params = FALSE;
            odbSetUserData( hQry, qry );
            con->default_qry = qry;
            qry->link = -1;
        }
        else {
            hQry = qry->hQry;
        }
    }
    else {
        qry = (odbtp_query*)odb;
        hQry = qry->hQry;
    }
    if( iQueryLen == 0 ) {
        if( lType == PHP_ODB_QUERY_SELECT_DB )
            php_odbtp_error(odb->hOdb, "Database was not specified" TSRMLS_CC);
        else if( lType == PHP_ODB_QUERY_PREPARE_PROC )
            php_odbtp_error(odb->hOdb, "Procedure was not specified" TSRMLS_CC);
        else
            php_odbtp_error(odb->hOdb, "Query was not specified" TSRMLS_CC);
        RETURN_FALSE;
    }
    odbtp_unattach_cols(qry TSRMLS_CC);
    odbtp_unattach_params(qry TSRMLS_CC);
    qry->col_pos = 0;

    switch( lType ) {
        case PHP_ODB_QUERY_EXECUTE:
            bOK = odbExecute( hQry, pszQuery );
            break;

        case PHP_ODB_QUERY_PREPARE:
            bOK = odbPrepare( hQry, pszQuery );
            break;

        case PHP_ODB_QUERY_PREPARE_PROC:
            bOK = odbPrepareProc( hQry, pszQuery );
            break;

        case PHP_ODB_QUERY_SELECT_DB:
            pszUseDB = (char*)emalloc( iQueryLen + 5 );
            strcpy( pszUseDB, "USE " );
            strcpy( pszUseDB + 4, pszQuery );
            bOK = odbExecute( hQry, pszUseDB );
            efree( pszUseDB );
            break;
    }
    if( !bOK ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    if( qry->link == -1 ) {
        ZEND_REGISTER_RESOURCE(return_value, qry, le_query);
        qry->link = Z_RESVAL_P(return_value);
    }
    else {
        ZVAL_RESOURCE(return_value, qry->link);
        zend_list_addref( qry->link );
    }
}
/* }}} */

/* {{{ php_odbtp_bind_param
 */
static void php_odbtp_bind_param(INTERNAL_FUNCTION_PARAMETERS, long lType)
{
    odbHANDLE    hQry;
    long         lDataLen = 0;
    long         lDataType = 0;
    long         lPrecision = -1;
    long         lSize = -1;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usParam;
    odbUSHORT    usType = (odbUSHORT)lType;
    zval*        zParam;
    zval*        zSqlType = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|llzll",
                              &rQry,
                              &zParam,
                              &lDataType,
                              &lDataLen,
                              &zSqlType,
                              &lSize,
                              &lPrecision ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usParam = odbtp_get_param_num(hQry, zParam TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    if( zSqlType ) {
        odbSHORT sSqlType;
        odbSHORT sPrecision;
        odbULONG ulSize;

        if( Z_TYPE_P(zSqlType) == IS_STRING ) {
            if( !odbDescribeSqlType( odbGetConnection( hQry ),
                                     Z_STRVAL_P(zSqlType), &sSqlType,
                                     &ulSize, &sPrecision ) )
            {
                php_odbtp_invalid(hQry, "Invalid SQL type name" TSRMLS_CC);
                RETURN_FALSE;
            }
            if( lSize >= 0 ) ulSize = lSize;
            if( lPrecision >= 0 ) sPrecision = (odbSHORT)lPrecision;
        }
        else {
            sSqlType = (odbSHORT)odbtp_read_zval_long( zSqlType TSRMLS_CC );
            ulSize = lSize >= 0 ? lSize : 0;
            sPrecision = (odbSHORT)(lPrecision >= 0 ? lPrecision : 0);
        }
        if( !odbBindParamEx( hQry, usParam, usType, (odbSHORT)lDataType,
                             lDataLen, sSqlType, ulSize, sPrecision,TRUE ) )
        {
            process_odbtp_error(hQry TSRMLS_CC);
            RETURN_FALSE;
        }
    }
    else if( !odbBindParam( hQry, usParam, usType, (odbSHORT)lDataType, lDataLen, TRUE ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ php_odbtp_param_info
 */
static void php_odbtp_param_info(INTERNAL_FUNCTION_PARAMETERS, long lType)
{
    odbHANDLE    hQry;
    odbPSTR      pszName;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usParam;
    zval*        zParam = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz",
                              &rQry,
                              &zParam ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usParam = odbtp_get_param_num(hQry, zParam TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    switch( lType ) {
        case PHP_ODB_INFO_PARAMTYPE:
            RETURN_LONG( odbParamType( hQry, usParam ) );

        case PHP_ODB_INFO_NAME:
            pszName = (odbPSTR)odbParamName( hQry, usParam );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_BINDTYPE:
            RETURN_LONG( odbParamDataType( hQry, usParam ) );

        case PHP_ODB_INFO_TYPENAME:
            pszName = (odbPSTR)odbParamSqlTypeName( hQry, usParam );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_LENGTH:
            RETURN_LONG( odbParamSize( hQry, usParam ) );

        case PHP_ODB_INFO_NUMBER:
            RETURN_LONG( (long)usParam );
    }
}
/* }}} */

/* {{{ php_odbtp_attach_variable
 */
static void php_odbtp_attach_variable(INTERNAL_FUNCTION_PARAMETERS, odbBOOL bAttachToCol )
{
    odbHANDLE    hQry;
    long         lDataLen = 0;
    long         lDataType = 0;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usId;
    zval*        zId;
    zval*        zVar;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rzz|ll",
                              &rQry,
                              &zId,
                              &zVar,
                              &lDataType,
                              &lDataLen ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !PZVAL_IS_REF(zVar) ) {
        php_odbtp_invalid(hQry, "Variable not passed by reference" TSRMLS_CC);
        RETURN_FALSE;
    }
    if( bAttachToCol ) {
        if( !(usId = odbtp_get_col_num(hQry, zId TSRMLS_CC)) ) {
            RETURN_FALSE;
        }
        if( lDataType != 0 ) {
            if( !odbBindCol( hQry, usId, (odbSHORT)lDataType, lDataLen, TRUE ) ) {
                process_odbtp_error(hQry TSRMLS_CC);
                RETURN_FALSE;
            }
        }
        odbSetColUserData( hQry, usId, zVar );
        qry->attached_cols = TRUE;
    }
    else {
        if( !(usId = odbtp_get_param_num(hQry, zId TSRMLS_CC)) ) {
            RETURN_FALSE;
        }
        if( lDataType != 0 ) {
            odbUSHORT usType;

            if( !(usType = odbParamType( hQry, usId )) ) {
                php_odbtp_error(hQry, "Parameter type has not been defined" TSRMLS_CC);
                RETURN_FALSE;
            }
            if( !odbBindParam( hQry, usId, usType, (odbSHORT)lDataType, lDataLen, TRUE ) ) {
                process_odbtp_error(hQry TSRMLS_CC);
                RETURN_FALSE;
            }
        }
        odbSetParamUserData( hQry, usId, zVar );
        qry->attached_params = TRUE;
    }
    ZVAL_ADDREF(zVar);
    RETURN_TRUE;
}
/* }}} */

/* {{{ php_odbtp_field_info
 */
static void php_odbtp_field_info(INTERNAL_FUNCTION_PARAMETERS, long lType)
{
    odbHANDLE    hQry;
    int          i;
    long         lBlob = 0;
    long         lNumeric = 0;
    odbPSTR      pszName;
    odbtp_query* qry;
    zval*        rQry;
    odbULONG     ulFlags;
    odbUSHORT    usCol;
    char         sz[128];
    zval*        zField = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|z",
                              &rQry,
                              &zField ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !zField ) {
        usCol = qry->col_pos + 1;
        if( usCol > odbGetTotalCols( hQry ) ) RETURN_FALSE;
    }
    else if( !(usCol = odbtp_get_col_num(hQry, zField TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    qry->col_pos = usCol;

    switch( lType ) {
        case PHP_ODB_INFO_ALL:
            object_init( return_value );
            pszName = (odbPSTR)odbColName( hQry, usCol );
            add_property_stringl( return_value, "name", pszName, strlen(pszName), 1 );
            add_property_long( return_value, "bindtype", odbColDataType( hQry, usCol ) );
            pszName = (odbPSTR)odbColSqlTypeName( hQry, usCol );
            add_property_stringl( return_value, "type", pszName, strlen(pszName), 1 );
            add_property_long( return_value, "max_length", odbColSize( hQry, usCol ) );
            pszName = (odbPSTR)odbColTable( hQry, usCol );
            add_property_stringl( return_value, "table", pszName, strlen(pszName), 1 );
            pszName = (odbPSTR)odbColSchema( hQry, usCol );
            add_property_stringl( return_value, "schema", pszName, strlen(pszName), 1 );
            pszName = (odbPSTR)odbColCatalog( hQry, usCol );
            add_property_stringl( return_value, "catalog", pszName, strlen(pszName), 1 );
            pszName = (odbPSTR)odbColBaseName( hQry, usCol );
            add_property_stringl( return_value, "basename", pszName, strlen(pszName), 1 );

            ulFlags = odbColFlags( hQry, usCol );
            add_property_long( return_value, "not_null", (ulFlags & ODB_COLINFO_NOTNULL) ? 1 : 0 );
            add_property_long( return_value, "unsigned", (ulFlags & ODB_COLINFO_UNSIGNED) ? 1 : 0 );
            add_property_long( return_value, "auto_increment", (ulFlags & ODB_COLINFO_AUTONUMBER) ? 1 : 0 );
            add_property_long( return_value, "primary_key", (ulFlags & ODB_COLINFO_PRIMARYKEY) ? 1 : 0 );
            add_property_long( return_value, "hidden", (ulFlags & ODB_COLINFO_HIDDEN) ? 1 : 0 );

            switch( odbColSqlType( hQry, usCol ) ) {
                case SQL_BIT:
                case SQL_TINYINT:
                case SQL_SMALLINT:
                case SQL_INT:
                case SQL_BIGINT:
                case SQL_REAL:
                case SQL_FLOAT:
                case SQL_DOUBLE:
                case SQL_NUMERIC:
                case SQL_DECIMAL:
                    lNumeric = 1; break;

                case SQL_LONGVARBINARY:
                    lBlob = 1; break;
            }
            add_property_long( return_value, "numeric", lNumeric );
            add_property_long( return_value, "blob", lBlob );
            break;

        case PHP_ODB_INFO_NAME:
            pszName = (odbPSTR)odbColName( hQry, usCol );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_BINDTYPE:
            RETURN_LONG( odbColDataType( hQry, usCol ) );

        case PHP_ODB_INFO_TYPENAME:
            pszName = (odbPSTR)odbColSqlTypeName( hQry, usCol );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_LENGTH:
            RETURN_LONG( odbColSize( hQry, usCol ) );

        case PHP_ODB_INFO_TABLE:
            pszName = (odbPSTR)odbColTable( hQry, usCol );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_SCHEMA:
            pszName = (odbPSTR)odbColSchema( hQry, usCol );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_CATALOG:
            pszName = (odbPSTR)odbColCatalog( hQry, usCol );
            RETURN_STRING( pszName, 1 );

        case PHP_ODB_INFO_FLAGS:
            ulFlags = odbColFlags( hQry, usCol );
            strcpy( sz, "" );

            if( ulFlags & ODB_COLINFO_NOTNULL )
                strcat( sz, "not_null " );
            if( ulFlags & ODB_COLINFO_UNSIGNED )
                strcat( sz, "unsigned " );
            if( ulFlags & ODB_COLINFO_AUTONUMBER )
                strcat( sz, "auto_increment " );
            if( ulFlags & ODB_COLINFO_PRIMARYKEY )
                strcat( sz, "primary_key " );
            if( ulFlags & ODB_COLINFO_HIDDEN )
                strcat( sz, "hidden " );

            switch( odbColSqlType( hQry, usCol ) ) {
                case SQL_BIT:
                case SQL_TINYINT:
                case SQL_SMALLINT:
                case SQL_INT:
                case SQL_BIGINT:
                case SQL_REAL:
                case SQL_FLOAT:
                case SQL_DOUBLE:
                case SQL_NUMERIC:
                case SQL_DECIMAL:
                    strcat( sz, "numeric " ); break;

                case SQL_LONGVARBINARY:
                    strcat( sz, "blob " ); break;
            }
            for( i = strlen(sz) - 1; i > 0 && sz[i] <= ' '; i-- ) sz[i] = 0;
            RETURN_STRING( sz, 1 );

        case PHP_ODB_INFO_BASENAME:
            pszName = (odbPSTR)odbColBaseName( hQry, usCol );
            RETURN_STRING( pszName, 1 );
    }
}
/* }}} */

/* {{{ php_odbtp_fetch_hash
 */
static void php_odbtp_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int iHashType)
{
    odbBOOL      bAddLong;
    odbBOOL      bAddDouble;
    odbBOOL      bAddString;
    odbBOOL      bAddZval;
    odbBOOL      bData;
    odbDOUBLE    dData;
    odbHANDLE    hQry;
    int          iRetVal;
    long         lData;
    long         lFetchParam = 0;
    long         lFetchType = ODB_FETCH_DEFAULT;
    odbPVOID     pData;
    char*        pszData;
    char*        pszKey;
    odbtp_query* qry;
    zval*        rQry;
    char         sz[24];
    uint         uiIndex;
    odbULONG     ulDataLen;
    odbUSHORT    usCol;
    odbUSHORT    usTotalCols;
    zval*        zData;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|ll",
                              &rQry,
                              &lFetchType,
                              &lFetchParam ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }

#if ODBTP_MSSQL /* included for compatibility with mssql_fetch_* functions */
    if( (lFetchType & MSSQL_FETCH) ) {
        switch( lFetchType ) {
            case MSSQL_ASSOC: iHashType = PHP_ODB_FETCH_ASSOC; break;
            case MSSQL_NUM:   iHashType = PHP_ODB_FETCH_ROW; break;
            default:          iHashType = PHP_ODB_FETCH_ARRAY;
        }
        lFetchType = ODB_FETCH_DEFAULT;
        lFetchParam = 0;
    }
#endif

    if( lFetchType == ODB_FETCH_DEFAULT && lFetchParam == 0 ) {
        if( !odbFetchRow( hQry ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            RETURN_FALSE;
        }
    }
    else {
        if( !odbFetchRowEx( hQry, (odbUSHORT)lFetchType, lFetchParam ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            RETURN_FALSE;
        }
    }
    if( odbNoData( hQry ) ) {
        RETURN_FALSE;
    }
    odbtp_get_attached_col_values(qry TSRMLS_CC);

    if( iHashType == PHP_ODB_FETCH_NONE ) {
        RETURN_TRUE;
    }
    if( iHashType == PHP_ODB_FETCH_OBJECT )
        iRetVal = object_init(return_value);
    else
        iRetVal = array_init(return_value);

    if( iRetVal == FAILURE ) {
        RETURN_FALSE;
    }
    usTotalCols = odbGetTotalCols( hQry );

    for( usCol = 1; usCol <= usTotalCols; usCol++ ) {
        uiIndex = usCol - 1;
        pszKey = (char*)odbColName( hQry, usCol );

        if( !(pData = odbColData( hQry, usCol )) ) {
            switch( iHashType ) {
                case PHP_ODB_FETCH_ARRAY:
                case PHP_ODB_FETCH_ASSOC:
                    add_assoc_null( return_value, pszKey );
                    if( iHashType == PHP_ODB_FETCH_ASSOC ) break;
                case PHP_ODB_FETCH_ROW:
                    add_index_null( return_value, uiIndex );
                    break;
                case PHP_ODB_FETCH_OBJECT:
                    add_property_null( return_value, pszKey );
            }
            continue;
        }
        bAddLong = FALSE;
        bAddDouble = FALSE;
        bAddString = FALSE;
        bAddZval = FALSE;

        switch( odbColDataType( hQry, usCol ) ) {
            case ODB_BIT:
                bData = *((odbPBYTE)pData) != 0 ? TRUE : FALSE;
                switch( iHashType ) {
                    case PHP_ODB_FETCH_ARRAY:
                    case PHP_ODB_FETCH_ASSOC:
                        add_assoc_bool( return_value, pszKey, bData );
                        if( iHashType == PHP_ODB_FETCH_ASSOC ) break;
                    case PHP_ODB_FETCH_ROW:
                        add_index_bool( return_value, uiIndex, bData );
                        break;
                    case PHP_ODB_FETCH_OBJECT:
                        add_property_bool( return_value, pszKey, bData );
                }
                break;

            case ODB_TINYINT:
                lData = *((odbPSTR)pData); bAddLong = TRUE; break;
            case ODB_UTINYINT:
                lData = *((odbPBYTE)pData); bAddLong = TRUE; break;

            case ODB_SMALLINT:
                lData = *((odbPSHORT)pData); bAddLong = TRUE; break;
            case ODB_USMALLINT:
                lData = *((odbPUSHORT)pData); bAddLong = TRUE; break;

            case ODB_INT:
                lData = *((odbPLONG)pData); bAddLong = TRUE; break;
            case ODB_UINT:
                lData = *((odbPULONG)pData); bAddLong = TRUE; break;

            case ODB_BIGINT:
                pszData = odbLongLongToStr( *((odbPLONGLONG)pData), &sz[23] );
                ulDataLen = strlen( pszData ); bAddString = TRUE; break;
            case ODB_UBIGINT:
                pszData = odbULongLongToStr( *((odbPULONGLONG)pData), &sz[23] );
                ulDataLen = strlen( pszData ); bAddString = TRUE; break;

            case ODB_REAL:
                dData = *((odbPFLOAT)pData); bAddDouble = TRUE; break;
            case ODB_DOUBLE:
                dData = *((odbPDOUBLE)pData); bAddDouble = TRUE; break;

            case ODB_DATETIME:
                MAKE_STD_ZVAL( zData );
                odbtp_set_zval_to_timestamp(zData, (odbPTIMESTAMP)pData TSRMLS_CC);
                bAddZval = TRUE; break;

            case ODB_GUID:
                MAKE_STD_ZVAL( zData );
                odbtp_set_zval_to_guid(zData, (odbPGUID)pData TSRMLS_CC);
                bAddZval = TRUE; break;

            default:
                if( ODBTP_G(truncation_errors) &&
                    odbColTruncated( hQry, usCol ) )
                {
                    process_odbtp_truncation(hQry, usCol, TRUE TSRMLS_CC);
                }
                ulDataLen = odbColDataLen( hQry, usCol );
                pszData = (char*)pData; bAddString = TRUE; break;
        }
        if( bAddLong ) {
            switch( iHashType ) {
                case PHP_ODB_FETCH_ARRAY:
                case PHP_ODB_FETCH_ASSOC:
                    add_assoc_long( return_value, pszKey, lData );
                    if( iHashType == PHP_ODB_FETCH_ASSOC ) break;
                case PHP_ODB_FETCH_ROW:
                    add_index_long( return_value, uiIndex, lData );
                    break;
                case PHP_ODB_FETCH_OBJECT:
                    add_property_long( return_value, pszKey, lData );
            }
        }
        else if( bAddDouble ) {
            switch( iHashType ) {
                case PHP_ODB_FETCH_ARRAY:
                case PHP_ODB_FETCH_ASSOC:
                    add_assoc_double( return_value, pszKey, dData );
                    if( iHashType == PHP_ODB_FETCH_ASSOC ) break;
                case PHP_ODB_FETCH_ROW:
                    add_index_double( return_value, uiIndex, dData );
                    break;
                case PHP_ODB_FETCH_OBJECT:
                    add_property_double( return_value, pszKey, dData );
            }
        }
        else if( bAddZval ) {
            switch( iHashType ) {
                case PHP_ODB_FETCH_ARRAY:
                case PHP_ODB_FETCH_ASSOC:
                    add_assoc_zval( return_value, pszKey, zData );
                    if( iHashType == PHP_ODB_FETCH_ASSOC ) break;
                case PHP_ODB_FETCH_ROW:
                    add_index_zval( return_value, uiIndex, zData );
                    break;
                case PHP_ODB_FETCH_OBJECT:
                    add_property_zval( return_value, pszKey, zData );
            }
        }
        else if( bAddString ) {
            switch( iHashType ) {
                case PHP_ODB_FETCH_ARRAY:
                case PHP_ODB_FETCH_ASSOC:
                    add_assoc_stringl( return_value, pszKey, pszData, ulDataLen, 1 );
                    if( iHashType == PHP_ODB_FETCH_ASSOC ) break;
                case PHP_ODB_FETCH_ROW:
                    add_index_stringl( return_value, uiIndex, pszData, ulDataLen, 1 );
                    break;
                case PHP_ODB_FETCH_OBJECT:
                    add_property_stringl( return_value, pszKey, pszData, ulDataLen, 1 );
            }
        }
    }
}
/* }}} */

/* {{{ php_odbtp_do_seek
 */
static void php_odbtp_do_seek(INTERNAL_FUNCTION_PARAMETERS, odbLONG lType)
{
    odbHANDLE    hQry;
    long         lRet;
    long         lRow;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usCol;
    zval*        zField;

    switch( lType ) {
        case PHP_ODB_SEEK_DATA:
            lRet = zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl",
                                         &rQry,
                                         &lRow );
            break;

        case PHP_ODB_SEEK_FIELD:
            lRet = zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz",
                                         &rQry,
                                         &zField );
            break;

        case PHP_ODB_SEEK_RESULT:
            lRet = zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz",
                                         &rQry,
                                         &lRow,
                                         &zField );
            break;
    }
    if( lRet == FAILURE ) return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( lType != PHP_ODB_SEEK_FIELD ) {
        if( !odbSeekRow( hQry, lRow + 1 ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            RETURN_FALSE;
        }
    }
    if( lType != PHP_ODB_SEEK_DATA ) {
        if( !(usCol = odbtp_get_col_num(hQry, zField TSRMLS_CC)) ) {
            RETURN_FALSE;
        }
        qry->col_pos = usCol - 1;
    }
    if( lType == PHP_ODB_SEEK_RESULT ) {
        if( !odbFetchRow( hQry ) ) {
            process_odbtp_error(hQry TSRMLS_CC);
            RETURN_FALSE;
        }
        if( odbNoData( hQry ) ) RETURN_NULL();

        qry->col_pos++;
        odbtp_get_field_value(return_value, hQry, usCol TSRMLS_CC);
        return;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ php_odbtp_do_row_op
 */
static void php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAMETERS, odbUSHORT usRowOp)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",
                              &rQry ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbDoRowOperation( hQry, usRowOp ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(odbtp)
{
	ZEND_INIT_MODULE_GLOBALS(odbtp, php_odbtp_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	le_connection = zend_register_list_destructors_ex(free_odbtp_connection, NULL, le_connection_name, module_number);
	le_query = zend_register_list_destructors_ex(free_odbtp_query, NULL, le_query_name, module_number);

	REGISTER_LONG_CONSTANT( "ODB_BINARY",   ODB_BINARY, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_BIGINT",   ODB_BIGINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_UBIGINT",  ODB_UBIGINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_BIT",      ODB_BIT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CHAR",     ODB_CHAR, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DATE",     ODB_DATE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DATETIME", ODB_DATETIME, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DOUBLE",   ODB_DOUBLE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FLOAT",    ODB_FLOAT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_GUID",     ODB_GUID, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_INT",      ODB_INT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_UINT",     ODB_UINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_NUMERIC",  ODB_NUMERIC, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_REAL",     ODB_REAL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_SMALLINT", ODB_SMALLINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_USMALLINT",ODB_USMALLINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TIME",     ODB_TIME, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TINYINT",  ODB_TINYINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_UTINYINT", ODB_UTINYINT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_WCHAR",    ODB_WCHAR, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_ATTR_DRIVER",         ODB_ATTR_DRIVER, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_FETCHROWCOUNT",  ODB_ATTR_FETCHROWCOUNT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_TRANSACTIONS",   ODB_ATTR_TRANSACTIONS, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DESCRIBEPARAMS", ODB_ATTR_DESCRIBEPARAMS, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_UNICODESQL",     ODB_ATTR_UNICODESQL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_FULLCOLINFO",    ODB_ATTR_FULLCOLINFO, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_QUERYTIMEOUT",   ODB_ATTR_QUERYTIMEOUT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_OICLEVEL",       ODB_ATTR_OICLEVEL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_TXNCAPABLE",     ODB_ATTR_TXNCAPABLE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_MAPCHARTOWCHAR", ODB_ATTR_MAPCHARTOWCHAR, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_VARDATASIZE",    ODB_ATTR_VARDATASIZE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_CACHEPROCS",     ODB_ATTR_CACHEPROCS, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_RIGHTTRIMTEXT",  ODB_ATTR_RIGHTTRIMTEXT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DSN",            ODB_ATTR_DSN, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DRIVERNAME",     ODB_ATTR_DRIVERNAME, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DRIVERVER",      ODB_ATTR_DRIVERVER, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DRIVERODBCVER",  ODB_ATTR_DRIVERODBCVER, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DBMSNAME",       ODB_ATTR_DBMSNAME, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DBMSVER",        ODB_ATTR_DBMSVER, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_SERVERNAME",     ODB_ATTR_SERVERNAME, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_USERNAME",       ODB_ATTR_USERNAME, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ATTR_DATABASENAME",   ODB_ATTR_DATABASENAME, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_DRIVER_UNKNOWN", ODB_DRIVER_UNKNOWN, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_MSSQL",   ODB_DRIVER_MSSQL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_JET",     ODB_DRIVER_JET, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_FOXPRO",  ODB_DRIVER_FOXPRO, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_ORACLE",  ODB_DRIVER_ORACLE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_SYBASE",  ODB_DRIVER_SYBASE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_MYSQL",   ODB_DRIVER_MYSQL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DRIVER_DB2",     ODB_DRIVER_DB2, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_TXN_NONE",            ODB_TXN_NONE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TXN_READUNCOMMITTED", ODB_TXN_READUNCOMMITTED, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TXN_READCOMMITTED",   ODB_TXN_READCOMMITTED, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TXN_REPEATABLEREAD",  ODB_TXN_REPEATABLEREAD, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TXN_SERIALIZABLE",    ODB_TXN_SERIALIZABLE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_TXN_DEFAULT",         ODB_TXN_DEFAULT, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_OIC_CORE",   ODB_OIC_CORE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_OIC_LEVEL1", ODB_OIC_LEVEL1, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_OIC_LEVEL2", ODB_OIC_LEVEL2, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_PARAM_NONE",      ODB_PARAM_NONE, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_PARAM_INPUT",     ODB_PARAM_INPUT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_PARAM_OUTPUT",    ODB_PARAM_OUTPUT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_PARAM_INOUT",     ODB_PARAM_INOUT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_PARAM_RETURNVAL", ODB_PARAM_RETURNVAL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_PARAM_RESULTCOL", ODB_PARAM_RESULTCOL, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_NULL",    ODB_NULL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_DEFAULT", ODB_DEFAULT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_IGNORE",  ODB_IGNORE, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_CURSOR_FORWARD", ODB_CURSOR_FORWARD, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CURSOR_STATIC",  ODB_CURSOR_STATIC, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CURSOR_KEYSET",  ODB_CURSOR_KEYSET, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CURSOR_DYNAMIC", ODB_CURSOR_DYNAMIC, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_CONCUR_DEFAULT",  ODB_CONCUR_DEFAULT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CONCUR_READONLY", ODB_CONCUR_READONLY, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CONCUR_LOCK",     ODB_CONCUR_LOCK, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CONCUR_ROWVER",   ODB_CONCUR_ROWVER, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_CONCUR_VALUES",   ODB_CONCUR_VALUES, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_FETCH_DEFAULT",  ODB_FETCH_DEFAULT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_FIRST",    ODB_FETCH_FIRST, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_LAST",     ODB_FETCH_LAST, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_NEXT",     ODB_FETCH_NEXT, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_PREV",     ODB_FETCH_PREV, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_ABS",      ODB_FETCH_ABS, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_REL",      ODB_FETCH_REL, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_BOOKMARK", ODB_FETCH_BOOKMARK, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_FETCH_REREAD",   ODB_FETCH_REREAD, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_ERROR",   ODB_ROWSTAT_ERROR, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_SUCCESS", ODB_ROWSTAT_SUCCESS, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_UPDATED", ODB_ROWSTAT_UPDATED, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_DELETED", ODB_ROWSTAT_DELETED, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_ADDED",   ODB_ROWSTAT_ADDED, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_NOROW",   ODB_ROWSTAT_NOROW, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "ODB_ROWSTAT_UNKNOWN", ODB_ROWSTAT_UNKNOWN, CONST_CS | CONST_PERSISTENT );

#if ODBTP_MSSQL /* mssql specific constants */
	REGISTER_LONG_CONSTANT( "MSSQL_ASSOC", MSSQL_ASSOC, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "MSSQL_NUM",   MSSQL_NUM, CONST_CS | CONST_PERSISTENT );
	REGISTER_LONG_CONSTANT( "MSSQL_BOTH",  MSSQL_BOTH, CONST_CS | CONST_PERSISTENT );

	REGISTER_LONG_CONSTANT("SQLTEXT",    ODB_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLVARCHAR", ODB_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLCHAR",    ODB_CHAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLINT1",    ODB_TINYINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLINT2",    ODB_SMALLINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLINT4",    ODB_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLBIT",     ODB_BIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLFLT4",    ODB_REAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLFLT8",    ODB_DOUBLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SQLFLTN",    ODB_NUMERIC, CONST_CS | CONST_PERSISTENT);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(odbtp)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(odbtp)
{
	ODBTP_G(default_link)=-1;
	ODBTP_G(last_error)=NULL;
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(odbtp)
{
    if( ODBTP_G(last_error) ) efree( ODBTP_G(last_error) );
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(odbtp)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "ODBTP (Open Database Transport Protocol) Support", "enabled");
	php_info_print_table_row(2, "ODBTP Library Version", ODBTP_LIB_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */


/* {{{ proto resource odbtp_connect(string host_or_interface[, string dbconnect_or_username[, string password[, string database]]])
   Connect to ODBTP service. */
PHP_FUNCTION(odbtp_connect)
{
    php_odbtp_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_LOGIN_NORMAL);
}
/* }}} */

/* {{{ proto resource odbtp_rconnect(string host_or_interface[, string dbconnect_or_username[, string password[, string database]]])
   Reserved connect to ODBTP service. */
PHP_FUNCTION(odbtp_rconnect)
{
    php_odbtp_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_LOGIN_RESERVED);
}
/* }}} */

/* {{{ proto resource odbtp_sconnect(string host_or_interface[, string dbconnect_or_username[, string password[, string database]]])
   Separate process connect to ODBTP service. */
PHP_FUNCTION(odbtp_sconnect)
{
    php_odbtp_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_LOGIN_SINGLE);
}
/* }}} */

/* {{{ proto string odbtp_version([resource odbtp_connection])
   Get ODBTP protocol version. */
PHP_FUNCTION(odbtp_version)
{
    php_odbtp_connect_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_VERSION);
}
/* }}} */

/* {{{ proto void odbtp_close([resource odbtp_connection[, bool disconnect_db]])
   Close connection to server. */
PHP_FUNCTION(odbtp_close)
{
    zend_bool         bDisconnectDb;
    odbtp_connection* con;
    odbHANDLE         hCon;
    long              id = -1;
    zval**            prCon = NULL;
    zval*             rCon = NULL;
    zval*             zDisconnectDb = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|rz",
                              &rCon,
                              &zDisconnectDb ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( (hCon = con->hCon) ) {
        odbHANDLE hQry;

        for( hQry = odbGetFirstQuery( hCon ); hQry; hQry = odbGetNextQuery( hCon ) )
            ((odbtp_query*)odbGetUserData( hQry ))->hQry = NULL;

        if( zDisconnectDb )
            bDisconnectDb = odbtp_read_zval_boolean( zDisconnectDb TSRMLS_CC );
        else
            bDisconnectDb = con->dont_pool_dbc;

        if( odbIsConnected( hCon ) && !odbLogout( hCon, bDisconnectDb ) ) {
            process_odbtp_error(hCon TSRMLS_CC);
        }
        odbFree( hCon );
        con->hCon = NULL;
    }
    if( rCon ) {
        zend_list_delete( Z_RESVAL_P(rCon) );
        if( Z_RESVAL_P(rCon) == ODBTP_G(default_link) )
            ODBTP_G(default_link) = -1;
    }
    else {
        zend_list_delete( ODBTP_G(default_link) );
        ODBTP_G(default_link) = -1;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string odbtp_connect_id([resource odbtp_connection])
   Get connection id. */
PHP_FUNCTION(odbtp_connect_id)
{
    php_odbtp_connect_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_ID);
}
/* }}} */

/* {{{ proto bool odbtp_load_data_types([resource odbtp_connection])
   Load data types. */
PHP_FUNCTION(odbtp_load_data_types)
{
    odbtp_connection* con;
    long              id = -1;
    zval**            prCon = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rCon ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbLoadDataTypes( con->hCon ) ) {
        process_odbtp_error(con->hCon TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool odbtp_use_row_cache([resource odbtp_connection[, bool use_cache[, long size]]])
   Enable / disable row caching. */
PHP_FUNCTION(odbtp_use_row_cache)
{
    zend_bool         bUseCache = TRUE;
    odbtp_connection* con;
    long              id = -1;
    long              lSize = 0;
    zval**            prCon = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|rbl",
                              &rCon,
                              &bUseCache,
                              &lSize ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbUseRowCache( con->hCon, bUseCache, lSize ) ) {
        process_odbtp_error(con->hCon TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto long odbtp_row_cache_size([resource odbtp_connection])
   Get row cache size. */
PHP_FUNCTION(odbtp_row_cache_size)
{
    php_odbtp_connect_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_ROW_CACHE_SIZE);
}
/* }}} */

/* {{{ proto bool odbtp_convert_all([resource odbtp_connection[, bool convert]])
   Enable / disable conversion of all fields to character type. */
PHP_FUNCTION(odbtp_convert_all)
{
    php_odbtp_set_option(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_OPTION_CONVERT_ALL);
}
/* }}} */

/* {{{ proto bool odbtp_convert_datetime([resource odbtp_connection[, bool convert]])
   Enable / disable conversion of datetime fields to character type. */
PHP_FUNCTION(odbtp_convert_datetime)
{
    php_odbtp_set_option(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_OPTION_CONVERT_DATETIME);
}
/* }}} */

/* {{{ proto bool odbtp_convert_guid([resource odbtp_connection[, bool convert]])
   Enable / disable conversion of GUID fields to character type. */
PHP_FUNCTION(odbtp_convert_guid)
{
    php_odbtp_set_option(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_OPTION_CONVERT_GUID);
}
/* }}} */

/* {{{ proto bool odbtp_dont_pool_dbc([resource odbtp_connection[, bool dont]])
   Enable / disable pooling of underlying database connection. */
PHP_FUNCTION(odbtp_dont_pool_dbc)
{
    php_odbtp_set_option(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_OPTION_DONT_POOL_DBC);
}
/* }}} */

/* {{{ proto mixed odbtp_get_attr( long attribute[, resource odbtp_connection])
   Gets attribute value */
PHP_FUNCTION(odbtp_get_attr)
{
    odbtp_connection* con;
    long              id = -1;
    long              lAttr;
    zval**            prCon = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|r",
                              &lAttr,
                              &rCon ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( odbIsTextAttr( lAttr ) ) {
        odbCHAR szVal[256];

        if( odbGetAttrText( con->hCon, lAttr, szVal, sizeof(szVal) ) )
            RETURN_STRING( szVal, 1 );
    }
    else {
        odbULONG ulVal;

        if( odbGetAttrLong( con->hCon, lAttr, &ulVal ) )
            RETURN_LONG( ulVal );
    }
    process_odbtp_error(con->hCon TSRMLS_CC);
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool odbtp_set_attr(long attribute, mixed value[, resource odbtp_connection])
   Sets attribute value */
PHP_FUNCTION(odbtp_set_attr)
{
    odbtp_connection* con;
    long              id = -1;
    long              lAttr;
    zval**            prCon = NULL;
    zval*             rCon = NULL;
    zval*             zVal;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz|r",
                              &lAttr,
                              &zVal,
                              &rCon ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( odbIsTextAttr( lAttr ) ) {
        char    sz[32];
        odbPSTR pszVal = odbtp_read_zval_string( zVal, sz, NULL TSRMLS_CC );

        if( odbSetAttrText( con->hCon, lAttr, pszVal ) )
            RETURN_TRUE;
    }
    else {
        odbULONG ulVal = (odbULONG)odbtp_read_zval_long( zVal TSRMLS_CC );

        if( odbSetAttrLong( con->hCon, lAttr, ulVal ) )
            RETURN_TRUE;
    }
    process_odbtp_error(con->hCon TSRMLS_CC);
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool odbtp_commit([resource odbtp_connection])
   Commit current transaction */
PHP_FUNCTION(odbtp_commit)
{
    odbtp_connection* con;
    long              id = -1;
    zval**            prCon = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rCon ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbCommit( con->hCon ) ) {
        process_odbtp_error(con->hCon TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool odbtp_rollback([resource odbtp_connection])
   Rollback current transaction */
PHP_FUNCTION(odbtp_rollback)
{
    odbtp_connection* con;
    long              id = -1;
    zval**            prCon = NULL;
    zval*             rCon = NULL;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|r",
                              &rCon ) == FAILURE )
        return;

    if( rCon )
        prCon = &rCon;
    else
        id = ODBTP_G(default_link);

    ZEND_FETCH_RESOURCE( con, odbtp_connection*, prCon, id, le_connection_name, le_connection );

    if( !con->hCon ) {
        process_odbtp_invalid(le_connection_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbRollback( con->hCon ) ) {
        process_odbtp_error(con->hCon TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string odbtp_get_message([resource odbtp_handle])
   Get current message from server for odbtp resource. */
PHP_FUNCTION(odbtp_get_message)
{
    php_odbtp_get_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, FALSE);
}
/* }}} */

/* {{{ proto string odbtp_get_error([resource odbtp_handle])
   Get current error message for odbtp resource. */
PHP_FUNCTION(odbtp_get_error)
{
    php_odbtp_get_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, TRUE);
}
/* }}} */

/* {{{ proto string odbtp_last_error([resource odbtp_connection])
   Gets the last ODBTP error message */
PHP_FUNCTION(odbtp_last_error)
{
    php_odbtp_get_last_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_LERROR_STRING);
}
/* }}} */

/* {{{ proto string odbtp_last_error_state([resource odbtp_connection])
   Gets the last ODBTP error state */
PHP_FUNCTION(odbtp_last_error_state)
{
    php_odbtp_get_last_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_LERROR_STATE);
}
/* }}} */

/* {{{ proto long odbtp_last_error_code([resource odbtp_connection])
   Gets the last ODBTP error code */
PHP_FUNCTION(odbtp_last_error_code)
{
    php_odbtp_get_last_error(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_LERROR_CODE);
}
/* }}} */

/* {{{ proto resource odbtp_allocate_query([resource odbtp_connection])
   Allocate a query resource */
PHP_FUNCTION(odbtp_allocate_query)
{
    php_odbtp_obtain_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, TRUE);
}
/* }}} */

/* {{{ proto resource odbtp_get_query([resource odbtp_connection[, long query_id]])
   Get query resource */
PHP_FUNCTION(odbtp_get_query)
{
    php_odbtp_obtain_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, FALSE);
}
/* }}} */

/* {{{ proto long odbtp_query_id(resource odbtp_query)
   Get query id */
PHP_FUNCTION(odbtp_query_id)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_ID);
}
/* }}} */

/* {{{ proto bool odbtp_set_cursor(resource odbtp_query[, long type[, long concurrency[, bool enable_bookmarks]]])
   Set cursor type */
PHP_FUNCTION(odbtp_set_cursor)
{
    zend_bool    bEnableBMs = FALSE;
    odbHANDLE    hQry;
    odbLONG      lConcur = ODB_CONCUR_DEFAULT;
    odbLONG      lType = ODB_CURSOR_FORWARD;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|llb",
                              &rQry,
                              &lType,
                              &lConcur,
                              &bEnableBMs ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbSetCursor( hQry, (odbUSHORT)lType, (odbUSHORT)lConcur, bEnableBMs ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto resource odbtp_query( string query[, resource odbtp_handle])
   Executes database query */
PHP_FUNCTION(odbtp_query)
{
    php_odbtp_do_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_QUERY_EXECUTE);
}
/* }}} */

/* {{{ proto resource odbtp_prepare( string query[, resource odbtp_handle])
   Prepares a database query for later execution */
PHP_FUNCTION(odbtp_prepare)
{
    php_odbtp_do_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_QUERY_PREPARE);
}
/* }}} */

/* {{{ proto resource odbtp_prepare_proc( string procedure[, resource odbtp_handle])
   Prepares a stored procedure later execution */
PHP_FUNCTION(odbtp_prepare_proc)
{
    php_odbtp_do_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_QUERY_PREPARE_PROC);
}
/* }}} */

/* {{{ proto bool odbtp_execute(resource odbtp_query[, bool skip_results])
   Executes previously prepared query */
PHP_FUNCTION(odbtp_execute)
{
    odbBOOL      bSkipResults = FALSE;
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|b",
                              &rQry,
                              &bSkipResults ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    odbtp_unattach_cols(qry TSRMLS_CC);
    qry->col_pos = 0;

    if( !odbtp_set_input_params(qry TSRMLS_CC) ) {
        RETURN_FALSE;
    }
    if( !odbExecute( hQry, NULL ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    if( bSkipResults ) {
        do {
            if( !odbFetchNextResult( hQry ) ) {
                process_odbtp_error(hQry TSRMLS_CC);
                RETURN_FALSE;
            }
        }
        while( !odbNoData( hQry ) );
    }
    if( !odbGetTotalCols( hQry ) && !odbtp_get_output_params(qry TSRMLS_CC) ) {
        RETURN_FALSE;
    }
    ZVAL_RESOURCE(return_value, qry->link);
    zend_list_addref( qry->link );
}
/* }}} */

/* {{{ proto long odbtp_num_params(resource odbtp_query)
   Get number of parameters in prepared query */
PHP_FUNCTION(odbtp_num_params)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_TOTALPARAMS);
}
/* }}} */

/* {{{ proto bool odbtp_attach_param(resource odbtp_query, mixed param_id, mixed var[, long data_type[, long length]])
   Attach variable to parameter from prepared statement */
PHP_FUNCTION(odbtp_attach_param)
{
    php_odbtp_attach_variable(INTERNAL_FUNCTION_PARAM_PASSTHRU, FALSE);
}
/* }}} */

/* {{{ proto long odbtp_type_param(resource odbtp_query, mixed param_id)
   Get the type of the parameter from prepared statement */
PHP_FUNCTION(odbtp_type_param)
{
    php_odbtp_param_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_PARAMTYPE);
}
/* }}} */

/* {{{ proto string odbtp_param_name(resource odbtp_query, mixed param_id)
   Get param name from prepared statement */
PHP_FUNCTION(odbtp_param_name)
{
    php_odbtp_param_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_NAME);
}
/* }}} */

/* {{{ proto long odbtp_param_bindtype(resource odbtp_query, mixed param_id)
   Get param ODB data type from prepared statement */
PHP_FUNCTION(odbtp_param_bindtype)
{
    php_odbtp_param_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_BINDTYPE);
}
/* }}} */

/* {{{ proto string odbtp_param_type(resource odbtp_query, mixed param_id)
   Get param type name from prepared statement */
PHP_FUNCTION(odbtp_param_type)
{
    php_odbtp_param_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_TYPENAME);
}
/* }}} */

/* {{{ proto long odbtp_param_length(resource odbtp_query, mixed param_id)
   Get param length from prepared statement */
PHP_FUNCTION(odbtp_param_length)
{
    php_odbtp_param_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_LENGTH);
}
/* }}} */

/* {{{ proto long odbtp_param_number(resource odbtp_query, mixed param_id)
   Get param number from prepared statement */
PHP_FUNCTION(odbtp_param_number)
{
    php_odbtp_param_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_NUMBER);
}
/* }}} */

/* {{{ proto bool odbtp_input(resource odbtp_query, mixed param_id[, long data_type[, long length[, mixed sql_type[, long size[, long precision]]]]])
   Bind parameter as input */
PHP_FUNCTION(odbtp_input)
{
    php_odbtp_bind_param(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_PARAM_INPUT);
}
/* }}} */

/* {{{ proto bool odbtp_output(resource odbtp_query, mixed param_id[, long data_type[, long length[, mixed sql_type[, long size[, long precision]]]]])
   Bind parameter as output */
PHP_FUNCTION(odbtp_output)
{
    php_odbtp_bind_param(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_PARAM_OUTPUT);
}
/* }}} */

/* {{{ proto bool odbtp_inout(resource odbtp_query, mixed param_id[, long data_type[, long length[, mixed sql_type[, long size[, long precision]]]]])
   Bind parameter as inout */
PHP_FUNCTION(odbtp_inout)
{
    php_odbtp_bind_param(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_PARAM_INOUT);
}
/* }}} */

/* {{{ proto bool odbtp_set(resource odbtp_query, mixed param_id[, mixed data])
   Set parameter value */
PHP_FUNCTION(odbtp_set)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usParam;
    zval*        zData = NULL;
    zval*        zParam;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|z",
                              &rQry,
                              &zParam,
                              &zData ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usParam = odbtp_get_param_num(hQry, zParam TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    if( !odbtp_set_param(hQry, usParam, zData, TRUE TSRMLS_CC) ) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto mixed odbtp_get(resource odbtp_query, mixed param_id)
   Get parameter value */
PHP_FUNCTION(odbtp_get)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usParam;
    zval*        zParam;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz",
                              &rQry,
                              &zParam ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usParam = odbtp_get_param_num(hQry, zParam TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    if( !odbGetParam( hQry, usParam, TRUE ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    odbtp_get_param_value(return_value, hQry, usParam TSRMLS_CC);
}
/* }}} */

/* {{{ proto long odbtp_num_fields(resource odbtp_query)
   Get number of fields in result */
PHP_FUNCTION(odbtp_num_fields)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_TOTALCOLS);
}
/* }}} */

/* {{{ proto bool odbtp_bind_field(resource odbtp_query, mixed field_id[, long data_type[, long length]])
   Bind field to specified data type */
PHP_FUNCTION(odbtp_bind_field)
{
    odbHANDLE    hQry;
    long         lDataLen = 0;
    long         lDataType = 0;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usCol;
    zval*        zField;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|ll",
                              &rQry,
                              &zField,
                              &lDataType,
                              &lDataLen ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usCol = odbtp_get_col_num(hQry, zField TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    if( !odbBindCol( hQry, usCol, (odbSHORT)lDataType, lDataLen, TRUE ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool odbtp_attach_field(resource odbtp_query, mixed field_id, mixed var[, long data_type[, long length]])
   Attach variable to field from result set */
PHP_FUNCTION(odbtp_attach_field)
{
    php_odbtp_attach_variable(INTERNAL_FUNCTION_PARAM_PASSTHRU, TRUE);
}
/* }}} */

/* {{{ proto object odbtp_fetch_field(resource odbtp_query[, mixed field_id])
   Get field info from current result */
PHP_FUNCTION(odbtp_fetch_field)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_ALL);
}
/* }}} */

/* {{{ proto string odbtp_field_name(resource odbtp_query[, mixed field_id])
   Get field name from current result */
PHP_FUNCTION(odbtp_field_name)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_NAME);
}
/* }}} */

/* {{{ proto long odbtp_field_bindtype(resource odbtp_query[, mixed field_id])
   Get field ODB data type from current result */
PHP_FUNCTION(odbtp_field_bindtype)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_BINDTYPE);
}
/* }}} */

/* {{{ proto string odbtp_field_type(resource odbtp_query[, mixed field_id])
   Get field type name from current result */
PHP_FUNCTION(odbtp_field_type)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_TYPENAME);
}
/* }}} */

/* {{{ proto long odbtp_field_length(resource odbtp_query[, mixed field_id])
   Get field length from current result */
PHP_FUNCTION(odbtp_field_length)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_LENGTH);
}
/* }}} */

/* {{{ proto long odbtp_field_table(resource odbtp_query[, mixed field_id])
   Get field table from current result */
PHP_FUNCTION(odbtp_field_table)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_TABLE);
}
/* }}} */

/* {{{ proto long odbtp_field_schema(resource odbtp_query[, mixed field_id])
   Get field's table schema from current result */
PHP_FUNCTION(odbtp_field_schema)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_SCHEMA);
}
/* }}} */

/* {{{ proto long odbtp_field_catalog(resource odbtp_query[, mixed field_id])
   Get field's table catalog from current result */
PHP_FUNCTION(odbtp_field_catalog)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_CATALOG);
}
/* }}} */

/* {{{ proto long odbtp_field_flags(resource odbtp_query[, mixed field_id])
   Get field flags from current result */
PHP_FUNCTION(odbtp_field_flags)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_FLAGS);
}
/* }}} */

/* {{{ proto long odbtp_field_basename(resource odbtp_query[, mixed field_id])
   Get field base name from current result */
PHP_FUNCTION(odbtp_field_basename)
{
    php_odbtp_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_BASENAME);
}
/* }}} */

/* {{{ proto long odbtp_num_rows(resource odbtp_query)
   Get number of rows in result */
PHP_FUNCTION(odbtp_num_rows)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_TOTALROWS);
}
/* }}} */

/* {{{ proto bool odbtp_fetch(resource odbtp_query[, long fetch_type[, long fetch_param]])
   Fetch row */
PHP_FUNCTION(odbtp_fetch)
{
    php_odbtp_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_FETCH_NONE);
}
/* }}} */

/* {{{ proto array odbtp_fetch_row(resource odbtp_query[, long fetch_type[, long fetch_param]])
   Fetch row as enumerated array */
PHP_FUNCTION(odbtp_fetch_row)
{
    php_odbtp_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_FETCH_ROW);
}
/* }}} */

/* {{{ proto array odbtp_fetch_assoc(resource odbtp_query[, long fetch_type[, long fetch_param]])
   Fetch row as an associative array */
PHP_FUNCTION(odbtp_fetch_assoc)
{
    php_odbtp_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_FETCH_ASSOC);
}
/* }}} */

/* {{{ proto array odbtp_fetch_array(resource odbtp_query[, long fetch_type[, long fetch_param]])
   Fetch row as both an enumerated and associative array */
PHP_FUNCTION(odbtp_fetch_array)
{
    php_odbtp_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_FETCH_ARRAY);
}
/* }}} */

/* {{{ proto object odbtp_fetch_object(resource odbtp_query[, long fetch_type[, long fetch_param]])
   Fetch row as an object */
PHP_FUNCTION(odbtp_fetch_object)
{
    php_odbtp_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_FETCH_OBJECT);
}
/* }}} */

/* {{{ proto mixed odbtp_field(resource odbtp_query, mixed field_id[, mixed new_value])
   Get/Set current field value */
PHP_FUNCTION(odbtp_field)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;
    char         szBuf[64];
    odbUSHORT    usCol;
    zval*        zData = NULL;
    zval*        zField;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|z",
                              &rQry,
                              &zField,
                              &zData ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usCol = odbtp_get_col_num(hQry, zField TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    if( !zData ) {
        odbtp_get_field_value(return_value, hQry, usCol TSRMLS_CC);
    }
    else {
        odbBOOL      bOK;
        odbBYTE      byData;
        odbDOUBLE    dData;
        odbFLOAT     fData;
        odbGUID      guidData;
        odbLONG      lDataLen;
        odbPVOID     pData;
        odbTIMESTAMP tsData;
        odbULONG     ulData;
        odbULONGLONG ullData;
        odbUSHORT    usData;

        if( Z_TYPE_P(zData) == IS_NULL ) {
            if( !odbSetColNull( hQry, usCol, TRUE ) ) {
                process_odbtp_error(hQry TSRMLS_CC);
                RETURN_FALSE;
            }
            RETURN_TRUE;
        }
        switch( odbColDataType( hQry, usCol ) ) {
            case ODB_BIT:
                byData = odbtp_read_zval_boolean( zData TSRMLS_CC );
                bOK = odbSetColByte( hQry, usCol, byData, TRUE );
                break;

            case ODB_TINYINT:
                byData = (odbCHAR)odbtp_read_zval_long( zData TSRMLS_CC );
                bOK = odbSetColByte( hQry, usCol, byData, TRUE );
                break;

            case ODB_UTINYINT:
                byData = (odbBYTE)odbtp_read_zval_long( zData TSRMLS_CC );
                bOK = odbSetColByte( hQry, usCol, byData, TRUE );
                break;

            case ODB_SMALLINT:
                usData = (odbSHORT)odbtp_read_zval_long( zData TSRMLS_CC );
                bOK = odbSetColShort( hQry, usCol, usData, TRUE );
                break;

            case ODB_USMALLINT:
                usData = (odbUSHORT)odbtp_read_zval_long( zData TSRMLS_CC );
                bOK = odbSetColShort( hQry, usCol, usData, TRUE );
                break;

            case ODB_INT:
            case ODB_UINT:
                ulData = odbtp_read_zval_long( zData TSRMLS_CC );
                bOK = odbSetColLong( hQry, usCol, ulData, TRUE );
                break;

            case ODB_BIGINT:
                ullData = odbStrToLongLong( odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC ) );
                bOK = odbSetColLongLong( hQry, usCol, ullData, TRUE );
                break;

            case ODB_UBIGINT:
                ullData = odbStrToULongLong( odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC ) );
                bOK = odbSetColLongLong( hQry, usCol, ullData, TRUE );
                break;

            case ODB_REAL:
                fData = (float)odbtp_read_zval_double( zData TSRMLS_CC );
                bOK = odbSetColFloat( hQry, usCol, fData, TRUE );
                break;

            case ODB_DOUBLE:
                dData = odbtp_read_zval_double( zData TSRMLS_CC );
                bOK = odbSetColDouble( hQry, usCol, dData, TRUE );
                break;

            case ODB_DATETIME:
                if( Z_TYPE_P(zData) == IS_OBJECT ) {
                    object_to_odbtp_timestamp(&tsData, zData TSRMLS_CC);
                }
                else if( Z_TYPE_P(zData) == IS_STRING ) {                pData = odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC );;
                    pData = odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC );;
                    odbStrToTimestamp( &tsData, (odbPCSTR)pData );
                }
                else {
                    ulData = odbtp_read_zval_long( zData TSRMLS_CC );
                    odbCTimeToTimestamp( &tsData, (odbLONG)ulData );
                }
                bOK = odbSetColTimestamp( hQry, usCol, &tsData, TRUE );
                break;

            case ODB_GUID:
                if( Z_TYPE_P(zData) == IS_OBJECT ) {
                    object_to_odbtp_guid(&guidData, zData TSRMLS_CC);
                }
                else {
                    pData = odbtp_read_zval_string( zData, szBuf, NULL TSRMLS_CC );;
                    odbStrToGuid( &guidData, (odbPCSTR)pData );
                }
                bOK = odbSetColGuid( hQry, usCol, &guidData, TRUE );
                break;

            default:
                pData = odbtp_read_zval_string( zData, szBuf, &lDataLen TSRMLS_CC );
                bOK = odbSetCol( hQry, usCol, pData, lDataLen, TRUE );
        }
        if( !bOK ) {
            process_odbtp_error(hQry TSRMLS_CC);
            RETURN_FALSE;
        }
        RETURN_TRUE;
    }
}
/* }}} */

/* {{{ proto bool odbtp_field_ignore(resource odbtp_query, mixed field_id)
   Ignore field */
PHP_FUNCTION(odbtp_field_ignore)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usCol;
    zval*        zField;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz",
                              &rQry,
                              &zField ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !(usCol = odbtp_get_col_num(hQry, zField TSRMLS_CC)) ) {
        RETURN_FALSE;
    }
    if( !odbSetColIgnore( hQry, usCol, TRUE ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool odbtp_row_refresh(resource odbtp_query)
   Refresh the current row */
PHP_FUNCTION(odbtp_row_refresh)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_REFRESH);
}
/* }}} */

/* {{{ proto bool odbtp_row_update(resource odbtp_query)
   Update the current row */
PHP_FUNCTION(odbtp_row_update)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_UPDATE);
}
/* }}} */

/* {{{ proto bool odbtp_row_delete(resource odbtp_query)
   Delete the current row */
PHP_FUNCTION(odbtp_row_delete)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_DELETE);
}
/* }}} */

/* {{{ proto bool odbtp_row_add(resource odbtp_query)
   Add a new row */
PHP_FUNCTION(odbtp_row_add)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_ADD);
}
/* }}} */

/* {{{ proto bool odbtp_row_lock(resource odbtp_query)
   Lock the current row */
PHP_FUNCTION(odbtp_row_lock)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_LOCK);
}
/* }}} */

/* {{{ proto bool odbtp_row_unlock(resource odbtp_query)
   Unlock the current row */
PHP_FUNCTION(odbtp_row_unlock)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_UNLOCK);
}
/* }}} */

/* {{{ proto bool odbtp_row_bookmark(resource odbtp_query)
   Use the current row for bookmark fetch */
PHP_FUNCTION(odbtp_row_bookmark)
{
    php_odbtp_do_row_op(INTERNAL_FUNCTION_PARAM_PASSTHRU, ODB_ROW_BOOKMARK);
}
/* }}} */

/* {{{ proto long odbtp_row_status(resource odbtp_query)
   Get the current row's status */
PHP_FUNCTION(odbtp_row_status)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_ROWSTATUS);
}
/* }}} */

/* {{{ proto long odbtp_affected_rows(resource odbtp_query)
   Get affected row count */
PHP_FUNCTION(odbtp_affected_rows)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_ROWCOUNT);
}
/* }}} */

/* {{{ proto array odbtp_fetch_output(resource odbtp_query)
   Fetch output parameters as an array */
PHP_FUNCTION(odbtp_fetch_output)
{
    odbBOOL      bAddLong;
    odbBOOL      bAddDouble;
    odbBOOL      bAddString;
    odbBOOL      bData;
    odbDOUBLE    dData;
    odbHANDLE    hQry;
    long         lData;
    odbPVOID     pData;
    char*        pszData;
    odbtp_query* qry;
    zval*        rQry;
    char         sz[24];
    uint         uiIndex;
    odbULONG     ulDataLen;
    odbUSHORT    usParam;
    odbUSHORT    usTotalParams;
    zval*        zData;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",
                              &rQry ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( (usTotalParams = odbGetTotalParams( hQry )) == 0 ) RETURN_FALSE;

    if( !odbGetOutputParams( hQry ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    if( array_init(return_value) == FAILURE ) {
        RETURN_FALSE;
    }
    for( usParam = 1; usParam <= usTotalParams; usParam++ ) {
        if( !(odbParamType( hQry, usParam ) & ODB_PARAM_OUTPUT) ||
            !odbGotParam( hQry, usParam ) )
        {
            continue;
        }
        uiIndex = usParam;

        if( !(pData = odbParamData( hQry, usParam )) ) {
            add_index_null( return_value, uiIndex );
            continue;
        }
        bAddLong = FALSE;
        bAddDouble = FALSE;
        bAddString = FALSE;

        switch( odbParamDataType( hQry, usParam ) ) {
            case ODB_BIT:
                bData = *((odbPBYTE)pData) != 0 ? TRUE : FALSE;
                add_index_bool( return_value, uiIndex, bData );
                break;

            case ODB_TINYINT:
                lData = *((odbPSTR)pData); bAddLong = TRUE; break;
            case ODB_UTINYINT:
                lData = *((odbPBYTE)pData); bAddLong = TRUE; break;

            case ODB_SMALLINT:
                lData = *((odbPSHORT)pData); bAddLong = TRUE; break;
            case ODB_USMALLINT:
                lData = *((odbPUSHORT)pData); bAddLong = TRUE; break;

            case ODB_INT:
                lData = *((odbPLONG)pData); bAddLong = TRUE; break;
            case ODB_UINT:
                lData = *((odbPULONG)pData); bAddLong = TRUE; break;

            case ODB_BIGINT:
                pszData = odbLongLongToStr( *((odbPLONGLONG)pData), &sz[23] );
                ulDataLen = strlen( pszData ); bAddString = TRUE; break;
            case ODB_UBIGINT:
                pszData = odbULongLongToStr( *((odbPULONGLONG)pData), &sz[23] );
                ulDataLen = strlen( pszData ); bAddString = TRUE; break;

            case ODB_REAL:
                dData = *((odbPFLOAT)pData); bAddDouble = TRUE; break;
            case ODB_DOUBLE:
                dData = *((odbPDOUBLE)pData); bAddDouble = TRUE; break;

            case ODB_DATETIME:
                MAKE_STD_ZVAL( zData );
                odbtp_set_zval_to_timestamp(zData, (odbPTIMESTAMP)pData TSRMLS_CC);
                add_index_zval( return_value, uiIndex, zData );
                break;

            case ODB_GUID:
                MAKE_STD_ZVAL( zData );
                odbtp_set_zval_to_guid(zData, (odbPGUID)pData TSRMLS_CC);
                add_index_zval( return_value, uiIndex, zData );
                break;

            default:
                if( ODBTP_G(truncation_errors) &&
                    odbParamTruncated( hQry, usParam ) )
                {
                    process_odbtp_truncation(hQry, usParam, FALSE TSRMLS_CC);
                }
                ulDataLen = odbParamDataLen( hQry, usParam );
                pszData = (char*)pData; bAddString = TRUE; break;
        }
        if( bAddLong ) {
            add_index_long( return_value, uiIndex, lData );
        }
        else if( bAddDouble ) {
            add_index_double( return_value, uiIndex, dData );
        }
        else if( bAddString ) {
            add_index_stringl( return_value, uiIndex, pszData, ulDataLen, 1 );
        }
    }
}
/* }}} */

/* {{{ proto bool odbtp_next_result(resource odbtp_query)
   Get next query result */
PHP_FUNCTION(odbtp_next_result)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",
                              &rQry ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    odbtp_unattach_cols(qry TSRMLS_CC);
    qry->col_pos = 0;

    if( !odbFetchNextResult( hQry ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    if( odbNoData( hQry ) ) {
        odbtp_get_output_params(qry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto void odbtp_free_query(resource odbtp_query)
   Free previosly allocated query resource. */
PHP_FUNCTION(odbtp_free_query)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",
                              &rQry ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( (hQry = qry->hQry) ) {
        odbHANDLE hCon = odbGetConnection( hQry );

        if( hCon ) {
            odbtp_connection* con = (odbtp_connection*)odbGetUserData( hCon );
            if( qry == con->default_qry ) con->default_qry = NULL;
            odbDropQry( hQry );
        }
        odbtp_unattach_cols(qry TSRMLS_CC);
        odbtp_unattach_params(qry TSRMLS_CC);
        odbFree( hQry );
        qry->hQry = NULL;
    }
    zend_list_delete( Z_RESVAL_P(rQry) );
}
/* }}} */

/* {{{ proto object odbtp_new_datetime()
   Create a new datetime object. */
PHP_FUNCTION(odbtp_new_datetime)
{
    odbTIMESTAMP ts;

    memset( (char*)&ts, 0, sizeof(odbTIMESTAMP) );

    if( !odbtp_timestamp_to_object(return_value, &ts TSRMLS_CC) ) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto long odbtp_datetime2ctime(object datetime)
   Convert a datetime object into a C Time value. */
PHP_FUNCTION(odbtp_datetime2ctime)
{
    odbTIMESTAMP tsTime;
    zval*        zTime;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o",
                              &zTime ) == FAILURE )
        return;

    object_to_odbtp_timestamp(&tsTime, zTime TSRMLS_CC);

    RETURN_LONG( odbTimestampToCTime( &tsTime ) );
}
/* }}} */

/* {{{ proto object odbtp_ctime2datetime(long ctime_val)
   Convert a C Time value into a datetime object. */
PHP_FUNCTION(odbtp_ctime2datetime)
{
    long         lTime;
    odbTIMESTAMP tsTime;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                              &lTime ) == FAILURE )
        return;

    odbCTimeToTimestamp( &tsTime, lTime );

    if( !odbtp_timestamp_to_object(return_value, &tsTime TSRMLS_CC) ) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto bool odbtp_detach(resource odbtp_query)
   Detach query object */
PHP_FUNCTION(odbtp_detach)
{
    odbHANDLE    hCon;
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",
                              &rQry ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( (hCon = odbGetConnection( hQry )) ) {
        odbtp_connection* con = (odbtp_connection*)odbGetUserData( hCon );
        if( qry == con->default_qry ) con->default_qry = NULL;
        odbDetachQry( hQry );
    }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool odbtp_is_detached(resource odbtp_query)
   Determine if query is detached */
PHP_FUNCTION(odbtp_is_detached)
{
    php_odbtp_query_info(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_INFO_DETACHED);
}
/* }}} */

/* {{{ proto bool odbtp_data_seek(resource odbtp_query, long row)
   Seek to specified row. */
PHP_FUNCTION(odbtp_data_seek)
{
    php_odbtp_do_seek(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_SEEK_DATA);
}
/* }}} */

/* {{{ proto bool odbtp_field_seek(resource odbtp_query, mixed field_id)
   Move to specified field offset. */
PHP_FUNCTION(odbtp_field_seek)
{
    php_odbtp_do_seek(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_SEEK_FIELD);
}
/* }}} */

/* {{{ proto mixed odbtp_result(resource odbtp_query, long row, mixed field_id)
   Get result data corresponding to row and field. */
PHP_FUNCTION(odbtp_result)
{
    php_odbtp_do_seek(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_SEEK_RESULT);
}
/* }}} */

/* {{{ proto long odbtp_fetch_batch(resource odbtp_query)
   Fetch the next batch of records */
PHP_FUNCTION(odbtp_fetch_batch)
{
    odbHANDLE    hQry;
    odbtp_query* qry;
    zval*        rQry;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r",
                              &rQry ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !odbFetchRowsIntoCache( hQry ) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_LONG( odbGetTotalRows( hQry ) );
}
/* }}} */

/* {{{ proto resource odbtp_select_db( string database[, resource odbtp_handle])
   Select database */
PHP_FUNCTION(odbtp_select_db)
{
    php_odbtp_do_query(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_ODB_QUERY_SELECT_DB);
}
/* }}} */

/* {{{ proto bool odbtp_bind(resource odbtp_query, string param_name, mixed var[, int type[, bool is_output[, bool is_null[, long maxlen]]]])
    Binds a parameter from a stored procedure or a remote stored procedure */
PHP_FUNCTION(odbtp_bind)
{
    zend_bool    bIsOutput = FALSE; /* ignored, included for mssql_bind compat. */
    zend_bool    bIsNull = FALSE; /* ignored, included for mssql_bind compat. */
    odbHANDLE    hQry;
    int          iParamLen;
    long         lMaxLen = 0; /* ignored, included for mssql_bind compat. */
    char*        pszParam;
    odbtp_query* qry;
    zval*        rQry;
    odbUSHORT    usParam;
    zval*        zType = NULL; /* ignored, included for mssql_bind compat. */
    zval*        zVar;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsz|zbbl",
                              &rQry,
                              &pszParam, &iParamLen,
                              &zVar,
                              &zType,
                              &bIsOutput,
                              &bIsNull,
                              &lMaxLen ) == FAILURE )
        return;

    ZEND_FETCH_RESOURCE( qry, odbtp_query*, &rQry, -1, le_query_name, le_query );

    if( !(hQry = qry->hQry) ) {
        process_odbtp_invalid(le_query_name TSRMLS_CC);
        RETURN_FALSE;
    }
    if( iParamLen == 0 ) {
        php_odbtp_invalid(hQry, "Parameter name not specified" TSRMLS_CC);
        RETURN_FALSE;
    }
    if( !strcasecmp( pszParam, "RETVAL" ) ) pszParam = "@RETURN_VALUE";

    if( !(usParam = odbParamNum( hQry, pszParam )) ) {
        process_odbtp_error(hQry TSRMLS_CC);
        RETURN_FALSE;
    }
    odbSetParamUserData( hQry, usParam, zVar );
    qry->attached_params = TRUE;
    ZVAL_ADDREF(zVar);

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string odbtp_guid_string(mixed guid [,bool short_format])
   Converts a GUID to its character string format */
PHP_FUNCTION(odbtp_guid_string)
{
    zend_bool bShortFormat = FALSE;
    int       i;
    long      lGuidStrLen = 36;
    char*     psz;
    char*     pszGuid;
    char      szGuid[40];
    zval*     zData;

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b",
                              &zData,
                              &bShortFormat ) == FAILURE )
        return;

    if( Z_TYPE_P(zData) == IS_OBJECT ) {
        odbGUID guidData;

        object_to_odbtp_guid(&guidData, zData TSRMLS_CC);
        pszGuid = odbGuidToStr( szGuid, &guidData );
    }
    else if( Z_TYPE_P(zData) == IS_STRING && Z_STRLEN_P(zData) == 36 ) {
        pszGuid = Z_STRVAL_P(zData);
    }
    else if( Z_TYPE_P(zData) == IS_STRING && Z_STRLEN_P(zData) == 16 ) {
        char* pGuid = Z_STRVAL_P(zData);

        psz = szGuid;

        for( i = 3; i >= 0; i-- ) {
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0xF0) >> 4 ];
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0x0F) ];
        }
        *(psz++) = '-';

        for( i = 5; i >= 4; i-- ) {
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0xF0) >> 4 ];
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0x0F) ];
        }
        *(psz++) = '-';

        for( i = 7; i >= 6; i-- ) {
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0xF0) >> 4 ];
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0x0F) ];
        }
        *(psz++) = '-';

        for( i = 8; i <= 9; i++ ) {
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0xF0) >> 4 ];
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0x0F) ];
        }
        *(psz++) = '-';

        for( i = 10; i <= 15; i++ ) {
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0xF0) >> 4 ];
            *(psz++) = odbHexTable[ (*(pGuid + i) & 0x0F) ];
        }
        pszGuid = szGuid;
    }
    else {
        php_odbtp_invalid(NULL, "Invalid GUID" TSRMLS_CC);
        RETURN_FALSE;
    }
    if( bShortFormat ) {
        odbPUSHORT pusGuid;
        odbUSHORT  usTemp;

        psz = szGuid;
        for( i = 0; i < 36; i++ ) {
            if( *(pszGuid + i) != '-' ) *(psz++) = *(pszGuid + i);
        }
        pusGuid = (odbPUSHORT)szGuid;

        usTemp = *(pusGuid + 3);
        *(pusGuid + 3) = *(pusGuid + 0);
        *(pusGuid + 0) = usTemp;
        usTemp = *(pusGuid + 2);
        *(pusGuid + 2) = *(pusGuid + 1);
        *(pusGuid + 1) = usTemp;

        usTemp = *(pusGuid + 5);
        *(pusGuid + 5) = *(pusGuid + 4);
        *(pusGuid + 4) = usTemp;

        usTemp = *(pusGuid + 7);
        *(pusGuid + 7) = *(pusGuid + 6);
        *(pusGuid + 6) = usTemp;

        lGuidStrLen = 32;
        pszGuid = szGuid;
    }
    RETURN_STRINGL( pszGuid, lGuidStrLen, 1 );
}
/* }}} */

/* {{{ proto string odbtp_flags(long sqltype, string sqltypename, long nullable)
   Determine column flags corresponding to column attributes */
PHP_FUNCTION(odbtp_flags)
{
    int   i;
    int   iSqlTypeNameLen;
    long  lNullable;
    long  lSqlType;
    char* psz;
    char* pszSqlTypeName;
    char  sz[128];

    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsl",
                              &lSqlType,
                              &pszSqlTypeName, &iSqlTypeNameLen,
                              &lNullable ) == FAILURE )
        return;

    for( psz = pszSqlTypeName, i = 0; *psz && i < 127; psz++, i++ )
        sz[i] = tolower( *psz );
    sz[i] = 0;

    if( strstr( sz, "identity" ) || !strcmp( sz, "counter" ) )
        strcpy( sz, "auto_increment " );
    else
        strcpy( sz, "" );

    if( !lNullable )
        strcat( sz, "not_null " );

    switch( lSqlType ) {
        case SQL_BIT:
        case SQL_TINYINT:
            strcat( sz, "unsigned " );
        case SQL_SMALLINT:
        case SQL_INT:
        case SQL_BIGINT:
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
        case SQL_NUMERIC:
        case SQL_DECIMAL:
            strcat( sz, "numeric " ); break;

        case SQL_LONGVARBINARY:
            strcat( sz, "blob " );

        default:
            strcat( sz, "unsigned " );
    }
    for( i = strlen(sz) - 1; i > 0 && sz[i] <= ' '; i-- ) sz[i] = 0;
    RETURN_STRING( sz, 1 );
}
/* }}} */

/* {{{ proto void odbtp_void_func()
   Does nothing */
PHP_FUNCTION(odbtp_void_func)
{
}
/* }}} */

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
