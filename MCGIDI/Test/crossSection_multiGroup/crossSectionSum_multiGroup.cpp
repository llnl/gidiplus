/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <set>

#include "MCGIDI.hpp"

void printVector( std::string &prefix, GIDI::Vector &vector );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    std::string mapFilename( "../../../GIDI/Test/all3T.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string neutronID( PoPI::IDs::neutron );
    std::string targetID = "O16";
    std::string protareFilename( map.protareFilename( neutronID, targetID ) );
    GIDI::ProtareSingle *protare;
    std::vector<std::string> libraries;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
        GIDI::ParticleSubstitution particleSubstitution;

        protare = new GIDI::ProtareSingle( construction, protareFilename, GIDI::FileType::XML, pops, particleSubstitution, libraries, GIDI_MapInteractionNuclearChars ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
        std::cout << "label = " << iter->heatedMultiGroup( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
    }

    std::string label( temperatures[0].heatedMultiGroup( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0.0 );

    GIDI::Transporting::Particle projectile( neutronID, groups_from_bdfls.viaLabel( "LLNL_gid_4" ) );
    projectile.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( projectile );
    particles.process( *protare, label );

    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::ProtareSingle *MCProtare;
    try {
        MCProtare = new MCGIDI::ProtareSingle( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    MCGIDI::MultiGroupHash multiGroupHash( *protare, temperatures[0] );
    std::size_t numberOfReactions = MCProtare->reactions( ).size( );

    std::cout << "              energy   group    # of non zero       cross section             delta     ratio" << std::endl;
    std::cout << "                       index       reaction       total            summed" << std::endl;
    std::cout << "                                cross sections" << std::endl;
    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 10.1 ) {
        std::cout << "temperature = " << temperature << std::endl;
        for( double energy = 1e-12; energy < 100; energy *= 1.2 ) {
            std::size_t hashIndex = multiGroupHash.index( energy );
            int numberOfNonZeroReactionCrossSections = 0;

            double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy );
            double crossSectionSum = 0;
            for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
                double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature, energy );
                if( reactionCrossSection > 0 ) ++numberOfNonZeroReactionCrossSections;
                crossSectionSum += reactionCrossSection;
            }

            double delta = crossSection - crossSectionSum;
            double ratio = 1;
            if( crossSection != 0 ) ratio = delta / crossSection;
            if( ratio < 1e-14 ) {
                delta = 0;
                ratio = 0;
            }
            std::cout << "    " << std::setw( 16 ) << energy << std::setw( 6 ) << hashIndex << "  " << std::setw( 10 ) << 
                    numberOfNonZeroReactionCrossSections << "  " << std::setw( 16 ) << crossSection << "  " << std::setw( 16 ) << 
                    crossSectionSum << "  " << std::setw( 8 ) << delta << "  " << std::setw( 8 ) << ratio;
            if( fabs( ratio ) > 1e-5 ) std::cout << " *****";
            std::cout << std::endl;
        }
    }

    delete protare;

    delete MCProtare;
}
