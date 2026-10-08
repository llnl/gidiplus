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
#include <iostream>
#include <iomanip>

#include "MCGIDI.hpp"

/*
=========================================================
*/
int main( int argc, char **argv ) {

    int numberOfMus = 101;
    std::string mapFilename( "../../../GIDI/Test/all.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string photonID( PoPI::IDs::photon );
    std::string targetID = "O16";
    GIDI::ProtareSingle *protare;
    std::vector<std::string> libraries;
    GIDI::Transporting::Particles particles;
    int reactionIndex = 0;
    double muMin = -1.0, muMax = 1.0;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
        protare = (GIDI::ProtareSingle *) map.protare( construction, pops, photonID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, photonID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::ProtareSingle *MCProtare;
    try {
        MCProtare = (MCGIDI::ProtareSingle *) MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    MCGIDI::Reaction *reaction = const_cast<MCGIDI::Reaction *>( MCProtare->reaction( reactionIndex ) );
    double threshold = MCProtare->threshold( reactionIndex );

    std::cout << "reaction (" << std::setw( 3 ) << reactionIndex << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
    if( threshold < 1e-13 ) threshold = 1e-13;

    FILE *fOutMu = fopen( "Data/mu.out", "w" );

    MCGIDI::OutputChannel &outputChannel = const_cast<MCGIDI::OutputChannel &>( reaction->outputChannel( ) );
    MCGIDI::Product *product = const_cast<MCGIDI::Product *>( outputChannel[0] );
    MCGIDI::Distributions::Distribution const *distribution = product->distribution( );
    MCGIDI::Distributions::CoherentPhotoAtomicScattering const *coherentPhotoAtomicScattering = dynamic_cast<MCGIDI::Distributions::CoherentPhotoAtomicScattering const *>( distribution );

    double dMu = ( muMax - muMin ) / ( numberOfMus - 1 );
    int index = 0;
    for( double energy = threshold; energy < 200; energy *= 10, ++index ) {
        fprintf( fOutMu, "\n\n# energy =  % .12e: index = %d\n", energy, index );

        double sum = 0;
        double mu1 = muMin, POfMu1 = 0;
        double mu2, POfMu2;
        for( long i1 = 0; i1 < numberOfMus; ++i1 ) {
            mu2 = muMin + i1 * dMu;

            if( i1 == ( numberOfMus - 1 ) ) mu2 = muMax;
            POfMu2 = coherentPhotoAtomicScattering->evaluate( energy, mu2 );
            fprintf( fOutMu, "     % .12e %.12e\n", mu2, POfMu2 );
            if( i1 != 0 ) sum += 0.5 * ( POfMu1 + POfMu2 ) * ( mu2 - mu1 );
            mu1 = mu2;
            POfMu1 = POfMu2;
        }
        printf( "energy =  % .12e   mu = %.12f  sum = %.9f   index = %d\n", energy, mu2, sum, index );
    }

    fclose( fOutMu );

    delete protare;

    delete MCProtare;
}
