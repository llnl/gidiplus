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
#include <iomanip>

#include "GIDI_testUtilities.hpp"

static char const *description = "Prints out the list of reaction indices for the specified ENDL C values.";

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

    argvOptions argv_options( "ENDLCValues", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "--minus", true, "Next argument is a negative C value (e.g., to include reaction 'sumOfRemainingOutputChannels' channels enter something like ENDLCValues 10 40 --minus 5." ) );

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photo_mode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );

    std::cout << stripDirectoryBase( protare->fileName( ) ) << std::endl;
    for( std::size_t i1 = 0; i1 < protare->numberOfReactions( ); ++i1 ) {
        GIDI::Reaction const *reaction = protare->reaction( i1 );

        std::cout << "    " << std::setw( 3 ) << i1 << " " << std::setw( 3 ) << reaction->ENDL_C( ) << " " << reaction->label( ) << std::endl;
    }
    GIDI::ProtareSingle const *protareSingle = protare->protare( 0 );
    GIDI::Reaction const *reaction = protareSingle->nuclearPlusCoulombInterferenceOnlyReaction( );
    if( reaction != nullptr ) {
        std::cout << "    " << std::setw( 3 ) << "-" << " " << std::setw( 3 ) << reaction->ENDL_C( ) << " " << reaction->label( ) << std::endl;
    }

    std::set<int> CValues;
    for( std::size_t i1 = 0; i1 < argv_options.m_arguments.size( ); ++i1 ) CValues.insert( asInt( argv[argv_options.m_arguments[i1]] ) );
    argvOption *minus = argv_options.find( "--minus" );
    for( std::vector<int>::iterator iter = minus->m_indices.begin( ); iter != minus->m_indices.end( ); ++iter ) CValues.insert( -asInt( argv[*iter] ) );

    std::cout << std::endl;
    std::cout << "    C values =";
    for( std::set<int>::iterator iter = CValues.begin( ); iter != CValues.end( ); ++iter ) std::cout << " " << *iter;
    std::cout << std::endl;

    GIDI::ExcludeReactionsSet indices = protare->reactionIndicesMatchingENDLCValues( CValues );

    std::cout << std::endl;
    std::cout << "    Matching indices are:" << std::endl;
    for( auto iter = indices.begin( ); iter != indices.end( ); ++iter ) std::cout << "        " << *iter << std::endl;

    delete protare;
}
