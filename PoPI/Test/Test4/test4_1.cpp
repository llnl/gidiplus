/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <iostream>

#include "PoPI.hpp"

void printInfo( PoPI::Database &database, char const *ID );
/*
=========================================================
*/
int main( LUPI_maybeUnused int argc, LUPI_maybeUnused char **argv ) {

    std::cerr << "    " << LUPI::FileInfo::basenameWithoutExtension( __FILE__ ) << std::endl;

    std::string fileName( "../../../TestData/PoPs/pops.xml" );
    std::string aliasFileName( "../../../TestData/PoPs/LLNL_alias.xml" );
    std::string metaStableFileName( "../../../TestData/PoPs/metastables_alias.xml" );

    try {
        PoPI::Database database( fileName );
        database.addFile( aliasFileName, false );
        database.addFile( metaStableFileName, false );

        printInfo( database, "Co58_m1" );
        printInfo( database, "Ag110_m1" );
        printInfo( database, "Am242_m1" );
        }
    catch (char const *str) {
        std::cout << str << std::endl;
    }
}
/*
=========================================================
*/
void printInfo( PoPI::Database &database, char const *ID ) {

    PoPI::IDBase const &particle = database.get<PoPI::IDBase>( ID );
    auto final = database.final( particle.index( ) );
    auto final2 = database.final( particle.index( ), true );

    std::cout << std::endl;
    std::cout << ID << "  final = " << database.final( particle.index( ) ) << "  final2 = " << final2 << std::endl;
    std::cout << "         final = " << database.final( final ) << "  final2 = " << database.final( final, true ) << std::endl;
    std::cout << "         final = " << database.final( final2 ) << "  final2 = " << database.final( final2, true ) << std::endl;
    std::cout << "         isAlias( " << final  << " ) = " << database.isAlias( final )  << "  isMetaStableAlias( " << final  << " ) = " << database.isMetaStableAlias( final ) << std::endl;
    std::cout << "         isAlias( " << final2 << " ) = " << database.isAlias( final2 ) << "  isMetaStableAlias( " << final2 << " ) = " << database.isMetaStableAlias( final2 ) << std::endl;
}
