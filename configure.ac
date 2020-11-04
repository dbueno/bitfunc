dnl Copyright 2012 Sandia Corporation. Under the terms of Contract
dnl DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
dnl or on behalf of the U.S. Government. Export of this program may require a
dnl license from the United States Government.

AC_INIT([bitfunc], [1.0.3], [denis.bueno@sandia.gov])
# error if unexpanded autoconf archive macros
m4_pattern_forbid([^AX_])
:${CFLAGS=""}
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR(bitfunc.h)
AC_CONFIG_MACRO_DIR([m4])
#BASIC_CFLAGS="-Wall -Wno-deprecated $CFLAGS"
#BASIC_CXXFLAGS="-Wall -Wno-deprecated $CFLAGS"
#DEFAULT_CXXFLAGS="$BASIC_CFLAGS -O3 -finline-limit=100000 -fexpensive-optimizations -fomit-frame-pointer -fno-strict-aliasing -fvariable-expansion-in-unroller -ftree-loop-linear"
#DEFAULT_CXXFLAGS="$BASIC_CXXFLAGS -O3 -finline-limit=100000 -fexpensive-optimizations -fomit-frame-pointer -fno-strict-aliasing -fvariable-expansion-in-unroller -ftree-loop-linear"
#CHOSEN_CFLAGS="$DEFAULT_CFLAGS"
#CHOSEN_CXXFLAGS="$DEFAULT_CXXFLAGS"

dnl make this an option only if it is accepted by the gcc on the system-Wsign-conversion -Wno-deprecated
WARN_CFLAGS="-Wall -Wextra -Wno-unused -Werror"
OPT_CFLAGS="-O3"
DEBUG_CFLAGS=
SHARED_CFLAGS=
PROF_CFLAGS=

dnl check for functions and headers
AC_CHECK_FUNCS([memset],,[AC_MSG_ERROR([Cannot find memset])])

AC_CHECK_FUNCS([pow],,[AC_CHECK_LIB([m],[pow],,[AC_MSG_ERROR(cannot find pow)])])
AC_CHECK_FUNCS([strchr],,[AC_MSG_ERROR([Cannot find strchr])])
AC_CHECK_FUNCS([strdup],,[AC_MSG_ERROR([Cannot find strdup])])
AC_CHECK_FUNCS([strndup],,[AC_MSG_ERROR([Cannot find strndup])])
AC_CHECK_FUNCS([strrchr],,[AC_MSG_ERROR([Cannot find strrchr])])
AC_CHECK_FUNCS([strtol],,[AC_MSG_ERROR([Cannot find strtol])])
AC_CHECK_FUNCS([strerror],,[AC_MSG_ERROR([Cannot find strerror])])
AC_CHECK_FUNCS([strtoul],,[AC_MSG_ERROR([Cannot find strtoul])])
AC_CHECK_HEADERS([fcntl.h],,[AC_MSG_ERROR([Cannot find fcntl.h])])
AC_CHECK_HEADERS([stddef.h],,[AC_MSG_ERROR([Cannot find stddef.h])])

dnl check for compiler features
AC_C_INLINE

dnl check for lib functions
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD

dnl check for some types
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_ARG_ENABLE([codemodel],
  AC_HELP_STRING([--enable-codemodel],[32, 64, or native (default native)]),
  [if test "x${enableval}" = "x32"; then CODEMODEL="-m32";
   elif test "x${enableval}" = "x64"; then CODEMODEL="-m64";
   elif test "x${enableval}" = "xnative"; then CODEMODEL=""; fi],
  [CODEMODEL=""]
  )

AC_ARG_ENABLE([debug],
  AC_HELP_STRING([--enable-debug],[turn on debugging]),
  [if test "x${enableval}" = "xyes"; then DEBUG_CFLAGS="-g"
   elif test "x${enableval}" = "xno"; then :
   else AC_MSG_ERROR([Invalid value ${enableval} to --enable-debug]); fi])

AC_ARG_ENABLE([optimize],
  AC_HELP_STRING([--disable-optimize],[turn off optimizations]),
  [if test "x${enableval}" = "xno"; then OPT_CFLAGS="-O0";
   elif test "x${enableval}" = "xyes"; then :
   else OPT_CFLAGS="${enableval}"; fi])

AC_ARG_ENABLE([llvm-py],
  AC_HELP_STRING([--enable-llvm-py],[enable llvm-py frontend]),
  [case "${enableval}" in
    yes) llvm_py=true ;;
    no)  llvm_py=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-llvm-py]) ;;
   esac],[llvm_py=false])
AM_CONDITIONAL([LLVM_PY_ENABLED], [test x$llvm_py = xtrue])

dnl TODO use LLVM_PY_ENABLED to test for llvm-py with some python


dnl some optimizations (-fomit-frame-pointer) are incompatible with profiling,
dnl so disable them
AC_ARG_ENABLE([profiling],
  AC_HELP_STRING([--enable-profiling],[turn on profiling with -pg]),
  [if test "x${enableval}" = "xyes"; then PROF_CFLAGS="-pg -g -O3"; OPT_CFLAGS=""
   elif test "x${enableval}" = "xno"; then :
   else AC_MSG_ERROR([Invalid value ${enableval} to --enable-profiling]); fi])

