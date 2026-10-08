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
#include <string>
#include <iostream>
#include <sstream>
#include <set>
#include <exception>
#include <stdexcept>
#include <cmath>

#include <GIDI.hpp>
#include <GIDI_testUtilities.hpp>

static char const *description = 
    "In addition to a map file, one or more PoPs files must be specified.\n";

static GIDI::Construction::Settings *constructionPtr = nullptr;
static double rtol = 1e-10;
static int errCount = 0;
static GIDI::Transporting::Mode MG_mode = GIDI::Transporting::Mode::multiGroup;
static GIDI::Transporting::DelayedNeutrons delayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
static bool skipMatrices = false;
// static char const *LLNL_transportables[] = { "n", "H1", "H2", "H3", "He3", "He4", "photon" };
// static int numberOfLLNL_transportables = sizeof( LLNL_transportables ) / sizeof( LLNL_transportables[0] );

void main2( int argc, char **argv );
void readProtare( GIDI::Map::ProtareBase const &a_protareEntry, PoPI::Database const &a_pops, std::stringstream &a_stringstream );
int checkVectors( std::stringstream &a_stringstream, std::string const &a_function, std::string const &a_pid, GIDI::Styles::TemperatureInfo const &temperature, 
        GIDI::Vector const &v1, GIDI::Vector const &v2 );
int checkMatrices( std::stringstream &a_stringstream, std::string const &a_function, std::string const &a_pid, GIDI::Styles::TemperatureInfo const &temperature, 
        GIDI::Matrix const &m1, GIDI::Matrix const &m2 );
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

    if( errCount > 0 ) exit( EXIT_FAILURE );
    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    PoPI::Database pops;
    LUPI::ArgumentParser argumentParser( __FILE__, description );

    LUPI::Positional *o_mapFile = argumentParser.add<LUPI::Positional>( "mapFile", "Path to map file." );
    LUPI::Positional *o_popsFiles = argumentParser.add<LUPI::Positional>( "popsFiles", "Path to pops files.", 1, -1 );
    LUPI::OptionTrue *o_lazyParsing = argumentParser.add<LUPI::OptionTrue>( "--lazyParsing", "If present, does lazy parsing for all read protares." );
    LUPI::OptionTrue *o_upscatter = argumentParser.add<LUPI::OptionTrue>( "--upscatter", "If present, multi-group upscatter data are read." );
    LUPI::OptionTrue *o_skipMatrices = argumentParser.add<LUPI::OptionTrue>( "--skipMatrices", "If present, multiGroupProductMatrix and multiGroupFissionMatrix are not called." );
    LUPI::OptionTrue *o_delayedNeutrons = argumentParser.add<LUPI::OptionTrue>( "--delayedNeutrons", "If present, fission delayed neutron data are included." );
    std::string rtolString( " Default is " + LUPI::Misc::doubleToShortestString( rtol ) + "." );
    LUPI::OptionStore *o_rtol = argumentParser.add<LUPI::OptionStore>( "--rtol", "The tolerance used for relative differences." + rtolString );

    argumentParser.parse( argc, argv );

    if( o_upscatter->counts( ) > 0 ) MG_mode = GIDI::Transporting::Mode::multiGroupWithSnElasticUpScatter;
    skipMatrices = o_skipMatrices->counts( ) > 0;
    if( o_delayedNeutrons->counts( ) > 0 ) delayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;
    if( o_rtol->counts( ) > 0 ) rtol = std::min( 1e-3, std::max( 1e-15, std::stod( o_rtol->value( ) ) ) );

    GIDI::Construction::ParseMode parseMode( GIDI::Construction::ParseMode::all );
    GIDI::Construction::Settings construction( parseMode, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    construction.setLazyParsing( o_lazyParsing->counts( )  > 0 );
    construction.setUseSystem_strtod( true );
    constructionPtr = &construction;

    std::string const &mapFilename( o_mapFile->value( 0 ) );

    for( std::size_t index = 0; index < o_popsFiles->counts( ); ++index ) {
        pops.addFile( o_popsFiles->value( index ), false );
    }

    GIDI::Map::Map map( mapFilename, pops );

    GIDI::Map::FindProtareEntries findProtareEntries;
    map.findProtareEntries( findProtareEntries, std::regex( ".*" ), std::regex( ".*" ) );
    std::size_t numberOfProtare = findProtareEntries.size( );
    std::vector<std::stringstream> stringstreams( numberOfProtare );

    for( std::size_t counter = 0; counter < numberOfProtare; ++counter ) {
        readProtare( *findProtareEntries[counter], pops, stringstreams[counter] );
        std::cout << stringstreams[counter].str( );
    }
}

