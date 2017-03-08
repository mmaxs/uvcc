
# additional settings, sources, and prerequisites for specific targets
ifdef WINDOWS
endif


export


# no default goal
# (define it with recursively expanded assignment from any empty variable)
.DEFAULT_GOAL = $@


# use generic rules for 'clean'
.PHONY: clean
clean:
	# clean parts related to test sources
	$(MAKE) -f $(ROOT)/Makefile.d/clean.rules $@
	# clean parts related to main sources
	$(MAKE) -C $(ROOT)/src -f $(ROOT)/Makefile.d/clean.rules $@


# use generic rules for all other targets
%:
	$(MAKE) -f $(ROOT)/Makefile.d/compile.rules $@
	$(MAKE) -f $(ROOT)/Makefile.d/link.rules $@


