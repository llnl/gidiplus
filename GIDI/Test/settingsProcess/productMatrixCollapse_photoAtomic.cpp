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
void productMatrixInfo( GIDI::Protare *protare, GIDI::Styles::TemperatureInfo temperature, std::string const &label, int neutronGID,
                        GIDI::Transporting::Groups_from_bdfls const &groups_from_bdfls,
                        GIDI::Transporting::Fluxes_from_bdfls const &fluxes_from_bdfls );
void productMatrixInfo2( GIDI::Protare *protare, GIDI::Styles::TemperatureInfo temperature, std::string const &label, int neutronGID, int photonGID,
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

    argvOptions argv_options( "productMatrixCollapse_photoAtomic", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askPid = false;

    parseTestOptions.parse( );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
    PoPI::Database pops;

    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::photon, "O" );

    std::cout << stripDirectoryBase( protare->fileName( ) ) << std::endl;

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        GIDI::PhysicalQuantity const &temperature = iter->temperature( );

        std::cout << "label = " << iter->heatedMultiGroup( ) << "  temperature = " << temperature.value( ) << std::endl;
    }

    std::string label( temperatures[0].heatedMultiGroup( ) );
    GIDI::Styles::TemperatureInfo temperature( temperatures[0] );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../bdfls", 0 );

    productMatrixInfo( protare, temperature, label, 23, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo( protare, temperature, label, 24, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo( protare, temperature, label, 25, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo( protare, temperature, label, 26, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo( protare, temperature, label, 4, groups_from_bdfls, fluxes_from_bdfls );

    delete protare;
}
/*
=========================================================
*/
void productMatrixInfo( GIDI::Protare *protare, GIDI::Styles::TemperatureInfo temperature, std::string const &label, int neutronGID,
                        GIDI::Transporting::Groups_from_bdfls const &groups_from_bdfls,
                        GIDI::Transporting::Fluxes_from_bdfls const &fluxes_from_bdfls ) {

    productMatrixInfo2( protare, temperature, label, neutronGID, 76, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo2( protare, temperature, label, neutronGID, 70, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo2( protare, temperature, label, neutronGID, 73, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo2( protare, temperature, label, neutronGID, 74, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo2( protare, temperature, label, neutronGID, 75, groups_from_bdfls, fluxes_from_bdfls );
    productMatrixInfo2( protare, temperature, label, neutronGID, 76, groups_from_bdfls, fluxes_from_bdfls );
}
/*
=========================================================
*/
void productMatrixInfo2( GIDI::Protare *protare, GIDI::Styles::TemperatureInfo temperature, std::string const &label, int neutronGID, int photonGID,
                        GIDI::Transporting::Groups_from_bdfls const &groups_from_bdfls,
                        GIDI::Transporting::Fluxes_from_bdfls const &fluxes_from_bdfls ) {

    LUPI::StatusMessageReporting smr1;
    std::size_t offset = 4;
    std::size_t prefixLength = outputChannelStringMaximumLength( protare );
    if( prefixLength < 32 ) prefixLength = 32;

    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::on );

    GIDI::Transporting::Particles particles;

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( neutronGID ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon, groups_from_bdfls.getViaGID( photonGID ) );
    photon.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( photon );

    particles.process( *protare, label );

    particles.print( );
    std::cout << std::endl;

    int maxOrder = protare->maximumLegendreOrder( smr1, settings, temperature, PoPI::IDs::photon );
    std::string prefix( "Total gamma-gamma matrix:" );
    prefix.insert( prefix.size( ), offset + prefixLength + 1 - prefix.size( ), ' ' );
    GIDI::Matrix uncollapsed = protare->multiGroupProductMatrix( smr1, settings, temperature, particles, PoPI::IDs::photon, 0 );
    printMatrix( prefix, maxOrder, uncollapsed );

    prefix = "Collapsed gamma-gamma matrix:";
    prefix.insert( prefix.size( ), offset + prefixLength + 1 - prefix.size( ), ' ' );
    GIDI::Matrix collapsed = GIDI::collapse( uncollapsed, settings, particles, 0, PoPI::IDs::photon );
    printMatrix( prefix, maxOrder, collapsed );

    maxOrder = protare->maximumLegendreOrder( smr1, settings, temperature, PoPI::IDs::neutron );
    prefix = "Total gamma-neutron matrix:";
    prefix.insert( prefix.size( ), offset + prefixLength + 1 - prefix.size( ), ' ' );
    uncollapsed = protare->multiGroupProductMatrix( smr1, settings, temperature, particles, PoPI::IDs::neutron, 0 );
    printMatrix( prefix, maxOrder, uncollapsed );

    prefix = "Collapsed gamma-neutron matrix:";
    prefix.insert( prefix.size( ), offset + prefixLength + 1 - prefix.size( ), ' ' );
    collapsed = GIDI::collapse( uncollapsed, settings, particles, 0, PoPI::IDs::photon );
    printMatrix( prefix, maxOrder, collapsed );

return;

    GIDI::Matrix summedCollapsed( 0, 0 );
    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction *reaction = protare->reaction( index );
        maxOrder = reaction->maximumLegendreOrder( smr1, settings, temperature, PoPI::IDs::photon );
        GIDI::Matrix m1 = reaction->multiGroupProductMatrix( smr1, settings, temperature, particles, PoPI::IDs::photon, 0 );
        std::string string( reaction->label( ) );

        string += ":: ";
        printMatrix( string, maxOrder, m1 );
    }
}
