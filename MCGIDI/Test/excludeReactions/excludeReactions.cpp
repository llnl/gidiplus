/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

static char const *description = "Reads a protare and removes some reactions. Each argument is an integer for a reaction to exclude.";

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <set>

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

int main2( int argc, char **argv );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        return( EXIT_FAILURE );
    }
    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
int main2( int argc, char **argv ) {

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "sampleProducts", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--MT5", false, "If present, ignoreENDF_MT5 is set ." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::photon );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );

    for( std::size_t i1 = 0; i1 < argv_options.m_arguments.size( ); ++i1 ) 
        reactionsToExclude.insert( static_cast<std::size_t>( asLong2( argv[argv_options.m_arguments[i1]] ) ) );

    GIDI::Map::Map map( mapFilename, pops );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    protare = map.protare( construction, pops, projectileID, targetID );

    std::cout << std::endl;
    std::cout << "    projectile = " << projectileID << std::endl;
    std::cout << "    target = " << targetID << std::endl;
    std::cout << std::endl;

    std::cout << "    GIDI::Protare number of reaction = " << protare->numberOfReactions( ) << std::endl;
    for( std::size_t reactionIndex = 0; reactionIndex < protare->numberOfReactions( ); ++reactionIndex ) {
        GIDI::Reaction const *reaction = protare->reaction( reactionIndex );

        std::cout << "        " << std::setw( 3 ) << reactionIndex << ") " << reaction->label( ) << std::endl;
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setIgnoreENDF_MT5( argv_options.find( "--MT5" )->present( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon, groups_from_bdfls.getViaGID( 70 ) );
    particles.add( photon );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    std::cout << std::endl;
    std::cout << "    MCGIDI::Protare number of reaction = " << MCProtare->numberOfReactions( ) << std::endl;

    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

        for( std::size_t reactionIndex2 = 0; reactionIndex2 < protare->numberOfReactions( ); ++reactionIndex2 ) {
            GIDI::Reaction const *reaction2 = protare->reaction( reactionIndex2 );

            if( reaction2->label( ) == reaction->label( ).c_str( ) ) {
                std::cout << "        " << std::setw( 3 ) << reactionIndex2 << ") ";
                break;
            }
        }
        std::cout << reaction->label( ).c_str( ) << std::endl;
    }

    delete MCProtare;
    delete protare;

    return( EXIT_SUCCESS );
}
