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
#include <set>

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

static char const *description = "Loops over energy at the specified temperature, sampling reactions. If projectile is a photon, see options *-pa* and *-pn*.";

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

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Transporting::Particles particles;
    char *endChar;
    long numberOfSamples = -1;
    unsigned long long rngState = 1;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    GIDI::Construction::PhotoMode photo_mode = GIDI::Construction::PhotoMode::nuclearOnly;
    argvOption2 *option;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "sampleReactions", description );

    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "-pa", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-pn* present." ) );
    argv_options.add( argvOption2( "-pn", false, "Include photo-nuclear protare if relevant. This is the default unless *-pa* present." ) );
    argv_options.add( argvOption2( "--fixedGrid", false, "Set fixed grid data. Only used if protare is only photo-atomic protare." ) );
    argv_options.add( argvOption2( "--temperature", true, "The temperature of the target material. Default is 2.58522e-8." ) );
    argv_options.add( argvOption2( "--energyMin", true, "The minimum energy for the energy loop. Default is 1e-12." ) );
    argv_options.add( argvOption2( "--energyMax", true, "The maximum energy for the energy loop. Default is 25." ) );
    argv_options.add( argvOption2( "-n", true, "The number of calls to sampleReaction. If negative, multiplied by -1000,000. Default is -1." ) );
    argv_options.add( argvOption2( "--quiet", false, "If present, some information is not printed." ) );
    argv_options.add( argvOption2( "--showReactions", false, "If present, the list of reactions is printed." ) );

    argv_options.parseArgv( argc, argv );

    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );
    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );

    if( argv_options.find( "-pa" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "-pn" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    GIDI::Map::Map map( mapFilename, pops );

    std::string temperatureString = argv_options.find( "--temperature" )->zeroOrOneOption( argv, "2.58522e-8" );
    double temperature = strtod( temperatureString.c_str( ), &endChar );
    if( *endChar != 0 ) throw "Invalid temperature input.";

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    GIDI::Protare *protare = map.protare( construction, pops, projectileID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    if( !argv_options.find( "--quiet" )->present( ) ) {
        for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
            std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
        }
    }

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    if( argv_options.find( "--fixedGrid" )->present( ) ) {
        GIDI::Groups groups( "../../../GIDI/Test/groups.xml" );
        MC.fixedGridPoints( groups.get<GIDI::Group>( "LLNL_gid_80" )->data( ) );
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    int numberOfReactions = (int) MCProtare->numberOfReactions( );

    if( argv_options.find( "--showReactions" )->present( ) ) {
        std::cout << "List of reactions:" << std::endl;
        for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

            std::cout << LUPI::Misc::argumentsToString( "    %5d ", reactionIndex ) << reaction->label( ).c_str( ) << std::endl;
        }
    }

    std::cout << "temperature = " << temperature << std::endl;

    option = argv_options.find( "-n" );
    numberOfSamples = option->asLong( argv, numberOfSamples );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    std::cout << "numberOfSamples = " << numberOfSamples << std::endl;

    option = argv_options.find( "--energyMin" );
    double energyMin = option->asDouble( argv, 1e-12 );
    option = argv_options.find( "--energyMax" );
    double energyMax = option->asDouble( argv, 25.0 );

    if( argv_options.find( "--showReactions" )->present( ) ) {
        std::cout << "       ";
        for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
            std::cout << LUPI::Misc::argumentsToString( " %8d ", reactionIndex );
        }
        std::cout << std::endl;
    }

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    for( double energy = energyMin; energy <= energyMax; energy *= 1.2 ) {
        int hashIndex = domainHash.index( energy );

        double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy );
        std::cout << "energy = " << energy << " " << "cross section = " << doubleToString2( "%13.6e", crossSection ) << std::endl;
        if( crossSection == 0.0 ) continue;

        input.setTemperatureAndEnergy( temperature, energy );

        std::vector<double> reactionCrossSections( numberOfReactions );
        std::cout << "      ";
        for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
            double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature, energy );
            std::cout << LUPI::Misc::argumentsToString( " %9.6f", reactionCrossSection / crossSection );
        }
        std::cout << std::endl;

        std::vector<long> counts( numberOfReactions + 1, 0 );
        for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
            int reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSection,
                    [&]() -> double { return float64RNG64( &rngState ); } );
            if( reactionIndex > numberOfReactions ) reactionIndex = numberOfReactions;
            ++counts[reactionIndex];
        }

        std::cout << "      ";
        for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
            double ratio = counts[i1];
            std::cout << LUPI::Misc::argumentsToString( " %9.6f", ratio / numberOfSamples );
        }
        std::cout << std::endl;

        std::cout << "      ";
        for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
            std::cout << LUPI::Misc::argumentsToString( " %9ld", counts[i1] );
        }
        std::cout << std::endl;
    }

    delete protare;

    delete MCProtare;
}
