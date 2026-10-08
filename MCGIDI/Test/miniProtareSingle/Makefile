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

#	./miniProtareSingle n H2 nuclear                 > Outputs/miniProtareSingle.n+H2.out
#	./miniProtareSingle n H2 nuclear --multiGroup    > Outputs/miniProtareSingle.n+H2.mg.out

check: floatDiffs miniProtareSingle 
	if [ ! -e Outputs ]; then mkdir Outputs; fi
	$(MAKE) checkSub Args='n H2 nuclear'                                    file='miniProtareSingle.n+H2.out'
	$(MAKE) checkSub Args='n H2 nuclear --multiGroup'                       file='miniProtareSingle.n+H2.mg.out'
	$(MAKE) checkSub Args='n O16 nuclear --crossSection 1e-40'              file='miniProtareSingle.n+O16.out'
	$(MAKE) checkSub Args='n O16 nuclear --crossSection 1e-40 --multiGroup' file='miniProtareSingle.n+O16.mg.out'
	$(MAKE) checkSub Args='H1 He3 nuclear '                                 file='miniProtareSingle.H1.He3.out'
	$(MAKE) checkSub Args='H1 He3 nuclear --multiGroup'                     file='miniProtareSingle.H1.He3.mg.out'
	$(MAKE) checkSub Args='H3 U235 nuclear'                                 file='miniProtareSingle.H3.U235.out'
	$(MAKE) checkSub Args='H3 U235 nuclear --multiGroup'                    file='miniProtareSingle.H3.U235.mg.out'
	$(MAKE) checkSub Args='photon H1 nuclear'                               file='miniProtareSingle.photon+H1.out'
	$(MAKE) checkSub Args='photon H1 nuclear --multiGroup'                  file='miniProtareSingle.photon+H1.mg.out'
	$(MAKE) checkSub Args='photon Pb208 nuclear'                            file='miniProtareSingle.photon+Pb208.out'
	$(MAKE) checkSub Args='photon Pb208 nuclear --multiGroup'               file='miniProtareSingle.photon+Pb208.mg.out'

checkSub:
	./miniProtareSingle $$Args > Outputs/$$file
	../Utilities/diff.com miniProtareSingle Benchmarks/$$file Outputs/$$file
