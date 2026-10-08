SHELL = /bin/sh

# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>

# These must be set by hand when we do a release.
gidiplus_major = 3
gidiplus_minor = 35
baseTag = GIDI_plus.$(gidiplus_major).$(gidiplus_minor).0

DIRS_GIDI_plus = LUPI Python HAPI GUPI PoPI CADI RISI GIDI MCGIDI include lib Doc
DIRS_default2 = numericalFunctions $(DIRS_GIDI_plus)
DIRS = pugixml $(DIRS_default2)

GIDI_PLUS_PATH ?= $(abspath .)
export GIDI_PLUS_PATH

include Makefile.paths

PREFIX = `pwd`/install

.PHONY: default all bin default2 pugixml pugixml_dummy install docs check clean realclean tar doDIRS

default:
	$(MAKE) _TARGET=default default2
	echo 'Note, bin executables not built, use target bin or all to build bin executables.'

all:
	$(MAKE) _TARGET=default default2
	$(MAKE) _TARGET=bin _DIRS="$(DIRS_GIDI_plus)" doDIRS

bin:
	$(MAKE) _TARGET=bin default2

default2: pugixml
	@echo
	@echo "INFO: GIDI_PLUS_PATH = $(GIDI_PLUS_PATH)"
	@echo "INFO: CXX            = $(CXX)"
	@echo "INFO: CXXFLAGS       = $(CXXFLAGS)"
	@echo "INFO: CC             = $(CC)"
	@echo "INFO: CFLAGS         = $(CFLAGS)"
	@echo "INFO: PREFIX         = $(PREFIX)"
	@echo "INFO: PUGIXML_PATH   = $(PUGIXML_PATH)"
	@echo "INFO: HDF5_PATH      = $(HDF5_PATH)"
	@echo "INFO: HDF5_INCLUDE   = $(HDF5_INCLUDE)"
	@echo "INFO: HDF5_LIB       = $(HDF5_LIB)"
	cd pugixml; $(MAKE) default CXXFLAGS="$(CXXFLAGS)"
	echo ""
	echo "  -- numericalFunctions --"
	cd numericalFunctions; $(MAKE) default
	$(MAKE) doDIRS _DIRS="$(DIRS_default2)"

pugixml:
	rm -rf pugixml pugixml-1.15
	unzip -q Misc/pugixml-1.15.zip
	ln -s pugixml-1.15 pugixml
	cd pugixml; tar -xf ../Misc/pugixml.addon.tar

pugixml_dummy:
	rm -rf pugixml-1.15 pugixml
	mkdir pugixml
	cp Misc/Makefile_dummy pugixml/Makefile

gidiplus_version.h: FORCE
	@if test -d "./.git"; then \
		echo "#define GIDIPLUS_MAJOR ${gidiplus_major}"  > gidiplus_version.h; \
		echo "#define GIDIPLUS_MINOR ${gidiplus_minor}" >> gidiplus_version.h; \
		git describe --long --match ${baseTag} | awk -F "-" '{if (NF>2) {printf("#define GIDIPLUS_PATCHLEVEL %d\n", $$(NF-1))} else {print "#define GIDIPLUS_PATCHLEVEL 0"}}' >> gidiplus_version.h; \
		git rev-parse HEAD                     | awk '{printf("#define GIDIPLUS_GIT %s\n" , $$1)}'                 >> gidiplus_version.h; \
		git ls-files -s numericalFunctions     | awk '{printf("#define NUMERICAL_FUNCTIONS_GIT %s\n" , $$2)}'      >> gidiplus_version.h; \
		git ls-files -s HAPI                   | awk '{printf("#define HAPI_GIT %s\n" , $$2)}'                     >> gidiplus_version.h; \
		git ls-files -s PoPI                   | awk '{printf("#define POPI_GIT %s\n" , $$2)}'                     >> gidiplus_version.h; \
		git ls-files -s GIDI                   | awk '{printf("#define GIDI_GIT %s\n" , $$2)}'                     >> gidiplus_version.h; \
		git ls-files -s MCGIDI                 | awk '{printf("#define MCGIDI_GIT %s\n" , $$2)}'                   >> gidiplus_version.h; \
	fi
	cat gidiplus_version.h

