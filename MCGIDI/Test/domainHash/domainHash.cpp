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

#include "MCGIDI.hpp"

static int check( std::vector<double> &domain );
static int check2( std::size_t bins, double domainMin, double domainMax, std::vector<double> &domain );
/*
=========================================================
*/
int main( LUPI_maybeUnused int argc, LUPI_maybeUnused char **argv ) {

    int errCount = 0;
    std::vector<double> domain1;

    std::cerr << "    " << __FILE__ << std::endl;

    domain1.push_back( 0.12 );
    domain1.push_back( 0.13 );
    domain1.push_back( 0.14 );
    domain1.push_back( 0.2 );
    errCount += check( domain1 );

    domain1.push_back( 1.41 );
    errCount += check( domain1 );

    domain1.push_back( 2.41 );
    domain1.push_back( 2.42 );
    domain1.push_back( 4.41 );
    domain1.push_back( 14.41 );
    domain1.push_back( 24.41 );
    errCount += check( domain1 );

    domain1.push_back( 34.41 );
    domain1.push_back( 44.41 );
    domain1.push_back( 144.41 );
    errCount += check( domain1 );

    domain1.push_back( 244.41 );
    domain1.push_back( 1044.41 );
    errCount += check( domain1 );

    domain1.push_back( 1144.41 );
    domain1.push_back( 10144.41 );
    errCount += check( domain1 );

    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    errCount += check( domain1 );

    domain1.erase( domain1.begin( ) );
    errCount += check( domain1 );

    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    errCount += check( domain1 );

    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    errCount += check( domain1 );

    domain1.erase( domain1.begin( ) );
    domain1.erase( domain1.begin( ) );
    errCount += check( domain1 );

    exit( errCount );
}
/*
=========================================================
*/
static int check( std::vector<double> &domain ) {

    int errCount = 0;

    errCount += check2(  3, 1., 100., domain );
    errCount += check2(  9, 1., 100., domain );
    errCount += check2( 10, 1., 100., domain );
    errCount += check2( 11, 1., 100., domain );

    return( errCount );
}
/*
=========================================================
*/
static int check2( std::size_t bins, double domainMin, double domainMax, std::vector<double> &domain ) {

    int errCount = 0;

    std::cout << std::endl;

    MCGIDI::DomainHash domainHash( bins, domainMin, domainMax );
    domainHash.print( true );

    std::cout << "  domain.size = " << domain.size( ) << std::endl;
    for( std::size_t i1 = 0; i1 < domain.size( ); ++i1 ) std::cout << "  " << domain[i1];
    std::cout << std::endl;

    MCGIDI::Vector<double> _domain( domain );
    auto map = domainHash.map( _domain );
    std::cout << "  map.size = " << map.size( ) << std::endl;
    for( std::size_t i1 = 0; i1 < map.size( ); ++i1 ) std::cout << "  " << map[i1];
    std::cout << std::endl;

    std::size_t lastValue = 0;
    for( std::size_t i1 = 0; i1 < map.size( ); ++i1 ) {
        if( map[i1] < lastValue ) ++errCount;
        lastValue = map[i1];
    }

    for( std::size_t i1 = 0; i1 < domain.size( ); ++i1 ) {
        auto index = domainHash.index( domain[i1] );
        auto mapIndex = map[index];

        std::cout << "  " << i1 << "  index = " << index << "  energy = " << domain[i1] << std::endl;

        if( domain[mapIndex] > domain[i1] ) ++errCount;
        if( ( mapIndex < map[map.size( )-1] ) && ( index < ( map.size( ) - 1 ) ) ) {
            mapIndex = map[index+1];
            if( domain[mapIndex] < domain[i1] ) {
                ++errCount;
            }
        }
    }

    return( errCount );
}
