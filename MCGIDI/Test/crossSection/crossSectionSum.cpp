/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <set>

#include "MCGIDI.hpp"

void printVector( std::string &prefix, GIDI::Vector &vector );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    std::string mapFilename( "../../../GIDI/Test/all3T.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string neutronID( PoPI::IDs::neutron );
    std::string targetID = "O16";
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
        protare = (GIDI::Protare *) map.protare( construction, pops, neutronID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    try {
        MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    std::size_t numberOfReactions = MCProtare->numberOfReactions( );
    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 10.1 ) {
        std::cout << "temperature = " << temperature << std::endl;
        for( double energy = 1e-12; energy < 100; energy *= 1.2 ) {
            int hashIndex = domainHash.index( energy );
            int numberOfNonZeroReactionCrossSections = 0;

            double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy );
            double crossSectionSum = 0;
            for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
                double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature, energy );
                if( reactionCrossSection > 0 ) ++numberOfNonZeroReactionCrossSections;
                crossSectionSum += reactionCrossSection;
            }

            double delta = crossSection - crossSectionSum;
            double ratio = 1;
            if( crossSection != 0 ) ratio = delta / crossSection;
#ifdef MCGIDI_USE_DOUBLES
            if( ratio < 1e-14 ) {
#else
            if( ratio < 1e-7 ) {
                crossSection = crossSectionSum;
#endif
                delta = 0;
                ratio = 0;
            }
            std::cout << "    " << energy << "  " << numberOfNonZeroReactionCrossSections << "  " << crossSection << "  " << crossSectionSum << "  " << delta << "  " << ratio;
            if( fabs( ratio ) > 1e-5 ) std::cout << " *****";
            std::cout << std::endl;
        }
    }

    delete protare;

    delete MCProtare;
}
