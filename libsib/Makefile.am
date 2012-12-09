lib_LTLIBRARIES = libsib.la

libsib_la_CFLAGS = -I$(top_srcdir)/include @DBUS_CFLAGS@ @GLIB_CFLAGS@ -Wall
libsib_la_LDFLAGS = @DBUS_LIBS@ @GLIB_LIBS@ -luuid 
libsib_la_LDFLAGS += -release @VERSION@

AM_CFLAGS = $(libsib_la_CFLAGS)
# Use alphabetical order with these so they are easy to find
libsib_la_SOURCES = \
	sib_marshal.c \
	sib_object.c

sib_marshal.c: ../include/sib_marshal.list
	glib-genmarshal --prefix=marshal ../include/sib_marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc sib_marshal.c \
	&& rm -f xgen-gmc xgen-gmc~
