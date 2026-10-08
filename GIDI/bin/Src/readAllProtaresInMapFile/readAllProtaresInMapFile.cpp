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
#include <set>
#include <exception>
#include <stdexcept>

#include "GIDI_testUtilities.hpp"

static char const *description = 
    "Reads evergy 'nth' protare (including TNSL) in the specified map file.\n"
    "In addition to a map file, one or more PoPs files must be specified.\n\n"
    "Example:\n"
    "    readAllProtaresInMapFile.cpp all.map pops.xml metaStable.xml\n\n"
    "The default for 'nth' is 1 and can be set with the '-n' options. Values for \nthe '-m' options are:\n\n"
    "        0 is GIDI::Construction::ParseMode::all,\n"
    "        1 is GIDI::Construction::ParseMode::multiGroupOnly,\n"
    "        2 is GIDI::Construction::ParseMode::MonteCarloContinuousEnergy,\n"
    "        3 is GIDI::Construction::ParseMode::excludeProductMatrices\n"
    "        4 is GIDI::Construction::ParseMode::readOnly.\n"
    "        5 is GIDI::Construction::ParseMode::outline.\n"
    "        6 is GIDI::Construction::ParseMode::noParsing.";

static int nth = 1;
static int countDown = 1;
static bool printLibraries = false;
static GIDI::Construction::Settings *constructionPtr = nullptr;
static int errCount = 0;
static bool printTiming = false;
static int maxDepth = 999999;

