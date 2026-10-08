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
#include <set>

#include "MCGIDI.hpp"

#include "GIDI_testUtilities.hpp"
#include "MCGIDI_testUtilities.hpp"

static char const *description = "Does some testing of the serialize methods which are used to broadcast for use in MPI and on GPUs.";

void main2( int argc, char **argv );
void printDetails( MCGIDI::Protare *MCProtare, GIDI::Styles::TemperatureInfos &temperatures, std::ostream &out );
void printProtareSingle( MCGIDI::ProtareSingle *MCProtare, std::ostream &out );
void printReaction( MCGIDI::Reaction *reaction, LUPI::DataBuffer &dataBufferReactions, LUPI::DataBuffer &dataBufferDistributions, std::ostream &out );
void printSizes( std::string const &a_prefix, LUPI::DataBuffer &a_dataBuffer, std::ostream &out, bool a_header );
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
    GIDI::Transporting::Particles particles;
    GIDI::Groups groups( "../../../GIDI/Test/groups.xml" );
    GIDI::Fluxes fluxFile( "../../../GIDI/Test/fluxes.xml" );
    std::set<int> reactionsToExclude;
    GIDI::Transporting::Mode transportingMode( GIDI::Transporting::Mode::MonteCarloContinuousEnergy );
    LUPI::StatusMessageReporting smr1;

    argvOptions argv_options( "memoryCheck", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "--detailed", false, "If pretent, detail infomation about the size of each reaction and other data are outputted.." ) );
    argv_options.add( argvOption( "--fixedGrid", false, "Set fixed grid data. Only used if protare is only photo-atomic protare." ) );
    argv_options.add( argvOption( "--multiGroup", false, "If present, multi-group data are loaded." ) );
    argv_options.add( argvOption( "--DBRC", false, "If present, calls turn on the DBRC upscatter data loading." ) );

    parseTestOptions.parse( );

    bool detailed = argv_options.find( "--detailed" )->present( );

    if( argv_options.find( "--multiGroup" )->present( ) ) transportingMode = GIDI::Transporting::Mode::multiGroup;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../../../GIDI/Test/all3T.map", construction, PoPI::IDs::neutron, "O16" );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    if( argv_options.find( "--DBRC" )->present( ) ) MC.setUpscatterModelDBRC( );

    if( argv_options.find( "--fixedGrid" )->present( ) ) {
        MC.fixedGridPoints( groups.get<GIDI::Group>( "LLNL_gid_80" )->data( ) );
    }

    GIDI::Functions::Function3dForm const *fluxes = fluxFile.get<GIDI::Functions::Function3dForm>( "LLNL_fid_1" );
    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, *groups.get<GIDI::Group>( "LLNL_gid_4" ), *fluxes, transportingMode );
    particles.add( neutron );

    GIDI::Transporting::Particle photon( PoPI::IDs::photon, *groups.get<GIDI::Group>( "LLNL_gid_70" ), *fluxes, transportingMode );
    particles.add( photon );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    LUPI::DataBuffer dataBuffer;

// Count phase
    int protareType = 0;
    if( MCProtare->protareType( ) == MCGIDI::ProtareType::composite ) { protareType = 1; }
    if( MCProtare->protareType( ) == MCGIDI::ProtareType::TNSL ) { protareType = 2; }
    dataBuffer.m_intIndex++;                                                        // Add 1 for storing the protare type
    MCProtare->serialize( dataBuffer, LUPI::DataBuffer::Mode::Count );
    dataBuffer.allocateBuffers( );
    dataBuffer.zeroIndexes( );

// Pack phase
    dataBuffer.m_intData[dataBuffer.m_intIndex++] = protareType;                    // Protare type is special
    MCProtare->serialize( dataBuffer, LUPI::DataBuffer::Mode::Pack );

// Memory phase
    dataBuffer.m_maxPlacementSize = MCProtare->memorySize( );
    dataBuffer.m_placementStart = reinterpret_cast<char *>( malloc( dataBuffer.m_maxPlacementSize ) );
    dataBuffer.m_placement = dataBuffer.m_placementStart;

