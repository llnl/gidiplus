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
#include <iomanip>
#include <set>

#include "MCGIDI.hpp"

#include "GIDI_testUtilities.hpp"

static char const *description = "Loops over temperature and energy, printing the total cross section. If projectile is a photon, see options *--pa* and *--pn*.";

void main2( int argc, char **argv );
void read_MCGIDI_protare( PoPI::Database const &a_pops, GIDI::Protare *a_protare, GIDI::Transporting::Groups_from_bdfls &groups_from_bdfls, 
                GIDI::Transporting::Fluxes_from_bdfls &fluxes_from_bdfls, GIDI::Styles::TemperatureInfos const &a_temperatures, 
                MCGIDI::Transporting::MC &a_settings, std::size_t a_productBinary, std::string &a_outputLines );
void printProductList( PoPI::Database const &a_pops, MCGIDI::Protare * a_protare, bool a_transportablesOnly, std::string &a_outputLines );
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
void main2( int argc, char **argv ) {

    std::size_t numberOfLoops = 1 << 7;
    PoPI::Database pops;
    argvOptions argv_options( __FILE__, description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "--ENDL99120", false, "If present, ENDL two 99120 products are list for a fission reaction." ) );

    parseTestOptions.parse( );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    if( argv_options.find( "--ENDL99120" )->present( ) )
        construction.setFissionResiduals( GIDI::Construction::FissionResiduals::ENDL99120 );
    construction.setLazyParsing( false );               // This code will fail in the threading loop below unless this is false.
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../../../GIDI/Test/Data/MG_MC/all_maps.map", 
        construction, PoPI::IDs::neutron, "O16" );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC settings( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0.0 );

    std::size_t i1;
    std::vector<std::string> outputList( numberOfLoops );

#pragma omp parallel private( i1 ) shared( pops, protare, groups_from_bdfls, fluxes_from_bdfls, temperatures, settings, outputList )
{
#pragma omp for schedule( dynamic ) nowait
    for( i1 = 0; i1 < numberOfLoops; ++i1 ) {

        read_MCGIDI_protare( pops, protare, groups_from_bdfls, fluxes_from_bdfls, temperatures, settings, i1, outputList[i1] );
    }
}

    for( auto iter = outputList.begin( ); iter != outputList.end( ); ++iter ) std::cout << *iter;

    delete protare;
}
/*
=========================================================
*/
void read_MCGIDI_protare( PoPI::Database const &a_pops, GIDI::Protare *a_protare, GIDI::Transporting::Groups_from_bdfls &groups_from_bdfls, 
                GIDI::Transporting::Fluxes_from_bdfls &fluxes_from_bdfls, GIDI::Styles::TemperatureInfos const &a_temperatures, 
                MCGIDI::Transporting::MC &a_settings, std::size_t a_productBinary, std::string &a_outputLines ) {

    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    std::map<std::string, std::string> particlesAndGIDs;

    particlesAndGIDs[PoPI::IDs::neutron] = "LLNL_gid_4";
    particlesAndGIDs["H1"] = "LLNL_gid_71";
    particlesAndGIDs["H2"] = "LLNL_gid_71";
    particlesAndGIDs["H3"] = "LLNL_gid_71";
    particlesAndGIDs["He3"] = "LLNL_gid_71";
    particlesAndGIDs["He4"] = "LLNL_gid_71";
    particlesAndGIDs[PoPI::IDs::photon] = "LLNL_gid_70";

    GIDI::Transporting::Particles particles;

    std::size_t productDigit = 1;
    a_outputLines += '\n';
    for( std::map<std::string, std::string>::iterator iter = particlesAndGIDs.begin( ); iter != particlesAndGIDs.end( ); ++iter, productDigit <<= 1 ) {
        if( productDigit & a_productBinary ) {
            GIDI::Transporting::MultiGroup multi_group = groups_from_bdfls.viaLabel( iter->second );
            GIDI::Transporting::Particle particle( iter->first, multi_group );

            particle.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
            particles.add( particle );

            a_outputLines += iter->first + " ";
        }
    }
    a_outputLines += '\n';

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *a_protare, a_pops, a_settings, particles, domainHash, a_temperatures, reactionsToExclude );

    MCProtare->setUserParticleIndexViaIntid( a_pops.intid( PoPI::IDs::neutron ), 0 );
    MCProtare->setUserParticleIndexViaIntid( a_pops.intid( "H2" ), 10 );
    MCProtare->setUserParticleIndexViaIntid( a_pops.intid( PoPI::IDs::photon ), 11 );

    printProductList( a_pops, MCProtare, true, a_outputLines );
    printProductList( a_pops, MCProtare, false, a_outputLines );

    delete MCProtare;
}
/*
=========================================================
*/
void printProductList( PoPI::Database const &a_pops, MCGIDI::Protare * a_protare, bool a_transportablesOnly, std::string &a_outputLines ) {

    MCGIDI::Vector<int> intids = a_protare->productIntids( a_transportablesOnly );
    MCGIDI::Vector<int> userIndices = a_protare->userProductIndices( a_transportablesOnly );
    std::string aSpace( " " );

    a_outputLines += "    transportables only = ";
    if( a_transportablesOnly ) {
        a_outputLines += "1"; }
    else {
        a_outputLines += "0";
    }
    a_outputLines += '\n';
    for( std::size_t i1 = 0; i1 < intids.size( ); ++i1 ) {
        PoPI::ParseIntidInfo parseIntidInfo( intids[i1] );
        std::string pid = parseIntidInfo.id( );
        PoPI::Base const &particle = a_pops.get<PoPI::Base>( pid );
        std::string index = std::to_string( intids[i1] );

        a_outputLines += "        ";
        a_outputLines +=  index;
        a_outputLines += " ";
        a_outputLines += particle.ID( );
        a_outputLines += " ";
        index = std::to_string( userIndices[i1] );
        a_outputLines += index;
        a_outputLines += '\n';
    }
}
