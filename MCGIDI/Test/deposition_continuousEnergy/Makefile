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

check: floatDiffs deposition_continuousEnergy photo-atomic
	$(PYTHON) deposition_continuousEnergy.py H1
	$(PYTHON) deposition_continuousEnergy.py O16
	$(PYTHON) deposition_continuousEnergy.py Th227
	$(PYTHON) deposition_continuousEnergy.py Th227 -d
	$(PYTHON) deposition_continuousEnergy.py Th227  --useSlowerContinuousEnergyConversion
	$(PYTHON) deposition_continuousEnergy.py Th227 -d --useSlowerContinuousEnergyConversion
	./deposition_continuousEnergy --map ../../../GIDI/Test/Data/MG_MC/all_maps.map --pid photon --tid H1 --pa > Outputs/deposition_continuousEnergy.photoat+H1.out
	$(PYTHON) diff.py deposition_continuousEnergy/photoat+H1.out  Benchmarks/deposition_continuousEnergy.photoat+H1.out Outputs/deposition_continuousEnergy.photoat+H1.out
	./photo-atomic > Outputs/photo-atomic.out
	$(PYTHON) diff.py deposition_continuousEnergy/photo-atomic Benchmarks/photo-atomic.out Outputs/photo-atomic.out
