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
	./map > Outputs/map.out
	../Utilities/diff.com map/map Benchmarks/map.out Outputs/map.out
	./map --map neutrons1.2.1.map > Outputs/map.2.1.out
	../Utilities/diff.com map/map.2.1 Benchmarks/map.2.1.out Outputs/map.2.1.out
	./nesting > Outputs/nesting.out
	../Utilities/diff.com map/nesting Benchmarks/nesting.out Outputs/nesting.out
	./libraries > Outputs/libraries.out
	../Utilities/diff.com map/libraries Benchmarks/libraries.out Outputs/libraries.out
