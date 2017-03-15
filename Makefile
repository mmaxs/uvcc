
# project directory
ROOT = $(CURDIR)
BUILD_ROOT = $(ROOT)/build


# libuv path
ifeq ($(platform),WINDOWS)
LIBUV = $(ROOT)/libuv-x64-v1.9.1.build10
endif


# CXX common settings and flags: CXX CXXSTD
CXX_FILE_SUFFIXES = .cpp .c
CXX_HEADER_SUFFIXES = .hpp .h

CXX = c++
ifeq ($(platform),WINDOWS)
CXX = x86_64-w64-mingw32-c++
endif
ifdef VERBOSE
CXX += -v
endif

CXXSTD = -std=c++1y


# preprocessor flags: CPPFLAGS IFLAGS
CPPFLAGS =
ifdef UVCC_DEBUG
CPPFLAGS += -D UVCC_DEBUG=$(UVCC_DEBUG)
else
CPPFLAGS += -D NDEBUG
endif
ifeq ($(platform),WINDOWS)
CPPFLAGS += -D _WIN32_WINNT=0x0601
endif

IFLAGS = -iquote $(ROOT)/src/include
ifeq ($(platform),WINDOWS)
IFLAGS = -iquote $(ROOT)/src/include -I $(LIBUV)/include
endif


# compiler flags: CXXFLAGS WFLAGS
CXXFLAGS = $(CXXSTD) -O2 -g -pipe

ifdef UVCC_DEBUG
WFLAGS += -Wall -Wpedantic -Wno-variadic-macros
else
WFLAGS += -Wall -Wpedantic
endif


# linker flags: LDFLAGS LDLIBS
LDFLAGS =
LDLIBS =

# static linking
#  use LDSTATIC variable to specify static linking options
#
#  for simple applications on Windows, we normally prefer the standard C/C++ libraries
#  to have been statically linked as far as the system usually lacks the gcc's runtime libraries,
#  e.g.:
#   LDSTATIC = -static-libgcc -static-libstdc++
#  or
#   make LDSTATIC="-static-libgcc -static-libstdc++" <goal>
#  or define the target-specific assignment for appropriate goals in local makefiles
#   <goal>: LDSTATIC = -static-libgcc -static-libstdc++
#
#  in any case all other the mingw32/gcc/OS libraries are forced to be always linked dynamically when
#  the -static flag is not specified, which also prevents dynamic linking with shared libraries at all,
#  therefore if there is a specific library that we want to link with dynamically, we should then
#  switch static linking off, specify the desired library, and then switch static linking state back on again
#  using LDLIBS variable, e.g.:
#   LDLIBS = -Wl,-Bdynamic -L $(LIBUV) -luv -Wl,-Bstatic
#  or, instead, just directly specify the desired .so/.dll file for dynamic linking with, e.g.:
#   LDLIBS += $(LIBUV)/libuv.dll
#  which can be more preferable in this case and is as good as specifing a particular .a/.lib file
#  for static linking with some desired library, e.g.:
#   LDLIBS += $(LIBUV)/libuv.a

# building a shared library
#  use Makefile.d/link.rules and define the target-specific assignment
#  to CXXFLAGS, LDFLAGS, and LDOUT for appropriate goals, e.g.
#   <goal>: CXXFLAGS += -fPIC
#   <goal>: private override LDFLAGS += -shared
#   <goal>: private LDOUT = lib$@.so
#
#  alternatively, use Makefile.d/link-so.rules which basically do the above but leaving aside CXXFLAGS
#  (so don't forget about specifying -fpic/-fPIC options requred for compilation stage),
#  also when Makefile.d/link-so.rules is used if SONAME_VERSION variable is nonempty, LDFLAGS is added with:
#   -Wl,-soname=$(notdir $(LDOUT)).$(SONAME_VERSION)
#  and if both SONAME_VERSION and SOFILE_VERSION variables are nonempty, linker output is set to
#   $(LDOUT).$(SONAME_VERSION).$(SOFILE_VERSION)


# AR common settings and flags
AR = ar
ARFLAGS = rvus


# common sources
SOURCES += $(ROOT)/src/*.cpp


export


# no default goal
# (define it with recursively expanded assignment from any empty variable)
.DEFAULT_GOAL = $@


# project goals and goal specific variables

%: test/% example/%;

test/%:
	$(MAKE) -C test $*

example/%:
	$(MAKE) -C example $*


.PHONY: doc doc-internal doc-commit
doc doc-internal doc-commit: doc-gh-pages = $(ROOT)/doc/html

doc:
	realpath -e "$(doc-gh-pages)"
	test "$$(realpath -e "$(doc-gh-pages)")" != "/"
	rm -vIrf "$(doc-gh-pages)"/*
	doxygen doc/Doxyfile-1.8.13

doc-internal:
	realpath -e "$(doc-gh-pages)"
	test "$$(realpath -e "$(doc-gh-pages)")" != "/"
	rm -vIrf "$(doc-gh-pages)"/*
	sed 's/^\(INTERNAL_DOCS\s*=\s*\)\S*$$/\1YES/' doc/Doxyfile-1.8.13 | doxygen -

doc-commit:
	realpath -e "$(doc-gh-pages)" || exit; \
	test "$$(realpath -e "$(doc-gh-pages)")" != "/" || exit; \
	cd "$(doc-gh-pages)"; \
	doc_version=$$(git log --format="%s" gh-pages); \
	let ++doc_version; \
	git checkout --orphan $$doc_version; \
	git add .; \
	git commit -m $$doc_version; \
	git branch -M gh-pages; \
	git -c gc.reflogExpireUnreachable=now -c gc.pruneExpire=now gc; \
	git push public


# the summary of the predefined implicit rules
#
# LINK.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
# COMPILE.cc = $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
# %.cc:
# %: %.cc
#         $(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
# %.o: %.cc
#         $(COMPILE.cc) $(OUTPUT_OPTION) $<
# .cc:
#         $(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
# .cc.o:
#         $(COMPILE.cc) $(OUTPUT_OPTION) $<
#
# LINK.cpp = $(LINK.cc)
# COMPILE.cpp = $(COMPILE.cc)
# %.cpp:
# %: %.cpp
#         $(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@
# %.o: %.cpp
#         $(COMPILE.cpp) $(OUTPUT_OPTION) $<
# .cpp:
#         $(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@
# .cpp.o:
#         $(COMPILE.cpp) $(OUTPUT_OPTION) $<
#
# LINK.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)
# COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
# %.c:
# %: %.c
#         $(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@
# %.o: %.c
#         $(COMPILE.c) $(OUTPUT_OPTION) $<
# .c:
#         $(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@
# .c.o:
#         $(COMPILE.c) $(OUTPUT_OPTION) $<
#