// Unpack phase
    MCGIDI::Protare *MCProtare2;
    dataBuffer.zeroIndexes( );

    protareType = dataBuffer.m_intData[(dataBuffer.m_intIndex)++];                  // Protare type is special
    if( protareType == 0 ) {
        MCProtare2 = new(dataBuffer.m_placementStart) MCGIDI::ProtareSingle( );
        dataBuffer.m_placement += sizeof( MCGIDI::ProtareSingle ); }
    else if( protareType == 1 ) {
        MCProtare2 = new(dataBuffer.m_placementStart) MCGIDI::ProtareComposite( );
        dataBuffer.m_placement += sizeof( MCGIDI::ProtareComposite ); }
    else if( protareType == 2 ) {
        MCProtare2 = new(dataBuffer.m_placementStart) MCGIDI::ProtareTNSL( );
        dataBuffer.m_placement += sizeof( MCGIDI::ProtareTNSL ); }
    else {
        
        throw std::runtime_error( LUPI::Misc::argumentsToString( "Bad protare type %d.", protareType ) );
    }

    MCProtare2->serialize( dataBuffer, LUPI::DataBuffer::Mode::Unpack );

    long actual = (long) ( dataBuffer.m_placement - dataBuffer.m_placementStart );
    long diff = (long) dataBuffer.m_maxPlacementSize - actual;
    if( diff != 0 ) {                                                               // diff = 0 is success.
        std::string Str = LUPI::Misc::argumentsToString( "Protare %s + %s, predicted size %ld, actual size %ld, diff %ld.", 
                MCProtare2->projectileID( ).c_str( ), MCProtare2->targetID( ).c_str( ), (long) dataBuffer.m_maxPlacementSize, actual, diff );
        throw std::runtime_error( Str );
    }

    std::ostringstream protare1stream, protare2stream;
    printDetails( MCProtare, temperatures, protare1stream );
    printDetails( MCProtare2, temperatures, protare2stream );

    if( detailed ) std::cout << protare2stream.str( );

    if( protare1stream.str( ) != protare2stream.str( ) ) {
        std::string Str = "Differences detected after deserialization!\n";
        throw std::runtime_error( Str );
    }

    delete protare;
    delete MCProtare;

    free( MCProtare2 );
}

/*
=========================================================
*/

void printDetails( MCGIDI::Protare *MCProtare, GIDI::Styles::TemperatureInfos &temperatures, std::ostream& out ) {

    LUPI::DataBuffer dataBuffer;

    MCProtare->serialize( dataBuffer, LUPI::DataBuffer::Mode::Count );
    std::size_t bytes = dataBuffer.m_intIndex * sizeof( dataBuffer.m_intData[0] );
    bytes += dataBuffer.m_floatIndex * sizeof( dataBuffer.m_floatData[0] );
    bytes += dataBuffer.m_doubleIndex * sizeof( dataBuffer.m_doubleData[0] );
    bytes += dataBuffer.m_charIndex * sizeof( dataBuffer.m_charData[0] );
    bytes += dataBuffer.m_longIndex * sizeof( dataBuffer.m_longData[0] );
    double bytesMillion = bytes / 1e6;
    out << "Detail information: total bytes = " << bytes << " (" << bytesMillion << " million)." << std::endl;
    out << "Number of temperatures = " << temperatures.size( ) << std::endl;

    printSizes( "Protare", dataBuffer, out, true );
    printSizes( "Protare", dataBuffer, out, false );
    for( std::size_t protareIndex = 0; protareIndex < MCProtare->numberOfProtares( ); ++protareIndex ) {
        MCGIDI::ProtareSingle *protareSingle = MCProtare->protare( protareIndex );
        printProtareSingle( protareSingle, out );
    }
}

/*  
=========================================================
*/
    
