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
void printVector( std::string &prefix, GIDI::Vector &vector );
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
    LUPI::StatusMessageReporting smr1;

    argvOptions argv_options( "crossSections", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    argv_options.add( argvOption( "--noRutherford", false, "If present, Rutherford is removed from charged particle elastic scattering." ) );

    parseTestOptions.parse( );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../../../GIDI/Test/all3T.map", construction, PoPI::IDs::neutron, "O16" );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].heatedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    if( argv_options.find( "--noRutherford" )->present( ) ) MC.setNuclearPlusCoulombInterferenceOnly( true );
    MC.sampleNonTransportingParticles( true );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    std::cout << std::endl;
    std::cout << "Is ProtareSingle " << ( MCProtare->protareType( ) == MCGIDI::ProtareType::single ) << std::endl;
    std::cout << "Is ProtareComposite " << ( MCProtare->protareType( ) == MCGIDI::ProtareType::composite ) << std::endl;
    std::cout << "Is ProtareTNSL " << ( MCProtare->protareType( ) == MCGIDI::ProtareType::TNSL ) << std::endl;
    std::cout << std::endl;

    std::cout << std::endl;
    MCGIDI::Vector<double> MC_temperatures = MCProtare->temperatures( 0 );
    std::cout << "MCGIDI temperatures:" << std::endl;
    for( auto iter = MC_temperatures.begin( ); iter != MC_temperatures.end( ); ++iter ) {
        std::cout << "    " << *iter << std::endl;
    }
    std::cout << std::endl;

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    std::cout << "List of reactions" << std::endl;
    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *(MCProtare->reaction( i1 ));

        std::cout << "    reaction: " << std::left << std::setw( 40 ) << reaction.label( ).c_str( ) << ":  final Q = " << reaction.finalQ( 0 ) 
                << " threshold = " << LUPI::Misc::doubleToString3( "%.6g", reaction.crossSectionThreshold( ), true ) << std::endl;
    }

    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 10.1 ) {
        std::cout << "temperature = " << temperature << std::endl;
        for( double energy = 1e-12; energy < 100; energy *= 1.2 ) {
            std::size_t hashIndex = domainHash.index( energy );

            double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy );
            std::cout << "    energy = " << energy << " crossSection = " << crossSection << std::endl;
        }
    }

    delete protare;

    delete MCProtare;
}
