lib_LTLIBRARIES = libm3_parse_n_gen.la

libm3_parse_n_gen_la_CFLAGS = -I$(top_srcdir)/include @GLIB_CFLAGS@ @DBUS_CFLAGS@ -Wall 
libm3_parse_n_gen_la_LDFLAGS = @DBUS_LIBS@ @GLIB_LIBS@ -lexpat 
libm3_parse_n_gen_la_LDFLAGS += -release @VERSION@

AM_CFLAGS = $(m3_parse_n_gen_la_CFLAGS)

# Use alphabetical order with these so they are easy to find

libm3_parse_n_gen_la_SOURCES = \
	m3_parse_n_gen.c \
	m3_sib_tokens.c
