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

static char const *description = "This program prints the multi-group available energy for a protare and its reactions.";

void main2( int argc, char **argv );
void printVector( GIDI::Protare *protare, GIDI::Transporting::MG &settings, GIDI::Styles::TemperatureInfo temperature, GIDI::Transporting::Particles &particles, char const *prefix, 
        GIDI::TransportCorrectionType transportCorrectionType, GIDI::Transporting::DelayedNeutrons delayedNeutrons );
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

    argvOptions argv_options( "availableEnergy", description );
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
    std::cout << "label = " << label << std::endl;

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../bdfls", 0 );

    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::on );

    GIDI::Transporting::Particles particles;

    GIDI::Transporting::Particle neutron( protare->projectile( ).ID( ), groups_from_bdfls.getViaGID( 4 ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );

    particles.process( *protare, label );

    printVector( protare, settings, temperature, particles, "transport correction: None", GIDI::TransportCorrectionType::None, GIDI::Transporting::DelayedNeutrons::on );
    printVector( protare, settings, temperature, particles, "transport correction: None", GIDI::TransportCorrectionType::None, GIDI::Transporting::DelayedNeutrons::off );
    printVector( protare, settings, temperature, particles, "transport correction: Pendlebury", GIDI::TransportCorrectionType::Pendlebury, GIDI::Transporting::DelayedNeutrons::on );
    printVector( protare, settings, temperature, particles, "transport correction: Pendlebury", GIDI::TransportCorrectionType::Pendlebury, GIDI::Transporting::DelayedNeutrons::off );
#if 0
    printVector( protare, settings, temperature, particles, "transport correction: LLNL", GIDI::TransportCorrectionType::LLNL, GIDI::Transporting::DelayedNeutrons::on );
    printVector( protare, settings, temperature, particles, "transport correction: LLNL", GIDI::TransportCorrectionType::LLNL, GIDI::Transporting::DelayedNeutrons::off );
    printVector( protare, settings, temperature, particles, "transport correction: Ferguson", GIDI::TransportCorrectionType::Ferguson, GIDI::Transporting::DelayedNeutrons::on );
    printVector( protare, settings, temperature, particles, "transport correction: Ferguson", GIDI::TransportCorrectionType::Ferguson, GIDI::Transporting::DelayedNeutrons::off );
#endif

    delete protare;
}
/*
=========================================================
*/
void printVector( GIDI::Protare *protare, GIDI::Transporting::MG &settings, GIDI::Styles::TemperatureInfo temperature, GIDI::Transporting::Particles &particles, char const *prefix,
        GIDI::TransportCorrectionType transportCorrectionType, GIDI::Transporting::DelayedNeutrons delayedNeutrons ) {

    LUPI::StatusMessageReporting smr1;

    int intMaxOrder = protare->maximumLegendreOrder( smr1, settings, temperature, protare->projectile( ).ID( ) );
    if( intMaxOrder > -1 ) {
        std::size_t maxOrder = static_cast<std::size_t>( intMaxOrder );
        for( std::size_t order = 0; order < maxOrder; ++order ) {
            try {
                settings.setDelayedNeutrons( delayedNeutrons );
                GIDI::Vector transportCorrection = protare->multiGroupTransportCorrection( smr1, settings, temperature, particles, order, transportCorrectionType, 0.0 );
                std::string message( prefix );

                message += LUPI::Misc::argumentsToString( ": delayedNeutrons (%s)", 
                        ( delayedNeutrons == GIDI::Transporting::DelayedNeutrons::on ? "true"  : "false" ) );
                message += LUPI::Misc::argumentsToString( ": order = %d::", order );
                transportCorrection.print( message ); }
            catch (char const *str) {
                std::cout << str << std::endl;
                exit( EXIT_FAILURE );
            }
        }
    }
}
