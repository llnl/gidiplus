/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <MCGIDI.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <bins.hpp>

#define nBins 1000

static char const *description = "This code calculates the reaction rate for a projectile with a Maxwellian distribution heading a target at the same temperature using upscatter model A.";

void main2( int argc, char **argv );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv );
        exit( EXIT_SUCCESS ); }
    catch (std::exception &exception) {
        std::cerr << exception.what( ) << std::endl; }
    catch (char const *str) {
        std::cout << str << std::endl; }
    catch (std::string &str) {
        std::cout << str << std::endl;
    }
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    long numberOfSamples = -1;
    unsigned long long rngState = 1;

    argvOptions2 argv_options( "sampleReactions_multiGroup", description );

    argv_options.add( argvOption2( "-n", true, "The number of sample. If negative, multiplied by minus one million. Default is -1." ) );
    argv_options.parseArgv( argc, argv );

    numberOfSamples = argv_options.find( "-n" )->asLong( argv, numberOfSamples );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;

    double averageEnergy = 0.0;
    double betaMin = 1e-5, betaMax = 100;
    Bins betaBins( nBins, betaMin, betaMax, true );
    Bins energyBins( nBins, 0.5 * betaMin * betaMin, 0.5 * betaMax * betaMax, true );
    long printSamples = numberOfSamples / 100;
    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
        if( ( ( numberOfSamples + 1 ) % printSamples ) == 0 ) std::cerr << i1 << " of " << numberOfSamples << std::endl;
        double beta = sampleBetaFromMaxwellian( [&]() -> double { return float64RNG64( &rngState ); } );
        double energy = beta * beta;    // The 1/2 * m here is multiplied by sqrt( 2 T / m )^2 but since T is taken to be 1, the sqrt(2 / m)^2 is just 2 / m.
        betaBins.accrue( beta );
        energyBins.accrue( energy );
        averageEnergy += energy;
    }

    betaBins.print( stdout, "# Betas", false, true );
    energyBins.print( stdout, "# Energies", false, true );
    averageEnergy /= numberOfSamples;
    std::cout << "\n# Average energy = " << averageEnergy << std::endl;

    exit( EXIT_SUCCESS );
}
