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

#include <statusMessageReporting.h>
#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"
#include <bins.hpp>

/*
=========================================================
*/
int main( int argc, char **argv ) {

    long numberOfSamples = 1000 * 1000;
    int numberOfBins = 101;
    std::string mapFilename( "../../../GIDI/Test/all.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string photonID( PoPI::IDs::photon );
    std::string targetID = "O16";
    int photonIndex = pops[photonID];
    std::string protareFilename( map.protareFilename( photonID, targetID ) );
    GIDI::ProtareSingle *protare;
    std::vector<std::string> libraries;
    GIDI::Transporting::Particles particles;
    int reactionIndex = 1;
    unsigned long long rngState = 1;
    char *message;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
        protare = (GIDI::ProtareSingle *) map.protare( construction, pops, photonID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, photonID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    GIDI::Transporting::Particle projectile( photonID, GIDI::Transporting::Mode::MonteCarloContinuousEnergy );
    particles.add( projectile );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::ProtareSingle *MCProtare;
    try {
        MCProtare = new MCGIDI::ProtareSingle( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    MCGIDI::Sampling::StdVectorProductHandler products;
    MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
    double threshold = MCProtare->threshold( reactionIndex );

    std::cout << "reaction (" << std::setw( 3 ) << reactionIndex << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
    if( threshold < 1e-13 ) threshold = 1e-13;

    FILE *fOutEnergy = fopen( "energy.out", "w" );
    FILE *fOutMu = fopen( "mu.out", "w" );

    Bins energyBins( numberOfBins, 0.0, 1.0 );
    Bins muBins( numberOfBins, -1.0, 1.0 );

    int energyIndex = 0;
    for( double energy = threshold; energy < 200; energy *= 10, ++energyIndex ) {
        energyBins.setDomain( energy / ( 1 + 2.0 * energy / 0.510998946269 ), energy );
        energyBins.clear( );
        muBins.clear( );
        for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
            products.clear( );
            reaction->sampleProducts( MCProtare, energy, input, [&]( ) -> double { return float64RNG64( &rngState ); }, 
                    [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
            for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                MCGIDI::Sampling::Product const &product = products[i2];

                if( product.m_productIndex == photonIndex ) {
                    energyBins.accrue( product.m_kineticEnergy );

                    muBins.accrue( product.m_pz_vz / sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz ) );
                }
            }
        }

        message = smr_allocateFormatMessage( "#product energy for projectile energy = %e\n", energy );
        energyBins.print( fOutEnergy, message );
        free( message );

        message = smr_allocateFormatMessage( "#product mu for projectile energy = %e\n", energy );
        muBins.print( fOutMu, message );
        free( message );
    }

    delete protare;

    delete MCProtare;
}
