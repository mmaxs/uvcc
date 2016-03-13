
# additional settings, sources, and prerequisites for specific targets
windows/%: LDLIBS += -lWs2_32


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

# windows-specific builds use generic rules
windows/%:
	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $*
	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $*

# use generic rules for all other targets
%:
	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $@
	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $@


