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

static char const *description = "Prints out multi-group data.";

int main2( int argc, char **argv );
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
int main2( int argc, char **argv ) {

    LUPI::StatusMessageReporting smr1;
    argvOptions argv_options( "TNSL", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askPid = false;

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photo_mode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../Data/MG_MC/neutrons/all.map", construction, PoPI::IDs::neutron, "tnsl-Al27" );

    std::cout << stripDirectoryBase( protare->fileName( ), "/GIDI/Test/" ) << std::endl;

    bool isTNSL = protare->protareType( ) == GIDI::ProtareType::TNSL;
    std::cout << "Is TNSL protare = " << isTNSL << std::endl;
    std::cout << "projectile = " << protare->projectile( ).ID( ) << " target = " << protare->target( ).ID( ) << " evaluation = " << protare->evaluation( ) << std::endl;

    std::cout << "  " << "Number of reactions = " << protare->numberOfReactions( ) << std::endl;

    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction const *reaction = protare->reaction( index );

        std::cout << "    " << reaction->label( ) << std::endl;
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

    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::on );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../bdfls", 0 );

    GIDI::Transporting::Particle neutronParticle( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
    neutronParticle.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );

    GIDI::Transporting::Particles particles;
    particles.add( neutronParticle );
    particles.process( *protare, temperatures[0].heatedMultiGroup( ) );

    std::vector<double> doubles = protare->groupBoundaries( settings, temperatures[0], PoPI::IDs::neutron );
    printVectorOfDoubles( "Neutron group boundaries :: ", doubles );

    doubles = protare->groupBoundaries( settings, temperatures[0], PoPI::IDs::photon );
    printVectorOfDoubles( "Photon group boundaries :: ", doubles );

    GIDI::Vector vector = protare->multiGroupInverseSpeed( smr1, settings, temperatures[0] );
    printVector( "Inverse speed :: ", vector );

    vector = protare->multiGroupCrossSection( smr1, settings, temperatures[0] );
    printVector( "Total cross section :: ", vector );

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

            GIDI::Matrix matrix = protare->multiGroupProductMatrix( smr1, settings, temperatures[0], particles, PoPI::IDs::neutron, order );
            printMatrix( "    Neutron product matrix", -2, matrix );
        }
    }

    delete protare;

    return( EXIT_SUCCESS );
}