/*
=========================================================
*/
void readProtare( GIDI::Map::ProtareBase const &a_protareEntry, PoPI::Database const &a_pops, std::stringstream &a_stringstream ) {

    std::string throwMessage;
    GIDI::ProtareSingle *protare = nullptr;
    GIDI::ParticleSubstitution particleSubstitution;
    std::vector<std::string> libraries;
    LUPI::StatusMessageReporting smr;

    try {
        a_stringstream << "    " << a_protareEntry.path( ) << std::endl;

        protare = a_protareEntry.protareSingle( *constructionPtr, a_pops, particleSubstitution );
        if( protare->multiGroupSummedReaction( ) == nullptr ) {
            a_stringstream << "          no multigroup sum data present." << std::endl; }
        else {
            std::size_t maxLegendreOrder = 1;                       // Need to get from protare.
            std::set<std::string> pids;
            GIDI::Transporting::Particles particles;
            protare->productIDs( pids, particles, false );

            GIDI::Transporting::MG settings1( protare->projectile( ).ID( ), MG_mode, delayedNeutrons );
            GIDI::Transporting::MG settings2( protare->projectile( ).ID( ), MG_mode, delayedNeutrons );
            settings2.setUseMultiGroupSummedData( false );

            if( delayedNeutrons == GIDI::Transporting::DelayedNeutrons::on ) {
                if( !protare->isDelayedFissionNeutronComplete( ) ) {
                    a_stringstream << "        WARNING: delayed neutron fission data are incomplete and are not included." << std::endl;
                    settings1.setDelayedNeutrons( GIDI::Transporting::DelayedNeutrons::off );
                    settings2.setDelayedNeutrons( GIDI::Transporting::DelayedNeutrons::off );
                }
            }

            GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

            std::vector<std::string> transportables;
            transportables.push_back( protare->projectile( ).ID( ) );
            for( auto iterTransportable = transportables.begin( ); iterTransportable != transportables.end( ); ++iterTransportable ) {
                GIDI::Transporting::Particle particle( *iterTransportable );
                particles.add( particle );
            }

/*
Still need to add multiGroupTransportCorrection call.
*/

            for( auto iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
                checkVectors( a_stringstream, "multiGroupCrossSection", "", *iter, protare->multiGroupCrossSection( smr, settings1, *iter ), 
                                                                   protare->multiGroupCrossSection( smr, settings2, *iter ) );
                checkVectors( a_stringstream, "multiGroupQ", "", *iter, protare->multiGroupQ( smr, settings1, *iter, true ),
                                                        protare->multiGroupQ( smr, settings2, *iter, true ) );
                checkVectors( a_stringstream, "multiGroupAvailableEnergy", "", *iter, protare->multiGroupAvailableEnergy( smr, settings1, *iter ), 
                                                                      protare->multiGroupAvailableEnergy( smr, settings2, *iter ) );
                checkVectors( a_stringstream, "multiGroupDepositionEnergy", "", *iter, protare->multiGroupDepositionEnergy( smr, settings1, *iter, particles ), 
                                                                       protare->multiGroupDepositionEnergy( smr, settings2, *iter, particles ) );
                checkVectors( a_stringstream, "multiGroupAvailableMomentum", "", *iter, protare->multiGroupAvailableMomentum( smr, settings1, *iter ), 
                                                                        protare->multiGroupAvailableMomentum( smr, settings2, *iter ) );
                checkVectors( a_stringstream, "multiGroupDepositionMomentum", "", *iter, protare->multiGroupDepositionMomentum( smr, settings1, *iter, particles ), 
                                                                         protare->multiGroupDepositionMomentum( smr, settings2, *iter, particles ) );

// This should be loop over all particles in LLNL_transportables but first need to handle transportables with unspecified multiplicity or distribution.
                for( auto iterTransportable = transportables.begin( ); iterTransportable != transportables.end( ); ++iterTransportable ) {
                    checkVectors( a_stringstream, "multiGroupMultiplicity", *iterTransportable, *iter, 
                            protare->multiGroupMultiplicity( smr, settings1, *iter, *iterTransportable ),
                            protare->multiGroupMultiplicity( smr, settings2, *iter, *iterTransportable ) );

                    checkVectors( a_stringstream, "multiGroupAverageEnergy", "", *iter, protare->multiGroupAverageEnergy( smr, settings1, *iter, *iterTransportable ), 
                                                                        protare->multiGroupAverageEnergy( smr, settings2, *iter, *iterTransportable ) );
                    checkVectors( a_stringstream, "multiGroupAverageMomentum", "", *iter, protare->multiGroupAverageMomentum( smr, settings1, *iter, *iterTransportable ), 
                                                                          protare->multiGroupAverageMomentum( smr, settings2, *iter, *iterTransportable ) );
                    checkVectors( a_stringstream, "multiGroupGain", "", *iter, protare->multiGroupGain( smr, settings1, *iter, *iterTransportable ), 
                                                               protare->multiGroupGain( smr, settings2, *iter, *iterTransportable ) );
// This should be loop over max Legendre order.
                    for( std::size_t lOrder = 0; lOrder < maxLegendreOrder; ++lOrder ) {
                        if( skipMatrices ) break;
                        checkMatrices( a_stringstream, "multiGroupProductMatrix", *iterTransportable, *iter,
                                protare->multiGroupProductMatrix( smr, settings1, *iter, particles, *iterTransportable, lOrder ),
                                protare->multiGroupProductMatrix( smr, settings2, *iter, particles, *iterTransportable, lOrder ) );
                    }
                }

                checkVectors( a_stringstream, "multiGroupFissionNeutronMultiplicity", "", *iter, protare->multiGroupFissionNeutronMultiplicity( smr, settings1, *iter ),
                                                                                 protare->multiGroupFissionNeutronMultiplicity( smr, settings2, *iter ) );
                checkVectors( a_stringstream, "multiGroupFissionGammaMultiplicity", "", *iter, protare->multiGroupFissionGammaMultiplicity( smr, settings1, *iter ), 
                                                                               protare->multiGroupFissionGammaMultiplicity( smr, settings2, *iter ) );

                for( std::size_t lOrder = 0; lOrder < maxLegendreOrder; ++lOrder ) {
                    checkMatrices( a_stringstream, "multiGroupFissionMatrix", "", *iter,
                            protare->multiGroupFissionMatrix( smr, settings1, *iter, particles, lOrder ),
                            protare->multiGroupFissionMatrix( smr, settings2, *iter, particles, lOrder ) );
                }
            }
        } }
    catch (char const *str) {
        throwMessage = str; }
    catch (std::string str) {
        throwMessage = str; }
    catch (std::exception &exception) {
        throwMessage = exception.what( );
    }

    if( throwMessage != "" ) {
        ++errCount;
        a_stringstream << "            ERROR: throw with message '" << throwMessage << "'" << std::endl;
    }

    delete protare;
}

