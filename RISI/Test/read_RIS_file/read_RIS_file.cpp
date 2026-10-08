/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <iostream>

#include <RISI.hpp>

static char const *description = "Reads in a **RIS** file and prints it.";

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

    LUPI::ArgumentParser argumentParser( __FILE__, description );
    LUPI::Positional *inputPathArgument = argumentParser.add<LUPI::Positional>( "inputPath", "The path to the ris file to read.", 0, 1 );

    argumentParser.parse( argc, argv );

    std::string path( "../../../LUPI/Test/splitString/test.ris" );
    if( inputPathArgument->counts( ) > 0 ) path = inputPathArgument->value( );

    GIDI::RISI::Projectiles projectiles;

    GIDI::RISI::readRIS( path, "MeV", projectiles );
    bool verbose = true;
    if( !verbose ) {
        projectiles.print( ); }
    else {
        projectiles.printAsRIS_file( );
    }
}
