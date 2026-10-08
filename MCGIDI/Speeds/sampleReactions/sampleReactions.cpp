/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

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
    GIDI::Transporting::Particles particles;
    std::vector<std::string> libraries;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    clock_t time0, time1;
    long numberOfSamples = 1000 * 1000, sampleTemperatures = 0, sampleEnergies = 0;

    std::cout << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cout << " " << argv[i1];
    std::cout << std::endl;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
    time0 = clock( );
    time1 = time0;
    GIDI::Protare *protare = map.protare( construction, pops, "n", "O16" );
    printTime( "    load GIDI: ", time1 );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    GIDI::Styles::TemperatureInfo temperature = temperatures[0];

    std::string label( temperature.heatedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, "n", &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 100.0 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );
    printTime( "    load MCGIDI: ", time1 );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    for( double temperature2 = 1e-8; temperature2 < 1e-1; temperature2 *= 10.0, ++sampleTemperatures ) {
        clock_t time1_1 = clock( );
        clock_t time2_1 = time1_1;

        long energyIndex = 0;
        for( double energy = 1e-12; energy < 100.1; energy *= 10.0, ++energyIndex ) {
            input.setTemperatureAndEnergy( temperature2, energy );
            int hashIndex = domainHash.index( energy );
            double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature2, energy );

            for( long i1 = 0; i1 <= numberOfSamples; ++i1 ) MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, 
                    crossSection, [&]() -> double { return myRNG( nullptr ); } );
            printTime_energy( "            energies: ", energyIndex, energy, time2_1 );
        }
        sampleEnergies = energyIndex;
        std::cout << std::endl;
        printTime_double( "        temperature: ", temperature2, time1_1 );
    }

    long sampled = sampleTemperatures * sampleEnergies * numberOfSamples;

    printSpeeds( __FILE__, time1, sampled );
    printTime( "    total sample: ", time1 );
    printTime( "    total: ", time0 );

    std::cout << "total sampled = " << sampled << "  (" << std::setprecision( 3 ) << (double) sampled << ")" << std::endl;

    delete protare;

    delete MCProtare;
}
