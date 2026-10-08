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

#include "LUPI.hpp"
#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

static char const *description = "Loops over temperature and energy, printing the total cross section. If projectile is a photon, see options *-a* and *-n*.";

/*
=========================================================
*/
int main( int argc, char **argv ) {

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    GIDI::Construction::PhotoMode photo_mode = GIDI::Construction::PhotoMode::nuclearOnly;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "crossSections", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--pa", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-n* present." ) );
    argv_options.add( argvOption2( "--pn", false, "Include photo-nuclear protare if relevant. This is the default unless *-a* present." ) );
    argv_options.add( argvOption2( "-n", false, "If present, add neutron as transporting particle." ) );
    argv_options.add( argvOption2( "-p", false, "If present, add photon as transporting particle." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );

    if( argv_options.find( "--pa" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "--pn" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    GIDI::Map::Map map( mapFilename, pops );

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
        protare = map.protare( construction, pops, projectileID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedMultiGroup( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].heatedMultiGroup( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0.0 );

    if( ( projectileID == PoPI::IDs::neutron ) || argv_options.find( "-n" )->present( ) ) {
        GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
        neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
        particles.add( neutron );
    }

    if( ( projectileID == PoPI::IDs::photon ) || argv_options.find( "-p" )->present( ) ) {
        GIDI::Transporting::Particle photon( PoPI::IDs::photon, groups_from_bdfls.getViaGID( 70 ) );
        photon.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
        particles.add( photon );
    }

    particles.process( *protare, label );

    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );
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

    MCGIDI::MultiGroupHash multiGroupHash( *protare, temperatures[0] );

    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *MCProtare->reaction( i1 );

        std::cout << std::setw( 40 ) << reaction.label( ).c_str( ) << "  threshold = "
                << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true ) 
                << "  threshold = " << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true ) << std::endl;
    }

    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *MCProtare->reaction( i1 );

        std::cout << "    reaction: " << reaction.label( ).c_str( ) << std::endl;
    }

    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 100.0 ) {
        std::cout << "temperature = " << doubleToString2( "%8.1e", temperature ) << "                       deposition energy  deposition momentum  production energy" << std::endl;
        for( double energy = 1e-12; energy < 25; energy *= 1.1 ) {
            std::size_t hashIndex = multiGroupHash.index( energy );

            std::cout << "    energy = " << std::setw( 16 ) << energy << " index = " << std::setw( 6 ) << hashIndex;
            std::cout << doubleToString2( " %16.8e",    MCProtare->depositionEnergy( hashIndex, temperature, energy ) );
            std::cout << doubleToString2( "    %16.8e", MCProtare->depositionMomentum( hashIndex, temperature, energy ) );
            std::cout << doubleToString2( "    %16.8e", MCProtare->productionEnergy( hashIndex, temperature, energy ) );
            std::cout << std::endl;
        }
    }

    delete protare;

    delete MCProtare;
}