install: default
	mkdir -p $(PREFIX)/include
	cp -p include/* $(PREFIX)/include
	mkdir -p $(PREFIX)/lib
	cp -p lib/libgidiplus.a $(PREFIX)/lib

docs:
	$(MAKE) doDIRS _DIRS="$(DIRS_GIDI_plus)" _TARGET=docs

check:
	$(MAKE) doDIRS _DIRS="$(DIRS)" _TARGET=check

clean: FORCE 
	if [ ! -e pugixml ]; then $(MAKE) pugixml_dummy; fi
	$(MAKE) doDIRS _DIRS="$(DIRS)" _TARGET=clean

realclean: pugixml_dummy
	$(MAKE) doDIRS _DIRS="$(DIRS)" _TARGET=realclean
	rm -rf pugixml test_install

tar: gidiplus_version.h
	if git status | grep "new commit"; then echo "ERROR: 'git submodule update --recursive' needed"; exit 1; fi
	$(MAKE) -s realclean
	fileName=`git describe --long --match ${baseTag} | awk -F "-" '{(NF>2) ? patchVal=$$(NF-1) : patchVal=0; printf "gidiplus-%s.%s.%s.%s", ${gidiplus_major}, ${gidiplus_minor}, patchVal, $$NF}'`; \
	if [ "$$fileName" == "" ]; then exit; fi; \
	rm -rf ../$$fileName; \
	mkdir ../$$fileName; \
	absolutePath=`cd ../$$fileName; pwd`; \
	git archive --format=tar HEAD | (cd ../$$fileName && tar -xf -); \
	git submodule foreach --recursive "git archive --prefix=\$$displaypath/ --format=tar HEAD | (cd $$absolutePath && tar -xf -)"; \
	cp -r GIDI/Test/Data ../$$fileName/GIDI/Test; \
	find ../$$fileName -iname ".git*" -exec rm {} \; ; \
	find ../$$fileName -iname "Makefile.popskit" -exec rm {} \; ; \
	rm -rf ../$${fileName}/custom_hooks ../$${fileName}/GIDI/Test/GIDI_Test_Data.tar.gz; \
	cp gidiplus_version.h ../$$fileName; \
	cd ../; \
	tar -cf $${fileName}.tar $$fileName

snapshot: gidiplus_version.h
	fileName=`git describe --long --match ${baseTag} | awk -F "-" '{(NF>2) ? patchVal=$$(NF-1) : patchVal=0; printf "gidiplus-%s.%s.%s.%s-snapshot", ${gidiplus_major}, ${gidiplus_minor}, patchVal, $$NF}'`; \
	if [ "$$fileName" == "" ]; then exit; fi; \
	rm -rf ../$$fileName; \
	mkdir ../$$fileName; \
	git ls-files --recurse-submodules -z | tar --null -T - gidiplus_version.h GIDI/Test/Data -czf ../$$fileName/tmp.tar.gz; \
	cd ../$$fileName; \
	tar -xf tmp.tar.gz; \
	rm tmp.tar.gz; \
	find . -iname ".git*" -exec rm {} \; ; \
	find . -iname "Makefile.popskit" -exec rm {} \; ; \
	rm -rf custom_hooks GIDI/Test/GIDI_Test_Data.tar.gz; \
	cd ../; \
	tar -cf $${fileName}.tar $$fileName

doDIRS:
	SAVED_PWD=`pwd`; \
	for DIR in $(_DIRS); do cd $$DIR; $(MAKE) $(_TARGET) CXXFLAGS="$(CXXFLAGS)"; cd $$SAVED_PWD; done

FORCE:
