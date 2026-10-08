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
	./listOfTransportableSuites > Outputs/listOfTransportableSuites.out
	../Utilities/diff.com listOfTransportableSuites/listOfTransportableSuites Benchmarks/listOfTransportableSuites.out Outputs/listOfTransportableSuites.out
