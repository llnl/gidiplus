SHELL = /bin/sh

# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>

.PHONY: default compile check clean realclean doDirs

DIRS = Utilities matrix VectorAndMatrixReverse map parseXML groupBoundaries inverseSpeed productIDs crossSections productMatrix \
	transportCorrection availableEnergy multiplicity Q averageProductData depositionData \
	settingsProcess multiGroup temperatures splitString ancestry readAll photoAtomicScattering photoScattering TNSL gain \
	multiGroups fluxes ENDLCValues activeReactions delayedNeutronProducts sumsCheck modifiedCrossSection RIS_viaMap \
	areAllProductsTracked regions1dAsXYs1d suitePop targetInfo listOfTransportableSuites

default:
	cd Utilities; $(MAKE)

compile:
	$(MAKE) doDirs TARGET=default

check:
	echo ""
	echo "  -- Checking GIDI --"
	$(MAKE) doDirs TARGET=check

clean:
	$(MAKE) doDirs TARGET=clean

realclean:
	$(MAKE) doDirs TARGET=realclean

doDirs:
	for dir in $(DIRS); do cd $$dir; $(MAKE) $(TARGET); cd ../; done
