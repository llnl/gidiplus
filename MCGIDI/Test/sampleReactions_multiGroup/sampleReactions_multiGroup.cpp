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

static char const *description = "Loops over energy at the specified temperature, sampling reactions. If projectile is a photon, see options *-pa* and *-pn*.";

/*
=========================================================
*/
int main( int argc, char **argv ) {

    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Protare *protare;
    GIDI::Transporting::Particles particles;
    char *endChar;
    long numberOfSamples = -1;
    unsigned long long rngState = 1;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    GIDI::Construction::PhotoMode photo_mode = GIDI::Construction::PhotoMode::nuclearOnly;
    argvOption2 *option;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    argvOptions2 argv_options( "sampleReactions_multiGroup", description );

    argv_options.add( argvOption2( "--pid", true, "The PoPs id of the projectile." ) );
    argv_options.add( argvOption2( "--tid", true, "The PoPs id of the target." ) );
    argv_options.add( argvOption2( "--map", true, "The map file to use." ) );
    argv_options.add( argvOption2( "-pa", false, "Include photo-atomic protare if relevant. If present, disables photo-nuclear unless *-pn* present." ) );
    argv_options.add( argvOption2( "-pn", false, "Include photo-nuclear protare if relevant. This is the default unless *-pa* present." ) );
    argv_options.add( argvOption2( "--gid", true, "The group id of the projectile. Default is 'LLNL_gid_4' (LLNL_gid_70) for neutron (photon) as the projectile." ) );
    argv_options.add( argvOption2( "--temperature", true, "The temperature of the target material." ) );
    argv_options.add( argvOption2( "--energyMin", true, "The minimum energy for the energy loop. Default is 1e-12." ) );
    argv_options.add( argvOption2( "--energyMax", true, "The maximum energy for the energy loop. Default is 25." ) );
    argv_options.add( argvOption2( "-n", true, "The number of calls to sampleReaction. If negative, multiplied by minus one million. Default is -1." ) );
    argv_options.add( argvOption2( "--quiet", false, "If present, some information is not printed." ) );
    argv_options.add( argvOption2( "--showReactions", false, "If present, the list of reactions is printed." ) );

    argv_options.parseArgv( argc, argv );

    std::string projectileID = argv_options.find( "--pid" )->zeroOrOneOption( argv, PoPI::IDs::neutron );
    std::string targetID = argv_options.find( "--tid" )->zeroOrOneOption( argv, "O16" );
    std::string mapFilename = argv_options.find( "--map" )->zeroOrOneOption( argv, "../../../GIDI/Test/all3T.map" );

    if( argv_options.find( "-pa" )->present( ) ) {
        photo_mode = GIDI::Construction::PhotoMode::atomicOnly;
        if( argv_options.find( "-pn" )->present( ) ) photo_mode = GIDI::Construction::PhotoMode::nuclearAndAtomic;
    }

    GIDI::Map::Map map( mapFilename, pops );

    std::string temperatureString = argv_options.find( "--temperature" )->zeroOrOneOption( argv, "2.58522e-8" );
    double temperature = strtod( temperatureString.c_str( ), &endChar );
    if( *endChar != 0 ) throw "Invalid temperature input.";

    try {
        GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, photo_mode );
        protare = map.protare( construction, pops, projectileID, targetID ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    if( !argv_options.find( "--quiet" )->present( ) ) {
        for( GIDI::Styles::TemperatureInfos::const_iterator iter = temperatures.begin( ); iter != temperatures.end( ); ++iter ) {
            std::cout << "label = " << iter->heatedCrossSection( ) << "  temperature = " << iter->temperature( ).value( ) << std::endl;
        }
    }

    std::string label( temperatures[0].heatedMultiGroup( ) );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0.0 );

    std::string gid( "LLNL_gid_4" );
    if( projectileID == PoPI::IDs::photon ) gid = "LLNL_gid_70";
    gid = argv_options.find( "--gid" )->zeroOrOneOption( argv, gid );

    GIDI::Transporting::MultiGroup multi_group = groups_from_bdfls.viaLabel( gid );
    GIDI::Transporting::Particle projectile( projectileID, multi_group );
    projectile.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( projectile );
    particles.process( *protare, label );

    MCGIDI::Transporting::MC MC( pops, projectileID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );
    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    try {
        MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude ); }
    catch (char const *str) {
        std::cout << str << std::endl;
        exit( EXIT_FAILURE );
    }

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    std::size_t numberOfReactions = MCProtare->numberOfReactions( );
    MCGIDI::MultiGroupHash multiGroupHash( *protare, particles );

    std::cout << "Reaction info" << std::endl;
    std::cout << "                                                 threshold   threshold  offset" << std::endl;
    MCGIDI::ProtareSingle const *prior_protare = nullptr;
    std::size_t i2 = 0;
    for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
        MCGIDI::Reaction const &reaction = *MCProtare->reaction( i1 );
        MCGIDI::ProtareSingle const *protare_single = MCProtare->protareWithReaction( i1 );
        MCGIDI::HeatedCrossSectionMultiGroup const &heated_multigroup_cross_sections = *protare_single->heatedMultigroupCrossSections( ).heatedCrossSections( )[0];

        ++i2;
        if( prior_protare != protare_single ) i2 = 0;
        prior_protare = protare_single;

        std::cout << std::setw( 5 ) << i1 << " " << std::setw( 40 ) << reaction.label( ).c_str( ) << std::setw( 12 ) 
                << LUPI::Misc::doubleToString3( "%12.6g", reaction.crossSectionThreshold( ), true )
                << LUPI::Misc::doubleToString3( "%12.6g", MCProtare->threshold( i1 ), true )
                << std::setw( 8 ) << heated_multigroup_cross_sections.thresholdOffset( i2 ) << std::endl;
    }

    std::cout << "Group boundaries" << std::endl;
    std::cout << "  index    boundary  cross section      augmented    per-cent" << std::endl;
    std::cout << "                                    cross section      change" << std::endl;
    MCGIDI::Vector<double> const &boundaries( multiGroupHash.boundaries( ) );

    for( std::size_t i1 = 0; i1 < MCProtare->numberOfProtares( ); ++i1 ) {
        MCGIDI::ProtareSingle const *protare_single = MCProtare->protare( i1 );
        MCGIDI::HeatedCrossSectionMultiGroup const &heatedCrossSectionMultiGroup = *protare_single->heatedMultigroupCrossSections( ).heatedCrossSections( )[0];

        for( std::size_t i3 = 0; i3 < boundaries.size( ); ++i3 ) {
            std::size_t i4 = i3;

            if( i3 >= ( boundaries.size( ) - 1 ) ) i4 = boundaries.size( ) - 2;
            MCGIDI::HeatedCrossSectionsMultiGroup const &heated_cross_sections = protare_single->heatedMultigroupCrossSections( );
            double crossSection = heated_cross_sections.crossSection( i4, 0.0 );
            double augmentedCrossSection = heated_cross_sections.crossSection( i4, 0.0, true );
            double percentChange = 0.0;

            if( crossSection != 0.0 ) percentChange = 100 * ( augmentedCrossSection / crossSection - 1.0 );

            std::cout << LUPI::Misc::argumentsToString( "%7d", i3 ) << LUPI::Misc::argumentsToString( " %11.7g", boundaries[i3] )
                    << LUPI::Misc::argumentsToString( "%15.10g", crossSection ) 
                    << LUPI::Misc::argumentsToString( "%15.10g", augmentedCrossSection ) << LUPI::Misc::argumentsToString( " %11.3g", percentChange );

            for( std::size_t i5 = 0; i5 < protare_single->numberOfReactions( ); ++i5 ) {
                MCGIDI::HeatedReactionCrossSectionMultiGroup const &reaction = *heatedCrossSectionMultiGroup[i5];

                if( reaction.offset( ) == i3 ) std::cout << "  " << reaction.augmentedThresholdCrossSection( ) << " (" << i5 << ")";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "temperature = " << temperature << std::endl;

    option = argv_options.find( "-n" );
    numberOfSamples = option->asLong( argv, numberOfSamples );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    std::cout << "numberOfSamples = " << numberOfSamples << std::endl;

    option = argv_options.find( "--energyMin" );
    double energyMin = option->asDouble( argv, 1e-12 );
    option = argv_options.find( "--energyMax" );
    double energyMax = option->asDouble( argv, multi_group.boundaries( ).back( ) );

    std::cout << "      ";
    for( std::size_t i5 = 0; i5 < MCProtare->numberOfReactions( ); ++i5 ) {
        std::cout << LUPI::Misc::argumentsToString( " %9d", static_cast<int>( i5 ) );
    }
    std::cout << std::endl;

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );

    for( double energy = energyMin; energy <= energyMax; energy *= 2.0 ) {
        std::size_t hashIndex = multiGroupHash.index( energy );

        std::cout << "energy = " << std::setw( 15 ) << std::setprecision( 10 ) << energy << "  group index = " << std::setw( 4 ) << hashIndex << std::endl;
        double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy );

        if( crossSection == 0.0 ) continue;

        double crossSectionAugmented = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy, true );
        std::cout << "      crossSection = " << std::setw( 12 ) << crossSection << "  crossSectionAugmented = " << std::setw( 12 ) << crossSectionAugmented << 
                " ratio = " << crossSectionAugmented / crossSection << std::endl;
        std::cout << "      ";
        for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
            double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature, energy );
            std::cout << LUPI::Misc::argumentsToString( " %9.6f", reactionCrossSection / crossSection );
        }
        std::cout << std::endl;

        std::cout << "      ";
        for( std::size_t i1 = 0; i1 < numberOfReactions; ++i1 ) {
            double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, temperature, energy, true );
            std::cout << LUPI::Misc::argumentsToString( " %9.6f", reactionCrossSection / crossSection );
        }
        std::cout << std::endl;

        input.setTemperatureAndEnergy( temperature, energy );
        std::vector<long> counts( numberOfReactions + 2, 0 );               // 2 extra for null reaction and crossSection more than sum over reactions.
        for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
            std::size_t reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSectionAugmented,
                    [&]() -> double { return float64RNG64( &rngState ); } );
            if( reactionIndex == MCGIDI_nullReaction ) {
                reactionIndex = numberOfReactions + 1; }                                    // Null reaction.
            else {
                if( reactionIndex >  numberOfReactions ) reactionIndex = numberOfReactions;              // This should not happend.
            }
            ++counts[reactionIndex];
        }

        std::cout << "      ";
        for( std::size_t i1 = 0; i1 < numberOfReactions + 2; ++i1 ) {
            double ratio = static_cast<double>( counts[i1] );
            std::cout << LUPI::Misc::argumentsToString( " %9.6f", ratio / static_cast<double>( numberOfSamples ) );
        }
        std::cout << std::endl;

        std::cout << "      ";
        for( std::size_t i1 = 0; i1 < numberOfReactions + 2; ++i1 ) {
            std::cout << LUPI::Misc::argumentsToString( " %9ld", counts[i1] );
        }
        std::cout << std::endl;
    }

    delete protare;

    delete MCProtare;
}
