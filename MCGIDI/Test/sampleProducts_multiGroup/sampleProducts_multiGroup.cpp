/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

static char const *description = "Loops over each reaction does 1 product sampling at various projectile energies\nstarting at the reaction's threshold energy.";

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <set>
#include <stdarg.h>

#include "LUPI.hpp"
#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

/*
=========================================================
*/
int main( int argc, char **argv ) {

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    unsigned long long rngState = 1;
    double energyDomainMax = 20.0;
    std::size_t numberOfFissionSamples = 100 * 1000;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    GIDI::Construction::PhotoMode photo_mode = GIDI::Construction::PhotoMode::nuclearOnly;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "sampleProducts_multiGroup", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--pa", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-n* present." ) );
    argv_options.add( argvOption2( "--pn", false, "Include photo-nuclear protare if relevant. This is the default unless *-a* present." ) );
    argv_options.add( argvOption2( "-d", false, "If present, fission delayed neutrons are included with product sampling." ) );
    argv_options.add( argvOption2( "--all", false, "If present, all particles are sampled; otherwise only transporting particles are sampled." ) );
    argv_options.add( argvOption2( "-n", false, "If present, add neutron as transporting particle." ) );
    argv_options.add( argvOption2( "-p", false, "If present, add photon as transporting particle." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    int neutronIndex = pops[PoPI::IDs::neutron];
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );

    GIDI::Transporting::DelayedNeutrons delayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
    if( argv_options.find( "-d" )->present( ) ) delayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;

    if( argv_options.find( "--pa" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "--pn" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    GIDI::Map::Map map( mapFilename, pops );

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
        protare = (GIDI::Protare *) map.protare( construction, pops, projectileID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedMultiGroup( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }
    std::string label( temperatures[0].heatedMultiGroup( ) );

    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, delayedNeutrons, energyDomainMax );
    MC.sampleNonTransportingParticles( argv_options.find( "--all" )->present( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    if( ( projectileID == PoPI::IDs::neutron ) || argv_options.find( "-n" )->present( ) ) {
        GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
        neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
        particles.add( neutron );
    }

    if( ( projectileID == PoPI::IDs::photon ) || argv_options.find( "-p" )->present( ) ) {
        GIDI::Transporting::Particle photon( PoPI::IDs::photon, groups_from_bdfls.getViaGID( 70 ) );
        photon.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
        particles.add( photon );
    }
    particles.process( *protare, label );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    try {
        MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    std::size_t numberOfReactions = MCProtare->numberOfReactions( );

    MCGIDI::Sampling::StdVectorProductHandler products;
    for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( i1 );
        double threshold = MCProtare->threshold( i1 );

        std::cout << "reaction (" << std::setw( 3 ) << i1 << ") = " << reaction->label( ).c_str( ) 
                << "  threshold = " << LUPI::Misc::doubleToString3( "%.6g", threshold, true ) << std::endl;
        if( threshold < 1e-13 ) threshold = 1e-13;
        for( double energy = threshold; energy < 100; energy *= 2 ) {
            products.clear( );

            std::cout << "    energy = " << LUPI::Misc::doubleToString3( "%.6g", energy, true )  << std::endl;
            input.setTemperatureAndEnergy( 0.0, energy );
            reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); }, 
                    [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
            for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                MCGIDI::Sampling::Product const &product = products[i2];

                PoPI::ParseIntidInfo parseIntidInfo( product.m_productIntid );
                PoPI::Particle const &particle = pops.particle(  parseIntidInfo.id( ) );
                std::cout << "        productIndex " << std::setw( 12 ) << particle.ID( );
                if( product.m_sampledType == MCGIDI::Sampling::SampledType::unspecified ) {
                    std::cout << " unspecified distribution" << std::endl; }
                else {
                    std::cout << " KE = " << product.m_kineticEnergy << std::endl;
                }
            }
        }
    }

    for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( i1 );

        if( reaction->hasFission( ) ) {
            double threshold = MCProtare->threshold( i1 );
            if( threshold < 1e-13 ) threshold = 1e-13;

            std::cout << std::endl;
            std::cout << "Scanning for delayed fission neutrons" << std::endl;
            std::cout << "    reaction (" << std::setw( 3 ) << i1 << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
            for( double energy = threshold; energy < 100; energy *= 10 ) {
                long totalFissionNeutrons = 0, delayedFissionNeutrons = 0;
                std::vector<long> delayedFissionNeutronIndexCounts( 10, 0 );

                for( std::size_t i2 = 0; i2 < numberOfFissionSamples; ++i2 ) {
                    products.clear( );
                    input.setTemperatureAndEnergy( 0.0, energy );
                    reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); }, 
                            [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                    for( std::size_t i3 = 0; i3 < products.size( ); ++i3 ) {
                        MCGIDI::Sampling::Product const &product = products[i3];

                        if( product.m_productIndex == neutronIndex ) {
                            ++totalFissionNeutrons;
                            if( product.m_delayedNeutronIndex > -1 ) {
                                ++delayedFissionNeutrons;
                                ++delayedFissionNeutronIndexCounts[product.m_delayedNeutronIndex];
                            }
                        }
                    }
                }

                double totalMultiplicity = totalFissionNeutrons / (double) numberOfFissionSamples, delayedMultiplicity = delayedFissionNeutrons / (double) numberOfFissionSamples;
                std::cout << "        energy = " << energy << " total neutrons = " << totalFissionNeutrons << " (" << doubleToString2( "%.4f", totalMultiplicity )
                        << ") delayed neutrons = " << delayedFissionNeutrons << " (" << doubleToString2( "%.3e", delayedMultiplicity ) << ")";
                if( delayedFissionNeutrons > 0 ) {
                    for( std::size_t i3 = 0; i3 < delayedFissionNeutronIndexCounts.size( ); ++i3 ) std::cout << " " << delayedFissionNeutronIndexCounts[i3];
                }
                std::cout << std::endl;
            }
        }
    }

    delete protare;

    delete MCProtare;
}
