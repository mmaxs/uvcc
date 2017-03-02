
# project directory
ROOT = $(CURDIR)
BUILD_ROOT = $(ROOT)/build


# libuv path
ifdef WINDOWS
LIBUV = $(ROOT)/libuv-x64-v1.9.1.build10
endif


# CXX common settings and flags
CXX_FILE_SUFFIXES = .cpp .c
CXX_HEADER_SUFFIXES = .hpp .h
CXX = c++
ifdef WINDOWS
CXX = x86_64-w64-mingw32-c++
endif
CXXSTD = -std=c++1y


# preprocessor flags
IFLAGS = -iquote $(ROOT)/src/include
ifdef WINDOWS
IFLAGS = -iquote $(ROOT)/src/include -I $(LIBUV)/include
endif
CPPFLAGS = $(IFLAGS)
ifndef DEBUG
CPPFLAGS += -D NDEBUG
endif
ifdef WINDOWS
CPPFLAGS += -D _WIN32_WINNT=0x0601
endif


# compiler flags
CXXFLAGS = $(CXXSTD) -Wall -Wpedantic -O2 -g -pipe


# linker flags
LDFLAGS =
LDLIBS = -luv
ifdef WINDOWS
LDFLAGS = -static-libgcc -static-libstdc++
LDLIBS = -L $(LIBUV) -luv
endif

# static linking
# use LDSTATIC variable to specify static linking options, e.g.:
#   LDSTATIC = -static
# or
#   make LDSTATIC="-static -static-libgcc -static-libstdc++" <goal>
# or define the target-specific assignment for appropriate goals in local makefiles
#   <goal>: LDSTATIC = -static

# linking a shared library
# use Makefile.d/link.rules and define the target-specific assignment
# to CXXFLAGS, LDFLAGS, and LDOUT for appropriate goals in local makefiles, e.g.
#   <goal>: CXXFLAGS += -fPIC
#   <goal>: private LDFLAGS += -shared
#   <goal>: private LDOUT = lib$@.so
# alternatively, use Makefile.d/link-so.rules which basically do the above but leaving aside CXXFLAGS
# also when Makefile.d/link-so.rules is used
# 	if SONAME_VERSION is nonempty LDFLAGS is added with -Wl,-soname=$(notdir $(LDOUT)).$(SONAME_VERSION)
# 	if both SONAME_VERSION and SOFILE_VERSION are nonempty linker output is set to $(LDOUT).$(SONAME_VERSION).$(SOFILE_VERSION)


# AR common settings and flags
AR = ar
ARFLAGS = rvus


# common sources
SOURCES += $(ROOT)/src/*.cpp


export


# no default goal
# (define it with recursively expanded assignment from any empty variable)
.DEFAULT_GOAL = $@


# project goals

%: test/% example/%;

test/%:
	$(MAKE) -C test $*

example/%:
	$(MAKE) -C example $*


.PHONY: doc
doc:
	rm -rf doc/html/*
	doxygen doc/Doxyfile-1.8.13



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

