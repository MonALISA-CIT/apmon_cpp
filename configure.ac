AC_INIT([ApMon_cpp], [2.2.8])
AC_PREREQ(2.53)

AC_CONFIG_SRCDIR([ApMon.h])
AM_INIT_AUTOMAKE([ApMon_cpp], 2.2.8)
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
#AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
#AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL

AC_CHECK_PROG([HAVEDOXYGEN],[doxygen],[yes],[no])
AM_CONDITIONAL(WITHDOXYGEN, test x"$HAVEDOXYGEN" = xyes)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h sys/timeb.h unistd.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM


AC_CONFIG_FILES([Makefile
		examples/Makefile])
AC_OUTPUT
