TESTS = testsuite
check_PROGRAMS = testsuite

testsuite_SOURCES = \
	test.cpp \
	$(top_srcdir)/src/option.cpp
nodist_testsuite_SOURCES = gtest/gtest.h gtest/gtest-all.cc

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS = -std=c++17 --coverage $(warning_options) $(sanitizer_flags)
## XXX: Warning suppresions(workaround) for Google Test header("gtest/gtest.h").
AM_CXXFLAGS += -Wno-ctor-dtor-privacy -Wno-duplicated-branches \
-Wno-effc++ -Wno-inline -Wno-missing-declarations -Wno-multiple-inheritance \
-Wno-namespaces -Wno-null-dereference -Wno-sign-conversion -Wno-suggest-attribute=format \
-Wno-suggest-attribute=noreturn -Wno-suggest-attribute=pure -Wno-suggest-final-methods \
-Wno-suggest-final-types -Wno-suggest-override -Wno-padded -Wno-switch-default \
-Wno-switch-enum -Wno-templates -Wno-undef -Wno-unused-const-variable \
-Wno-unused-macros -Wno-useless-cast -Wno-zero-as-null-pointer-constant
LDADD = @LIBS@

gtest_ver = release-1.10.0

$(testsuite_SOURCES) gtest/gtest-all.cc: gtest/gtest.h
gtest/gtest.h: googletest-$(gtest_ver)
	$^/googletest/scripts/fuse_gtest_files.py $^/googletest .

googletest-$(gtest_ver):
	wget -nv -O - https://github.com/google/googletest/archive/$(gtest_ver).tar.gz | tar xzvf -

check-local:
	gcovr -s -r $(top_srcdir) --filter='^$(top_srcdir)/src/' --html --html-details -o coverage.html
	$(RM) *.gcda $(top_builddir)/src/*.gcda

clean-local:
	$(RM) -r *.log *.trs *.css *.html *.gcda *.gcno
