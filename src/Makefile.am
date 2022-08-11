lib_LTLIBRARIES = libmasterkey.la
libmasterkey_la_SOURCES = \
	common.hpp \
	fwd.hpp \
	misc.hpp \
	option.hpp \
	option.cpp \
	sched.hpp \
	sched.cpp \
	sighandler.hpp \
	sighandler.cpp \
	target.hpp \
	target.cpp
libmasterkey_la_LDFLAGS = -version-info $(so_version_info)

bin_PROGRAMS = masterkey
masterkey_DEPENDENCIES = libmasterkey.la
masterkey_SOURCES = \
	common.hpp \
	misc.hpp \
	option.hpp \
	target.hpp \
	masterkey.cpp
masterkey_LDADD = libmasterkey.la

masterkey_CXXFLAGS = -std=c++17 $(warning_options)

.PHONY: list-warning-options
list-warning-options:
	$(AM_V_CXX)LANG=C $(CXX) -Q --help=warnings,^joined,^separate,common --help=warnings,^joined,^separate,c++ $(warning_options) -x c++ -o /dev/null /dev/null

clean-local:
	$(RM) -r *.gcno
