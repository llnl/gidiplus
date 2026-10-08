SHELL = /bin/sh

# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>

GIDI_PLUS_PATH ?= $(abspath ../../..)
CppSource = $(sort $(wildcard *.cpp))

include $(GIDI_PLUS_PATH)/Makefile.paths
include ../Makefile.check

check: $(Executables)
	if [ ! -e Outputs ]; then mkdir Outputs; fi
	./computeProducts ../../../GIDI/Test/Data/MG_MC/all.ris Th227 > Outputs/computeProducts.out
	-diff -r Benchmarks Outputs > /dev/null; if [ $$? -ne 0 ]; then echo "FAILURE: computeProducts"; fi
