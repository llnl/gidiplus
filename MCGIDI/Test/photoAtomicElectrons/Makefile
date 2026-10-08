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

clean:
	rm -rf photoAtomicElectrons *.o Outputs file1 file2 *.out *.core

#check: photoAtomicDopplerBroadening
#	if [ ! -e Outputs ]; then mkdir Outputs; fi
#	./photoAtomicDopplerBroadening > Outputs/photoAtomicDopplerBroadening.out
#	../Utilities/diff.com photoAtomicDopplerBroadening/photoAtomicDopplerBroadening Benchmarks/photoAtomicDopplerBroadening.out Outputs/photoAtomicDopplerBroadening.out
