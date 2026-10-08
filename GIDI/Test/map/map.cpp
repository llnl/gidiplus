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

static char const *description = "This program ...";

void main2( int argc, char **argv );
void printList( char const *prefix, std::vector<std::string> &list );
bool mapWalkCallBack( GIDI::Map::ProtareBase const *a_protareEntry, LUPI_maybeUnused std::string const &a_library,
                LUPI_maybeUnused void *a_data, LUPI_maybeUnused int a_level );
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
    

    GIDI::Map::BaseEntry::PathForm entered = GIDI::Map::BaseEntry::PathForm::entered;
    GIDI::Map::BaseEntry::PathForm cumulative = GIDI::Map::BaseEntry::PathForm::cumulative;
    GIDI::Map::BaseEntry::PathForm l_real = GIDI::Map::BaseEntry::PathForm::real;

    argvOptions argv_options( "map", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askPid = false;
    parseTestOptions.m_askTid = false;
    parseTestOptions.m_askPhotoAtomic = false;
    parseTestOptions.m_askPhotoNuclear = false;

    parseTestOptions.parse( );

    PoPI::Database pops;
    parseTestOptions.pops( pops, "../../../TestData/PoPs/pops.xml" );

    std::string fileName = argv_options.find( "--map" )->zeroOrOneOption( argv, "all.map" );
    GIDI::Map::Map *map = new GIDI::Map::Map( fileName, pops );

    std::cout << std::endl;

    std::cout << "GNDS format = " << map->formatVersion( ).format( ) << std::endl;

    std::cout << "library = " << map->library( ) << std::endl;

    std::cout << "isProtareAvailable( PoPI::IDs::neutron, H1 ) " << map->isProtareAvailable( PoPI::IDs::neutron, "H1" ) << std::endl;
    std::cout << "isProtareAvailable( PoPI::IDs::neutron, H2 ) " << map->isProtareAvailable( PoPI::IDs::neutron, "H2" ) << std::endl;
    std::cout << "isProtareAvailable( PoPI::IDs::neutron, H2 ) " << map->isProtareAvailable( PoPI::IDs::neutron, "H3" ) << std::endl;

    std::cout << "isProtareAvailable( PoPI::IDs::neutron, H1, 'ENDF/BVII.1' ) " << map->isProtareAvailable( PoPI::IDs::neutron, "H1", "", "ENDF/BVII.1" ) << std::endl;
    std::cout << "isProtareAvailable( PoPI::IDs::neutron, H1, 'ENDF/BVII.2' ) " << map->isProtareAvailable( PoPI::IDs::neutron, "H1", "", "ENDF/BVII.2" ) << std::endl;
    std::cout << "isProtareAvailable( PoPI::IDs::neutron, H1, 'ENDF/BVIII.1' ) " << map->isProtareAvailable( PoPI::IDs::neutron, "H1", "", "ENDF/BVIII.1" ) << std::endl;

    std::cout << "protareFilename( PoPI::IDs::neutron, H1 ) = <" << 
            stripDirectoryBase( map->protareFilename( PoPI::IDs::neutron, "H1" ) ) << ">" << std::endl;
    std::cout << "protareFilename( PoPI::IDs::neutron, H1 ) = <" << map->protareFilename( PoPI::IDs::neutron, "H1", "", "", entered ) << ">" << std::endl;
    std::cout << "protareFilename( PoPI::IDs::neutron, H1 ) = <" << 
            stripDirectoryBase( map->protareFilename( PoPI::IDs::neutron, "H1", "", "", cumulative ) ) << ">" << std::endl;
    std::cout << "protareFilename( PoPI::IDs::neutron, H1 ) = <" << 
            stripDirectoryBase( map->protareFilename( PoPI::IDs::neutron, "H1", "", "", l_real ) ) << ">" << std::endl;
    std::cout << "protareFilename( PoPI::IDs::neutron, H1, 'ENDF/BVII.1' ) = <" << map->protareFilename( PoPI::IDs::neutron, "H1", "", "ENDF/BVII.1", entered ) << ">" << std::endl;
    std::cout << "protareFilename( PoPI::IDs::neutron, H1, 'ENDF/BVII.2' ) = <" << map->protareFilename( PoPI::IDs::neutron, "H1", "", "ENDF/BVII.2", cumulative ) << ">" << std::endl;
    std::cout << "protareFilename( PoPI::IDs::neutron, H1, 'ENDF/BVIII.1' ) = <" << 
            stripDirectoryBase( map->protareFilename( PoPI::IDs::neutron, "H1", "", "ENDF/BVIII.1", l_real ) ) << ">" << std::endl;

    std::vector<std::string> list = map->availableEvaluations( PoPI::IDs::neutron, "H1" );
    printList( "availableEvaluations( PoPI::IDs::neutron, H1 ) =", list );

    list = map->availableEvaluations( PoPI::IDs::neutron, "H2" );
    printList( "availableEvaluations( PoPI::IDs::neutron, H2 ) =", list );

    list = map->availableEvaluations( PoPI::IDs::neutron, "O16" );
    printList( "availableEvaluations( PoPI::IDs::neutron, O16 ) =", list );

    list = map->availableEvaluations( PoPI::IDs::neutron, "O17" );
    printList( "availableEvaluations( PoPI::IDs::neutron, O17 ) =", list );

    std::cout << std::endl;
    map->walk( mapWalkCallBack, map );

    std::cout << std::endl;
    PoPI::Particle const &O16 = pops.get<PoPI::Particle>( "O16" );
    PoPI::PhysicalQuantity const *pMass = O16.mass( )[0];
    PoPI::PQ_double const &mass = dynamic_cast<PoPI::PQ_double const &>( *pMass );
    std::cout << "O16 mass = " << mass.value( ) << " " << mass.unit( ) << std::endl;

    delete map;
}
/*
=========================================================
*/
void printList( char const *prefix, std::vector<std::string> &list ) {

    std::string sep( "" );

    std::cout << prefix << " <";
    for( std::vector<std::string>::const_iterator iter = list.begin( ); iter != list.end( ); ++iter ) {
        std::cout << sep << "'" << *iter << "'";
        sep = ", ";
    }
    std::cout << ">" << std::endl;
}
/*
=========================================================
*/
bool mapWalkCallBack( GIDI::Map::ProtareBase const *a_protareEntry, LUPI_maybeUnused std::string const &a_library,
                LUPI_maybeUnused void *a_data, LUPI_maybeUnused int a_level ) {

    GIDI::Map::Map const *map = (GIDI::Map::Map const *) a_data;
    std::string path = stripDirectoryBase( a_protareEntry->path( ) );

    std::cout << path << std::endl;

    GIDI::Map::ProtareBase const *protareEntry = map->findProtareEntry( a_protareEntry->projectileID( ), a_protareEntry->targetID( ), "", a_protareEntry->evaluation( ) );
    if( protareEntry == nullptr ) {
        std::cout << "Oops" << std::endl; }
    else {
        std::cout << "    " << protareEntry->parent( )->library( ) << "    " << protareEntry->evaluation( ) << std::endl;
    }

    return( true );
}