AC_ARG_ENABLE([longjmp],
  AC_HELP_STRING([--enable-longjmp],[turn on error handling with setjmp/longjmp]),
  [if test "x${enableval}" = "xyes"; then
     AC_DEFINE([USE_LONGJMP],[1],[Use longjmp for exceptions])
   elif test "x${enableval}" = "xno"; then :
   else AC_MSG_ERROR([Invalid value ${enableval} to --enable-longjmp]); fi])

AC_ARG_WITH([picosat],
  AC_HELP_STRING([--with-picosat=PATH],[location of picosat]),
  [PICOSAT_CPPFLAGS="-I${withval}"; CPPFLAGS="$CPPFLAGS $PICOSAT_CPPFLAGS";
   PICOSAT_LDFLAGS="-L${withval}"; LDFLAGS="$LDFLAGS $PICOSAT_LDFLAGS";
   AC_DEFINE([INCREMENTAL_PICOSAT_ENABLED],[1],[Picosat library is available])])
AM_CONDITIONAL(USE_INCREMENTAL_PICOSAT, [test ! x"$PICOSAT_LDFLAGS" = x ])

dnl [ABC_CPPFLAGS="-I${withval}"; CPPFLAGS="$CPPFLAGS $ABC_CPPFLAGS -DLIN64 -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4";
dnl    ABC_LDFLAGS="-L${withval}"; LDFLAGS="$LDFLAGS $ABC_LDFLAGS -DLIN64 -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4";
dnl    AC_DEFINE([INCREMENTAL_ABC_ENABLED],[1],[ABC library is available])]
AC_ARG_WITH([abc],
  AC_HELP_STRING([--with-abc=PATH],[location of ABC]),
  [ABC_DIR="${withval}"],
  [with_abc=check])

AC_ARG_ENABLE([log], AC_HELP_STRING([--enable-log],[enable level-based logging (off by default)]))
AS_IF([test "x$enable_log" = "xyes"],[AC_DEFINE([FUNCSAT_LOG],[1],[Define to 1 to compile in level-based logging.])])
AC_DEFINE([__USE_POSIX2],[1],[So that I can get popen from stdio?])
AC_DEFINE([FUNCSAT_LOG],[1],[Logging from funcsat])
AC_DEFINE([COMPSOLVE_PARTIAL_ASSIGNMENT_COMPLETION],[1],[Enable completion of partial assignments from SAT competition backends])

AX_PROG_FLEX(,[AC_MSG_ERROR([flex is required])])
AC_PROG_CC
AC_PROG_CC_C99
if test "x${ac_cv_prog_x99}" = "xno"; then
  AC_MSG_ERROR([compiler does not support C99])
fi
AM_PROG_CC_C_O
AC_CONFIG_HEADERS([src/bitfunc/config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 test/Makefile
])

AC_ARG_ENABLE([perfprobes],[AS_HELP_STRING([--enable-perfprobes],[enable support for performance probes])],
[],
[enable_perfprobes=no])

have_perfprobes=no
AS_IF([test "x$enable_perfprobes" != xno],[
PKG_CHECK_MODULES([PERFPROBES],[perfprobes],[
AC_DEFINE([HAVE_PERFPROBES],[1],[Define to 1 if perfprobes is available])
have_perfprobes=yes
],[])])
AM_CONDITIONAL([HAVE_PERFPROBES],[test x$have_perfprobes = xyes])

AS_IF([test "x$enable_perfprobes" != xno -a "x$have_perfprobes" = xno],[
AC_MSG_WARN([performance probes requested but required libraries were not found])
])

AC_DEFUN([CHECK_PICOSAT], [
save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
CPPFLAGS="$PICOSAT_CPPFLAGS $CPPFLAGS"
LDFLAGS="$PICOSAT_LDFLAGS $LDFLAGS"
AC_CHECK_HEADER([picosat.h],[],AC_MSG_WARN(picosat.h cannot be found, disabling incremental picosat))
AC_CHECK_LIB([picosat],[picosat_init],,AC_MSG_WARN(cannot link to picosat))
CPPFLAGS="$save_CPPFLAGS"
LDFLAGS="$save_LDFLAGS"
])

