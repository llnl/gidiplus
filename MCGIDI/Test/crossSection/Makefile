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

check: floatDiffs crossSections crossSectionSum
	if [ ! -e Outputs ]; then mkdir Outputs; fi
	./crossSections > Outputs/crossSections.out
	../Utilities/diff.com crossSection/crossSections Benchmarks/crossSections.out Outputs/crossSections.out

	./crossSections --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map --pn > Outputs/crossSections.photon+O16.out
	../Utilities/diff.com crossSection/crossSections-1 Benchmarks/crossSections.photon+O16.out Outputs/crossSections.photon+O16.out
	./crossSections --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map --pn > Outputs/crossSections.photon+O16.out
	../Utilities/diff.com crossSection/crossSections-2 Benchmarks/crossSections.photon+O16.out Outputs/crossSections.photon+O16.out
	./crossSections --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map --pa > Outputs/crossSections.photon+O16.atomic.out
	../Utilities/diff.com crossSection/crossSections-3 Benchmarks/crossSections.photon+O16.atomic.out Outputs/crossSections.photon+O16.atomic.out
	./crossSections --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map --pa --pn > Outputs/crossSections.photon+O16.atomic+nuclear.out
	../Utilities/diff.com crossSection/crossSections-4 Benchmarks/crossSections.photon+O16.atomic+nuclear.out Outputs/crossSections.photon+O16.atomic+nuclear.out

	./crossSections --tid HinCH2 --map ../../../GIDI/Test/Data/MG_MC/neutrons/all.map > Outputs/crossSections.HinCH2.out
	../Utilities/diff.com crossSection/crossSections-HinCH2 Benchmarks/crossSections.HinCH2.out Outputs/crossSections.HinCH2.out

	./crossSectionSum > Outputs/crossSectionSum.out
	../Utilities/diff.com crossSectionSum/crossSectionSum Benchmarks/crossSectionSum.out Outputs/crossSectionSum.out
