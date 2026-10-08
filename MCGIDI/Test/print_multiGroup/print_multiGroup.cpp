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
#include <set>

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

static char const *description = "Prints the contents of the HeatedCrossSectionsMultiGroup data.";

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

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    GIDI::Construction::PhotoMode photo_mode = GIDI::Construction::PhotoMode::nuclearOnly;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "print_multiGroup", description );

    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "-a", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-n* present." ) );
    argv_options.add( argvOption2( "-n", false, "Include photo-nuclear protare if relevant. This is the default unless *-a* present." ) );
    argv_options.add( argvOption2( "--gid", true, "The group id to use for the projectile." ) );

    argv_options.parseArgv( argc, argv );

    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );
    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );

    if( argv_options.find( "-a" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "-n" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    GIDI::Map::Map map( mapFilename, pops );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    protare = map.protare( construction, pops, projectileID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].heatedMultiGroup( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0.0 );

    std::string gid( "LLNL_gid_4" );
    if( projectileID == PoPI::IDs::photon ) gid = "LLNL_gid_70";
    gid = argv_options.find( "--gid" )->zeroOrOneOption( argv, gid );
    GIDI::Transporting::MultiGroup multi_group = groups_from_bdfls.viaLabel( gid );
    GIDI::Transporting::Particle projectile( projectileID, multi_group );
    projectile.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( projectile );
    particles.process( *protare, label );

    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );
    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    MCGIDI::ProtareSingle const *protareSingle = MCProtare->protare( 0 );
    MCGIDI::HeatedCrossSectionsMultiGroup const &heatedMultigroupCrossSections = protareSingle->heatedMultigroupCrossSections( );
    heatedMultigroupCrossSections.write( stdout, 0 );

    delete protare;

    delete MCProtare;
}
