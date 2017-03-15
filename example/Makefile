
# additional settings, sources, and prerequisites for specific targets

ifdef DEBUG
CPPFLAGS += -U NDEBUG
else
CPPFLAGS += -D NDEBUG
endif

WFLAGS += -Wall -Wpedantic -Wno-variadic-macros -Wno-format-zero-length

ifeq ($(platform),WINDOWS)
LDSTATIC += -static-libgcc -static-libstdc++ -static
LDLIBS += $(LIBUV)/libuv.dll
else
LDLIBS += -luv
endif


export


# no default goal
# (define it with recursively expanded assignment from any empty variable)
.DEFAULT_GOAL = $@


# use generic rules for 'clean'
.PHONY: clean
clean:
	# clean parts related to example sources
	$(MAKE) -f $(ROOT)/Makefile.d/clean.rules $@
	# clean parts related to main sources
	$(MAKE) -C $(ROOT)/src -f $(ROOT)/Makefile.d/clean.rules $@


# use generic rules for all other targets
%:
	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $@
	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $@


