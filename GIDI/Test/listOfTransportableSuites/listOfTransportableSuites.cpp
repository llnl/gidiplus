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

#include "GIDI_testUtilities.hpp"

static char const *description = "This program prints the multi-group boundaries for each transportable particle in a GNDS file.";

void main2( int argc, char **argv );
void printVector( std::string const &prefix, std::string const &indent, nf_Buffer<double> const &vector );
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

    argvOptions argv_options( "listOfTransportableSuites", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askGNDS_File = true;

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photo_mode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );

    std::cout << stripDirectoryBase( protare->fileName( ), "/GIDI/Test/" ) << std::endl;

    auto listOfTransportableSuites = protare->listOfTransportableSuites( );

    std::cout << "Number of transportables = " << listOfTransportableSuites.size( ) << std::endl;
    int index = 0;
    for( auto iter = listOfTransportableSuites.begin( ); iter != listOfTransportableSuites.end( ); ++iter, ++index ) {
        std::cout << "Transportables at index " << index << std::endl;
        for( auto iter2 = (*iter)->begin( ); iter2 != (*iter)->end( ); ++iter2 ) {
            GIDI::Transportable const *transportable = static_cast<GIDI::Transportable *>( *iter2 );
            GIDI::Group const &group = transportable->group( );
            std::cout << "    Transportable \"" << transportable->label( ) << "\" with group id \"" 
                    << group.label( ) << " of length " << group.size( ) << "\"." << std::endl;
        }
    }

    delete protare;
}
/*
=========================================================
*/
void printVector( std::string const &prefix, std::string const &indent, nf_Buffer<double> const &vector ) {

    std::cout << prefix << ": size = " << vector.size( ) << std::endl;
    std::cout << indent;
    for( std::string::size_type index = 0; index < vector.size( ); ++index ) std::cout << " " << vector[index];
    std::cout << std::endl;
}
