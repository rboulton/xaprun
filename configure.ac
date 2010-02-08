dnl Process this file with autoconf to produce a configure script.

AC_INIT([xappy-server], [0.1], [http://tartarus.org/richard/xappy-server/])
AC_PREREQ([2.63])
AM_INIT_AUTOMAKE([1.10.1 -Wportability tar-ustar])

AC_CONFIG_SRCDIR([src/server.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
LT_PREREQ([2.2.6])
LT_INIT

dnl Checks for programs.
AC_PROG_CXX
AM_PROG_CC_C_O

dnl Run tests using the C++ compiler.
AC_LANG_CPLUSPLUS

dnl Check for xapian-core.
XO_LIB_XAPIAN([], [], [xapian-config])

dnl We want XAPIAN_CXXFLAGS to be used for configure tests.
save_CXXFLAGS=$CXXFLAGS
CXXFLAGS="$CXXFLAGS $XAPIAN_CXXFLAGS"

dnl disable "maintainer only" rules by default
AM_MAINTAINER_MODE

dnl mingw (for instance) lacks ssize_t
AC_CHECK_TYPE(ssize_t, int)
AC_TYPE_MODE_T
AC_TYPE_PID_T

dnl ******************************
dnl * Set special compiler flags *
dnl ******************************
dnl This section was derived from xapian-applications/omega/configure.ac

dnl Set flags to control warnings (enable more, or disable annoying ones).
if test yes = "$GXX"; then
  dnl Intel's C++ compiler is identified as "GXX" by autoconf's test - check
  dnl which we actually have.
  AC_EGREP_CPP(yes,
    [#ifdef __INTEL_COMPILER
     yes
     #endif
    ],
    [
      dnl Intel's compiler:
      dnl
      dnl -w1 stops the avalanche of uninteresting "remark" messages.
      dnl -wd... disables warnings which don't have good code workarounds.
      AM_CXXFLAGS="$AM_CXXFLAGS -Wall -w1 -wd177,1572"
      dnl Automatically add -Werror if maintainer mode is enabled.
      if test x$USE_MAINTAINER_MODE = xyes; then
	AM_CXXFLAGS="$AM_CXXFLAGS -Werror"
      fi
    ],
    [
      dnl GCC:
      dnl
      dnl -Wundef and -Wshadow were supported by g++ 3.0 (though -Wshadow was
      dnl buggy there).  We now require GCC >= 3.1, so can rely on them.
      dnl All the other options were supported by g++ 2.95 and there's little
      dnl likelihood Xapian will build with any earlier version, so there's
      dnl not much point worrying about whether older versions had them or not.
      AM_CXXFLAGS="$AM_CXXFLAGS -Wall -W -Wredundant-decls -Wpointer-arith -Wcast-qual -Wcast-align -Wno-long-long -Wformat-security -fno-gnu-keywords -Wundef -Wshadow"

      dnl The output of g++ --version seems to change with almost every minor
      dnl release so use the preprocessor macros which should be more robust.
      gxx_major_version=`echo __GNUC__|$CXX -E -|sed '/^#/d;s/ //g'`
      gxx_minor_version=`echo __GNUC_MINOR__|$CXX -E -|sed '/^#/d;s/ //g'`
      gxx_version=$gxx_major_version.$gxx_minor_version

      case $gxx_version in
      [[012]].* | 3.0)
	AC_MSG_ERROR([Xapian requires GCC 3.1 or later (you appear to have $gxx_version)])
	;;
      3.*) ;;
      *) dnl GCC >= 4.0
	dnl -Wstrict-null-sentinel was new 4.0.
	AM_CXXFLAGS="$AM_CXXFLAGS -Wstrict-null-sentinel"

	case $gxx_version in
	4.[[01]]) ;;
	4.2)
	  dnl -Wstrict-overflow was new in GCC 4.2.  Higher settings than 1
	  dnl have proved too noisy.
	  AM_CXXFLAGS="$AM_CXXFLAGS -Wstrict-overflow=1" ;;
	*)
	  dnl -Wlogical-op and -Wmissing-declarations (for C++) were added in
	  dnl GCC 4.3.
	  dnl
	  dnl -Winit-self was added in GCC 3.4, but up to at least GCC 4.4 it
	  dnl is ignored by g++ which warns for this case with -Wuninitialized
	  dnl (implied by -W).  But at some point this bug (filed upstream as
	  dnl http://gcc.gnu.org/PR34772) may get fixed so we pass this option
	  dnl anyway - we don't intend to use this idiom, so any instances are
	  dnl bugs we'd like to know about.
	  AM_CXXFLAGS="$AM_CXXFLAGS -Wstrict-overflow=1 -Winit-self -Wlogical-op -Wmissing-declarations" ;;
	esac

	dnl FIXME:
	dnl -Wconversion in older GCC versions is intended to help migration
	dnl from K&R to ISO C, and isn't useful for us.  In 4.3 it was renamed
	dnl to -Wtraditional-conversion and a new -Wconversion added which
	dnl sounds useful but is a bit buggy currently.  So we should consider
	dnl enabling -Wconversion once it is stabilised (GCC 4.4 or ...)
	dnl
	dnl -Wold-style-cast is interesting, but triggers for macros from
	dnl system headers (e.g. FD_SET) (tested with GCC 4.4).

	if test no = "$enable_visibility"; then
	  AC_DEFINE(XAPIAN_DISABLE_VISIBILITY, 1, [Define to disable use of visibility attributes])
	else
	  dnl Turn on visibility support for GCC >= 4.0.
	  AM_CXXFLAGS="$AM_CXXFLAGS -fvisibility=hidden"
	fi

	dnl Automatically add -Werror if maintainer mode is enabled and we're
	dnl using GCC4 or newer.  We don't do this for older GCCs as GCC 2.95
	dnl and some GCC 3.x compilers issue spurious warnings.
	if test x$USE_MAINTAINER_MODE = xyes; then
	  AM_CXXFLAGS="$AM_CXXFLAGS -Werror"
	fi
	;;
      esac
    ])
