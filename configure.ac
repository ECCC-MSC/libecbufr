#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#dnl Package version information
BUILD_DATE=`date +%d-%m-%Y`

AC_PREREQ(2.59)

m4_define(libecbufr_version_major, 0)
m4_define(libecbufr_version_minor, 9)
# An odd micro number indicates in-progress development, (eg. from git/cvs)
# An even micro number indicates a released version.
m4_define(libecbufr_version_micro, 4)

AC_INIT(libecbufr, libecbufr_version_major.libecbufr_version_minor.libecbufr_version_micro,[https://github.com/ECCC-MSC/libecbufr])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(./)

AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_SRCDIR([API/Sources/bufr_io.c])
AC_CONFIG_HEADERS([config.h])
#AM_CONFIG_HEADER(config.h) # This is obsolete
#AM_INIT_AUTOMAKE(ECBUFR, 0.8.2) # This is obsolete
AM_INIT_AUTOMAKE

# use these version macros in Makefile.am
VERSION_MAJOR=libecbufr_version_major
VERSION_MINOR=libecbufr_version_minor
VERSION_MICRO=libecbufr_version_micro
AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_MICRO)

dnl Shared library version information
dnl
dnl Current  = interface version (increment when change user interface to lib)
dnl Revision = revision number (increment when change implementation, reset to 0 for new interface)
dnl Age      = number of previous interfaces this one is compatible with (i.e. is a superset of)
dnl
dnl Suffix1  = [Current - Age] (on Linux, anyway, soname suffix is [C-A].A.R)
dnl
LIBTOOL_CURRENT=libecbufr_version_major
LIBTOOL_REVISION=libecbufr_version_minor
LIBTOOL_AGE=0
LIBTOOL_SUFFIX1=libecbufr_version_major
LIBTOOL_SUFFIX=${LIBTOOL_SUFFIX1}.${LIBTOOL_AGE}.${LIBTOOL_REVISION}

TOPDIR=`pwd`

AC_ARG_ENABLE(code-coverage, [  --enable-code-coverage  Adds -fprofile-arcs -ftest-coverage to CFLAGS to be able to use gcov], [ CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])

AC_ARG_ENABLE(cgi, [  --enable-cgi            build and install cgi component [[default=no]]], [
   if test "$enableval" = no; then
      build_dir_cgi=""
   else
      build_dir_cgi="cgi"
   fi

   if test "$CGI_PREFIX" = "" ; then
      if test "$CGI_BINDIR" = "" ; then
         CGI_BINDIR="/usr/lib/cgi-bin/libecbufr"
      fi
      if test "$CGI_DOCDIR" = "" ; then
         CGI_DOCDIR="/usr/share/doc/libecbufr/cgi"
      fi
      if test "$CGI_HTMLDIR" = "" ; then
         CGI_HTMLDIR="/var/www/libecbufr"
      fi
      if test "$CGI_EXDIR" = "" ; then
         CGI_EXDIR="/var/www/libecbufr/ex"
      fi
   else
      if test "$CGI_BINDIR" = "" ; then
         CGI_BINDIR="$CGI_PREFIX/cgi-bin"
      fi
      if test "$CGI_DOCDIR" = "" ; then
         CGI_DOCDIR="$CGI_PREFIX/doc"
      fi
      if test "$CGI_HTMLDIR" = "" ; then
         CGI_HTMLDIR="$CGI_PREFIX"
      fi
      if test "$CGI_EXDIR" = "" ; then
         CGI_EXDIR="$CGI_PREFIX/examples"
      fi
   fi

   AC_ARG_VAR(CGI_PREFIX, cgi component install path, default is "")
   AC_ARG_VAR(CGI_BINDIR, cgi component binary install path)
   AC_ARG_VAR(CGI_DOCDIR, cgi component documentation install path)
   AC_ARG_VAR(CGI_HTMLDIR, cgi component html install path)
   AC_ARG_VAR(CGI_EXDIR, cgi component examples install path)
],[build_dir_cgi=""])

AC_ARG_ENABLE(gcmem, [  --enable-gcmem            build and install my garbage collector [[default=no]]], [

   AC_DEFINE([USE_GCMEMORY],[1],[Flag enabling use of Memory Garbage Collector])
   AC_DEFINE([GCMEM_LISTNODE_SIZE],[65536],[GC Size for ListNode])
   AC_DEFINE([GCMEM_DESCRIPTOR_SIZE],[65536],[GC Size for Descriptor])
   AC_DEFINE([GCMEM_VALINT8_SIZE],[512],[GC Size for ValInt8])
   AC_DEFINE([GCMEM_VALINT32_SIZE],[65536],[GC Size for ValInt32])
   AC_DEFINE([GCMEM_VALINT64_SIZE],[512],[GC Size for ValInt64])
   AC_DEFINE([GCMEM_VALFLT32_SIZE],[512],[GC Size for ValFlt32])
   AC_DEFINE([GCMEM_VALFLT64_SIZE],[65536],[GC Size for ValFlt64])
   AC_DEFINE([GCMEM_VALSTRING_SIZE],[512],[GC Size for ValString])
],[])

AC_ARG_WITH(python,[  --with-python=ARG   Enable python bindings (ARG=yes or no)],,)

AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])
AC_MSG_CHECKING([for python bindings])
if test "$with_python" = "yes" ; then
  dnl
  dnl Check for Python executable in PATH
  dnl
  AM_PATH_PYTHON([2.7],, [:])
