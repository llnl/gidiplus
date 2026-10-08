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
#include <set>

#include "MCGIDI.hpp"

#include "GIDI_testUtilities.hpp"

static char const *description = "Loops over temperature and energy, printing the total cross section. If projectile is a photon, see options *-a* and *-n*.";

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
        std::cerr << str << std::endl;
        exit( EXIT_FAILURE ); }
    catch (std::string &str) {
        std::cerr << str << std::endl;
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    PoPI::Database pops;
    argvOptions argv_options( __FILE__, description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "--ENDL99120", false, "If present, ENDL two 99120 products are list for a fission reaction." ) );

    parseTestOptions.parse( );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    if( argv_options.find( "--ENDL99120" )->present( ) )
        construction.setFissionResiduals( GIDI::Construction::FissionResiduals::ENDL99120 );
    if( argv_options.find( "--ENDL99120" )->present( ) )
        construction.setFissionResiduals( GIDI::Construction::FissionResiduals::ENDL99120 );

    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../../../GIDI/Test/Data/MG_MC/all_maps.map",
        construction, PoPI::IDs::neutron, "O16" );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC settings( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    GIDI::Transporting::Particles particles;

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, settings, particles, domainHash, temperatures, reactionsToExclude );

    MCProtare->setUserParticleIndex( static_cast<int>( pops[PoPI::IDs::neutron] ), 0 );
    MCProtare->setUserParticleIndex( static_cast<int>( pops["H2"] ), 10 );
    MCProtare->setUserParticleIndex( static_cast<int>( pops[PoPI::IDs::photon] ), 11 );

    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

        std::cout << reaction->label( ).c_str( ) << std::endl;

        auto intids = reaction->productIntids( );
        auto indices = reaction->productIndices( );
        auto userIndices = reaction->userProductIndices( );
        for( std::size_t productIndex = 0; productIndex < intids.size( ); ++productIndex ) {
            int intid = intids[productIndex];
            int index = indices[productIndex];
            int mulIndex = reaction->productMultiplicity( index );
            int mulIntid = reaction->productMultiplicityViaIntid( intid );

            std::cout << "    " << intid << "  " << mulIndex << " " << userIndices[productIndex] << std::endl;
            if( mulIndex != mulIntid ) std::cout << "ERROR: mulIndex = " << mulIndex << " != mulIntid = " << mulIntid << std::endl;
        }
    }

    delete MCProtare;

    delete protare;
}
