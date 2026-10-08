/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

static char const *description = "Loops over range of incident energies and histograms the fission neutron multiplicity for the number of sampled fission neutrons.";

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <set>
#include <stdarg.h>

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"

#define numberOfPromptFissionNeutronBins 16

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

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    unsigned long long rngState = 1;
    double energyDomainMax = 20.0;
    std::size_t numberOfSamples = 100 * 1000;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    int neutronIntid = pops.intid( PoPI::IDs::neutron );

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "sampleTerrellPromptNeutronDistribution", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "-d", false, "If present, fission delayed neutrons are included with product sampling." ) );
    argv_options.add( argvOption2( "-t", false, "If present, prompt fission neutron multiplicity are sampled from the Terrell distribution." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/Data/MG_MC/all.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "Th227" );

    GIDI::Transporting::DelayedNeutrons delayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
    if( argv_options.find( "-d" )->present( ) ) delayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;

    GIDI::Map::Map map( mapFilename, pops );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    protare = (GIDI::Protare *) map.protare( construction, pops, projectileID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, delayedNeutrons, energyDomainMax );
    MC.wantTerrellPromptNeutronDistribution( argv_options.find( "-t" )->present( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    std::size_t numberOfReactions = MCProtare->numberOfReactions( );

    MCGIDI::Sampling::StdVectorProductHandler products;
    for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( i1 );

        if( reaction->hasFission( ) ) {
            double threshold = MCProtare->threshold( i1 );
            if( threshold < 1e-13 ) threshold = 1e-13;

            std::cout << std::endl;
            std::cout << "reaction (" << std::setw( 3 ) << i1 << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
            for( double energy = threshold; energy < 100; energy *= 10 ) {
                long totalFissionNeutrons = 0, delayedFissionNeutrons = 0;
                std::vector<long> promptFissionNeutronBins( numberOfPromptFissionNeutronBins, 0 );

                for( std::size_t i2 = 0; i2 < numberOfSamples; ++i2 ) {
                    long promptFissionNeutronCount = 0;

                    products.clear( );
                    input.setTemperatureAndEnergy( 0.0, energy );
                    reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                            [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                    for( std::size_t i3 = 0; i3 < products.size( ); ++i3 ) {
                        MCGIDI::Sampling::Product const &product = products[i3];

                        if( product.m_productIntid == neutronIntid ) {
                            ++totalFissionNeutrons;
                            if( product.m_delayedNeutronIndex < 0 ) {
                                ++promptFissionNeutronCount; }
                            else {
                                ++delayedFissionNeutrons;
                            }
                        }
                    }
                    if( promptFissionNeutronCount >= numberOfPromptFissionNeutronBins ) promptFissionNeutronCount = numberOfPromptFissionNeutronBins - 1;
                    ++promptFissionNeutronBins[promptFissionNeutronCount];
                }

                double totalMultiplicity = totalFissionNeutrons / (double) numberOfSamples, delayedMultiplicity = delayedFissionNeutrons / (double) numberOfSamples;
                std::cout << "    energy = " << energy << " total neutrons = " << totalFissionNeutrons << " (" << doubleToString2( "%.4f", totalMultiplicity )
                        << ") delayed neutrons = " << delayedFissionNeutrons << " (" << doubleToString2( "%.3e", delayedMultiplicity ) << ")";
                for( int i2 = 0; i2 < numberOfPromptFissionNeutronBins; ++i2 ) std::cout << " " << promptFissionNeutronBins[i2];
                std::cout << std::endl;
            }
        }
    }

    delete protare;

    delete MCProtare;
}
