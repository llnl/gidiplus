SHELL = /bin/sh

# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>

.PHONY: default bin all include lib check clean realclean docs doDIRS

DIRS = Src include lib Test

default: include lib
	cd Test; $(MAKE) default

all: default

include:
	cd include; $(MAKE)

lib:
	cd lib; $(MAKE)

check:
	cd Test; $(MAKE) check

clean:
	$(MAKE) doDIRS _DIRS="$(DIRS)" _TARGET=clean

realclean:
	$(MAKE) doDIRS _DIRS="$(DIRS)" _TARGET=realclean

docs:
#	doxygen Doc/doxygen/Doxyfile

doDIRS:
	SAVED_PWD=`pwd`; \
	for DIR in $(_DIRS); do cd $$DIR; $(MAKE) $(_TARGET); cd $$SAVED_PWD; done
