lib_LTLIBRARIES = libyadd.la
libyadd_la_SOURCES = \
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

bin_PROGRAMS = yadd
yadd_DEPENDENCIES = libyadd.la
yadd_SOURCES = \
	common.hpp \
	misc.hpp \
	option.hpp \
	target.hpp \
	yadd.cpp
yadd_LDADD = libyadd.la

yadd_CXXFLAGS = -std=c++17 $(warning_options)

.PHONY: list-warning-options
list-warning-options:
	$(AM_V_CXX)LANG=C $(CXX) -Q --help=warnings,^joined,^separate,common --help=warnings,^joined,^separate,c++ $(warning_options) -x c++ -o /dev/null /dev/null

clean-local:
	$(RM) -r *.gcno
