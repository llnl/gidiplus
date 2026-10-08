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
#include <fstream>
#include <iomanip>
#include <set>
#include <omp.h>

#include <statusMessageReporting.h>

#include <MCGIDI.hpp>

#include <GIDI_testUtilities.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <bins.hpp>

static char const *description = "This is an OMP version of broomstick.cpp. "
    "For a protare, samples specified reaction (or all if specified reaction index is negative via option '-r') many times (see option '-n') "
    "at the specified projectile energy, and creates an energy and angular spectrum for the specified outgoing particle (see option '--oid'). " 
    "To see a list of reactions and their indices, make the value for the '-r' option larger (larger than the number of reaction for the protare)."
    "\n\n"
    "Upscatter input and models:\n"
    "\n"
    "   --upscatter | model \n"
    "      value    |\n"
    "   ================================\n"
    "          0    | none (default)\n"
    "          1    | B\n"
    "          2    | BSnLimits\n"
    "          3    | DBRC\n"
    "Anything else causes a throw to be executed.\n"
    "\n"
    "Example for a projectile with energy 1.2 (in file units):\n"
    "    broomstick --map path/to/all.map --tid O16 -r 13 1.2";

/*
* The value threadMemoryGap is additional space between each sub unit of hist2d on each thread as it may be 
* needed to prevent two threads from accessing memory that is too close together which, I am told, can just 
* speed issues. The actual gap needed is not know so the value below is a stab at something.
*/
#define threadMemoryGap 256
std::string double2String( double a_value );
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
    std::set<int> reactionsToExclude;
    GIDI::Transporting::Particles particles, particlesEmpty;
    LUPI::StatusMessageReporting smr1;

    std::map<std::string, std::string> particlesAndGIDs;

    particlesAndGIDs[PoPI::IDs::neutron] = "LLNL_gid_4";
    particlesAndGIDs["H1"] = "LLNL_gid_71";
    particlesAndGIDs["H2"] = "LLNL_gid_71";
    particlesAndGIDs["H3"] = "LLNL_gid_71";
    particlesAndGIDs["He3"] = "LLNL_gid_71";
    particlesAndGIDs["He4"] = "LLNL_gid_71";
    particlesAndGIDs[PoPI::IDs::photon] = "LLNL_gid_70";

    argvOptions argv_options( __FILE__, description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );
    parseTestOptions.m_askOid = true;

    parseTestOptions.m_askGNDS_File = true;

    argv_options.add( argvOption( "-n", true, "Number of samples. If value is negative, it is multiplied by a million (i.e., -1000000)." ) );
    argv_options.add( argvOption( "--numberOfEBins", true, "Number of outgoing energy bins. Default is 1000." ) );
    argv_options.add( argvOption( "--numberOfMuBins", true, "Number of mu bins. Default is 1000." ) );
    argv_options.add( argvOption( "--energyMin", true, "The minimum energy for binning. Default is 1e-11." ) );
    argv_options.add( argvOption( "--energyMax", true, "The maximum energy for binning. Default is 20." ) );
    argv_options.add( argvOption( "--temperature", true, "The temperature of the material in MeV/k. Default is 2.53e-8 MeV/k (293.6 K)." ) );
    argv_options.add( argvOption( "--upscatter", true, "The upscatter model to use. Default is none." ) );
    argv_options.add( argvOption( "-r", true, 
            "Specifies a reaction index to sample. If negative, all reactions are sampled using the protare's sampleReaction method." ) );
    argv_options.add( argvOption( "--hist2dPath", true,
            "If present, the outgoing energy/angle 2-d histogram is written to the specified file in csv format." ) );
    argv_options.add( argvOption( "--numberOfThreads", true, "The number of OpenMP threads to use." ) );

    parseTestOptions.parse( );

    if( argv_options.m_arguments.size( ) != 1 ) throw "Need projectile energy.";

    std::string hist2dPath = argv_options.find( "--hist2dPath" )->zeroOrOneOption(argv, "");

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "/usr/gapps/data/nuclear/common/PoPs/pops.xml", "../../../../GIDI/Test/Data/MG_MC/all_maps.map", construction, PoPI::IDs::neutron, "O16" );

    GIDI::Transporting::Settings incompleteParticlesSetting( protare->projectile( ).ID( ), GIDI::Transporting::DelayedNeutrons::on );
    std::set<std::string> incompleteParticles;
    protare->incompleteParticles( incompleteParticlesSetting, incompleteParticles );
    std::cout << "# List of incomplete particles:";
    for( auto iter = incompleteParticles.begin( ); iter != incompleteParticles.end( ); ++iter ) {
        std::cout << " " << *iter;
    }
    std::cout << std::endl;

    std::string productID = argv_options.find( "--oid" )->zeroOrOneOption( argv, protare->projectile( ).ID( ) );
    long numberOfSamples = argv_options.find( "-n" )->asLong( argv, -1 );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    long numberOfEBins = argv_options.find( "--numberOfEBins" )->asLong( argv, 1000 );
    long numberOfMuBins = argv_options.find( "--numberOfMuBins" )->asLong( argv, 1000 );

    int reactionIndex = static_cast<int>( argv_options.find( "-r" )->asLong( argv, 999999 ) );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setThrowOnError( false );
    MC.setSampleNonTransportingParticles( true );

    double temperature_MeV_k = argv_options.find( "--temperature" )->asDouble( argv, 2.53e-8 );

    MCGIDI::Sampling::Upscatter::Model upscatterModel = MCGIDI::Sampling::Upscatter::Model::none;
    int upscatter = argv_options.find( "--upscatter" )->asInt( argv, 0 );
    switch( upscatter ) {
    case 0:
        break;
    case 1:
        upscatterModel = MCGIDI::Sampling::Upscatter::Model::B;
        MC.setUpscatterModelB( );
        break;
    case 2:
        upscatterModel = MCGIDI::Sampling::Upscatter::Model::BSnLimits;
        MC.setUpscatterModelBSnLimits( );
        break;
    case 3:
        upscatterModel = MCGIDI::Sampling::Upscatter::Model::DBRC;
        MC.setUpscatterModelDBRC( );
        break;
    default:
        throw "Invalid upscatter option.";
    }
    for( std::map<std::string, std::string>::iterator iter = particlesAndGIDs.begin( ); iter != particlesAndGIDs.end( ); ++iter ) {
        GIDI::Transporting::Particle particle( iter->first );
        particles.add( particle );
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particlesEmpty, domainHash, temperatures, reactionsToExclude );

    double energy_in = argv_options.asDouble( argv, 0 );
    int hashIndex = domainHash.index( energy_in );
    MCGIDI::URR_protareInfos URR_protareInfos;

    std::cout << std::endl;
    std::cout << "# List of reaction:" << std::endl;
    std::cout << "# index       threshold  cross section   label" << std::endl;
    std::cout << "# -----------------------------------------------------------" << std::endl;
    for( int reactionIndex2 = 0; reactionIndex2 < static_cast<int>( MCProtare->numberOfReactions( ) ); ++reactionIndex2 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex2 );

        std::cout << "# " << std::setw( 5 ) << reactionIndex2 << "  " << doubleToString( "%14.6e", reaction->crossSectionThreshold( ) ) 
                << doubleToString( " %14.6e ", reaction->crossSection( URR_protareInfos, hashIndex, temperature_MeV_k, energy_in ) ) 
                << "  " << reaction->label( ).c_str( ) << std::endl;
    }

    if( reactionIndex > static_cast<int>( MCProtare->numberOfReactions( ) ) ) {
        delete protare;
        delete MCProtare;
        exit( EXIT_SUCCESS );
    }

    int oidIndex = -1;
    int maxProductIndex = 0;
    std::cout << std::endl;
    for( auto particleIter = particles.particles( ).begin( ); particleIter != particles.particles( ).end( );  ++particleIter, ++maxProductIndex ) {
        MCProtare->setUserParticleIndex( pops[(*particleIter).first], maxProductIndex );
        if( (*particleIter).first == productID ) oidIndex = maxProductIndex;
        std::cout << "# particle ID/user defined index " << (*particleIter).first << " " << maxProductIndex << std::endl;
    }
    if (oidIndex == -1)
    {
        std::cout << "\nError: could not find outgoing particle with ID '" << productID << "'!" << std::endl;
        exit( EXIT_FAILURE );
    }

    std::cout << std::endl;
    std::cout << "# path is " << protare->realFileName( ) << std::endl;
    std::cout << "# projectile is " << MCProtare->projectileID( ).c_str( ) << std::endl;
    std::cout << "# target is " << MCProtare->targetID( ).c_str( ) << std::endl;
    std::cout << "# product is " << productID << std::endl;
    std::cout << "# projectile energy is " << energy_in << " MeV" << std::endl;
    if( reactionIndex >= 0 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
        std::cout << "# Reaction Info:" << std::endl;
        std::cout << "#     index: " << reactionIndex << std::endl;
        std::cout << "#     label: " << reaction->label( ).c_str( ) << std::endl;
        std::cout << "#     threshold: " << doubleToString( "%14.6e MeV", reaction->crossSectionThreshold( ) ) << std::endl;
    }

    int numberOfThreads = argv_options.find( "--numberOfThreads" )->asInt( argv, omp_get_num_threads( ) );
    if( numberOfThreads < 1 ) numberOfThreads = omp_get_max_threads( );
    if( numberOfThreads > omp_get_max_threads( ) ) numberOfThreads = omp_get_num_threads( );
    omp_set_num_threads( numberOfThreads );
    std::cout << "# number of OpenMP threads is " << numberOfThreads << std::endl;

    std::vector<long> numberOfDBRC_rejectionsVector( numberOfThreads, 0 );
    std::vector<long> numberOfDBRC_samplesVector( numberOfThreads, 0 );

    std::vector<unsigned long long> rngStates( 1024 * numberOfThreads );
    unsigned long long rngSeed = 1;
    for( auto iter = rngStates.begin( ); iter != rngStates.end( ); ++iter, ++rngSeed ) {
        *iter = rngSeed;
    }

    double energyMin = argv_options.find( "--energyMin" )->asDouble( argv, 1e-11 );
    double energyMax = argv_options.find( "--energyMax" )->asDouble( argv, 20.0 );
    std::vector<Bins> energyBinsVector( numberOfThreads, Bins( numberOfEBins, energyMin, energyMax, true ) );
    std::vector<Bins> muBinsVector( numberOfThreads, Bins( numberOfMuBins, -1.0, 1.0 ) );

    long hist2dThreadStep = numberOfEBins * numberOfMuBins + threadMemoryGap;
    std::vector<long> hist2d;
    if( argv_options.find( "--hist2dPath" )->present() ) {
        hist2d.resize( numberOfThreads * hist2dThreadStep, 0 );
    }

    double totalCrossSection = 0.0;
    if( reactionIndex < 0 ) {
        totalCrossSection = MCProtare->crossSection( URR_protareInfos, hashIndex, temperature_MeV_k, energy_in, true ); }
    else {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
        if( reaction->crossSectionThreshold( ) > energy_in ) {
            delete protare;
            delete MCProtare;

            exit( EXIT_SUCCESS );
        }
    }

    std::vector<MCGIDI::Sampling::Input> inputVector( numberOfThreads, MCGIDI::Sampling::Input( false, upscatterModel ) );
    for( auto iter = inputVector.begin( ); iter != inputVector.end( ); ++iter ) {
        (*iter).setTemperatureAndEnergy( temperature_MeV_k, energy_in );
    }

    int eBinIndex, muBinIndex, reactionIndex2;
    std::size_t productIndex;
    MCGIDI::Reaction const *reaction;
    double mu, speed;

    std::vector<MCGIDI::Sampling::StdVectorProductHandler> productsVector( numberOfThreads );
