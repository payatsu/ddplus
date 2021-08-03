bin_PROGRAMS = memmap
memmap_SOURCES = \
	memmap.cpp \
	misc.hpp \
	option.hpp \
	option.cpp

memmap_CXXFLAGS = -std=c++17 $(warning_options)

.PHONY: list-warning-options
list-warning-options:
	$(AM_V_CXX)LANG=C $(CXX) -Q --help=warnings,^joined,^separate,common --help=warnings,^joined,^separate,c++ $(warning_options) -x c++ -o /dev/null /dev/null

clean-local:
	$(RM) -r *.gcno
