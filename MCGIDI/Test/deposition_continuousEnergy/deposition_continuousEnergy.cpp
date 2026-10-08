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

#include "MCGIDI_testUtilities.hpp"

static char const *description = "Loops over temperature and energy, printing the total cross section. If projectile is a photon, see options *-pa* and *-pn*.";

void main2( int argc, char **argv );
void checkIntidGain( MCGIDI::Protare *MCProtare, std::size_t a_hashIndex, double a_temperature, double a_energy, int a_index, int a_intid );
/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv );
        exit( EXIT_SUCCESS ); }
    catch (std::exception &exception) {
        std::cerr << exception.what( ) << std::endl; }
    catch (char const *str) {
        std::cout << str << std::endl; }
    catch (std::string &str) {
        std::cout << str << std::endl;
    }
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    PoPI::Database pops;
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    GIDI::Construction::PhotoMode photo_mode = GIDI::Construction::PhotoMode::nuclearOnly;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "deposition_continuousEnergy", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pops", true, "A PoPs file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--pa", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-n* present." ) );
    argv_options.add( argvOption2( "--pn", false, "Include photo-nuclear protare if relevant. This is the default unless *-a* present." ) );
    argv_options.add( argvOption2( "-n", false, "If present, add neutron as transporting particle." ) );
    argv_options.add( argvOption2( "-p", false, "If present, add photon as transporting particle." ) );
    argv_options.add( argvOption2( "-d", false, "If present, fission delayed neutrons are included with product sampling." ) );
    argv_options.add( argvOption2( "--useSlowerContinuousEnergyConversion", false, "If present, old continuous energy conversion logic is used." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );

    argvOption2 *popsOption = argv_options.find( "--pops" );
    if( popsOption->present( ) ) {
        for( std::size_t index = 0; index < static_cast<std::size_t>( popsOption->m_counter ); ++index ) {
            pops.addFile( argv[popsOption->m_indices[index]], false );
        } }
    else {
        pops.addFile( "../../../TestData/PoPs/pops.xml", false );
    }

    if( argv_options.find( "--pa" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "--pn" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    GIDI::Map::Map map( mapFilename, pops );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
    protare = map.protare( construction, pops, projectileID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );

    if( ( projectileID == PoPI::IDs::neutron ) || argv_options.find( "-n" )->present( ) ) {
        GIDI::Transporting::Particle neutron( PoPI::IDs::neutron );
        particles.add( neutron );
    }

    if( ( projectileID == PoPI::IDs::photon ) || argv_options.find( "-p" )->present( ) ) {
        GIDI::Transporting::Particle photon( PoPI::IDs::photon );
        particles.add( photon );
    }

    GIDI::Transporting::DelayedNeutrons delayedNeutrons = GIDI::Transporting::DelayedNeutrons::off;
    if( argv_options.find( "-d" )->present( ) ) delayedNeutrons = GIDI::Transporting::DelayedNeutrons::on;

    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, delayedNeutrons, 20.0 );

    MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::continuousEnergy );
    MC.setUseSlowerContinuousEnergyConversion( argv_options.find( "--useSlowerContinuousEnergyConversion" )->present( ) );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *MCProtare->reaction( i1 );

        std::cout << std::setw( 40 ) << reaction.label( ).c_str( ) << "  threshold = " 
                << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true )
                << "  threshold = " << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true ) << std::endl;
    }

    std::cout << "List of reactions:" << std::endl;
    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *MCProtare->reaction( i1 );

        std::cout << "    reaction: " << reaction.label( ).c_str( ) << std::endl;
    }

    int neutronIndex = static_cast<int>( pops[PoPI::IDs::neutron] );
    int photonIndex = static_cast<int>( pops[PoPI::IDs::photon] );
    for( double temperature = 1e-8; temperature < 2e-3; temperature *= 100.0 ) {
        std::cout << "temperature = " << doubleToString2( "%8.1e", temperature ) << "                          cross section deposition energy  deposition momentum  production energy";
        if( particles.hasParticle( PoPI::IDs::neutron ) ) std::cout << "      neutron gain";
        if( particles.hasParticle( PoPI::IDs::photon ) ) std::cout << "         photon gain";
        std::cout << std::endl;
        for( double energy = 1e-12; energy < 45.0; energy *= 2.0 ) {
            std::size_t hashIndex = domainHash.index( energy );

            std::cout << "    energy = " << std::setw( 16 ) << energy << " index = " << std::setw( 6 ) << hashIndex;
            std::cout << doubleToString2( " %16.8e",    MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy ) );
            std::cout << doubleToString2( " %16.8e",    MCProtare->depositionEnergy( hashIndex, temperature, energy ) );
            std::cout << doubleToString2( "    %16.8e", MCProtare->depositionMomentum( hashIndex, temperature, energy ) );
            std::cout << doubleToString2( "    %16.8e", MCProtare->productionEnergy( hashIndex, temperature, energy ) );
            if( particles.hasParticle( PoPI::IDs::neutron ) ) std::cout << doubleToString2( "    %16.8e", MCProtare->gain( hashIndex, temperature, energy, neutronIndex ) );
            if( particles.hasParticle( PoPI::IDs::photon ) ) std::cout << doubleToString2( "    %16.8e", MCProtare->gain( hashIndex, temperature, energy, photonIndex ) );
            std::cout << std::endl;

            if( particles.hasParticle( PoPI::IDs::neutron ) ) checkIntidGain( MCProtare, hashIndex, temperature, energy, neutronIndex, PoPI::Intids::neutron );
            if( particles.hasParticle( PoPI::IDs::photon ) ) checkIntidGain( MCProtare, hashIndex, temperature, energy, photonIndex, PoPI::Intids::photon );
        }
    }

    delete protare;

    delete MCProtare;
}

/*
=========================================================
*/
void checkIntidGain( MCGIDI::Protare *MCProtare, std::size_t a_hashIndex, double a_temperature, double a_energy, int a_index, int a_intid ) {

    double gainIndex = MCProtare->gain( a_hashIndex, a_temperature, a_energy, a_index );
    double gainIntid = MCProtare->gainViaIntid( a_hashIndex, a_temperature, a_energy, a_intid );

    if( gainIndex != gainIntid ) 
        std::cout << "ERROR: gain and gainViaIntid difference (" << gainIndex << " vs " << gainIntid << ") for intid = " << a_intid 
                << " at energy " << a_energy << std::endl;
}
