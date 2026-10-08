/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <iostream>

#include <PoPI.hpp>

void main2( int argc, char **argv );
void checkStaticIntid( PoPI::Database const &a_pops, std::string const &a_id, int a_intid );

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

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );

    checkStaticIntid( pops, PoPI::IDs::neutron, PoPI::Intids::neutron );
    checkStaticIntid( pops, PoPI::IDs::photon, PoPI::Intids::photon );
    checkStaticIntid( pops, PoPI::IDs::electron, PoPI::Intids::electron );
    checkStaticIntid( pops, PoPI::IDs::FissionProductENDL99120, PoPI::Intids::FissionProductENDL99120 );
    checkStaticIntid( pops, PoPI::IDs::FissionProductENDL99125, PoPI::Intids::FissionProductENDL99125 );

    int intidCounter = 0;
    auto size = pops.size( );
    for( std::size_t i1 = 0; i1 < size; ++i1 ) {
        PoPI::Base const &base = pops.get<PoPI::Base>( i1 );

        if( base.isParticle( ) ) {
            PoPI::IDBase const &particle = static_cast<PoPI::IDBase const &>( base );

            int intid = pops.intid( particle.ID( ) );
            PoPI::ParseIntidInfo intidInfo( intid );

            std::cout << particle.ID( )
                        << " " << intid
                        << " " << intidInfo.id( )
                        << std::endl;
        }

        int intid2 = base.intid( );
        if( intid2 > -1 ) {
            ++intidCounter;
            std::size_t index = 0;
            try {
                index = pops.indexFromIntid( intid2 ); }
            catch (std::exception &exception) {
                std::cerr << exception.what( ) << std::endl;
                continue;
            }
            if( index != i1 ) {
                std::cout << "index != i1 for id = " << base.ID( ) << ", intid = " << intid2 << ", i1 = " << i1 <<  " and index = " << index << std::endl;
            } }
        else {
            
        }
    }
    std::cout << "size = " << size << std::endl;
    std::cout << "intidCounter = " << intidCounter << std::endl;
}

/*
=========================================================
*/
void checkStaticIntid( PoPI::Database const &a_pops, std::string const &a_id, int a_intid ) {

    int intid = a_pops.intid( a_id );

    if( intid != a_intid ) std::cout << "For " << a_id << ", intid (" << intid << ") != a_intid (" << a_intid << ")" << std::endl;
}