else
  PYTHON=""
  AC_MSG_RESULT([disabled])
fi

BINDINGS=""

if test "x$PYTHON" != "x"; then

  AC_MSG_CHECKING([for python setuptools])
  SETUPTEST='try:
  import setuptools
  print(1)
except ImportError:
  pass'
  PY_HAVE_SETUPTOOLS=`$PYTHON -c "$SETUPTEST"`
  if test "$PY_HAVE_SETUPTOOLS" = "1"; then
    AC_MSG_RESULT([found])
  else
    AC_MSG_RESULT([not found])
  fi

  AC_MSG_CHECKING([for cython])
  CYTHONTEST='try:
  import cython
  print(1)
except ImportError:
  pass'
  PY_HAVE_CYTHON=`$PYTHON -c "$CYTHONTEST"`
  if test "$PY_HAVE_CYTHON" = "1"; then
    if test "$PY_HAVE_SETUPTOOLS" = "1"; then
      AC_MSG_RESULT([found])
      BINDINGS="python $BINDINGS"
      AC_OUTPUT( Bindings/python/Makefile )
      AC_SUBST([BINDINGS])
    fi
  else
    AC_MSG_RESULT([not found])
  fi
fi


# on IRIX machines: use cc instead of gcc, use -c99 and remove -std=gnu99
#                  CFLAGS="$CFLAGS -c99 -I${PWD}/API/Headers -D_GNU_SOURCE"
# For gcc
# -D_REENTRANT   : so that library compatible with Linux Threads
# -fPIC          : for shared library
# should be stripped with :   strip --strip-unneeded thelibrary
# shared libraries should be installed in /usr/lib  
# otherwise, /etc/ld.so.conf should contains reference to that directory at post-install and
#            removed from at post-removal
#
# no -fPIC       : for static library
#CFLAGS="$CFLAGS -I${PWD}/API/Headers -std=gnu99 -g -D_REENTRANT"
# add "-static" to have movable binaries
CFLAGS="$CFLAGS -I${PWD}/API/Headers -I${PWD}/API/Sources -std=gnu99 -D_REENTRANT -fPIC -O"
LDFLAGS="$LDFLAGS -L${PWD}/API/Sources"

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
#
AC_GNU_SOURCE

# AC_PROG_RANLIB is rendered obsolete by AC_PROG_LIBTOOL
#AC_PROG_RANLIB
# seems AC_PROG_LIBTOOL not available on IRIX, and should be commented 
AC_PROG_LIBTOOL

AC_PATH_PROG(DOXYGEN, doxygen, no)
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes)
AM_CONDITIONAL(HAVE_PKGCONFIG, test "$HAVE_PKGCONFIG" = "yes")

# Checks for libraries.
#AM_PATH_CHECK("0.9.4") #this is apparently deprecated (message from configure)
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], CHECK_AVAILABLE="yes", CHECK_AVAILABLE="no")

# Checks for header files.
#AC_HEADER_STDC
AC_CHECK_HEADERS(stdio.h stdlib.h string.h math.h limits.h sys/types.h time.h ctype.h)

# Checks for typedefs, structures, and compiler characteristics.
#AC_C_CONST
#AC_STRUCT_TM

#  Check endianness 
AC_C_BIGENDIAN

#--------------------------------------------------------------------
# On a few very rare systems, all of the libm.a stuff is
# already in libc.a.  Set compiler flags accordingly.
# Also, Linux requires the "ieee" library for math to work
# right (and it must appear before "-lm").
#--------------------------------------------------------------------
MATH_LIBS=""
AC_CHECK_FUNC(sin, , MATH_LIBS="-lm")
AC_CHECK_LIB(ieee, main, [MATH_LIBS="-lieee $MATH_LIBS"])

dnl Checks for precise integer types
AC_CHECK_HEADERS([stdint.h inttypes.h sys/int_types.h values.h])
AC_CHECK_TYPES([uint64_t, uint128_t])


# Checks for library functions.
AC_CHECK_LIB( c, main )
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
#AC_CHECK_FUNCS([strdup])

dnl Set environment variables.
# AC_SUBST(PACKAGE)
# AC_SUBST(RELEASE)
AC_SUBST(BUILD_DATE)
AC_SUBST(build_dir_cgi)

AM_MAINTAINER_MODE

# Internationalization support
AM_GNU_GETTEXT_VERSION([0.16.1])
AM_GNU_GETTEXT([external])

# Set a conditional value for the 'check' package.
AM_CONDITIONAL(CHECK_ACTIVATED, test "$CHECK_AVAILABLE" = yes)

AC_OUTPUT(Makefile \
      po/Makefile.in \
      API/Makefile \
      API/Headers/Makefile \
      API/Headers/private/Makefile \
      API/Headers/bufr_api.h \
      API/Sources/Makefile \
      API/version \
      Bindings/Makefile \
      Utilities/Makefile \
      Utilities/po/Makefile.in \
      Tables/Makefile \
      Test/Makefile \
      Test/po/Makefile.in \
      Docs/Makefile \
      Test/BUFR/Makefile \
      Test/Dump/Makefile \
      Test/Unit_Tests/Makefile \
      Examples/Makefile
		cgi/Makefile
		)