AC_DEFUN([CHECK_ABC], [
  LIBABC=
  AS_IF([test "x$with_abc" != xno],
    [if test -n "$ABC_DIR"; then
       save_CPPFLAGS="$CPPFLAGS"
       save_LDFLAGS="$LDFLAGS"
       save_LIBS="$LIBS"
       CPPFLAGS="-I$ABC_DIR -I$ABC_DIR/src"
       LDFLAGS="-L$ABC_DIR"
       LIBS=""
     fi
     AC_CHECK_SIZEOF([void *])
     AC_CHECK_SIZEOF([long])
     AC_CHECK_SIZEOF([int])
     AC_DEFINE([LIN64],[1], [Define for linux 64-bit (abc)])
     AC_SUBST([ABC_CPPFLAGS], ["-I$ABC_DIR -I$ABC_DIR/src"])
     AC_SUBST([ABC_LDFLAGS], [-L$ABC_DIR])
     AC_CHECK_HEADER([base/abc/abc.h],[],
       [if test "x$with_abc" != xcheck; then
          AC_MSG_FAILURE([--with-abc was given, but test for abc.h failed])
        fi])
     AC_CHECK_LIB([abc], [Abc_Start],
       [AC_SUBST([LIBABC], ["-labc -lreadline"])
        AC_DEFINE([HAVE_LIBABC],[1],[ABC library is available])
       ],
       [if test "x$with_abc" != xcheck; then
          AC_MSG_FAILURE([--with-abc was given, but test for abc failed])
        fi
       ], -lreadline)
    if test -n "$ABC_DIR"; then
      CPPFLAGS="$save_CPPFLAGS"
      LDFLAGS="$save_LDFLAGS"
      LIBS="$save_LIBS"

      CPPFLAGS="$CPPFLAGS $ABC_CPPFLAGS"
      LDFLAGS="$LDFLAGS $ABC_LDFLAGS"
    fi
    ])
  LIBS="$LIBABC $LIBS"
])


dnl AM_CFLAGS="$SHARED_CFLAGS $OPT_CFLAGS $DEBUG_CFLAGS $WARN_CFLAGS $PROF_CFLAGS"

dnl AC_ARG_VAR([NOTANGLE], [notangle executable])
dnl AC_CHECK_PROGS([NOTANGLE], [notangle])
dnl AC_ARG_VAR([NOWEAVE], [noweave executable])
dnl AC_PATH_PROG([NOWEAVE], [noweave])
dnl AS_IF(dnl [test "x$NOWEAVE" = "x"], [AC_MSG_ERROR([noweave not found, ensure noweb system is available])],
dnl       [test "x$NOTANGLE" = "x"], [AC_MSG_ERROR([notangle not found, ensure noweb system is available])])

DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([bitfunc])

AC_ARG_VAR([CTANGLE], [ctangle executable])
AC_CHECK_PROGS([CTANGLE], [ctangle])
AC_ARG_VAR([CWEAVE], [cweave executable])
AC_CHECK_PROGS([CWEAVE], [cweave])
AM_CONDITIONAL([HAVE_CWEB],[test "x$CTANGLE" != "x" -a "x$CWEAVE" != "x"])
dnl I don't use AM_COND_IF because some modern systems still don't support it =[
if test "x$CTANGLE" != "x" -a "x$CWEAVE" != "x"
then
  true;
else
  AC_MSG_WARN([cweb not found - funcsat cannot be built if changed])
fi

AC_ARG_VAR([PDFTEX], [pdftex executable])
AC_CHECK_PROGS([PDFTEX], [pdftex])
AS_IF([test "x$PDFTEX" = "x"], [AC_MSG_WARN([pdftex not found, cannot build docs])])
AM_CONDITIONAL([HAVE_PDFTEX],[test "x$PDFTEX" != "x"])

AM_PATH_PYTHON([2.6],, [:])
AX_PYTHON_DEVEL

AC_ARG_VAR(CYTHON, [cython executable])
AC_CHECK_PROGS(CYTHON, [cython cython-2.7], "no")
AM_CONDITIONAL([HAVE_CYTHON],[test "x$CYTHON" != "xno"])
dnl I don't use AM_COND_IF because some modern systems still don't support it =[
if test "x$CYTHON" = "xno"; then
  AC_MSG_WARN([cython not found - python bindings cannot be changed])
fi

AC_SUBST([CODEMODEL])
AC_SUBST([FUNCSAT_CPPFLAGS])
AC_SUBST([FUNCSAT_LDFLAGS])
AC_SUBST([PICOSAT_CPPFLAGS])
AC_SUBST([PICOSAT_LDFLAGS])
AC_SUBST([DEBUG_CFLAGS])
AC_SUBST([OPT_CFLAGS])
AC_SUBST([WARN_CFLAGS])
AC_SUBST([SHARED_CFLAGS])
AC_SUBST([PROF_CFLAGS])
AC_HEADER_STDC()
AC_HEADER_ASSERT()
AC_HEADER_STAT()
AC_HEADER_STDBOOL()
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([sys/resource.h])
AC_CHECK_LIB([m],[log],,AC_MSG_FAILURE("math library not found"))
AC_CHECK_LIB([c],[strndup],
        AC_DEFINE([HAVE_STRNDUP],[1],[strndup is available]))
AC_SEARCH_LIBS([setjmp],[c],
  AC_SEARCH_LIBS([longjmp],[c],
    AC_DEFINE([HAVE_SETJMP_LONGJMP],[1],[setjmp & longjmp are available])))

dnl todo guard with abc
AC_CHECK_HEADERS([zlib.h],AC_CHECK_LIB(z,gzopen),AC_MSG_WARN([reading .cnf.gz disabled]))
CHECK_PICOSAT
CHECK_ABC
AC_CONFIG_SUBDIRS([funcsat])
AC_OUTPUT