void main2( int argc, char **argv );
void walk( std::string const &a_indent, std::string const &mapFilename, PoPI::Database const &pops, int depth );
void readProtare( std::string const &a_indent, std::string const &protareFilename, PoPI::Database const &pops, std::vector<std::string> &a_libraries, bool a_targetRequiredInGlobalPoPs );
void printUsage( );
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
    argvOptions argv_options( "readAllProtaresInMapFile", description );

    argv_options.add( argvOption( "-f", false, "Use nf_strtod instead of the system strtod." ) );
    argv_options.add( argvOption( "-l", false, "Print libraries for each Protare." ) );
    argv_options.add( argvOption( "-m",  true, "Which GIDI::Construction::Settings flag to use." ) );
    argv_options.add( argvOption( "-n",  true, "If present, only every nth protare is read where 'n' is the next argument." ) );
    argv_options.add( argvOption( "-t", false, "Prints read time information for each protare read." ) );
    argv_options.add( argvOption( "--lazyParsing", false, "If true, does lazy parsing for all read protares." ) );
    argv_options.add( argvOption( "--maxDepth", true, "The maximum nesting depth that the map file will be descended (e.g., 0 will only do the top map file. Default is no maximum depth." ) );

    argv_options.parseArgv( argc, argv );

    if( argv_options.m_arguments.size( ) < 2 ) {
        std::cerr << std::endl << "----- Need map file name and at least one pops file -----" << std::endl << std::endl;
        argv_options.help( );
    }

    printLibraries = argv_options.find( "-l" )->m_counter > 0;
    printTiming = argv_options.find( "-t" )->m_counter > 0;
    maxDepth = argv_options.find( "--maxDepth" )->asInt( argv, maxDepth );

    int mode = argv_options.find( "-m" )->asInt( argv, 0 );
    GIDI::Construction::ParseMode parseMode( GIDI::Construction::ParseMode::all );
    if( mode == 1 ) {
        parseMode = GIDI::Construction::ParseMode::multiGroupOnly; }
    else if( mode == 2 ) {
        parseMode = GIDI::Construction::ParseMode::MonteCarloContinuousEnergy ; }
    else if( mode == 3 ) {
        parseMode = GIDI::Construction::ParseMode::excludeProductMatrices; }
    else if( mode == 4 ) {
        parseMode = GIDI::Construction::ParseMode::readOnly; }
    else if( mode == 5 ) {
        parseMode = GIDI::Construction::ParseMode::outline; }
    else if( mode == 6 ) {
        parseMode = GIDI::Construction::ParseMode::noParsing;
    }
    GIDI::Construction::Settings construction( parseMode, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    construction.setUseSystem_strtod( argv_options.find( "-f" )->m_counter > 0 );
    construction.setLazyParsing( argv_options.find( "--lazyParsing" )->m_counter > 0 );
    constructionPtr = &construction;

    nth = argv_options.find( "-n" )->asInt( argv, 1 );
    if( nth < 0 ) nth = 1;

    std::string const &mapFilename( argv[argv_options.m_arguments[0]] );

    for( std::size_t index = 1; index < argv_options.m_arguments.size( ); ++index ) pops.addFile( argv[argv_options.m_arguments[index]], false );

    LUPI::Timer timer;
    walk( "    ", mapFilename, pops, 0 );
    if( printTiming ) std::cout << std::endl << LUPI::Misc::doubleToString3( "Total CPU %7.3f s", timer.deltaTime( ).CPU_time( ) )
            << LUPI::Misc::doubleToString3( " wall %6.3f s", timer.deltaTime( ).wallTime( ) ) << std::endl;
}
/*
=========================================================
*/
void walk( std::string const &a_indent, std::string const &mapFilename, PoPI::Database const &pops, int depth ) {

    if( depth > maxDepth ) return;

    std::string const &indent2 = a_indent + "    ";

    std::cout << a_indent << mapFilename << std::endl;
    GIDI::Map::Map map( mapFilename, pops );

    for( std::size_t i1 = 0; i1 < map.size( ); ++i1 ) {
        GIDI::Map::BaseEntry const *entry = map[i1];

        std::string path = entry->path( GIDI::Map::BaseEntry::PathForm::cumulative );

        if( entry->name( ) == GIDI_importChars ) {
            walk( indent2, path, pops, depth + 1 ); }
        else if( ( entry->name( ) == GIDI_protareChars ) || ( entry->name( ) == GIDI_TNSLChars ) ) {
            std::vector<std::string> libraries;

            entry->libraries( libraries );
            readProtare( indent2, path, pops, libraries, entry->name( ) == GIDI_protareChars ); }
        else {
            std::cerr << "ERROR: unknown map entry name: " << entry->name( ) << std::endl;
        }
    }
}
/*
=========================================================
*/
void readProtare( std::string const &a_indent, std::string const &protareFilename, PoPI::Database const &pops, std::vector<std::string> &a_libraries, bool a_targetRequiredInGlobalPoPs ) {

    --countDown;
    if( countDown != 0 ) return;
    countDown = nth;

    std::string throwMessage;
    GIDI::Protare *protare = nullptr;
    GIDI::ParticleSubstitution particleSubstitution;

    try {
        std::cout << a_indent << protareFilename;

        LUPI::Timer timer;
        protare = new GIDI::ProtareSingle( *constructionPtr, protareFilename, GIDI::FileType::XML, pops, particleSubstitution, a_libraries, 
                GIDI_MapInteractionNuclearChars, a_targetRequiredInGlobalPoPs );
        if( printTiming ) std::cout << LUPI::Misc::doubleToString3( " CPU %6.3f s", timer.deltaTime( ).CPU_time( ) )
                << LUPI::Misc::doubleToString3( " wall %6.3f s", timer.deltaTime( ).wallTime( ) );

        if( printLibraries ) {
            std::cout << ": libraries =";
            std::vector<std::string> libraries( protare->libraries( ) );
            for( std::vector<std::string>::iterator iter = libraries.begin( ); iter != libraries.end( ); ++iter ) std::cout << " " << *iter;
        }
        std::cout << std::endl; }
    catch (char const *str) {
        throwMessage = str; }
    catch (std::string str) {
        throwMessage = str; }
    catch (std::exception &exception) {
        throwMessage = exception.what( );
    }

    if( throwMessage != "" ) {
        ++errCount;
        std::cout << "ERROR: throw with message '" << throwMessage << "'" << std::endl;
    }

    delete protare;
}
