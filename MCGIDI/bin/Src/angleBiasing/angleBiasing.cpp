/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <set>

#include <statusMessageReporting.h>

#include <MCGIDI.hpp>

#include <GIDI_testUtilities.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <bins.hpp>

static char const *description = "For a specified projectile energy (first positional argument) and outgoing product \n" \
    "(specified via option --oid), this code generates an energy spectrum for the product \n" \
    "going into the request cosine angle (mu) in the lab frame (second positional argument).";

void main2( int argc, char **argv );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv );
        exit( EXIT_SUCCESS ); }
    catch (std::exception &exception) {
        std::cerr << exception.what( ) << std::endl; }
    catch (char const *str) {
        std::cout << str << std::endl; }
    catch (std::string &str) {
        std::cout << str << std::endl;
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    PoPI::Database pops;
    unsigned long long rngState = 1;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    GIDI::Transporting::Particles particles;
    double temperature_keV_K = 2.582e-5;
    LUPI::StatusMessageReporting smr1;

    std::map<std::string, std::string> particlesAndGIDs;

    particlesAndGIDs[PoPI::IDs::neutron] = "LLNL_gid_4";
    particlesAndGIDs["H1"] = "LLNL_gid_71";
    particlesAndGIDs["H2"] = "LLNL_gid_71";
    particlesAndGIDs["H3"] = "LLNL_gid_71";
    particlesAndGIDs["He3"] = "LLNL_gid_71";
    particlesAndGIDs["He4"] = "LLNL_gid_71";
    particlesAndGIDs[PoPI::IDs::photon] = "LLNL_gid_70";

    argvOptions argv_options( "angleBiasing", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askOid= true;
    parseTestOptions.m_askGNDS_File = true;

    argv_options.add( argvOption( "-r", true, "Specifies the reaction index for sampling angle biasing. If negative, all reactions are listed and then the code exits." ) );
    argv_options.add( argvOption( "-n", true, "Number of samples. If value is negative, it is multiplied by -1000000." ) );
    argv_options.add( argvOption( "--numberOfBins", true, "Number of sample bins. Default is 1000." ) );

    parseTestOptions.parse( );

    if( argv_options.m_arguments.size( ) != 2 ) throw "Need projectile energy and outgoing particle angle (in lab frame).";

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../../TestData/PoPs/pops.xml", "../../../../GIDI/Test/Data/MG_MC/all_maps.map", construction, PoPI::IDs::neutron, "O16" );

    std::string productID = argv_options.find( "--oid" )->zeroOrOneOption( argv, protare->projectile( ).ID( ) );
    long numberOfSamples = argv_options.find( "-n" )->asLong( argv, -1 );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    std::size_t numberOfBins = static_cast<std::size_t>( argv_options.find( "--numberOfBins" )->asLong( argv, 1000 ) );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../../GIDI/Test/bdfls", 0.0 );

    for( std::map<std::string, std::string>::iterator iter = particlesAndGIDs.begin( ); iter != particlesAndGIDs.end( ); ++iter ) {
        if( iter->first == productID ) {
            GIDI::Transporting::MultiGroup multi_group = groups_from_bdfls.viaLabel( iter->second );
            GIDI::Transporting::Particle particle( iter->first, multi_group );

            particle.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
            particles.add( particle );
        }
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    int productIndex = static_cast<int>( pops[productID] );

    double energy_in = argv_options.asDouble( argv, 0 );
    double mu_lab = argv_options.asDouble( argv, 1 );
    if( mu_lab < -1.0 ) throw "mu_lab must be greater than or equal to -1.0";
    if( mu_lab > 1.0 ) throw "mu_lab must be less than or equal to 1.0";

    long reactionIndex = argv_options.find( "-r" )->asLong( argv, 0 );
    if( reactionIndex >= static_cast<long>( MCProtare->numberOfReactions( ) ) ) throw "Invalid reaction index.";
    if( reactionIndex < 0 ) {
        std::cout << "List of reaction indices, thresholds and labels are:" << std::endl;
        for( std::size_t reactionIndex1 = 0; reactionIndex1 < MCProtare->numberOfReactions( ); ++reactionIndex1 ) {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex1 );

            std::cout << std::setw( 4 ) << reactionIndex1 << "  " << doubleToString( "%14.6e", reaction->crossSectionThreshold( ) ) << "  " << reaction->label( ).c_str( ) << std::endl;
        } }
    else {

        MCGIDI::Reaction const *reaction = MCProtare->reaction( static_cast<std::size_t>( reactionIndex ) );

        double energy_out = 1, weight;
        weight = reaction->angleBiasing( productIndex, temperature_keV_K, energy_in, mu_lab, energy_out, 
                [&]() -> double { return float64RNG64( &rngState ); } );
        double energyMin = energy_out, energyMax = energy_out;

        long numberOfSamples2 = 1000;
        if( 100 * numberOfSamples2 < numberOfSamples ) numberOfSamples2 = numberOfSamples / 100;
        for( long sampleIndex = 0; sampleIndex < numberOfSamples2; ++sampleIndex ) {
            weight = reaction->angleBiasing( productIndex, temperature_keV_K, energy_in, mu_lab, energy_out,
                    [&]() -> double { return float64RNG64( &rngState ); } );
            if( weight != 0.0 ) {
                if( energy_out < energyMin ) energyMin = energy_out;
                if( energy_out > energyMax ) energyMax = energy_out;
            }
        }

        energyMin *= 0.5;
        energyMax *= 1.2;
        double deltaEnergy = energyMax - energyMin;
        if( deltaEnergy > energyMin ) energyMin = 0.0;

        if( energyMin == energyMax ) energyMax = 1.0 + 1.1 * energyMin;

        Bins bins( numberOfBins, energyMin, energyMax );

        for( long sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex ) {
            weight = reaction->angleBiasing( productIndex, temperature_keV_K, energy_in, mu_lab, energy_out,
                    [&]() -> double { return float64RNG64( &rngState ); } );
            if( weight != 0.0 ) bins.accrue( energy_out, weight );
        }

        bins.print( stdout, "", true );
    }

    delete protare;
    delete MCProtare;

    exit( EXIT_SUCCESS );
}
