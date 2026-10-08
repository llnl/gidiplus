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
    std::vector<std::string> libraries;
    GIDI::Transporting::Particles particles;
    std::set<int> reactionsToExclude;
    clock_t time0, time1;
    long numberOfSamples = 100 * 1000, sampleTemperatures = 0, sampleEnergies = 0;
    void *rngState = nullptr;
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

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );
    MCGIDI::Sampling::StdVectorProductHandler products;
    int numberOfReactions = (int) MCProtare->numberOfReactions( );

    std::cout << "Reactions:" << std::endl;
    for( int reactionIndex = 0; reactionIndex < numberOfReactions; ++reactionIndex ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

        std::cout << "    " << reactionIndex << "  " << reaction->label( ).c_str( ) << std::endl;
    }
    std::cout << std::endl;

    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 10.1, ++sampleTemperatures ) {
        clock_t time1_1 = clock( );
        clock_t time2_1 = time1_1;
        clock_t time3_1 = time1_1;

        for( int reactionIndex = 0; reactionIndex < numberOfReactions; ++reactionIndex ) {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
            double threshold = MCProtare->threshold( reactionIndex );

            long energyIndex = 0;
            if( threshold < 1e-12 ) threshold = 1e-12;
            for( double energy = threshold; energy < 100.1; energy *= 10, ++energyIndex ) {
                input.setTemperatureAndEnergy( temperature, energy );
                for( long sampleIndex = 0; sampleIndex <= numberOfSamples; ++sampleIndex ) {
                    products.clear( );
                    reaction->sampleProducts( MCProtare, input, [&]() -> double { return myRNG( &rngState); },
                            [&] (MCGIDI::Sampling::Product &a_product) -> void { products.push_back( a_product ); }, products );
                }
                printTime_energy( "                energy: ", energyIndex, energy, time3_1 );
            }
            sampleEnergies = energyIndex;
            std::cout << std::endl;
            std::string label1 = LUPI::Misc::argumentsToString( "            reaction %d: ", reactionIndex );
            printTime( label1, time2_1 );
        }
        printTime_double( "        temperature: ", temperature, time1_1 );
        std::cout << std::endl;
    }

    long sampled = sampleTemperatures * sampleEnergies * numberOfReactions * numberOfSamples;

    printSpeeds( __FILE__, time1, sampled );
    printTime( "total sample time : ", time1 );
    printTime( "total time : ", time0 );

    std::cout << "total sampled = " << sampled << "  (" << std::setprecision( 3 ) << (double) sampled << ")" << std::endl;

    delete protare;

    delete MCProtare;
}

