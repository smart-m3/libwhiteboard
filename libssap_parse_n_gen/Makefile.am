lib_LTLIBRARIES = libssap_parse_n_gen.la

libssap_parse_n_gen_la_CFLAGS = -I$(top_srcdir)/include @GLIB_CFLAGS@ @DBUS_CFLAGS@ -Wall 
libssap_parse_n_gen_la_LDFLAGS = @DBUS_LIBS@ @GLIB_LIBS@ -lexpat 
libssap_parse_n_gen_la_LDFLAGS += -release @VERSION@


AM_CFLAGS = $(ssap_parse_n_gen_la_CFLAGS)

# Use alphabetical order with these so they are easy to find

libssap_parse_n_gen_la_SOURCES = \
	sibmsg.c \
	ssap_sib_tokens.c \
	ssap_parse_n_gen.c
