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

#include <GIDI_testUtilities.hpp>

static char const *description = "Set active to false for the list of reaction indices specified by the non-optional arguments and then calls various multiGroup* functions (e.g., multiGroupCrossSection).";

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
        std::cout << str << std::endl;
        exit( EXIT_FAILURE ); }
    catch (std::string &str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    argvOption *option;

    argvOptions argv_options( "activeReactions", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "-c", true, "Print list of reactions matching ENDL C-value. Multiple -c options are supported." ) );
    argv_options.add( argvOption( "-C", true, "Set reactions matching ENDL C-value as inactive. Multiple -C options are supported." ) );
    argv_options.add( argvOption( "--invert", false, "Inverts the action of -C." ) );
    argv_options.add( argvOption( "-a", false, "If present, sets a_checkActiveState to true in call to reactionIndicesMatchingENDLCValues." ) );

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photonMode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photonMode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );

    GIDI::Transporting::Particles particles;
    parseTestOptions.particles( particles );

    option = argv_options.find( "-C" );
    std::set<int> CValues;
    for( std::size_t i1 = 0; i1 < option->m_counter; ++i1 ) CValues.insert( asInt( argv[option->m_indices[i1]] ) );
    GIDI::ExcludeReactionsSet reactionIndices1 = protare->reactionIndicesMatchingENDLCValues( CValues );
    for( auto iter = reactionIndices1.begin( ); iter != reactionIndices1.end( ); ++iter ) {
        GIDI::Reaction *reaction = protare->reaction( *iter );

        reaction->setActive( false );
    }

    option = argv_options.find( "--invert" );
    if( option->present( ) ) {
        for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
            GIDI::Reaction *reaction = protare->reaction( index );

            reaction->setActive( !reaction->active( ) );
        }

        for( std::size_t index = 0; index < protare->numberOfOrphanProducts( ); ++index ) {
            GIDI::Reaction *reaction = protare->orphanProduct( index );

            reaction->setActive( false );
        }
    }

    CValues.clear( );
    option = argv_options.find( "-c" );
    for( std::size_t i1 = 0; i1 < option->m_counter; ++i1 ) CValues.insert( asInt( argv[option->m_indices[i1]] ) );
    bool a_checkActiveState = argv_options.find( "-a" )->present( );
    reactionIndices1 = protare->reactionIndicesMatchingENDLCValues( CValues, a_checkActiveState );
    std::vector<std::size_t> reactionIndices2;
    for( auto iter = reactionIndices1.begin( ); iter != reactionIndices1.end( ); ++iter ) reactionIndices2.push_back( *iter );
    std::cout << "List of reaction indicies matching '-c' option:" << std::endl;
    for( auto iter = reactionIndices2.begin( ); iter != reactionIndices2.end( ); ++iter ) std::cout << "  " << *iter;
    std::cout << std::endl;

    std::cout << "    Number of reactions = " << protare->numberOfReactions( ) << std::endl;
    std::cout << "  index  label                                     C  state" << std::endl;
    std::size_t length = outputChannelStringMaximumLength( protare );
    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction const *reaction = protare->reaction( index );
        bool in_cList = false;

        for( auto iter = reactionIndices2.begin( ); iter != reactionIndices2.end( ); ++iter ) {
            if( *iter == index ) in_cList = true;
        }
        std::cout << longToString( "    %3ld  ", static_cast<long>( index ) ) << fillString( reaction->label( ), length, Justification::left, false )
                << intToString( "  %3d", reaction->ENDL_C( ) ) << intToString( "    %d", reaction->active( ) )
                << intToString( "    %d", in_cList ) << std::endl;
    }
    std::cout << std::endl;

    delete protare;
}
