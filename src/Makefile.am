lib_LTLIBRARIES = libwhiteboard.la

libwhiteboard_la_LDFLAGS = @DBUS_LIBS@ @GLIB_LIBS@ -luuid 
libwhiteboard_la_CFLAGS = -I$(top_srcdir)/include @DBUS_CFLAGS@ @GLIB_CFLAGS@ -Wall
libwhiteboard_la_LDFLAGS += -release @VERSION@



AM_CFLAGS = $(libwhiteboard_la_CFLAGS)

# Use alphabetical order with these so they are easy to find
libwhiteboard_la_SOURCES = \
	whiteboard_discovery.c \
	whiteboard_marshal.c \
	whiteboard_sib_access.c \
	whiteboard_node.c 

whiteboard_marshal.c: ../include/whiteboard_marshal.list
	glib-genmarshal --prefix=marshal ../include/whiteboard_marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc whiteboard_marshal.c \
	&& rm -f xgen-gmc xgen-gmc~
