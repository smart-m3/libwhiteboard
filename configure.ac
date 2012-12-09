ubuntu nn#############################################################################
#
#       WhiteBoard library
#
#       Copyright (C) Nokia Corporation. All rights reserved.
#
#       
#
#############################################################################

AC_INIT()
AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE(libwhiteboard, 2.2-beta1)
AM_CONFIG_HEADER(config.h)

AC_LANG_C
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_CHECK_HEADERS(stdlib.h string.h,,AC_MSG_ERROR(Required header file missing))

AC_CHECK_HEADERS(expat.h,,AC_MSG_ERROR(Required header file missing, expat installed?))

AC_CHECK_HEADERS(uuid/uuid.h,,AC_MSG_ERROR(Required header file missing, uuid installed?))

PKG_CHECK_MODULES(DBUS,
[
        dbus-1 >= 0.61
        dbus-glib-1 >= 0.61
])

AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(GLIB,
[
        glib-2.0 >= 2.8.6
	gobject-2.0 >= 2.8.6
])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

##############################################################################
# Check compile target
##############################################################################

case $host in
        i386-* | i686-*)
               CFLAGS="$CFLAGS -Di686"
               USE_i686=yes
               USE_ARM=no
               ;;
        arm-*)
               CFLAGS="$CFLAGS -DARM"
               USE_ARM=yes
               USE_i686=no
               ;;
esac

AM_CONDITIONAL(ARM, test "x$USE_ARM"="xyes")
AM_CONDITIONAL(i686, test "x$USE_i686"="xyes")

#############################################################################
# Check whether debug prints should be used
#############################################################################
AC_ARG_WITH(debug,
        AS_HELP_STRING([--with-debug],
                       [Print additional debug messages (default = no)]),
        [
	AC_DEFINE([WHITEBOARD_DEBUG],[1],[Print debug messages])
	with_debug=yes
	],
        [with_debug=no]
)


#############################################################################
# Check whether debug prints should be used
#############################################################################
AC_ARG_WITH(ssaprole,
        AS_HELP_STRING([--with-ssaprole=ARG],
	               [Which SSAP role, available values kp, sib, both. (Default=both)]),
        [
	case "x$withval" in
		"xkp")
			with_ssaprole=kp
			with_sib=no
			AC_DEFINE([SIBUSER_ROLE],,[Compile user role parsers])
			;;
		"xsib")
			with_ssaprole=sib
			with_sib=yes
			AC_DEFINE([SIB_ROLE],,[Compile SIB role parsers])
			;;
		"xboth")
			with_ssaprole=both
			with_sib=yes
			AC_DEFINE([SIB_ROLE],,[Compile SIB role parsers])
			AC_DEFINE([SIBUSER_ROLE],,[Compile user role parsers])
			;;
		*)
			AC_MSG_ERROR(Invalid value for --with-ssaprole)
		
	esac
	],
        [with_ssaprole=both
		AC_DEFINE([SIB_ROLE],,[Compile SIB role parsers])
		AC_DEFINE([SIBUSER_ROLE],,[Compile user role parsers])
		with_sib=yes
	]
)

AM_CONDITIONAL(WITHSIB, test "x$with_sib"="xyes" )
#AM_CONDITIONAL( SSAP_ROLE_SIB, test "x$with_ssaprole" == "xsib" )
#AM_CONDITIONAL( SSAP_ROLE_BOTH, test "x$with_ssaprole" == "xboth" )

#############################################################################
# Check whether timestamp prints should be used
#############################################################################
AC_ARG_WITH(timestamps, 
	AS_HELP_STRING([--with-timestamps],
		       [Print timestamps for benchmarking (default = no)]),
	[AC_DEFINE([WHITEBOARD_TIMESTAMP_ENABLED],[1],[Print timestamp messages])],
	[with_timestamps=no])

#############################################################################
# Check whether unit tests should be built
#############################################################################
AC_ARG_WITH(unit-tests,
        [AS_HELP_STRING([--with-unit-tests], [Build unit tests (default = no)])],
        [AC_DEFINE([WHITEBOARD_UNIT_TESTS_ENABLED], [1], [Build unit tests])
         PKG_CHECK_MODULES(CHECK, check >= 0.9.4)
         with_unit_tests=yes],
        [with_unit_tests=no]
)
AM_CONDITIONAL(UNIT_TESTS, test $with_unit_tests=yes)

#############################################################################
# Check whether doxygen documentation should be created
#############################################################################
AC_ARG_WITH(docs,
        AS_HELP_STRING([--with-docs],
                       [Create additional doxygen documentation (default = no)]),
        [with_docs=yes],
        [with_docs=no]
)
AM_CONDITIONAL(WITH_DOCS, test $with_docs = yes)

AC_OUTPUT(
	Makefile
	include/Makefile
	libssap_parse_n_gen/Makefile
	libm3_parse_n_gen/Makefile
	libutil/Makefile
	libsib/Makefile
	src/Makefile
	etc/Makefile
	etc/libwhiteboard.pc
	etc/libssap_parse_n_gen.pc
	etc/libm3_parse_n_gen.pc
	etc/libwhiteboard-util.pc
	etc/libsib.pc
)
echo "Debug logs: ${with_debug}"
echo "SSAP ROLE: ${with_ssaprole}"
