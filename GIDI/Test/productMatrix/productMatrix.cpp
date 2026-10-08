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

static char const *description = "Prints out transfer matrices for given projectile/target/product.";

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

    LUPI::StatusMessageReporting smr1;
    argvOptions argv_options( "productMatrix", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askGNDS_File = true;
    parseTestOptions.m_askDelayedFissionNeutrons = true;
    parseTestOptions.m_askOid = true;

    argv_options.add( argvOption( "--order", true, "If present, Legendre order of the product matrix to retrieve." ) );

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photo_mode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );

    std::cout << stripDirectoryBase( protare->fileName( ), "/GIDI/Test/" ) << std::endl;

    std::string productID = argv_options.find( "--oid" )->zeroOrOneOption( argv, protare->projectile( ).ID( ) );

    GIDI::Transporting::DelayedNeutrons includeDelayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
    if( argv_options.find( "--delayed" )->present( ) ) includeDelayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;
    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, includeDelayedNeutrons );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    GIDI::Styles::TemperatureInfo temperature = temperatures[0];
    std::string label( temperature.heatedMultiGroup( ) );
    GIDI::Styles::HeatedMultiGroup const &heatedMultiGroup = *(protare->styles( ).get<GIDI::Styles::HeatedMultiGroup>( label ));

    GIDI::Suite const &transportables = heatedMultiGroup.transportables( );
    GIDI::Transportable const *transportable = nullptr;
    for( std::size_t i1 = 0; i1 < transportables.size( ); ++i1 ) {
        GIDI::Transportable const *transportable2 = transportables.get<GIDI::Transportable const>( i1 );
        if( transportable2->pid( ) == productID ) {
            transportable = transportable2;
            break;
        }
    }
    if( transportable == nullptr ) {
        std::cout << "No transportable data for " << productID << "." << std::endl; }
    else {
        GIDI::Transporting::Particles particles;
        GIDI::Group group( transportable->group( ) );
        GIDI::Transporting::Particle productParticle( productID, group, GIDI::Transporting::Mode::multiGroup );

        GIDI::Transporting::Flux flux( "flat", 0.0 );
        double fluxEnergies[2] = { 0.0, 0.0 }, fluxValues[2] = { 1.0, 1.0 };
        fluxEnergies[1] = group.grid( ).data( )[group.grid( ).data( ).size( ) - 1];
        GIDI::Transporting::Flux_order flux_0_order = GIDI::Transporting::Flux_order( 0, 2, fluxEnergies, fluxValues );
        flux.addFluxOrder( flux_0_order );
        productParticle.appendFlux( flux );

        particles.add( productParticle );
        particles.process( *protare, label );

        std::string prefix( "Total " + productID + " production matrix: " );
        int maxOrder = protare->maximumLegendreOrder( smr1, settings, temperature, productID );

        std::size_t order = static_cast<std::size_t>( argv_options.find( "--order" )->asInt( argv ) );
        GIDI::Matrix m1 = protare->multiGroupProductMatrix( smr1, settings, temperature, particles, productID, order );
        printMatrix( prefix, maxOrder, m1 );

        prefix = "Total fission matrix: ";
        m1 = protare->multiGroupFissionMatrix( smr1, settings, temperature, particles, order );
        printMatrix( prefix, -2, m1 );

        for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
            GIDI::Reaction const *reaction = protare->reaction( index );
            maxOrder = reaction->maximumLegendreOrder( smr1, settings, temperature, productID );
            m1 = reaction->multiGroupProductMatrix( smr1, settings, temperature, particles, productID, order );
            std::string string( reaction->label( ) );

            string += ": ";
            printMatrix( string, maxOrder, m1 );

            m1 = reaction->multiGroupFissionMatrix( smr1, settings, temperature, particles, order );
            string = "  fission:";
            if( m1.size( ) > 0 ) printMatrix( string, -2, m1 );
        }
    }

    delete protare;
}
