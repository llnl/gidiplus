/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <set>

#include <statusMessageReporting.h>

#include <GIDI_testUtilities.hpp>

static char const *description = "Displays the list of incomplete particles for the requested protare and each of its reactions.";
static std::string indent( "      " );

void main2( int argc, char **argv );
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

    std::vector<std::string> libraries;
    GIDI::Transporting::DelayedNeutrons delayedNeutrons( GIDI::Transporting::DelayedNeutrons::on );

    argvOptions argv_options( "targetInfo", description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );

    parseTestOptions.m_askGNDS_File = true;

    parseTestOptions.parse( );

    GIDI::Construction::PhotoMode photo_mode = parseTestOptions.photonMode( GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    PoPI::Database pops;
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", "../all.map", construction, PoPI::IDs::neutron, "O16" );
    GIDI::Transporting::Settings settings( PoPI::IDs::neutron, delayedNeutrons );

    GIDI::ProtareSingle const *protareSingle = protare->protare( 0 );
    GIDI::TargetInfo::TargetInfo const &targetInfo = protareSingle->targetInfo( );

    GUPI::WriteInfo writeInfo;
    targetInfo.toXMLList( writeInfo );
    writeInfo.print( );

    std::cout << std::endl;
    auto const &isotopicAbundances = targetInfo.isotopicAbundances( );
    auto const &chemicalElements = isotopicAbundances.chemicalElements( );
    for( auto iterChemicalElement = chemicalElements.begin( ); iterChemicalElement != chemicalElements.end( ); ++iterChemicalElement ) {
        auto const *chemicalElement = dynamic_cast<GIDI::TargetInfo::ChemicalElement *>( *iterChemicalElement );
        std::cout << "Chemical element " << chemicalElement->symbol( ) << std::endl;
        auto &nuclides = chemicalElement->nuclides( );
        for( auto iterNuclide = nuclides.begin( ); iterNuclide != nuclides.end( ); ++iterNuclide ) {
            auto const *nuclide = dynamic_cast<GIDI::TargetInfo::Nuclide *>( *iterNuclide );
            std::cout << "    " << nuclide->pid( ) << " " << nuclide->atomFraction( ) << std::endl;
        }
    }

    delete protare;

    exit( EXIT_SUCCESS );
}
