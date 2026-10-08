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

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

/*
=========================================================
*/
int main( int argc, char **argv ) {

    std::string mapFilename( "../../../GIDI/Test/all3T.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string neutronID( PoPI::IDs::neutron );
    std::string targetID( "O16" );
    int photonIndex = pops[PoPI::IDs::photon];
    long numberOfSamples = 10 * 1000;
    GIDI::Protare *protare;
    std::vector<std::string> libraries;
    GIDI::Transporting::Particles particles;
    unsigned long long rngState = 1;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
        protare = (GIDI::Protare *) map.protare( construction, pops, neutronID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 30.0 );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon );
    particles.add( photon );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::ProtareSingle *MCProtare;
    try {
        MCProtare = (MCGIDI::ProtareSingle *) MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    std::cout << std::setw( 40 ) << std::left << "reaction" << " | threshold" << std::endl;
    for( int i1 = 0; i1 < 60; ++i1 ) std::cout << "-";
    std::cout << std::endl;
    for( std::size_t reactionIndex = 0; reactionIndex < protare->numberOfReactions( ); ++reactionIndex ) {
        GIDI::Reaction const *reaction = protare->reaction( reactionIndex );
        std::cout << std::setw( 40 ) << std::left << reaction->label( ).c_str( ) << " | " << reaction->crossSectionThreshold( ) << std::endl;
    }

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    std::size_t numberOfReactions = MCProtare->reactions( ).size( );

    std::vector<double> energies;
    energies.push_back(  1.0 );
    energies.push_back(  5.0 );
    energies.push_back( 10.0 );
    energies.push_back( 15.0 );
    energies.push_back( 20.0 );
    energies.push_back( 25.0 );
    energies.push_back( 30.0 );

    MCGIDI::Sampling::StdVectorProductHandler products;
    for( std::size_t energyIndex = 0; energyIndex < energies.size( ); ++energyIndex ) {
        double energy = energies[energyIndex];

        std::cout << "energy = " << energy << std::endl;

        for( std::size_t reactionIndex = 0; reactionIndex < numberOfReactions; ++reactionIndex ) {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
            double threshold = MCProtare->threshold( reactionIndex );
            long gammaCounter = 0;

            if( threshold < energy ) {
                for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
                    products.clear( );

                    input.setTemperatureAndEnergy( 0.0, energy );
                    reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                        [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                    for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                        MCGIDI::Sampling::Product const &product = products[i2];
                        if( product.m_productIndex == photonIndex ) ++gammaCounter;

                    }
                }
            }

            std::cout << "        gammaCounter = " << std::right << std::setw( 8 ) << gammaCounter;
            std::cout << " : reaction (" << std::right << std::setw( 3 ) << reactionIndex << ") = " << std::setw( 40 ) << std::left << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
        }
    }

    delete protare;

    delete MCProtare;
}
