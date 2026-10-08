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

check: memoryCheck
	./memoryCheck
	./memoryCheck --DBRC
	./memoryCheck --map ${GIDI_PLUS_PATH}/GIDI/Test/Data/MG_MC/all.map --pid photon --tid O16
	./memoryCheck --map ${GIDI_PLUS_PATH}/GIDI/Test/Data/MG_MC/neutrons/all.map --tid HinCH2
