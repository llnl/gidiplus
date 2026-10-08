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

#define nBins 501

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

    std::string mapFilename( "../../../GIDI/Test/all3T.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string neutronID( PoPI::IDs::neutron );
    std::string targetID( "O16" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    double temperature_MeV = 1e-3;
    long numberOfSamples = 10 * 1000 * 1000;
    long bins[nBins+1];
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    unsigned long long rngState = 1;

    for( int i1 = 0; i1 <= nBins; ++i1 ) bins[i1] = 0;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    if( argc > 1 ) targetID = argv[1];

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearOnly );
    protare = map.protare( construction, pops, neutronID, targetID );
    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    for( GIDI::Styles::TemperatureInfos::iterator temperature = temperatures.begin( ); temperature < temperatures.end( ); ++temperature ) {
        temperature->print( );
    }

    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setUpscatterModelA( );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;

    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );
    MCGIDI::ProtareSingle *MCProtareSingle = MCProtare->protare( 0 );

    PoPI::Base const &target = pops.get<PoPI::Base>( targetID );
    std::string Str = LUPI::Misc::argumentsToString( "sampleTargetBetaForUpscatterModelA.%s.dat", target.ID( ).c_str( ) );
    FILE *fOut;
    if( ( fOut = fopen( Str.c_str( ), "w" ) ) == nullptr ) throw "error opening output file";

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::B );
    input.setTemperatureAndEnergy( temperature_MeV, temperature_MeV );

    MCGIDI::Reaction const *reaction;
    for( std::size_t i1 = 0; i1 < MCProtareSingle->numberOfReactions( ); ++i1 ) {
        reaction = MCProtareSingle->reaction( i1 );
        if( reaction->ENDF_MT( ) == 102 ) break;
    }
    input.m_reaction = reaction;

    double targetMass = MCProtareSingle->targetMass( );
    double betaTargetThermal = MCGIDI_particleBeta( targetMass, temperature_MeV );
    double betaTargetMax = 5.0 * betaTargetThermal;
    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
        if( MCProtareSingle->sampleTargetBetaForUpscatterModelA( input, [&]( ) -> double { return float64RNG64( &rngState ); } ) ) {
            int bin = (int) ( nBins * input.m_targetBeta / betaTargetMax );
            if( bin > nBins ) bin = nBins;
            bins[bin]++;
        }
    }

    double betaPerBin = betaTargetMax / nBins;
    std::cout << "# betaTargetMax = " << betaTargetMax << std::endl;
    std::cout << "# betaTargetThermal = " << betaTargetThermal << std::endl;
    for( int i1 = 0; i1 <= nBins; ++i1 ) {
        double binVelocity = ( i1 + 0.5 ) * betaPerBin;
        double binEnergy = 0.5 * targetMass * binVelocity * binVelocity;
        double probability = bins[i1] / (double) numberOfSamples;
        double dEnergy = binVelocity * targetMass * betaPerBin;

        fprintf( fOut, "%e %e %e %e %e %9ld\n", binEnergy, probability / dEnergy, binVelocity, binVelocity / betaTargetThermal, probability, bins[i1] );
    }
    fclose( fOut );

    exit( EXIT_SUCCESS );
}
