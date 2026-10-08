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
    LUPI::StatusMessageReporting smr1;

    argvOptions argv_options( "activeReactions", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "-r", true, "Set specified reaction as inactive. Multiple -r options are supported." ) );
    argv_options.add( argvOption( "-C", true, "Set reactions matching ENDL C-value  as inactive. Multiple -C options are supported." ) );
    argv_options.add( argvOption( "--invert", false, "Inverts the action of -r and -C." ) );
    argv_options.add( argvOption( "--track", true, "Add the specfied particle to the list of particles to track." ) );

    parseTestOptions.m_askTracking = true;

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photonMode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photonMode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );

    GIDI::Transporting::Particles particles;
    parseTestOptions.particles( particles );

    option = argv_options.find( "-r" );
    for( std::size_t i1 = 0; i1 < option->m_counter; ++i1 ) {
        int intReactionIndex = asInt( argv[option->m_indices[i1]] );
        std::size_t reactionIndex = static_cast<std::size_t>( intReactionIndex );

        if( ( intReactionIndex < 0 ) || ( reactionIndex > protare->numberOfReactions( ) ) ) 
            throw GIDI::Exception( "Reaction index " + intToString( "%d", static_cast<int>( reactionIndex ) ) + " out of range." );
        GIDI::Reaction *reaction = protare->reaction( reactionIndex );

        reaction->setActive( false );
    }

    option = argv_options.find( "-C" );
    for( std::size_t i1 = 0; i1 < option->m_counter; ++i1 ) {
        std::set<int> CValues;
        CValues.insert( asInt( argv[option->m_indices[i1]] ) );

        GIDI::ExcludeReactionsSet reactionIndices = protare->reactionIndicesMatchingENDLCValues( CValues );

        for( auto iter = reactionIndices.begin( ); iter != reactionIndices.end( ); ++iter ) {
            GIDI::Reaction *reaction = protare->reaction( *iter );

            reaction->setActive( false );
        }
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

    std::cout << "    Number of reactions = " << protare->numberOfReactions( ) << std::endl;
    std::size_t length = outputChannelStringMaximumLength( protare );
    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction const *reaction = protare->reaction( index );

        std::cout << longToString( "    %3ld ", static_cast<long>( index ) ) << fillString( reaction->label( ), length, Justification::left, false ) 
                << intToString( "  %d", reaction->active( ) ) << std::endl;
    }
    std::cout << std::endl;

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        GIDI::PhysicalQuantity const &temperature = iter->temperature( );

        std::cout << "    temperature = " << temperature.value( ) << " multi-group label = " << iter->heatedMultiGroup( ) << std::endl;
    }
    std::cout << std::endl;

    std::string label( temperatures[0].heatedMultiGroup( ) );
    particles.process( *protare, label );

    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::on );

    GIDI::Vector vector = protare->multiGroupCrossSection( smr1, settings, temperatures[0] );
    printVector( "Total cross section :: ", vector );

    vector = protare->multiGroupQ( smr1, settings, temperatures[0], false );
    printVector( "Q (initial) :: ", vector );
    vector = protare->multiGroupQ( smr1, settings, temperatures[0], true );
    printVector( "Q (final)   :: ", vector );

    vector = protare->multiGroupAvailableEnergy( smr1, settings, temperatures[0] );
    printVector( "Available energy   :: ", vector );

    vector = protare->multiGroupAvailableMomentum( smr1, settings, temperatures[0] );
    printVector( "Available momentum :: ", vector );

    std::set<std::string> ids;
    protare->productIDs( ids, particles, false );
    printIDs( "Product IDs (all)           : ", ids );

    ids.clear( );
    protare->productIDs( ids, particles, true );
    printIDs( "Product IDs (transportable) : ", ids );

    std::vector<std::string> particleIDs = particles.sortedIDs( );
    std::string header;
    for( auto iter = particleIDs.begin( ); iter != particleIDs.end( ); ++iter ) {

        std::cout << std::endl;

        vector = protare->multiGroupAverageEnergy( smr1, settings, temperatures[0], *iter );
        header = *iter + " average energy :: ";
        printVector( header, vector );

        vector = protare->multiGroupAverageMomentum( smr1, settings, temperatures[0], *iter );
        header = *iter + " average momentum :: ";
        printVector( header, vector );

        vector = protare->multiGroupMultiplicity( smr1, settings, temperatures[0], *iter );
        header = *iter + " multiplicity :: ";
        printVector( header, vector );

        int intMaxOrder = protare->maximumLegendreOrder( smr1, settings, temperatures[0], *iter );
        if( intMaxOrder > -1 ) {
            std::size_t maxOrder = static_cast<std::size_t>( intMaxOrder );
            for( std::size_t order = 0; order < maxOrder; ++order ) {
                GIDI::Matrix matrix = protare->multiGroupProductMatrix( smr1, settings, temperatures[0], particles, *iter, order );

                if( matrix.size( ) == 0 ) {
                    matrix = settings.multiGroupZeroMatrix( particles, *iter );
                }
                header = "    " + *iter + " product matrix for Legendre order " + intToString( "%d", static_cast<int>( order ) );
                printMatrix( header, -2, matrix );
            }
        }
    }

    delete protare;
}
