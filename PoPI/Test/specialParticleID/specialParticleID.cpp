/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdio.h>
#include <iostream>

#include "PoPI.hpp"

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
void main2( LUPI_maybeUnused int argc, LUPI_maybeUnused char **argv ) {

    std::cerr << "    " << LUPI::FileInfo::basenameWithoutExtension( __FILE__ ) << std::endl;

    std::string fileName( "../../../TestData/PoPs/pops.xml" );
    std::string aliasFileName( "../../../TestData/PoPs/LLNL_alias.xml" );
    std::string metaStableFileName( "../../../TestData/PoPs/metastables_alias.xml" );

    PoPI::Database database( fileName );
    database.addFile( aliasFileName, false );
    database.addFile( metaStableFileName, false );

    printf( "   ID       familiar nuclide  nucleus\n" );
    printf( "   ----------------------------------\n" );
    std::size_t numberOfParticles = database.size( );
    for( std::size_t index = 0; index < numberOfParticles; ++index ) {
        PoPI::Base const &base = database.get<PoPI::Base>( index );

        std::string familiar = PoPI::specialParticleID(PoPI::SpecialParticleID_mode::familiar, base.ID( ));
        std::string nuclide = PoPI::specialParticleID(PoPI::SpecialParticleID_mode::nuclide, base.ID( ));
        std::string nucleus = PoPI::specialParticleID(PoPI::SpecialParticleID_mode::nucleus, base.ID( ));
        if( ( familiar != nuclide ) || ( familiar != nucleus ) || ( nuclide != nucleus ) ) {
            printf( "   %-8s %-8s %-8s %-8s\n", base.ID( ).c_str( ), familiar.c_str( ), nuclide.c_str( ), nucleus.c_str( ) );
        }
    }
}
