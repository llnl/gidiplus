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
#include <set>

#include "MCGIDI.hpp"

#include "MCGIDI_testUtilities.hpp"
#include <bins.hpp>

static char const *description = "Reads in a protare and if it has URR probability tables, samples from the tables. \n"
                                 "Besides options, protare's file name followed by one or more pops files must be specified\n"
                                 "(e.g., sampleURR_probabilityTables n+U235.xml pops.xml).";

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

    exit( EXIT_FAILURE );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {                        // Useful for detecting memory leaks with valgrind.

    PoPI::Database pops;
    GIDI::Transporting::Particles particles;
    std::vector<std::string> libraries;
    char *endChar;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    unsigned long long rngState = 1;
    long numberOfDomainSteps = 10;

    argvOptions2 argv_options( "sampleURR_probabilityTables", description );

    argv_options.add( argvOption2( "-e", true, "If present, only the specified energy is sampled (e.g., to run on 10 keV specify -e 0.01)." ) );
    argv_options.add( argvOption2( "-n", true, "Number of 1000 samples for each reaction for each energy (e.g., -n 20 means 20000 samples)." ) );
    argv_options.add( argvOption2( "-r", true, "If present, only the specified reaction is sampled. Allowed values are c, e, f, t or a for capture, elastic, fission, total or all, respectively (e.g., -r t )." ) );
    argv_options.add( argvOption2( "--temperature", true, "The temperature of the target material." ) );

    argv_options.parseArgv( argc, argv );

    if( argv_options.m_arguments.size( ) < 2 ) {
        std::cerr << std::endl << "----- Need protare's file name and at least one pops file -----" << std::endl << std::endl;
        argv_options.help( );
    }

    for( std::size_t i1 = 1; i1 < argv_options.m_arguments.size( ); ++i1 ) pops.addFile( argv[argv_options.m_arguments[i1]], true );

    std::string dummy = argv_options.find( "-n" )->zeroOrOneOption( argv, "1000" );
    long numberOfSamples = 1000 * strtol( dummy.c_str( ), &endChar, 10 );

    std::string temperatureString = argv_options.find( "--temperature" )->zeroOrOneOption( argv, "2.58522e-8" );
    double temperature = strtod( temperatureString.c_str( ), &endChar );
    if( *endChar != 0 ) throw "Invalid temperature input.";

    std::string fileName = argv[argv_options.m_arguments[0]];
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearOnly );
    GIDI::ProtareSingle *GIDI_protare = new GIDI::ProtareSingle( construction, fileName, GIDI::FileType::XML, pops, libraries );
    GIDI::Styles::TemperatureInfos temperatures = GIDI_protare->temperatures( );

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, GIDI_protare->projectile( ).ID( ), &GIDI_protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.want_URR_probabilityTables( true );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    LUPI::StatusMessageReporting smr1;
    MCGIDI::Protare *MC_protare = MCGIDI::protareFromGIDIProtare( smr1, *GIDI_protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MC_protare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    std::cout << "# " << GIDI_protare->realFileName( ) << std::endl;
    std::cout << "# Has URR probability tables = " << MC_protare->hasURR_probabilityTables( ) << std::endl;
    if( MC_protare->hasURR_probabilityTables( ) ) {
        std::size_t numberOfReactions = MC_protare->numberOfReactions( );
        double domain_min( MC_protare->URR_domainMin( ) );
        double domain_max( MC_protare->URR_domainMax( ) );
        Bins cross_section_bins( 10000, 10.0, 15.0, true );

        std::cout << "# temperature = " << temperature << std::endl;
        std::cout << "# numberOfSamples = " << numberOfSamples << std::endl;
        std::cout << "# URR domain minimum = " << domain_min << std::endl;
        std::cout << "# URR domain maximum = " << domain_max << std::endl;
        std::cout << std::endl << std::endl;

        std::string reactionType = argv_options.find( "-r" )->zeroOrOneOption( argv, "a" );
        if( argv_options.find( "-e" )->present( ) ) {
            numberOfDomainSteps = 0;
            domain_min = argv_options.find( "-e" )->asDouble( argv, domain_min );
        }
        for( int energyIndex = 0; energyIndex <= numberOfDomainSteps; ++energyIndex ) {
            double energy = domain_min;
            if( numberOfDomainSteps != 0 ) {
                energy = domain_min + energyIndex * ( domain_max - domain_min ) / numberOfDomainSteps;
                if( energyIndex == numberOfDomainSteps ) energy = domain_max;
            }
            std::size_t hashIndex = domainHash.index( energy );

            std::cout << "# energy = " << energy << std::endl;
            std::string fileName( "sampleURR_probabilityTables_" );
            fileName += longToString( "%.3d", energyIndex );
            fileName += ".out";

            FILE *fOut = fopen( fileName.c_str( ), "w" );
            fprintf( fOut, "# Projectile energy = %.5e\n", energy );

            double cross_section_min = 0.0;
            double cross_section_max = 0.0;
            double cross_section_mean = 0.0;

            if( ( reactionType == "a" ) || ( reactionType == "t" ) ) {
                std::cout << "#    total" << std::endl;
                cross_section_bins.clear( );

                for( long sampleIndex = 0; sampleIndex < 1000; ++sampleIndex ) {
                    URR_protare_infos.updateProtare( MC_protare, energy, [&]() -> double { return float64RNG64( &rngState ); } );
                    double cross_section = MC_protare->crossSection( URR_protare_infos, hashIndex, 0.0, energy );

                    if( sampleIndex == 0 ) cross_section_min = cross_section;
                    if( cross_section < cross_section_min ) cross_section_min = cross_section;
                    if( cross_section > cross_section_max ) cross_section_max = cross_section;
                }
                cross_section_bins.setDomain( 0.5 * cross_section_min, 2.0 * cross_section_max );

                for( long sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex ) {
                    URR_protare_infos.updateProtare( MC_protare, energy, [&]() -> double { return float64RNG64( &rngState ); } );

                    double cross_section = MC_protare->crossSection( URR_protare_infos, hashIndex, 0.0, energy );
                    cross_section_bins.accrue( cross_section );
                    cross_section_mean += cross_section;
                }

                std::string label( "#    total" );
                cross_section_bins.print( fOut, "#    total" );
                fprintf( fOut, "#    mean cross section = %.7e (%.7e)\n", cross_section_bins.meanX( ), cross_section_mean / numberOfSamples );
            }

            if( reactionType != "t" ) {
                for( std::size_t reaction_index = 0; reaction_index < numberOfReactions; ++reaction_index ) {
                    if( !MC_protare->reactionHasURR_probabilityTables( reaction_index ) ) continue;

                    MCGIDI::Reaction const *reaction = MC_protare->reaction( reaction_index );

                    if( reactionType == "e" ) {
                        if( reaction->ENDF_MT( ) != 2 ) continue; }
                    else if( reactionType == "c" ) {
                        if( reaction->ENDF_MT( ) != 102 ) continue; }
                    else if( reactionType == "f" ) {
                        if( reaction->ENDF_MT( ) != 18 ) continue;
                    }

                    std::cout << "#    " << reaction->label( ).c_str( ) << std::endl;
                    cross_section_bins.clear( );

                    cross_section_min = 0.0;
                    cross_section_max = 0.0;
                    cross_section_mean = 0.0;
                    for( long sampleIndex = 0; sampleIndex < 1000; ++sampleIndex ) {
                        URR_protare_infos.updateProtare( MC_protare, energy, [&]() -> double { return myRNG(&seed); } );
                        double cross_section = MC_protare->reactionCrossSection( reaction_index, URR_protare_infos, hashIndex, 0.0, energy );

                        if( sampleIndex == 0 ) cross_section_min = cross_section;
                        if( cross_section < cross_section_min ) cross_section_min = cross_section;
                        if( cross_section > cross_section_max ) cross_section_max = cross_section;
                    }
                    cross_section_bins.setDomain( 0.5 * cross_section_min, 2.0 * cross_section_max );

                    for( long sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex ) {
                        URR_protare_infos.updateProtare( MC_protare, energy, [&]() -> double { return float64RNG64( &rngState ); } );

                        double cross_section = MC_protare->reactionCrossSection( reaction_index, URR_protare_infos, hashIndex, 0.0, energy );
                        cross_section_bins.accrue( cross_section );
                        cross_section_mean += cross_section;
                    }

                    std::string label( "#    " );
                    label += reaction->label( ).c_str( );
                    cross_section_bins.print( fOut, label.c_str( ) );
                    fprintf( fOut, "#    mean cross section = %.7e (%.7e)\n", cross_section_bins.meanX( ), cross_section_mean / numberOfSamples );
                }
            }

            fclose( fOut );
        }
    }

    delete GIDI_protare;

    delete MC_protare;
}
