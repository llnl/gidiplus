/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

static char const *description = "Reads a photo-atomic protare and loops over each reaction. For each reaction, one product sample is done at a list of projectile energies.";

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <set>

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

int main2( int argc, char **argv );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        return( EXIT_FAILURE );
    }
    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
int main2( int argc, char **argv ) {

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    std::string projectileID( PoPI::IDs::photon );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    unsigned long long rngState = 1;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "sampleProducts", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--all", false, "If present, all particles are sampled; otherwise only transporting particles are sampled." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all.map" );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );

    GIDI::Map::Map map( mapFilename, pops );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
    protare = map.protare( construction, pops, projectileID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.sampleNonTransportingParticles( argv_options.find( "--all" )->present( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon, groups_from_bdfls.getViaGID( 70 ) );
    photon.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( photon );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    std::size_t numberOfReactions = MCProtare->numberOfReactions( );

    MCGIDI::Sampling::StdVectorProductHandler products;
    for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( i1 );
        double threshold = MCProtare->threshold( i1 );

        std::cout << "reaction (" << std::setw( 3 ) << i1 << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
        if( threshold < 1e-13 ) threshold = 1e-13;
        for( double energy = threshold; energy < 100; energy *= 2 ) {
            products.clear( );

            std::cout << "    energy = " << energy << std::endl;
            input.setTemperatureAndEnergy( 0.0, energy );
            reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                    [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );

            for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                MCGIDI::Sampling::Product const &product = products[i2];
                PoPI::ParseIntidInfo parseIntidInfo( product.m_productIntid );
                PoPI::Particle const &particle = pops.particle( parseIntidInfo.id( ) );
                std::cout << "        productIndex " << std::setw( 12 ) << particle.ID( );
                if( product.m_sampledType == MCGIDI::Sampling::SampledType::unspecified ) {
                    std::cout << " unspecified distribution" << std::endl; }
                else {
                    double p = sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz );
                    std::cout << " KE = " << product.m_kineticEnergy << "  p = " << p << "  m_px_vx = " << product.m_px_vx << 
                            "  m_py_vy = " << product.m_py_vy << "  m_pz_vz = " << product.m_pz_vz << std::endl;
                }
            }
        }
    }

    delete protare;
    delete MCProtare;

    return( EXIT_SUCCESS );
}
