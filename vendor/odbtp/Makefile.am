include_HEADERS = odbtp.h
lib_LTLIBRARIES = libodbtp.la
libodbtp_la_SOURCES = odbtp.c odbtp.h odbtpbuild.h sockutil.c sockutil.h
libodbtp_la_LDFLAGS = -version-info 1:1:0

noinst_PROGRAMS = odbtpcli
odbtpcli_SOURCES = odbtpcli.c
odbtpcli_LDADD = -lodbtp
odbtpcli_LDFLAGS = -L.
