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

#include "LUPI.hpp"
#include "MCGIDI.hpp"

#include "GIDI_testUtilities.hpp"
#include "MCGIDI_testUtilities.hpp"

static char const *description = "Loops over temperature and energy, printing the total cross section. If projectile is a photon, see options *-a* and *-n*.";

void main2( int argc, char **argv );
void printCrossSection( MCGIDI::Protare *protare, MCGIDI::URR_protareInfos const &URR_protare_info, MCGIDI::DomainHash const &domainHash, double temperature );
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

    PoPI::Database pops;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    double TNSL_maximumTemperature = 0.0;
    LUPI::StatusMessageReporting smr1;

    argvOptions argv_options( "TNSL", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askPid = false;
    parseTestOptions.m_askPhotoAtomic = false;
    parseTestOptions.m_askPhotoNuclear = false;
    parseTestOptions.m_askGNDS_File = true;

    parseTestOptions.parse( );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../../../GIDI/Test/Data/MG_MC/all_maps.map", construction, PoPI::IDs::neutron, "tnsl-Al27" );

    if( protare == nullptr ) throw std::runtime_error( "No matching protare file found." );

    std::cout << "GIDI temperatures:" << std::endl;
    GIDI::Styles::TemperatureInfos temperatureInfos = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatureInfos.begin( ); iter != temperatureInfos.end( ); ++iter ) {
        std::cout << "    label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatureInfos[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatureInfos, reactionsToExclude );

    std::cout << std::endl;
    std::cout << "Is ProtareSingle " << ( MCProtare->protareType( ) == MCGIDI::ProtareType::single ) << std::endl;
    std::cout << "Is ProtareComposite " << ( MCProtare->protareType( ) == MCGIDI::ProtareType::composite ) << std::endl;
    std::cout << "Is ProtareTNSL " << ( MCProtare->protareType( ) == MCGIDI::ProtareType::TNSL ) << std::endl;
    std::cout << "Is GNDS TNSL protareSingle " << ( MCProtare->isTNSL_ProtareSingle( ) ) << std::endl;
    std::cout << std::endl;

    if( MCProtare->protareType( ) == MCGIDI::ProtareType::TNSL ) {
        MCGIDI::ProtareTNSL *MCProtare2 = static_cast<MCGIDI::ProtareTNSL *>( MCProtare );

        TNSL_maximumTemperature = MCProtare2->TNSL_maximumTemperature( );

        std::cout << "TNSL" << std::endl;
        std::cout << "    maximumEnergy = " << MCProtare2->TNSL_maximumEnergy( ) << std::endl;
        std::cout << "    maximumTemperature = " << MCProtare2->TNSL_maximumTemperature( ) << std::endl;

        MCGIDI::Protare const *MCProtare3 = MCProtare2->TNSL( );
        MCGIDI::Vector<double> temperatures = MCProtare3->temperatures( 0 );
        std::cout << "    temperatures:" << std::endl;
        for( auto iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) std::cout << "        " << *iter << std::endl;
        std::cout << std::endl;
    }

    std::cout << std::endl;
    MCGIDI::Vector<double> temperatures = MCProtare->temperatures( 0 );
    std::cout << "MCGIDI temperatures:" << std::endl;
    for( auto iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) std::cout << "    " << *iter << std::endl;
    std::cout << std::endl;

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    std::cout << "List of reactions" << std::endl;
    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *(MCProtare->reaction( i1 ));

        std::cout << "    reaction: " << std::left << std::setw( 40 ) << reaction.label( ).c_str( ) << ":  final Q = " << reaction.finalQ( 0 ) << " threshold = " 
                  << LUPI::Misc::doubleToString3( "%.6g", reaction.crossSectionThreshold( ), true ) << std::endl;
    }

    std::vector<double> temperatures2;
    for( auto iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) temperatures2.push_back( *iter );

    if( TNSL_maximumTemperature > 1.1 * temperatures2.back( ) ) temperatures2.push_back( TNSL_maximumTemperature );
    temperatures2.push_back( 2.0 * temperatures2.back( ) );
    temperatures2.push_back( 2.0 * temperatures2.back( ) );

    double temperature1 = 0;
    for( auto iter = temperatures2.begin( ); iter != temperatures2.end( ); ++iter ) {
        printCrossSection( MCProtare, URR_protare_infos, domainHash, 0.5 * ( temperature1 + *iter ) );
        printCrossSection( MCProtare, URR_protare_infos, domainHash, *iter );
        temperature1 = *iter;
    }

    delete MCProtare;

    delete protare;
}
/*
=========================================================
*/
void printCrossSection( MCGIDI::Protare *protare, MCGIDI::URR_protareInfos const &URR_protare_infos, MCGIDI::DomainHash const &domainHash, double temperature ) {

    std::cout << std::endl;
    std::cout << "temperature = " << temperature << std::endl;
    for( double energy = 1e-12; energy < 1e-5; energy *= 1.2 ) {
        std::size_t hashIndex = domainHash.index( energy );

        double crossSection = protare->crossSection( URR_protare_infos, hashIndex, temperature, energy );
        std::cout << "    energy = " << energy << " crossSection = " << crossSection << std::endl;
    }
}