#pragma omp parallel for private( reactionIndex2, reaction, productIndex, eBinIndex, speed, mu, muBinIndex )
    for( long sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex ) {
        int threadId = omp_get_thread_num( );
        reactionIndex2 = reactionIndex;
        if( reactionIndex2 < 0 ) reactionIndex2 = MCProtare->sampleReaction( inputVector[threadId], URR_protareInfos, hashIndex, 
                totalCrossSection, [&]( ) -> double { return float64RNG64( &(rngStates[1024 * threadId]) ); } );
        reaction = MCProtare->reaction( reactionIndex2 );

        productsVector[threadId].clear( );
        reaction->sampleProducts( MCProtare, inputVector[threadId], [&]( ) -> double { return float64RNG64( &(rngStates[1024 * threadId]) ); }, 
                [&]( MCGIDI::Sampling::Product &a_product ) -> void { productsVector[threadId].push_back( a_product ); }, productsVector[threadId] );
        for( productIndex = 0; productIndex < productsVector[threadId].size( ); ++productIndex ) {
            MCGIDI::Sampling::Product const &product = productsVector[threadId][productIndex];
            if( product.m_userProductIndex != oidIndex ) continue;

            if( product.m_numberOfDBRC_rejections >= 0 ) {
                numberOfDBRC_rejectionsVector[threadId] += product.m_numberOfDBRC_rejections;
                ++numberOfDBRC_samplesVector[threadId];
            }

            eBinIndex = energyBinsVector[threadId].accrue( product.m_kineticEnergy, 1.0 );
            speed = sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz );
            mu = 0.0;
            if( speed != 0.0 ) mu = product.m_pz_vz / speed;
            muBinIndex = muBinsVector[threadId].accrue( mu, 1.0 );

            if( hist2d.size( ) > 0 ) {
                if( ( 0 <= eBinIndex ) && ( eBinIndex <= numberOfEBins ) && ( 0 <= muBinIndex ) && ( muBinIndex <= numberOfMuBins ) ) {
                    long hist2dIndex = threadId * hist2dThreadStep + eBinIndex * numberOfMuBins + muBinIndex;
                    hist2d[hist2dIndex] += 1;
                }
            }
        }
    }

    long numberOfDBRC_samples = 0;
    for( auto iter = numberOfDBRC_samplesVector.begin( ); iter != numberOfDBRC_samplesVector.end( ); ++iter )
        numberOfDBRC_samples += *iter;
    if( numberOfDBRC_samples > 0 ) {
        long numberOfDBRC_rejections = 0;
        for( auto iter = numberOfDBRC_rejectionsVector.begin( ); iter != numberOfDBRC_rejectionsVector.end( ); ++iter )
            numberOfDBRC_rejections += *iter;
        double averageDBRC = numberOfDBRC_rejections / (double) numberOfDBRC_samples;

        std::cout << "# Number of DBRC samples = " << numberOfDBRC_samples << std::endl;
        std::cout << "# Total number of DBRC rejections = " << numberOfDBRC_rejections << std::endl;
        std::cout << "# Average number of DBRC rejections per sample = " << LUPI::Misc::doubleToString3( "%.3e", averageDBRC ) << std::endl;
    }

    std::string header = "# energy spectrum P(E') for " + productID + ":";
    Bins energyBins( numberOfEBins, energyMin, energyMax, true );
    for( auto iter = energyBinsVector.begin( ); iter != energyBinsVector.end( ); ++iter ) energyBins.merge( *iter );
    energyBins.print( stdout, header.c_str( ), true );

    header = "# angular spectrum P(mu) for " + productID + ":";
    Bins muBins( numberOfMuBins, -1.0, 1.0 );
    for( auto iter = muBinsVector.begin( ); iter != muBinsVector.end( ); ++iter ) muBins.merge( *iter );
    muBins.print( stdout, header.c_str( ), true );

    if( hist2d.size( ) > 0 ) {
        std::vector<long> hist2dSum( numberOfEBins * numberOfMuBins, 0 );
        for( int threadIndex = 0; threadIndex < numberOfThreads; ++threadIndex ) {
            for( eBinIndex = 0; eBinIndex < numberOfEBins; ++eBinIndex ) {
                for( muBinIndex = 0; muBinIndex < numberOfMuBins; ++muBinIndex ) {
                    int index = eBinIndex * numberOfMuBins + muBinIndex;
                    hist2dSum[index] += hist2d[threadIndex * hist2dThreadStep + index];
                }
            }
        }

        std::ofstream fout;
        fout.open( hist2dPath );
        fout << "# Projectile: " << protare->projectile().ID() << std::endl;
        fout << "# Target: " << protare->target().ID() << std::endl;
        fout << "# Projectile energy: " << energy_in << " MeV, product: " << productID << std::endl;

        std::vector<double> edges = energyBins.edges();
        fout << "# Energy bin boundaries (MeV): " << edges[0];
        for( std::size_t i1 = 1; i1 < edges.size( ); ++i1 )
            fout << "," << edges[i1];
        fout << std::endl;

        edges = muBins.edges();
        fout << "# Angle bin boundaries mu: " << edges[0];
        for( std::size_t i1 = 1; i1 < edges.size( ); ++i1 )
            fout << "," << edges[i1];
        fout << std::endl;

        fout << "# E |" << std::endl;
        fout << "#   V" << std::endl;
        fout << "# mu ->" << std::endl;
        for( long i1 = 0; i1 < numberOfEBins; ++i1 ) {
            char sep[2] = "";
            for( long i2 = 0; i2 < numberOfMuBins; ++i2 ) {
                fout << sep << hist2dSum[i1 * numberOfMuBins + i2];
                sep[0] = ',';
            }
            fout << std::endl;
        }

        fout.close();
    }

    delete protare;
    delete MCProtare;

    exit( EXIT_SUCCESS );
}

/*
=========================================================
*/
std::string double2String( double a_value ) {

    return( LUPI::Misc::doubleToString3( "%15.8e", a_value ) );
}
