/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <math.h>
#include <iostream>

#include <GIDI_testUtilities.hpp>

static char const *description = "This program prints the multi-group available energy for a protare and its reactions.";

void main2( int argc, char **argv );
void readProtare( ParseTestOptions &a_parseTestOption, GIDI::Construction::PhotoMode photoMode, GIDI::Transporting::Particle const &photonParticle );
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

    argvOptions argv_options( __FILE__, description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askPid = false;
    parseTestOptions.m_askGNDS_File = true;
    parseTestOptions.m_askPhotoAtomic = false;
    parseTestOptions.m_askPhotoNuclear = false;

    parseTestOptions.parse( );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../bdfls", 0 );

    GIDI::Transporting::Particle photonParticle( PoPI::IDs::photon, groups_from_bdfls.getViaGID( 70 ) );
    photonParticle.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );

    std::cout << "Atomic only." << std::endl;
    readProtare( parseTestOptions, GIDI::Construction::PhotoMode::atomicOnly, photonParticle );

    std::cout << std::endl;
    std::cout << "Nuclear only." << std::endl;
    readProtare( parseTestOptions, GIDI::Construction::PhotoMode::nuclearOnly, photonParticle );

    std::cout << std::endl;
    std::cout << "Nuclear and atomic." << std::endl;
    readProtare( parseTestOptions, GIDI::Construction::PhotoMode::nuclearAndAtomic, photonParticle );
}
/*
=========================================================
*/
void readProtare( ParseTestOptions &a_parseTestOptions, GIDI::Construction::PhotoMode photoMode, GIDI::Transporting::Particle const &photonParticle ) {

    LUPI::StatusMessageReporting smr1;
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photoMode );
    PoPI::Database pops;
    GIDI::Protare *protare = a_parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::photon, "O16" );
    if( protare == nullptr ) {
        std::string targetID = a_parseTestOptions.m_argvOptions.find( "--tid" )->zeroOrOneOption( a_parseTestOptions.m_argv, "O16" );
        std::cout << "protare for " << targetID << " not found." << std::endl;
        return;
    }

    for( std::size_t i1 = 0; i1 < protare->numberOfProtares( ); ++i1 ) {
        std::cout << stripDirectoryBase( protare->fileName( i1 ), "/GIDI/Test/" ) << std::endl;
    }

    std::cout << "  " << "Number of reactions = " << protare->numberOfReactions( ) << std::endl;

    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction *reaction = protare->reaction( index );

        GIDI::OutputChannel *outputChannel = reaction->outputChannel( );
        GIDI::Product *product = outputChannel->products( ).get<GIDI::Product>( 0 );
        GIDI::Suite &distribution = product->distribution( );

        std::cout << "    " << reaction->label( ) << std::endl;
        std::cout << "      " << product->particle( ).ID( ) << std::endl;

        for( std::size_t i2 = 0; i2 < distribution.size( ); ++i2 ) {
            GIDI::Distributions::Distribution *form = distribution.get<GIDI::Distributions::Distribution>( i2 );

            std::cout << "       distribution form moniker = " << form->moniker( ) << std::endl;
        }
    }

    std::cout << std::endl;

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( std::size_t temperatureIndex = 0; temperatureIndex < temperatures.size( ); ++temperatureIndex ) {
        GIDI::Styles::TemperatureInfo &temperatureInfo = temperatures[temperatureIndex];
        GIDI::PhysicalQuantity temperature( temperatureInfo.temperature( ) );

        std::cout << "temperatureIndex = " << temperatureIndex << ":   temperature = " << temperature.value( ) << " " << temperature.unit( ) << std::endl;
        std::cout << "    heatedCrossSection = <" << temperatureInfo.heatedCrossSection( ) << ">" << std::endl;
        std::cout << "    griddedCrossSection = <" << temperatureInfo.griddedCrossSection( ) << ">" << std::endl;
        std::cout << "    heatedMultiGroup = <" << temperatureInfo.heatedMultiGroup( ) << ">" << std::endl;
        std::cout << "    SnElasticUpScatter = <" << temperatureInfo.SnElasticUpScatter( ) << ">" << std::endl;
    }

    std::string label( temperatures[0].heatedMultiGroup( ) );
    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::on );

    GIDI::Transporting::Particles particles;
    particles.add( photonParticle );
    particles.process( *protare, label );

    std::vector<double> doubles = protare->groupBoundaries( settings, temperatures[0], PoPI::IDs::neutron );
    printVectorOfDoubles( "Neutron group boundaries :: ", doubles );

    doubles = protare->groupBoundaries( settings, temperatures[0], PoPI::IDs::photon );
    printVectorOfDoubles( "Photon group boundaries :: ", doubles );

    GIDI::Vector vector = protare->multiGroupCrossSection( smr1, settings, temperatures[0] );
    printVector( "Total cross section :: ", vector );

    vector = protare->multiGroupInverseSpeed( smr1, settings, temperatures[0] );
    printVector( "Inverse speed :: ", vector );

    vector = protare->multiGroupQ( smr1, settings, temperatures[0], false );
    printVector( "Q (initial) :: ", vector );
    vector = protare->multiGroupQ( smr1, settings, temperatures[0], true );
    printVector( "Q (final)   :: ", vector );

    vector = protare->multiGroupAvailableEnergy( smr1, settings, temperatures[0] );
    printVector( "Available energy   :: ", vector );

    vector = protare->multiGroupAvailableMomentum( smr1, settings, temperatures[0] );
    printVector( "Available momentum :: ", vector );

    vector = protare->multiGroupAverageEnergy( smr1, settings, temperatures[0], PoPI::IDs::neutron );
    printVector( "Neutron average energy :: ", vector );

    vector = protare->multiGroupAverageEnergy( smr1, settings, temperatures[0], PoPI::IDs::photon );
    printVector( "Photon average energy :: ", vector );

    vector = protare->multiGroupAverageMomentum( smr1, settings, temperatures[0], PoPI::IDs::neutron );
    printVector( "Neutron average momentum :: ", vector );

    vector = protare->multiGroupAverageMomentum( smr1, settings, temperatures[0], PoPI::IDs::photon );
    printVector( "Photon average momentum :: ", vector );

    vector = protare->multiGroupMultiplicity( smr1, settings, temperatures[0], PoPI::IDs::neutron );
    printVector( "Neutron multiplicity :: ", vector );

    vector = protare->multiGroupMultiplicity( smr1, settings, temperatures[0], PoPI::IDs::photon );
    printVector( "Photon multiplicity :: ", vector );

    std::set<std::string> ids;
    protare->productIDs( ids, particles, false );
    printIDs( "Product IDs (all)           : ", ids );

    ids.clear( );
    protare->productIDs( ids, particles, true );
    printIDs( "Product IDs (transportable) : ", ids );

    int intMaxOrder = protare->maximumLegendreOrder( smr1, settings, temperatures[0], PoPI::IDs::photon );
    if( intMaxOrder > -1 ) {
        std::size_t maxOrder = static_cast<std::size_t>( intMaxOrder );
        for( std::size_t order = 0; order < maxOrder; ++order ) {

            std::cout << "Data for Legendre order " << order << std::endl;

            vector = protare->multiGroupTransportCorrection( smr1, settings, temperatures[0], particles, order, GIDI::TransportCorrectionType::Pendlebury, 0.0 );
            printVector( "    Transport correction ::", vector );

            GIDI::Matrix matrix = protare->multiGroupProductMatrix( smr1, settings, temperatures[0], particles, PoPI::IDs::photon, order );
            printMatrix( "    Photon product matrix", -2, matrix );
        }
    }

// depositionData

    delete protare;
}
