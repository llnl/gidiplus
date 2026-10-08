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

static char const *description = "Loops over energy, reaction and particle ids, printing the multiplicity for each particle.";

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
    argv_options.add( argvOption( "-d", false, "If present, fission delayed neutrons are included with product sampling." ) );

    parseTestOptions.parse( );

    GIDI::Transporting::DelayedNeutrons delayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
    if( argv_options.find( "-d" )->present( ) ) delayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    if( argv_options.find( "--ENDL99120" )->present( ) )
        construction.setFissionResiduals( GIDI::Construction::FissionResiduals::ENDL99120 );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../../../GIDI/Test/Data/MG_MC/all_maps.map",
        construction, PoPI::IDs::neutron, "O16" );

    std::cout << "      " << stripDirectoryBase( protare->fileName( ) ) << std::endl;

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC settings( pops, protare->projectile( ).ID( ), &protare->styles( ), label, delayedNeutrons, 20.0 );
    settings.setThrowOnError( false );

    GIDI::Transporting::Particles particles;

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron );
    particles.add( neutron );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon );
    particles.add( photon );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, settings, particles, domainHash, temperatures, reactionsToExclude );

    for( double energy = 2e-11; energy < 21.0; energy *= 1e3 ) {
        std::cout << std::endl << "      energy = " << doubleToString( "%14.6e", energy ) << std::endl;
        for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

            std::cout << "        " << reaction->label( ).c_str( ) << std::endl;

            auto indices = reaction->productIndices( );
            for( std::size_t productIndex = 0; productIndex < indices.size( ); ++productIndex ) {
                int index = indices[productIndex];
                PoPI::Base const &base= pops.get<PoPI::Base>( static_cast<std::size_t>( index ) );
                if( base.isParticle() ) {
                    double aveIndex = reaction->productAverageMultiplicity( index, energy );
                    double aveIntid = reaction->productAverageMultiplicityViaIntid( base.intid( ), energy );

                    std::cout << "          " << std::left << std::setw( 10 ) << base.ID( ) << "  " << std::setw( 5 ) << base.intid( )
                            << "  " << doubleToString( "%14.6e", aveIndex ) << std::endl;
                    if( aveIndex != aveIntid ) std::cout << "ERROR: aveIndex = " << aveIndex << " != aveIntid = " << aveIntid << std::endl;
                }
            }
        }
    }

    delete MCProtare;

    delete protare;
}
