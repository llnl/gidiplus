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

check: floatDiffs sampleReactions
	if [ ! -e Outputs ]; then mkdir Outputs; fi
	./sampleReactions > Outputs/sampleReactions.out
	../Utilities/diff.com sampleReactions/sampleReactions Benchmarks/sampleReactions.out Outputs/sampleReactions.out

	./sampleReactions --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map > Outputs/sampleReactions.photon+O16.out
	../Utilities/diff.com sampleReactions/sampleReactions-1 Benchmarks/sampleReactions.photon+O16.out Outputs/sampleReactions.photon+O16.out
	./sampleReactions --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map -pn > Outputs/sampleReactions.photon+O16.out
	../Utilities/diff.com sampleReactions/sampleReactions-2 Benchmarks/sampleReactions.photon+O16.out Outputs/sampleReactions.photon+O16.out
	./sampleReactions --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map -pa > Outputs/sampleReactions.photon+O16.atomic.out
	../Utilities/diff.com sampleReactions/sampleReactions-3 Benchmarks/sampleReactions.photon+O16.atomic.out Outputs/sampleReactions.photon+O16.atomic.out
	./sampleReactions --pid photon --tid O16 --map ../../../GIDI/Test/Data/MG_MC/all.map -pa -pn > Outputs/sampleReactions.photon+O16.atomic+nuclear.out
	../Utilities/diff.com sampleReactions/sampleReactions-4 Benchmarks/sampleReactions.photon+O16.atomic+nuclear.out Outputs/sampleReactions.photon+O16.atomic+nuclear.out

	./sampleReactions --tid HinCH2 --map ../../../GIDI/Test/Data/MG_MC/neutrons/all.map > Outputs/sampleReactions.HinCH2.out
	../Utilities/diff.com sampleReactions/sampleReactions-HinCH2 Benchmarks/sampleReactions.HinCH2.out Outputs/sampleReactions.HinCH2.out
