#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([masterkey], [0.0.1], [BUG-REPORT-ADDRESS])
so_version_info=0:0:0
AC_SUBST([so_version_info])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
AC_LANG([C++])
AC_CONFIG_SRCDIR([src/masterkey.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_SED
LT_INIT
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX


# Checks for libraries.
AC_CHECK_LIB([pthread], [main]) # Google Test requires pthread on POSIX system.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS([memset munmap strerror])
AC_CHECK_DECLS([htobe16 htobe32 htobe64])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
[warning_options="-Wextra -Wcast-align -Wstrict-aliasing -Wshadow "\
`LANG=C ${CXX} -fsyntax-only -Q --help=warnings,^joined,^separate,common --help=warnings,^joined,^separate,c++ |
${GREP} -v '\@<:@enabled\@:>@\|-Wabi\|-Waggregate-return\|-Wchkp\|-Wc90-c99-compat\|-Wnamespaces\|-Wpadded\|-Wredundant-tags\|-Wsuggest-attribute=const\|-Wsystem-headers\|-Wtemplates\|-Wtraditional@<:@^-@:>@' |
${GREP} -oe '-W@<:@@<:@:graph:@:>@@:>@\+' |
${SED} -e 's/<@<:@0-9,@:>@\+>//' |
${SED} -e '$!s/$/ \\\\/'`]
AC_SUBST([warning_options])
[system_include_dirs=`LANG=C ${CPP} ${CPPFLAGS} -v -x c /dev/null -o /dev/null 2>&1 |
${SED} -e '/^#include "/,/^End of search list\.$/p;d' |
${SED} -e '/^ /{s///;s/$/\/\\\\*/;p};d' |
${SED} -e '$!s/$/ \\\\/'`]
AC_SUBST([system_include_dirs])
AC_ARG_ENABLE([sanitizer],
	[AS_HELP_STRING([--enable-sanitizer], [enable sanitizer])],
	[AS_VAR_IF([enable_sanitizer], [yes],
		[sanitizer_flags='-static-libasan -static-liblsan -static-libubsan'])
	AC_SUBST([sanitizer_flags])])

AS_IF([which git > /dev/null && test -d ${srcdir}/.git], [
	git_commit_id=`git -C ${srcdir} rev-parse --short HEAD``git -C ${srcdir} status --porcelain | grep -qe '^@<:@^?@:>@@<:@^? @:>@' && echo '*'`
	AC_DEFINE_UNQUOTED([GIT_COMMIT_ID], ["${git_commit_id}"], [Git commit ID])
])

AC_OUTPUT