void printProtareSingle( MCGIDI::ProtareSingle *MCProtare, std::ostream& out ) {

    LUPI::DataBuffer dataBuffer;
    LUPI::DataBuffer dataBufferReactions;
    LUPI::DataBuffer dataBufferDistributions;
    MCProtare->serialize( dataBuffer, LUPI::DataBuffer::Mode::Count );
    printSizes( "ProtareSingle", dataBuffer, out, true );
    printSizes( "ProtareSingle", dataBuffer, out, false );
    out << "  " << MCProtare->projectileID( ).c_str( ) << " + " << MCProtare->targetID( ).c_str( )
                << ", interaction = " << MCProtare->interaction( ).c_str( ) << std::endl;

    printSizes( LUPI::Misc::argumentsToString( "%-44s", "" ), dataBuffer, out, true );

    dataBuffer.zeroIndexes( );
    MCGIDI::HeatedCrossSectionsContinuousEnergy &heatedCrossSections = MCProtare->heatedCrossSections( );
    heatedCrossSections.serialize( dataBuffer, LUPI::DataBuffer::Mode::Count );
    printSizes( LUPI::Misc::argumentsToString( "%-44s", "Continuous energy cross sections" ), dataBuffer, out, false );

    dataBuffer.zeroIndexes( );
    MCGIDI::HeatedCrossSectionsMultiGroup &heatedMultigroupCrossSections = MCProtare->heatedMultigroupCrossSections( );
    heatedMultigroupCrossSections.serialize( dataBuffer, LUPI::DataBuffer::Mode::Count );
    printSizes( LUPI::Misc::argumentsToString( "%-44s", "Multi-group cross sections" ), dataBuffer, out, false );

    out << "Reactions:" << std::endl;
    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
        MCGIDI::Reaction *reaction = const_cast<MCGIDI::Reaction *>( MCProtare->reaction( reactionIndex ) );
        printReaction( reaction, dataBufferReactions, dataBufferDistributions, out );
    }
    printSizes( LUPI::Misc::argumentsToString( "    %-32s", "total distributions" ), dataBufferDistributions, out, false );
    printSizes( LUPI::Misc::argumentsToString( "    %-32s", "total reactions" ), dataBufferReactions, out, false );

    out << "Orphan products:" << std::endl;
    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfOrphanProducts( ); ++reactionIndex ) {
        MCGIDI::Reaction *reaction = const_cast<MCGIDI::Reaction *>( MCProtare->orphanProduct( reactionIndex ) );
        printReaction( reaction, dataBufferReactions, dataBufferDistributions, out );
    }
    printSizes( LUPI::Misc::argumentsToString( "    %-40s", "total distributions" ), dataBufferDistributions, out, false );
    printSizes( LUPI::Misc::argumentsToString( "    %-32s", "total reactions" ), dataBufferReactions, out, false );
}

/*
=========================================================
*/

void printReaction( MCGIDI::Reaction *reaction, LUPI::DataBuffer &dataBufferReactions, LUPI::DataBuffer &dataBufferDistributions, std::ostream& out ) {

    std::string prefix = LUPI::Misc::argumentsToString( "    %-40s", reaction->label( ).c_str( ) );
    LUPI::DataBuffer dataBuffer;

    reaction->serialize( dataBuffer, LUPI::DataBuffer::Mode::Count );
    printSizes( prefix, dataBuffer, out, false );
    reaction->serialize( dataBufferReactions, LUPI::DataBuffer::Mode::Count );

    for( MCGIDI_VectorSizeType productIndex = 0; productIndex < reaction->numberOfProducts( ); ++productIndex ) {
        MCGIDI::Product *product = const_cast<MCGIDI::Product *>( reaction->product( productIndex ) );
        LUPI::DataBuffer dataBuffer2;

        product->serialize( dataBuffer2, LUPI::DataBuffer::Mode::Count );
        prefix = LUPI::Misc::argumentsToString( "        %-36s", product->ID( ).c_str( ) );
        printSizes( prefix, dataBuffer2, out, false );

        dataBuffer2.zeroIndexes( );
        MCGIDI::Distributions::Distribution *distribution = product->distribution( );
        MCGIDI::serializeDistribution( dataBuffer2, LUPI::DataBuffer::Mode::Count, distribution );
        printSizes( LUPI::Misc::argumentsToString( "            %-32s", "distribution" ), dataBuffer2, out, false );
        MCGIDI::serializeDistribution( dataBufferDistributions, LUPI::DataBuffer::Mode::Count, distribution );
    }
}

/*
=========================================================
*/

void printSizes( std::string const &a_prefix, LUPI::DataBuffer &a_dataBuffer, std::ostream& out, bool a_header ) {

    if( a_header ) {
        std::size_t size = a_prefix.size( ) + 2;
        for( std::size_t index = 0; index < size; ++index ) out << " ";
        out << LUPI::Misc::argumentsToString( "%12s %12s %12s %12s %12s", "int", "float", "double", "char", "long" )
            << std::endl; }
    else {
        out << a_prefix << ": "
                << LUPI::Misc::argumentsToString( "%12lu %12lu %12lu %12lu %12lu", 
                a_dataBuffer.m_intIndex, a_dataBuffer.m_floatIndex, a_dataBuffer.m_doubleIndex, a_dataBuffer.m_charIndex, 
                a_dataBuffer.m_longIndex ) << std::endl;
    }
}
