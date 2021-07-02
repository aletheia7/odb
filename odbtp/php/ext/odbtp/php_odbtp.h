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
/* $Id: php_odbtp.h,v 1.22 2005/04/19 20:38:03 rtwitty Exp $ */
#ifndef PHP_ODBTP_H
#define PHP_ODBTP_H

extern zend_module_entry odbtp_module_entry;
#define phpext_odbtp_ptr &odbtp_module_entry

#ifdef PHP_WIN32
#define PHP_ODBTP_API __declspec(dllexport)
#else
#define PHP_ODBTP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3) || (PHP_MAJOR_VERSION >= 6)
#undef ZVAL_REFCOUNT
#undef ZVAL_ADDREF
#undef ZVAL_DELREF
#define ZVAL_REFCOUNT Z_REFCOUNT_P
#define ZVAL_ADDREF Z_ADDREF_P
#define ZVAL_DELREF Z_DELREF_P
#endif

PHP_MINIT_FUNCTION(odbtp);
PHP_MSHUTDOWN_FUNCTION(odbtp);
PHP_RINIT_FUNCTION(odbtp);
PHP_RSHUTDOWN_FUNCTION(odbtp);
PHP_MINFO_FUNCTION(odbtp);

PHP_FUNCTION(odbtp_connect);
PHP_FUNCTION(odbtp_rconnect);
PHP_FUNCTION(odbtp_sconnect);
PHP_FUNCTION(odbtp_version);
PHP_FUNCTION(odbtp_close);
PHP_FUNCTION(odbtp_connect_id);
PHP_FUNCTION(odbtp_load_data_types);
PHP_FUNCTION(odbtp_use_row_cache);
PHP_FUNCTION(odbtp_row_cache_size);
PHP_FUNCTION(odbtp_convert_all);
PHP_FUNCTION(odbtp_convert_datetime);
PHP_FUNCTION(odbtp_convert_guid);
PHP_FUNCTION(odbtp_dont_pool_dbc);
PHP_FUNCTION(odbtp_get_attr);
PHP_FUNCTION(odbtp_set_attr);
PHP_FUNCTION(odbtp_commit);
PHP_FUNCTION(odbtp_rollback);
PHP_FUNCTION(odbtp_get_message);
PHP_FUNCTION(odbtp_get_error);
PHP_FUNCTION(odbtp_last_error);
PHP_FUNCTION(odbtp_last_error_state);
PHP_FUNCTION(odbtp_last_error_code);
PHP_FUNCTION(odbtp_allocate_query);
PHP_FUNCTION(odbtp_query_id);
PHP_FUNCTION(odbtp_get_query);
PHP_FUNCTION(odbtp_set_cursor);
PHP_FUNCTION(odbtp_query);
PHP_FUNCTION(odbtp_prepare);
PHP_FUNCTION(odbtp_prepare_proc);
PHP_FUNCTION(odbtp_execute);
PHP_FUNCTION(odbtp_num_params);
PHP_FUNCTION(odbtp_attach_param);
PHP_FUNCTION(odbtp_type_param);
PHP_FUNCTION(odbtp_param_name);
PHP_FUNCTION(odbtp_param_bindtype);
PHP_FUNCTION(odbtp_param_type);
PHP_FUNCTION(odbtp_param_length);
PHP_FUNCTION(odbtp_param_number);
PHP_FUNCTION(odbtp_input);
PHP_FUNCTION(odbtp_output);
PHP_FUNCTION(odbtp_inout);
PHP_FUNCTION(odbtp_set);
PHP_FUNCTION(odbtp_get);
PHP_FUNCTION(odbtp_num_fields);
PHP_FUNCTION(odbtp_bind_field);
PHP_FUNCTION(odbtp_attach_field);
PHP_FUNCTION(odbtp_fetch_field);
PHP_FUNCTION(odbtp_field_name);
PHP_FUNCTION(odbtp_field_bindtype);
PHP_FUNCTION(odbtp_field_type);
PHP_FUNCTION(odbtp_field_length);
PHP_FUNCTION(odbtp_field_table);
PHP_FUNCTION(odbtp_field_schema);
PHP_FUNCTION(odbtp_field_catalog);
PHP_FUNCTION(odbtp_field_flags);
PHP_FUNCTION(odbtp_field_basename);
PHP_FUNCTION(odbtp_num_rows);
PHP_FUNCTION(odbtp_fetch);
PHP_FUNCTION(odbtp_fetch_row);
PHP_FUNCTION(odbtp_fetch_assoc);
PHP_FUNCTION(odbtp_fetch_array);
PHP_FUNCTION(odbtp_fetch_object);
PHP_FUNCTION(odbtp_field);
PHP_FUNCTION(odbtp_field_ignore);
PHP_FUNCTION(odbtp_row_refresh);
PHP_FUNCTION(odbtp_row_update);
PHP_FUNCTION(odbtp_row_delete);
PHP_FUNCTION(odbtp_row_add);
PHP_FUNCTION(odbtp_row_lock);
PHP_FUNCTION(odbtp_row_unlock);
PHP_FUNCTION(odbtp_row_bookmark);
PHP_FUNCTION(odbtp_row_status);
PHP_FUNCTION(odbtp_affected_rows);
PHP_FUNCTION(odbtp_fetch_output);
PHP_FUNCTION(odbtp_next_result);
PHP_FUNCTION(odbtp_free_query);
PHP_FUNCTION(odbtp_new_datetime);
PHP_FUNCTION(odbtp_datetime2ctime);
PHP_FUNCTION(odbtp_ctime2datetime);
PHP_FUNCTION(odbtp_detach);
PHP_FUNCTION(odbtp_is_detached);
PHP_FUNCTION(odbtp_data_seek);
PHP_FUNCTION(odbtp_field_seek);
PHP_FUNCTION(odbtp_result);
PHP_FUNCTION(odbtp_fetch_batch);
PHP_FUNCTION(odbtp_select_db);
PHP_FUNCTION(odbtp_bind);
PHP_FUNCTION(odbtp_guid_string);
PHP_FUNCTION(odbtp_flags);
PHP_FUNCTION(odbtp_void_func);

ZEND_BEGIN_MODULE_GLOBALS(odbtp)
	long            default_link;
	unsigned short  default_port;
	char*           last_error;
	char*           interface_file;
	char*           datetime_format;
	char*           guid_format;
	long            detach_default_queries;
	long            truncation_errors;
ZEND_END_MODULE_GLOBALS(odbtp)

/* In every utility function you add that needs to use variables
   in php_odbtp_globals, call TSRM_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMG_CC
   after the last function argument and declare your utility function
   with TSRMG_DC after the last declared argument.  Always refer to
   the globals in your function as ODBTP_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define ODBTP_G(v) TSRMG(odbtp_globals_id, zend_odbtp_globals *, v)
#else
#define ODBTP_G(v) (odbtp_globals.v)
#endif

#endif	/* PHP_ODBTP_H */