/*
=========================================================
*/
int checkVectors( std::stringstream &a_stringstream, std::string const &a_function, std::string const &a_pid, GIDI::Styles::TemperatureInfo const &temperature, 
                GIDI::Vector const &v1, GIDI::Vector const &v2 ) {

    int errors = 0;
    std::string function( a_function );

    if( a_pid != "" ) function += ": '" + a_pid + "'";

    if( v1.size( ) != v2.size( ) ) {
        a_stringstream << "        For " << function << " vectors have different sizes: " << std::to_string( v1.size( ) ) << " vs. " << std::to_string( v1.size( ) ) << std::endl; }
    else { 
        std::size_t maxIndex = 0;
        double maxRelDiff = 0.0;
        double maxDiff = 0.0;
        std::vector<std::size_t> indices;
        std::stringstream stringstreamIndex;

        for( std::size_t index = 0; index < v1.size( ); ++index ) {
            double diff = v2[index] - v1[index];
            double max = std::max( std::fabs( v1[index] ), std::fabs( v2[index] ) );

            if( std::fabs( diff ) > rtol * max ) {
                indices.push_back( index );
                stringstreamIndex << " " << index;
                ++errors;

                double relDiff = diff / max;                            // This prior if test should guarantee that max is not 0.
                if( std::fabs( relDiff ) > std::fabs( maxRelDiff ) ) {
                    maxIndex = index;
                    maxRelDiff = relDiff;
                    maxDiff = diff;
                }
            }
        }
        if( indices.size( ) != 0 ) {
                a_stringstream << "        " << function << " at temperature " << temperature.temperature( ) << ": worst case " 
                                << v1[maxIndex] << " (pre-calc.) vs. " << v2[maxIndex] << " (summed) at index " << maxIndex << " with differnce " 
                                << maxDiff << ":" << stringstreamIndex.str( ) << std::endl;
        }
    }

    return( errors );
}

/*
=========================================================
*/
int checkMatrices( std::stringstream &a_stringstream, std::string const &a_function, std::string const &a_pid, GIDI::Styles::TemperatureInfo const &temperature, 
                GIDI::Matrix const &m1, GIDI::Matrix const &m2 ) {

    int errors = 0;

    if( m1.size( ) != m2.size( ) ) {
        a_stringstream << "        For " << a_function << " matrices have different sizes: " << std::to_string( m1.size( ) ) << " vs. " << std::to_string( m1.size( ) ) << std::endl;
        ++errors; }
    else {
        for( std::size_t index = 0; index < m1.size( ); ++index ) {
            std::string message( a_function + ": maxtrix row " + std::to_string( index ) );
            errors += checkVectors( a_stringstream, message, a_pid, temperature, m1[index], m2[index] );
        }
    }

    return( errors );
}
