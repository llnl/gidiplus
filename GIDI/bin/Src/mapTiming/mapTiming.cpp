/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <iomanip>

#include "LUPI.hpp"
#include "GIDI.hpp"

static char const *description = "Times how long it takes to load a map file.";

/*
=========================================================
*/
int main( int argc, char **argv ) {

    PoPI::Database pops;
    LUPI::ArgumentParser argumentParser( __FILE__, description );

    LUPI::Positional *mapFileNamePositional = argumentParser.add<LUPI::Positional>( "mapFileName", "Path to the map file whose instantiation will be timed." );
    LUPI::OptionStore *numberOfLoopsOptions = argumentParser.add<LUPI::OptionStore>( "-n", "Number of times to instantiate map file for timing." );
    argumentParser.parse( argc, argv );

    long numberOfLoops = 10;
    if( numberOfLoopsOptions->counts( ) > 0 ) numberOfLoops = std::stol( numberOfLoopsOptions->value( ) );

    pops.addFile( "/usr/gapps/data/nuclear/common/pops.xml", false );
    pops.addFile( "/usr/gapps/data/nuclear/common/metastables_alias.xml", false );

    std::string mapFilename = mapFileNamePositional->value( 0 );

    LUPI::Timer timer;

    for( long i1 = 0; i1 < numberOfLoops; ++i1 ) {
        GIDI::Map::Map map( mapFilename, pops );
    }

    std::cout << "Number of loops = " << numberOfLoops << std::endl;

    LUPI::DeltaTime deltaTimer = timer.deltaTime( );
    std::cout << deltaTimer.toString( ) << std::endl;

    double CPU_timePerLoad = deltaTimer.CPU_time( ) / static_cast<double>( numberOfLoops );
    std::cout << "CPU time per instantiation  = " << CPU_timePerLoad << std::endl;

    double wallTimePerLoad = deltaTimer.wallTime( ) / static_cast<double>( numberOfLoops );
    std::cout << "Wall time per instantiation = " << wallTimePerLoad << std::endl;

    exit( EXIT_SUCCESS );
}
