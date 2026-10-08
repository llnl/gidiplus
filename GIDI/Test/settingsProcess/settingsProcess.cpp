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

#include "GIDI_testUtilities.hpp"

static char const *description = "";

void main2( int argc, char **argv );
void crossSectionInfo( GIDI::Protare *protare, GIDI::Styles::TemperatureInfo temperature, std::string const &label, int neutronGID,
                        GIDI::Transporting::Groups_from_bdfls const &groups_from_bdfls,
                        GIDI::Transporting::Fluxes_from_bdfls const &fluxes_from_bdfls );
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

    argvOptions argv_options( "settingsProcess", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askGNDS_File = true;

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photo_mode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );

    std::cout << stripDirectoryBase( protare->fileName( ), "/GIDI/Test/" ) << std::endl;

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        GIDI::PhysicalQuantity const &temperature = iter->temperature( );

        std::cout << "label = " << iter->heatedMultiGroup( ) << "  temperature = " << temperature.value( ) << std::endl;
    }

    GIDI::Styles::TemperatureInfo temperature( temperatures[0] );
    std::string label( temperatures[0].heatedMultiGroup( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../bdfls", 0 );

    crossSectionInfo( protare, temperature, label, 23, groups_from_bdfls, fluxes_from_bdfls );
    crossSectionInfo( protare, temperature, label, 24, groups_from_bdfls, fluxes_from_bdfls );
    crossSectionInfo( protare, temperature, label, 25, groups_from_bdfls, fluxes_from_bdfls );
    crossSectionInfo( protare, temperature, label, 26, groups_from_bdfls, fluxes_from_bdfls );
    crossSectionInfo( protare, temperature, label, 4, groups_from_bdfls, fluxes_from_bdfls );

    delete protare;
}
/*
=========================================================
*/
void crossSectionInfo( GIDI::Protare *protare, GIDI::Styles::TemperatureInfo temperature, std::string const &label, int neutronGID,
                        GIDI::Transporting::Groups_from_bdfls const &groups_from_bdfls,
                        GIDI::Transporting::Fluxes_from_bdfls const &fluxes_from_bdfls ) {

    LUPI::StatusMessageReporting smr1;
    std::size_t offset = 4;
    std::size_t prefixLength = outputChannelStringMaximumLength( protare );
    if( prefixLength < 32 ) prefixLength = 32;

    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::off );

    GIDI::Transporting::Particles particles;

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( neutronGID ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon, groups_from_bdfls.getViaGID( 70 ) );
    photon.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( photon );

    particles.process( *protare, label );

    particles.print( );
    std::cout << std::endl;

    std::string prefix( "Uncollapsed total cross section:: " );
    prefix.insert( prefix.size( ), offset + prefixLength + 2 - prefix.size( ), ' ' );
    GIDI::Vector crossSection = protare->multiGroupCrossSection( smr1, settings, temperature );
    printVector( prefix, crossSection );

    GIDI::Vector collapsed = GIDI::collapse( crossSection, settings, particles, 0 );
    prefix = "collapsed cross section::";
    prefix.insert( prefix.size( ), offset + prefixLength + 2 - prefix.size( ), ' ' );
    printVector( prefix, collapsed );

    GIDI::Vector summedCollapsed( 0 );
    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction *reaction = protare->reaction( index );

        crossSection = reaction->multiGroupCrossSection( smr1, settings, temperature );
        prefix = outputChannelPrefix( offset, prefixLength, reaction );
        prefix += ":";
        printVector( prefix, crossSection );

        std::string::size_type length( prefix.size( ) );
        prefix.erase( );
        prefix.insert( 0, length - 2, ' ' );
        collapsed = GIDI::collapse( crossSection, settings, particles, 0 );
        prefix += "::";
        printVector( prefix, collapsed );
        summedCollapsed += collapsed;
    }
    prefix = "Summed collapsed cross section::";
    prefix.insert( prefix.size( ), offset + prefixLength + 2 - prefix.size( ), ' ' );
    printVector( prefix, summedCollapsed );
}
