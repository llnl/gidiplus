/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <iostream>

#include "GIDI_testUtilities.hpp"

static std::size_t m1rows = 4, m1columns = 5;

/*
=========================================================
*/
int main( int argc, char **argv ) {

    std::size_t row, column;
    GIDI::Matrix m1( m1rows, m1columns );

    printCodeArguments( __FILE__, argc, argv );

    std::cout << std::endl;

    std::cout << "Matrix 1, zeros" << std::endl;
    m1.print( "" );

    for( row = 0; row < m1rows; ++row ) {
        for( column = 0; column < m1columns; ++column ) {
            m1.set( row, column, static_cast<double>( row * m1columns + column ) );
//            m1[row][column] = row * m1columns + column;
        }
    }
    std::cout << std::endl << "Matrix 1" << std::endl;
    m1.print( "" );

    GIDI::Matrix m2( m1rows, m1columns );
    for( row = 0; row < m1rows; ++row ) {
        for( column = 0; column < m1columns; ++column ) m2.set( row, column, m1[m1rows-row-1][m1columns-column-1]);
    }
    std::cout << std::endl << "Matrix 2" << std::endl;
    m2.print( "" );

    GIDI::Matrix m3( m1rows, m1columns );
    m3 = m1 + m2;
    std::cout << std::endl << "Matrix 3: m1 + m2" << std::endl;
    m3.print( "" );

    m3 -= m2;
    std::cout << std::endl << "Matrix 3: m3 -= m2" << std::endl;
    m3.print( "" );

    m3 -= m1;
    std::cout << std::endl << "Matrix 3: m3 -= m1" << std::endl;
    m3.print( "" );

    GIDI::Matrix m4( 0, 0 );
    std::cout << std::endl << "Matrix 4: null matrix" << std::endl;
    m4.print( "" );

    m4 += m1;
    std::cout << std::endl << "Matrix 4: null matrix +=" << std::endl;
    m4.print( "" );

    GIDI::Matrix m5( 0, 0 );
    m1 += m5;
    std::cout << std::endl << "Matrix 5: matrix += null" << std::endl;
    m1.print( "" );

    GIDI::Matrix m6( 0, 0 );
    m6 -= m1;
    std::cout << std::endl << "Matrix 6: null matrix -=" << std::endl;
    m6.print( "" );

    GIDI::Matrix m7( 0, 0 );
    m1 -= m7;
    std::cout << std::endl << "Matrix 7: matrix -= null" << std::endl;
    m1.print( "" );
}
