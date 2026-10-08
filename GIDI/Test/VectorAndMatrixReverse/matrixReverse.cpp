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

int checkMatrix( std::size_t n1, std::size_t n2 );
void print( std::string label, GIDI::Matrix &m1 );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    printCodeArguments( __FILE__, argc, argv );

    int errorCount = 0;

    errorCount += checkMatrix( 1, 1 );
    errorCount += checkMatrix( 2, 2 );
    errorCount += checkMatrix( 3, 3 );
    errorCount += checkMatrix( 4, 4 );
    errorCount += checkMatrix( 5, 5 );
    errorCount += checkMatrix( 6, 6 );
    errorCount += checkMatrix( 11, 11 );
    errorCount += checkMatrix( 14, 14 );

    errorCount += checkMatrix( 2, 4 );
    errorCount += checkMatrix( 2, 5 );
    errorCount += checkMatrix( 4, 3 );
    errorCount += checkMatrix( 5, 3 );
    errorCount += checkMatrix( 5, 10 );
    errorCount += checkMatrix( 5, 11 );

    exit( errorCount );
}
/*
=========================================================
*/
int checkMatrix( std::size_t n1, std::size_t n2 ) {

    int errorCount = 0;
    GIDI::Matrix m1( n1, n2 );

    std::cout << std::endl;
    std::cout << "size = " << n1 << "  " << n2 << std::endl;
    std::cout << "Matrix 1, zeros" << std::endl;
    print( "    matrix:", m1 );
    m1.reverse( );
    print( "    reversed:", m1 );
    m1.reverse( );
    print( "    reversed again:", m1 );

    std::cout << "Matrix 1, linear" << std::endl;
    std::size_t i3 = 0;
    for( std::size_t i2 = 0; i2 < n1; ++i2 ) {
        GIDI::Vector &v1 = m1[i2];
        for( std::size_t i1 = 0; i1 < n2; ++i1, ++i3 ) v1[i1] = static_cast<double>( i3 );
    }
    print( "    matrix:", m1 );
    m1.reverse( );
    print( "    reversed:", m1 );

    GIDI::Matrix m2( n1, n2 );
    i3 = n1 * n2 - 1;
    for( std::size_t i2 = 0; i2 < n1; ++i2 ) {
        GIDI::Vector &v1 = m2[i2];
        for( std::size_t i1 = 0; i1 < n2; ++i1, --i3 ) v1[i1] = static_cast<double>( i3 );
    }
    print( "    m2 matrix:", m2 );

    for( std::size_t i2 = 0; i2 < n1; ++i2 ) {
        for( std::size_t i1 = 0; i1 < n2; ++i1 ) {
            if( m1[i2][i1] != m2[i2][i1] ) ++errorCount;
        }
    }

    m2 = m1;
    m1.reverse( );
    print( "    reversed again:", m1 );

    m1 += m2;
    print( "    summed:", m1 );

    double sum = static_cast<double>( n1 * n2 - 1 );
    for( std::size_t i2 = 0; i2 < n1; ++i2 ) {
        for( std::size_t i1 = 0; i1 < n2; ++i1 ) {
            if( m1[i2][i1] != sum ) ++errorCount;
        }
    }

    if( errorCount ) std::cout << "errorCount = " << errorCount << std::endl;

    return( errorCount );
}
/*
============================================================
*/
void print( std::string label, GIDI::Matrix &m1 ) {

    std::cout << label << std::endl;
    m1.print( "" );
}
