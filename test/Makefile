
# additional settings, sources, and prerequisites for specific targets
windows/%: LDLIBS += -lWs2_32


export


# no default goal
# (define it with recursively expanded assignment from any empty variable)
.DEFAULT_GOAL = $@


# pattern rules shall contain recipes to not to be ignored by make
# as a user redefinition of the built-in rules to cancel them;
# therefore we duplicate the commands from the generic rule here
#hello% example%: $(libnetcc)
#	# use generic rules for compiling and linking
#	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $@
#	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $@

# building the required library
#$(libnetcc):
#	$(MAKE) -C $(ROOT) lib/netcc


# use generic rules for 'clean'
clean:
	$(MAKE) -f $(ROOT)/Makefile.d/clean.rules $@
#	# clean parts related to additional sources
#	$(MAKE) -C $(ROOT) lib/$@

# windows-specific builds use generic rules
windows/%:
	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $*
	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $*

# use generic rules for all other targets
%:
	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $@
	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $@


