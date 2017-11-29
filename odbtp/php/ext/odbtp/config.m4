dnl $Id: config.m4,v 1.8 2004/06/20 20:19:46 rtwitty Exp $
dnl config.m4 for extension odbtp

PHP_ARG_WITH(odbtp, for ODBTP support,
[  --with-odbtp[=DIR]      Include ODBTP support])

PHP_ARG_WITH(odbtp-mssql, for ODBTP/MSSQL support,
[  --with-odbtp-mssql[=DIR] Include ODBTP with MSSQL-DB support])

if test "$PHP_ODBTP" != "no" -o "$PHP_ODBTP_MSSQL" != "no"; then
  # --with-odbtp -> check if with mssql support
  if test "$PHP_ODBTP_MSSQL" != "no"; then
    AC_DEFINE(ODBTP_MSSQL,1,[ ])
  fi

  # --with-odbtp -> check with-path
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="/include/odbtp.h"
  if test "$PHP_ODBTP" != "no" -a -r $PHP_ODBTP/; then
    ODBTP_DIR=$PHP_ODBTP
  elif test "$PHP_ODBTP_MSSQL" != "no" -a -r $PHP_ODBTP_MSSQL/; then
    ODBTP_DIR=$PHP_ODBTP_MSSQL
  else # search default path list
    AC_MSG_CHECKING([for odbtp files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        ODBTP_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$ODBTP_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the ODBTP distribution])
  fi

  # --with-odbtp -> add include path
  PHP_ADD_INCLUDE($ODBTP_DIR/include)

  # --with-odbtp -> chech for library
  if test ! -f "$ODBTP_DIR/lib/libodbtp.a"; then
    AC_MSG_ERROR([libodbtp.a not found])
  fi

  # --with-odbtp -> add library
  PHP_ADD_LIBRARY_WITH_PATH(odbtp, $ODBTP_DIR/lib, ODBTP_SHARED_LIBADD)

  AC_DEFINE(HAVE_ODBTP,1,[ ])
  PHP_NEW_EXTENSION(odbtp, php_odbtp.c, $ext_shared)
  PHP_SUBST(ODBTP_SHARED_LIBADD)
fi
