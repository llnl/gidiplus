/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <iostream>
#include <stdlib.h>

#include "GIDI_testUtilities.hpp"

int checkVector( std::size_t n1 );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    printCodeArguments( __FILE__, argc, argv );

    int errorCount = 0;

    errorCount += checkVector( 5 );
    errorCount += checkVector( 6 );
    errorCount += checkVector( 15 );
    errorCount += checkVector( 26 );

    exit( errorCount );
}
/*
=========================================================
*/
int checkVector( std::size_t n1 ) {

    int errorCount = 0;
    GIDI::Vector v1( n1 );

    std::cout << std::endl;
    std::cout << "size = " << n1 << std::endl;
    std::cout << "Vector 1, zeros" << std::endl;
    v1.print( "    vector:         " );
    v1.reverse( );
    v1.print( "    reversed:       " );
    v1.reverse( );
    v1.print( "    reversed again: " );

    std::cout << "Vector 1, linear" << std::endl;
    for( std::size_t i1 = 0; i1 < n1; ++i1 ) v1[i1] = static_cast<double>( i1 );
    v1.print( "    vector:         " );
    v1.reverse( );
    v1.print( "    reversed:       " );

    GIDI::Vector v2( n1 );
    for( std::size_t i1 = 0; i1 < n1; ++i1 ) v2[i1] = static_cast<double>( n1 - i1 - 1 );
    v2.print( "    v2 vector:      " );
    for( std::size_t i1 = 0; i1 < n1; ++i1 ) {
        if( v1[i1] != v2[i1] ) ++errorCount;
    }

    v2 = v1;
    v1.reverse( );
    v1.print( "    reversed again: " );

    v1 += v2;
    v1.print( "    summed:         " );

    for( std::size_t i1 = 0; i1 < n1; ++i1 ) {
        if( v1[i1] != static_cast<double>( n1 - 1 ) ) ++errorCount;
    }

    return( errorCount );
}
