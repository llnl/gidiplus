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

#include "GIDI.hpp"
#include "GIDI_testUtilities.hpp"

static std::size_t verbose = 0;
static int nth = 1;
static int countDown = 1;

static char const *description = "Reads in each protare in the map file but only at the HAPI level. This program\n"
    " was written to test thd speed to parse in the basic HAPI structure (i.e., it does not create\n"
    "any of the GIDI structure).";

void main2( int argc, char **argv );
void walk( std::string const &mapFilename, PoPI::Database &a_pops );
void flashReadProtare( std::string const &a_filename );
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

    PoPI::Database pops;
    argvOptions argv_options( "flashReadAllProtaresInMapFile", description, 1 );

    argv_options.add( argvOption( "-v", false, "Amount of verbosity." ) );
    argv_options.add( argvOption( "-n", true, "If present, only every nth protare is read where 'n' is the next argument." ) );

    argv_options.parseArgv( argc, argv );

    verbose = argv_options.find( "-v" )->m_counter;
    nth = argv_options.find( "-n" )->asInt( argv, 1 );
    if( nth < 0 ) nth = 1;

    if( argv_options.m_arguments.size( ) != 1 ) {
        throw "Only one argument is allowed and it is required. The argument must be a path to a map file.";
    }

    std::string mapFile( argv[argv_options.m_arguments[0]] );
    walk( mapFile, pops );
}
/*
=========================================================
*/
void walk( std::string const &mapFilename, PoPI::Database &a_pops ) {


    if( verbose ) std::cout << "    " << mapFilename << std::endl;
    GIDI::Map::Map map( mapFilename, a_pops );

    for( std::size_t i1 = 0; i1 < map.size( ); ++i1 ) {
        GIDI::Map::BaseEntry const *entry = map[i1];

        std::string path = entry->path( GIDI::Map::BaseEntry::PathForm::cumulative );

        if( entry->name( ) == GIDI_importChars ) {
            walk( path, a_pops ); }
        else if( ( entry->name( ) == GIDI_protareChars ) || ( entry->name( ) == GIDI_TNSLChars ) ) {
            flashReadProtare( path ); }
        else {
            std::cerr << "    ERROR: unknown map entry name: " << entry->name( ) << std::endl;
        }
    }
}
/*
=========================================================
*/
void flashReadProtare( std::string const &a_filename ) {

    --countDown;
    if( countDown != 0 ) return;
    countDown = nth;

    if( verbose > 1 ) std::cout << "        " << a_filename << std::endl;

    HAPI::File *doc = new HAPI::PugiXMLFile( a_filename.c_str( ), "flashReadProtare" );
    delete doc;
}
