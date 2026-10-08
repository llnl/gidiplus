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
#include <math.h>

#include <MCGIDI.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <GIDI_testUtilities.hpp>


static MCGIDI::URR_protareInfos URR_protare_infos;

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
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    // Default values
    long numberOfSamples = 100 * 1000 * 1000;
    std::string mapFilename( "../upscatterModelB/Data/upscatterModelB.map" );
    mapFilename = "/usr/gapps/data/nuclear/development/GNDS_2.0/ENDL2009/ENDL2009.5-direct-rc5.1/all.map";
    std::string neutronID( PoPI::IDs::neutron );

    // Command line argument parsing
    argvOptions argv_options( __FILE__, "Upscatter model A sample reactions test program" );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );
    parseTestOptions.m_askOid = true;

    parseTestOptions.m_askGNDS_File = true;

    argv_options.add( argvOption( "--map", true, "Map file path" ) );
    argv_options.add( argvOption( "--pops", true, "PoPs XML file path" ) );
    argv_options.add( argvOption( "--timeSteps", true, "Number of time steps (default: 5001)" ) );
    argv_options.add( argvOption( "--initialEnergy", true, "Initial energy in MeV (default: 0.1)" ) );
    argv_options.add( argvOption( "--temperatureMeV", true, "Material temperature in MeV (default: 0.01)" ) );
    argv_options.add( argvOption( "--multiGroup", false, "If present, use multi-group cross section lookup mode instead of continuous energy." ) );
    argv_options.add( argvOption( "--userGrid", false, "If present, use log-spaced group boundaries for model A upscatter (616 groups)." ) );

    parseTestOptions.parse( );

    // Apply parsed options
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "Li6" );
    std::string mapFile = argv_options.find( "--map" )->zeroOrOneOption( argv, mapFilename );
    std::string popsFile = argv_options.find( "--pops" )->zeroOrOneOption( argv, "../../../TestData/PoPs/pops.xml" );
    int nTimeSteps = argv_options.find( "--timeSteps" )->asInt( argv, 5001 );
    double initialEnergy = argv_options.find( "--initialEnergy" )->asDouble( argv, 1e-1 );
    double temperature_MeV = argv_options.find( "--temperatureMeV" )->asDouble( argv, 1e-2 );
    bool multiGroup = argv_options.find( "--multiGroup" )->present();

    PoPI::Database pops( popsFile );
    GIDI::Map::Map map( mapFile, pops );
    int neutronIndex( pops[neutronID] );

    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    unsigned long long rngState = 1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Protare *protare = map.protare( construction, pops, neutronID, targetID );
    std::cout << protare->realFileName( ) << std::endl;
    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    std::string label( temperatures[0].griddedCrossSection( ) );
    if( multiGroup ) {
        label = temperatures[0].heatedMultiGroup( );
    }
    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setUpscatterModelA( );

    if( argv_options.find( "--userGrid" )->present() ) {
        std::vector<double> energy_grid;
        const int num_energies = 616;
        const double start_energy = 1e-11;
        const double end_energy = 20.0;
        energy_grid.reserve(num_energies);
        double log_energy = log10(start_energy);
        double log_energy_step = (log10(end_energy) - log_energy) / (num_energies - 1);
        for (int i = 0; i < num_energies; ++i) {
            energy_grid.push_back(std::pow(10,log_energy + i * log_energy_step));
        }
        MC.setUpscatterModelAGroupBoundaries( energy_grid );
    }

    if( multiGroup ) {
        MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );
    }

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );
    if( multiGroup ) {
        particles.process( *protare, label );
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::MultiGroupHash multiGroupHash( *protare, particles );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    URR_protare_infos.setup( protares );

    PoPI::Base const &target = pops.get<PoPI::Base>( targetID );
    std::string fileNamePrefix = "relax." + target.ID( );
    std::string Str = LUPI::Misc::argumentsToString( "relax.%s.dat", target.ID( ).c_str( ) );
    FILE *fOut;
    if( ( fOut = fopen( Str.c_str( ), "w" ) ) == nullptr ) throw "error opening output file";

    MCGIDI::Sampling::StdVectorProductHandler products;
    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::A );

    int numberOfReactions = (int) MCProtare->numberOfReactions( );
    std::vector<long> counts( numberOfReactions + 1, 0 );

    std::cout << "List of reactions:" << std::endl;
    for( int reactionIndex = 0; reactionIndex < numberOfReactions; ++reactionIndex ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

        std::cout << LUPI::Misc::argumentsToString( "    %5d %-32s %g", reactionIndex, reaction->label( ).c_str( ), 
                MCProtare->threshold( reactionIndex ) ) << std::endl;
    }
    std::cout << std::endl;

    double energy = 1.72099;
    int hashIndex = domainHash.index( energy );
    if( multiGroup ) {
        hashIndex = multiGroupHash.index( energy );
    }
    input.setTemperatureAndEnergy( temperature_MeV, energy );
    double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature_MeV, energy );
    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
        int reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSection, 
                [&]() -> double { return float64RNG64( &rngState ); } );
        if( reactionIndex > numberOfReactions ) reactionIndex = numberOfReactions;
        ++counts[reactionIndex];
    }

    std::vector<double> reactionCrossSections( numberOfReactions );
    std::cout << "      ";
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature_MeV, energy );
        std::cout << LUPI::Misc::argumentsToString( " %9.3e", reactionCrossSection );
    }
    std::cout << std::endl;

    std::cout << "      ";
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature_MeV, energy );
        std::cout << LUPI::Misc::argumentsToString( " %9.6f", reactionCrossSection / crossSection );
    }
    std::cout << std::endl;

    std::cout << "      ";
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        double ratio = counts[i1];
        std::cout << LUPI::Misc::argumentsToString( " %9.6f", ratio / numberOfSamples );
    }
    std::cout << std::endl;

    std::cout << "      ";
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        std::cout << LUPI::Misc::argumentsToString( " %9ld", counts[i1] );
    }
    std::cout << std::endl;

    delete protare;
    delete MCProtare;

    exit( EXIT_SUCCESS );
}
