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
void energyLoop( GIDI::Protare *a_protare, PoPI::Database &a_pops, GIDI::Transporting::Particles &a_particles, MCGIDI::DomainHash &a_domainHash, 
                MCGIDI::Transporting::MC &a_MC, GIDI::Styles::TemperatureInfos a_temperatures, GIDI::ExcludeReactionsSet &a_reactionsToExclude,
                MCGIDI::URR_protareInfos &a_URR_protare_infos, bool a_printPairDiff );
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

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "photo-atomic", description );

    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "-n", false, "If present, add neutron as transporting particle." ) );
    argv_options.add( argvOption2( "-p", false, "If present, add photon as transporting particle." ) );
    argv_options.add( argvOption2( "--useSlowerContinuousEnergyConversion", false, "If present, old continuous energy conversion logic is used." ) );

    argv_options.parseArgv( argc, argv );

    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/Data/MG_MC/photo-atomic/all.map" );
    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::photon );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "H1" );

    GIDI::Map::Map map( mapFilename, pops );
    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::atomicOnly );
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

        std::cout << std::setw( 50 ) << std::left << reaction.label( ).c_str( ) << "  threshold = " 
                << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true )
                << "  threshold = " << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true ) << std::endl;
    }

    delete MCProtare;

    energyLoop( protare, pops, particles, domainHash, MC, temperatures, reactionsToExclude, URR_protare_infos, false );

    for( std::size_t reactionIndex = 0; reactionIndex < protare->numberOfReactions( ); ++reactionIndex ) {
        for( std::size_t reactionIndex2 = 0; reactionIndex2 < protare->numberOfReactions( ); ++reactionIndex2 ) {
            GIDI::Reaction *reaction = protare->reaction( reactionIndex2 );
            reaction->setActive( false );
        }
        GIDI::Reaction *reaction = protare->reaction( reactionIndex );
        reaction->setActive( true );
        energyLoop( protare, pops, particles, domainHash, MC, temperatures, reactionsToExclude, URR_protare_infos, reaction->isPairProduction( ) );
    }

    delete protare;

}

/*
=========================================================
*/
void energyLoop( GIDI::Protare *a_protare, PoPI::Database &a_pops, GIDI::Transporting::Particles &a_particles, MCGIDI::DomainHash &a_domainHash, 
                MCGIDI::Transporting::MC &a_MC, GIDI::Styles::TemperatureInfos a_temperatures, GIDI::ExcludeReactionsSet &a_reactionsToExclude,
                MCGIDI::URR_protareInfos &a_URR_protare_infos, bool a_printPairDiff ) {

    LUPI::StatusMessageReporting smr1;
    int photonIndex = static_cast<int>( a_pops[PoPI::IDs::photon] );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *a_protare, a_pops, a_MC, a_particles, a_domainHash, a_temperatures, a_reactionsToExclude );
    double temperature = 1e-8;

    std::cout << std::endl;
    std::cout << "List of reactions:" << std::endl;
    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
        std::cout << "    " << reaction->label( ).c_str( ) << std::endl;
    }
    std::cout << "  temperature = " << doubleToString2( "%8.1e", temperature ) << std::endl;
    std::cout << "          energy index    cross section deposition energy  deposition momentum  production energy";
    if( a_particles.hasParticle( PoPI::IDs::photon ) ) std::cout << "        photon gain";
    std::cout << std::endl;
    for( double energy = 1e-1; energy < 45.0; energy *= 1.4 ) {
        std::size_t hashIndex = a_domainHash.index( energy );
        double crossSection = MCProtare->crossSection( a_URR_protare_infos, hashIndex, temperature, energy );
        double depositionEnergy = MCProtare->depositionEnergy( hashIndex, temperature, energy );

        std::cout << std::setw( 16 ) << energy << std::setw( 6 ) << hashIndex;
        std::cout << doubleToString2( " %16.8e",    crossSection );
        std::cout << doubleToString2( " %16.8e",    depositionEnergy );
        std::cout << doubleToString2( "    %16.8e", MCProtare->depositionMomentum( hashIndex, temperature, energy ) );
        std::cout << doubleToString2( "    %16.8e", MCProtare->productionEnergy( hashIndex, temperature, energy ) );
        if( a_particles.hasParticle( PoPI::IDs::photon ) ) std::cout << doubleToString2( "    %16.8e", MCProtare->gain( hashIndex, temperature, energy, photonIndex ) );
        if( a_printPairDiff && ( crossSection != 0 ) ) {
            double diff = ( energy - depositionEnergy / crossSection ) / PoPI_electronMass_MeV_c2;
            std::cout << doubleToString2( "    %12.4e", diff );
        }
        std::cout << std::endl;
        if( a_particles.hasParticle( PoPI::IDs::photon ) ) checkIntidGain( MCProtare, hashIndex, temperature, energy, photonIndex, PoPI::Intids::photon );
    }

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
