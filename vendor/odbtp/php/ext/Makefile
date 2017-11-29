#
#  Module specific variables
#
MODULE=php_odbtp.so
MODULE_OBJS=php_odbtp.o odbtp.o sockutil.o
#MODULE_CFLAGS=-DHAVE_ODBTP -DCOMPILE_DL_ODBTP
# Uncomment the below line and comment the above line to include MSSQL support
MODULE_CFLAGS=-DHAVE_ODBTP -DCOMPILE_DL_ODBTP -DODBTP_MSSQL
MODULE_INCLUDES=-I../.. -I./odbtp

#
# PHP specific variables
#
PHP_INCROOT=/usr/include/php5
PHP_INCLUDES=-I$(PHP_INCROOT) -I$(PHP_INCROOT)/main \
	-I$(PHP_INCROOT)/TSRM -I$(PHP_INCROOT)/Zend

#
#  Compile and link variables.  
#
CC=gcc
CFLAGS=-O2 -fpic $(MODULE_CFLAGS)
LINK=$(CC) -shared
#
# MacOS X
#CFLAGS=-O2 -Wall -fPIC -fno-common $(MODULE_CFLAGS)
#LINK=$(CC) -bundle -flat_namespace -undefined suppress

all: $(MODULE)

$(MODULE): $(MODULE_OBJS)
	$(LINK) $(CFLAGS) -o $(MODULE) $(MODULE_OBJS)

php_odbtp.o: ./odbtp/php_odbtp.c
	$(CC) $(CFLAGS) $(PHP_INCLUDES) $(MODULE_INCLUDES) -o php_odbtp.o -c $?

odbtp.o: ../../odbtp.c
	$(CC) $(CFLAGS) $(PHP_INCLUDES) $(MODULE_INCLUDES) -o odbtp.o -c $?

sockutil.o: ../../sockutil.c
	$(CC) $(CFLAGS) $(PHP_INCLUDES) $(MODULE_INCLUDES) -o sockutil.o -c $?

clean:
	rm -f *.o $(MODULE)

