bin_PROGRAMS = ddplus
ddplus_SOURCES = \
	common.hpp \
	ddplus.cpp \
	fwd.hpp \
	misc.hpp \
	option.hpp \
	option.cpp \
	target.hpp \
	target.cpp

ddplus_CXXFLAGS = -std=c++17 $(warning_options)

.PHONY: list-warning-options
list-warning-options:
	$(AM_V_CXX)LANG=C $(CXX) -Q --help=warnings,^joined,^separate,common --help=warnings,^joined,^separate,c++ $(warning_options) -x c++ -o /dev/null /dev/null

clean-local:
	$(RM) -r *.gcno
