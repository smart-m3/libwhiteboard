lib_LTLIBRARIES = libwhiteboard-util.la

libwhiteboard_util_la_CFLAGS = -I$(top_srcdir)/include @DBUS_CFLAGS@ @GLIB_CFLAGS@ -Wall
libwhiteboard_util_la_LDFLAGS = @DBUS_LIBS@ @GLIB_LIBS@
libwhiteboard_util_la_LDFLAGS += -release @VERSION@

AM_CFLAGS = $(libwhiteboard_la_CFLAGS)

# Use alphabetical order with these so they are easy to find
libwhiteboard_util_la_SOURCES = \
	whiteboard_command.c \
	whiteboard_log.c \
	whiteboard_util.c