fi

AC_SUBST(AM_CXXFLAGS)

AH_BOTTOM(
[
/* For GCC >= 3.0 (and Intel's C++ compiler, which also defines __GNUC__),
 * we can use __builtin_expect to give the compiler hints about branch
 * prediction.  See HACKING for how to use these.
 */
#if defined __GNUC__ && __GNUC__ >= 3
/* The arguments of __builtin_expect() are both long, so use !! to ensure that
 * the first argument is always an integer expression, and always 0 or 1, but
 * still has the same truth value for the if or while it is used in.
 */
# define rare(COND) __builtin_expect(!!(COND), 0)
# define usual(COND) __builtin_expect(!!(COND), 1)
#else
# define rare(COND) (COND)
# define usual(COND) (COND)
#endif
])

AC_ARG_ENABLE(quiet,
  [AS_HELP_STRING([--enable-quiet], [Enable quiet building [default=yes]])],
  [case ${enableval} in
    yes|no) ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-quiet) ;;
  esac])

dnl Pass `--quiet' to libtool if quiet building is requested.  Using an
dnl AC_SUBST-ed value like this allows the choice to be easily overridden
dnl if you want to see exactly what make and libtool are up to - just run
dnl make like this: `make QUIET='
QUIET=
if test yes = "$enable_quiet"; then
  QUIET=--quiet
fi
AC_SUBST(QUIET)

dnl Restore CXXFLAGS to those the user specified or autoconf defaulted to.
CXXFLAGS=$save_CXXFLAGS

dnl Required for auto regeneration to work in a combined maintainer-mode tree.
: ${AUTOM4TE=autom4te}
AC_SUBST([AUTOM4TE])

dnl **************************
dnl * Build the output files *
dnl **************************

AC_CONFIG_FILES([Makefile docs/Makefile])
AC_CONFIG_FILES([makemanpage], [chmod +x makemanpage])
AC_OUTPUT
