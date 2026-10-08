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

/*
=========================================================
*/
int main( LUPI_maybeUnused int argc, LUPI_maybeUnused char **argv ) {

    std::cerr << "    " << LUPI::FileInfo::basenameWithoutExtension( __FILE__ ) << std::endl;

    std::string fileName( "../../../TestData/PoPs/pops.xml" );

    try {
        PoPI::Database database( fileName );

        auto O16Index = database["O16"];

        PoPI::Particle const &O16_1 = database.get<PoPI::Particle>( O16Index );
        std::cout << "O16 -> " << O16_1.ID( ) << "  " << O16Index << "  " << O16_1.ID( ) << std::endl;
        std::cout << "O16 -> " << O16_1.ID( ) << " mass = " << O16_1.massValue( "amu" ) << std::endl;

        std::string O16ID( "O16" );
        PoPI::Particle const &O16_2 = database.get<PoPI::Particle>( O16ID );
        std::cout << "O16 -> " << O16_2.ID( ) << "  " << O16Index << "  " << O16_2.ID( ) << std::endl;
        std::cout << "O16 -> " << O16_2.ID( ) << " mass = " << O16_2.massValue( "amu" ) << std::endl;

        PoPI::Particle const &O16_3 = database.get<PoPI::Particle>( "O16" );
        std::cout << "O16 -> " << O16_3.ID( ) << "  " << O16Index << "  " << O16_3.ID( ) << std::endl;
        std::cout << "O16 -> " << O16_3.ID( ) << " mass = " << O16_3.massValue( "amu" ) << std::endl; }
    catch (char const *str) {
        std::cout << str << std::endl;
    }
}
