SHELL = /bin/sh

# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>

.PHONY: default compile check clean realclean doDirs

DIRS_nonGPUS = Utilities domainHash crossSection sampleReactions sampleProducts \
		crossSection_multiGroup sampleReactions_multiGroup sampleProducts_multiGroup deposition_multiGroup \
		deposition_continuousEnergy productIndices samplePhotoAtomic sampleTerrellPromptNeutronDistribution \
		excludeReactions TNSL memoryCheck print_multiGroup modifiedCrossSection photoAtomicDopplerBroadening \
		miniProtareSingle

DIRS_GPUS = gpuTest

DIRS = $(DIRS_nonGPUS) $(DIRS_GPUS)

default:
	cd Utilities; $(MAKE)

compile:
	$(MAKE) doDirs TARGET=default

check:
	echo ""
	echo "  -- Checking MCGIDI --"
	$(MAKE) doDirs TARGET=check DIRS="$(DIRS_nonGPUS)"

check_GPU:
	echo ""
	echo "  -- Checking MCGIDI GPU --"
	$(MAKE) doDirs TARGET=check DIRS="$(DIRS_GPUS)"

check_all: check check_GPU

clean:
	$(MAKE) doDirs TARGET=clean

realclean:
	$(MAKE) doDirs TARGET=realclean

doDirs:
	for dir in $(DIRS); do cd $$dir; $(MAKE) $(TARGET); cd ../; done
