bin_PROGRAMS = yadd
yadd_SOURCES = \
	common.hpp \
	yadd.cpp \
	fwd.hpp \
	misc.hpp \
	option.hpp \
	option.cpp \
	sched.hpp \
	sched.cpp \
	target.hpp \
	target.cpp

yadd_CXXFLAGS = -std=c++17 $(warning_options)

.PHONY: list-warning-options
list-warning-options:
	$(AM_V_CXX)LANG=C $(CXX) -Q --help=warnings,^joined,^separate,common --help=warnings,^joined,^separate,c++ $(warning_options) -x c++ -o /dev/null /dev/null

clean-local:
	$(RM) -r *.gcno
