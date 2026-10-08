/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <MCGIDI.hpp>
#include <MCGIDI_testUtilities.hpp>

static char const *description = "This code calculates the reaction rate for a projectile with a Maxwellian distribution heading a target at the same temperature using upscatter model A.";

void main2( int argc, char **argv );
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

    long numberOfSamples = -1;
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Transporting::Particles particles;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    unsigned long long rngState = 1;
    MCGIDI::URR_protareInfos URR_protare_infos;
    char *endChar;

    argvOptions2 argv_options( "sampleReactions_multiGroup", description );

    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "--temperature", true, "The temperature of the target material." ) );
    argv_options.add( argvOption2( "-n", true, "The number of calls to sampleReaction. If negative, multiplied by minus one million. Default is -1." ) );
    argv_options.add( argvOption2( "--gid", true, "The group id to use for the upscater model A cross section." ) );
    argv_options.add( argvOption2( "-v", false, "Verbosity flag. If at least two entered, the multi-group cross section used for upscatter model A will be printed." ) );

    argv_options.parseArgv( argc, argv );

    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, "H2" );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "H2" );
    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );
    GIDI::Map::Map map( mapFilename, pops );

    std::string temperatureString = argv_options.find( "--temperature" )->zeroOrOneOption( argv, "1e-3" );
    double temperature_MeV = strtod( temperatureString.c_str( ), &endChar );
    if( *endChar != 0 ) throw "Invalid temperature input.";

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Protare *protare = map.protare( construction, pops, projectileID, targetID );
    if( argv_options.find( "-v" )->present( ) ) 
        std::cout << protare->realFileName( ) << std::endl;

    for( int reactionIndex = 0; reactionIndex < (int) protare->numberOfReactions( ); ++reactionIndex ) {
        GIDI::Reaction *reaction = protare->reaction( reactionIndex );
        if( reactionIndex == 0 ) reaction->setActive( false );
    }
    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setUpscatterModelA( );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    std::string gid = argv_options.find( "--gid" )->zeroOrOneOption( argv, "" );
    if( gid != "" ) {
        GIDI::Transporting::MultiGroup groupBoundaries = groups_from_bdfls.viaLabel( gid );
        MC.setUpscatterModelAGroupBoundaries( groupBoundaries.boundaries( ) );
    }

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    URR_protare_infos.setup( protares );

    MCGIDI::Sampling::StdVectorProductHandler products;
    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::A );

    int numberOfReactions = (int) MCProtare->numberOfReactions( );
    std::vector<double> reactionRate( numberOfReactions + 1, 0 );

    if( argv_options.find( "-v" )->present( ) ) {
        std::cout << "List of reactions:" << std::endl;
        for( int reactionIndex = 0; reactionIndex < numberOfReactions; ++reactionIndex ) {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );

            std::cout << LUPI::Misc::argumentsToString( "    %5d %-32s %g", reactionIndex, reaction->label( ).c_str( ), 
                    MCProtare->threshold( reactionIndex ) ) << std::endl;
        }
        std::cout << std::endl;
    }

    numberOfSamples = argv_options.find( "-n" )->asLong( argv, numberOfSamples );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    if( argv_options.find( "-v" )->present( ) )
        std::cout << "numberOfSamples = " << numberOfSamples << std::endl;

    double densityFactor = 1e-24 * MCGIDI_speedOfLight_cm_sec;
    double projectileThermalSpeed = MCGIDI_particleBeta( MCProtare->projectileMass( ), temperature_MeV );
    double averageEnergy = 0.0;
    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
        double beta = projectileThermalSpeed * sampleBetaFromMaxwellian( [&]() -> double { return float64RNG64( &rngState ); } );
        double energy = MCGIDI::particleKineticEnergy( MCProtare->projectileMass( ), beta );
        averageEnergy += energy;
        int hashIndex = domainHash.index( energy );
        input.setTemperatureAndEnergy( temperature_MeV, energy );
        double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature_MeV, energy );
        int reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSection, 
                [&]() -> double { return float64RNG64( &rngState ); } );
        if( reactionIndex > numberOfReactions ) reactionIndex = numberOfReactions;
        reactionRate[reactionIndex] += crossSection * beta * densityFactor;
    }

    std::cout << "      " << LUPI::Misc::argumentsToString( " %12.4e", temperature_MeV );
    for( std::size_t i1 = 0; i1 < reactionRate.size( ); ++i1 ) {
        std::cout << LUPI::Misc::argumentsToString( " %12.4e", reactionRate[i1] / numberOfSamples );
    }
    std::cout << std::endl;

    if( argv_options.find( "-v" )->present( ) )
        std::cout << std::endl << "Average projectile energy = " << LUPI::Misc::argumentsToString( " %12.4e", averageEnergy / numberOfSamples ) 
                << std::endl;
    if( argv_options.find( "-v" )->m_counter > 1 ) {
        MCGIDI::ProtareSingle *MCProtareSingle = MCProtare->protare( 0 );
        auto upscatterModelACrossSection = MCProtareSingle->upscatterModelACrossSection( );
        std::cout << std::endl << "Upscatter model A multi-group cross section: " << upscatterModelACrossSection.size( ) << std::endl;
        std::size_t index = 0;
        for( ; index < upscatterModelACrossSection.size( ); ++index ) {
            std::cout << LUPI::Misc::argumentsToString( " %18.10e", MCProtareSingle->upscatterModelAGroupEnergies( )[index] )
                    << LUPI::Misc::argumentsToString( " %18.10e", upscatterModelACrossSection[index] )
                    << LUPI::Misc::argumentsToString( " %18.10e", MCProtareSingle->upscatterModelAGroupVelocities( )[index] ) << std::endl;
            std::cout << LUPI::Misc::argumentsToString( " %18.10e", MCProtareSingle->upscatterModelAGroupEnergies( )[index+1] )
                    << LUPI::Misc::argumentsToString( " %18.10e", upscatterModelACrossSection[index] )
                    << LUPI::Misc::argumentsToString( " %18.10e", MCProtareSingle->upscatterModelAGroupVelocities( )[index+1] ) << std::endl;
        }
    }

    delete protare;
    delete MCProtare;

    exit( EXIT_SUCCESS );
}
