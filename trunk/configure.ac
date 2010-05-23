#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(Poker-On-Terminal, 0.1, BUG-REPORT-ADDRESS)
AC_CONFIG_AUX_DIR([confd])
AM_INIT_AUTOMAKE([foreign])
# AC_CONFIG_SRCDIR([client.c])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h locale.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h ncurses.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([alarm gettimeofday memset setlocale socket strpbrk])

# Checks for libraries.
AC_ARG_WITH([ncurses],
	    [  --with-ncurses  using ncurses instead of ncursesw(not recommend)],
	    [AC_CHECK_LIB([ncurses], [main], [CFLAGS="$CFLAGS -DNCURSES" LIBS="$LIBS -lncurses"],
			  [echo "libncurse was not found"; exit -1;])],
	    [AC_CHECK_LIB([ncursesw], [main], [CFLAGS="$CFLAGS -DNCURSESW" LIBS="$LIBS -lncursesw"],
	     [echo "============"; echo; echo "libncursew was not found";
	      echo "WE RECOMMEND YOU TO INSTALL ncursesw for better display.";
	      echo "you can use ncurses instead of ncursesw, but this is NOT recommended.";
	      echo "use --with-ncurses to ignore this message";
	      echo; echo "============"; exit -1;])])
AC_PROG_RANLIB

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
