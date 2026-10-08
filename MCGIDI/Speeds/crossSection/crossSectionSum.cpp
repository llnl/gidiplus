/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <iostream>
#include <iomanip>

#include "MCGIDI.hpp"

#include "utilities4Speed.hpp"

void main2( int argc, char **argv );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv ); }
    catch (std::exception &exception) {
        std::cerr << exception.what( ) << std::endl;
        exit( EXIT_FAILURE ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE ); }
    catch (std::string &str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    std::string mapFilename( "../../../GIDI/Test/all3T.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string protareFilename( map.protareFilename( "n", "O16" ) );
    clock_t time0, time1;
    long numberOfSamples = 1000 * 1000, sampled = 0;
    std::vector<std::string> libraries;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet  reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cout << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cout << " " << argv[i1];
    std::cout << std::endl;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
    time0 = clock( );
    time1 = time0;
    GIDI::Protare *protare = map.protare( construction, pops, "n", "O16" );
    printTime( "    load GIDI: ", time1 );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, "n", &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 100.0 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );
    printTime( "    load MCGIDI: ", time1 );

    std::size_t numberOfReactions = MCProtare->numberOfReactions( );

    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *MCProtare->reaction( i1 );
        std::string reactionLabel = LUPI::Misc::argumentsToString( "%-40s: ", reaction.label( ).c_str( ) );

        std::cout << "    reaction: " << reactionLabel << " final Q = " << reaction.finalQ( 0 ) << " threshold = " << reaction.crossSectionThreshold( ) << std::endl;
    }

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 10.1 ) {
        clock_t time1_1 = clock( );
        clock_t time2_1 = time1_1;
        clock_t time3_1 = time1_1;
        long energyIndex = 0;

        for( double energy = 1e-12; energy < 100.1; energy *= 10, ++energyIndex ) {
            int hashIndex = domainHash.index( energy );

            for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
                for( long i2 = 0; i2 <= numberOfSamples; ++i2 ) MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature, energy );
                sampled += numberOfSamples;
                printTime_reaction( "                reaction: ", i1, time3_1 );
            }
            std::cout << std::endl;
            std::string timeLabel = LUPI::Misc::argumentsToString( "            energies %.4e: ", energy );
            printTime( timeLabel, time2_1 );
        }
        printTime_double( "        temperature", temperature, time1_1 );
        std::cout << std::endl;
    }

    printSpeeds( __FILE__, time1, sampled );
    printTime( "    lookup: ", time1 );
    printTime( "    total: ", time0 );

    std::cout << "total sampled = " << sampled << "  (" << std::setprecision( 3 ) << (double) sampled << ")" << std::endl;

    delete protare;

    delete MCProtare;
}
