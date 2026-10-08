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

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

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
        std::cout << str << std::endl;
        exit( EXIT_FAILURE ); }
    catch (std::string &str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

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

    argvOptions2 argv_options( "sampleProducts", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--pa", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-pn* is specified." ) );
    argv_options.add( argvOption2( "--pn", false, "Include photo-nuclear protare if relevant. This is the default unless *-pa* is specified." ) );
    argv_options.add( argvOption2( "-d", false, "If present, fission delayed neutrons are included with product sampling." ) );
    argv_options.add( argvOption2( "--all", false, "If present, all particles are sampled; otherwise only transporting particles are sampled." ) );
    argv_options.add( argvOption2( "-n", false, "If present, add neutron as transporting particle." ) );
    argv_options.add( argvOption2( "-p", false, "If present, add photon as transporting particle." ) );
    argv_options.add( argvOption2( "--nonRawTNSL", false, "If present, TNSL double differential data (and not distribution data) are used to sample elastic neutrons." ) );
    argv_options.add( argvOption2( "--electron", false, "If present and the protare is photo-atomic, electrons are transported." ) );
    argv_options.add( argvOption2( "--ENDL99120", false, "If present, ENDL two 99120 products are list for a fission reaction." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    int projectileIntid = pops.intid( projectileID );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );

    GIDI::Transporting::DelayedNeutrons delayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
    if( argv_options.find( "-d" )->present( ) ) delayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;

    if( argv_options.find( "--pa" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "--pn" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    bool transportElectrons = argv_options.find( "--electron" )->present( );

    GIDI::Map::Map map( mapFilename, pops );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    if( argv_options.find( "--ENDL99120" )->present( ) )
        construction.setFissionResiduals( GIDI::Construction::FissionResiduals::ENDL99120 );

    protare = (GIDI::Protare *) map.protare( construction, pops, projectileID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, delayedNeutrons, energyDomainMax );
    MC.sampleNonTransportingParticles( argv_options.find( "--all" )->present( ) );
    MC.set_wantRawTNSL_distributionSampling( !argv_options.find( "--nonRawTNSL" )->present( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    if( ( projectileID == PoPI::IDs::neutron ) || argv_options.find( "-n" )->present( ) ) {
        GIDI::Transporting::Particle neutron( PoPI::IDs::neutron );
        particles.add( neutron );
    }

    if( ( projectileID == PoPI::IDs::photon ) || argv_options.find( "-p" )->present( ) ) {
        GIDI::Transporting::Particle photon( PoPI::IDs::photon );
        particles.add( photon );
    }

    if( transportElectrons && !particles.hasParticle( PoPI::IDs::electron ) ) {
        for( std::size_t protareIndex = 0; protareIndex < protare->numberOfProtares( ); ++protareIndex ) {
            GIDI::ProtareSingle *protareSingle = protare->protare( protareIndex );

            if( protareSingle->isPhotoAtomic( ) ) {
                GIDI::Transporting::Particle electron( PoPI::IDs::electron );
                particles.add( electron );
                break;
            }
        }
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCProtare->setUserParticleIndex( pops[PoPI::IDs::neutron], 0 );
    MCProtare->setUserParticleIndex( pops["H2"], 10 );
    MCProtare->setUserParticleIndex( pops[PoPI::IDs::photon], 11 );
    MCProtare->setUserParticleIndex( pops[PoPI::IDs::electron], 12 );
    MCProtare->setUserParticleIndex( pops[PoPI::IDs::FissionProductENDL99120], 13 );

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

                std::cout << "        productIndex " << std::setw( 4 ) << product.m_productIndex << " " << std::setw( 4 ) << product.m_userProductIndex;
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

                input.setTemperatureAndEnergy( 0.0, energy );
                for( std::size_t i2 = 0; i2 < numberOfFissionSamples; ++i2 ) {
                    products.clear( );
                    reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                            [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                    for( std::size_t i3 = 0; i3 < products.size( ); ++i3 ) {
                        MCGIDI::Sampling::Product const &product = products[i3];

                        if( product.m_productIntid == projectileIntid ) {
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
