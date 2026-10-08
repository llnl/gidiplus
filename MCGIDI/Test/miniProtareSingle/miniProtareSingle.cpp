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

#include <LUPI.hpp>
#include <MCGIDI.hpp>

#include <GIDI_testUtilities.hpp>
#include <MCGIDI_testUtilities.hpp>

static char const *description = "Creates an mini GIDI::ProtareSingle, loads it into an MCGIDI::ProtareSingle and checks that it works.";
//    " The interaction flag should be 'nuclear' for now.";

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
    unsigned long long rngState = 1;
    unsigned long long rngState2 = 1;
    std::map<std::string, std::string> particlesAndGIDs;
    std::string XML_outputPath;

    particlesAndGIDs[PoPI::IDs::neutron] = "LLNL_gid_4";
    particlesAndGIDs["H1"] = "LLNL_gid_71";
    particlesAndGIDs["H2"] = "LLNL_gid_71";
    particlesAndGIDs["H3"] = "LLNL_gid_71";
    particlesAndGIDs["He3"] = "LLNL_gid_71";
    particlesAndGIDs["He4"] = "LLNL_gid_71";
    particlesAndGIDs[PoPI::IDs::photon] = "LLNL_gid_70";

    GIDI::Groups groups( "../../../GIDI/Test/groups.xml" );
    GIDI::Fluxes fluxFile( "../../../GIDI/Test/fluxes.xml" );
    
    LUPI::ArgumentParser argumentParser( __FILE__, description );
    LUPI::Positional *projectileIdArgument = argumentParser.add<LUPI::Positional>( "projectileId", "The GNDS PoPs id of the projectile.", 1, 1 );
    LUPI::Positional *targetIdArgument = argumentParser.add<LUPI::Positional>( "targetId", "The GNDS PoPs id of the target.", 1, 1 );
    LUPI::Positional *interactionArgument = argumentParser.add<LUPI::Positional>( "interaction", "The protare's interaction flag.", 1, 1 );
    LUPI::OptionStore *popsOption = argumentParser.add<LUPI::OptionStore>( "--pops", "PoPs files to load.", 1, -1 );
    LUPI::OptionStore *crossSectionOption = argumentParser.add<LUPI::OptionStore>( "--crossSection", "Cross section value for all energies. Default is 1.0.", 0, -1 );
    LUPI::OptionTrue *multiGroupOption = argumentParser.add<LUPI::OptionTrue>( "--multiGroup", "If present, use multi-group data otherwise use continuous energy.", 0, -1 );
    LUPI::OptionStore *XML_outputPathOption = argumentParser.add<LUPI::OptionStore>( "--XML", "If present, the XML is written to the specifed path.", 0, -1 );

    argumentParser.parse( argc, argv );

    if( interactionArgument->value( ) != GIDI_MapInteractionNuclearChars )
        throw LUPI::Exception( "Currently, interaction argument must be 'nuclear'." );

    if( popsOption->values( ).size( ) == 0 ) pops.addFile( "../../../TestData/PoPs/pops.xml", false );
    for( std::size_t index = 0; index < popsOption->values( ).size( ); ++index ) {
        pops.addFile( popsOption->value( index ), false );
    }

    double crossSectionValue = asDouble( crossSectionOption->valueWithDefault( "0.0" ) );

    if( !pops.exists( projectileIdArgument->value( ) ) ) throw LUPI::Exception( "Projetile id not in pops." );
    if( !pops.exists( targetIdArgument->value( ) ) ) throw LUPI::Exception( "Target id not in pops." );

    if( XML_outputPathOption->values( ).size( ) != 0 ) XML_outputPath = XML_outputPathOption->value( );

    bool useMultiGroup = multiGroupOption->isTrue( );
    GIDI::Transporting::Mode mode = useMultiGroup ? GIDI::Transporting::Mode::multiGroup : 
            GIDI::Transporting::Mode::MonteCarloContinuousEnergy;
    GIDI::Functions::Function3dForm const *flux = fluxFile.get<GIDI::Functions::Function3dForm>( "LLNL_fid_1" );
    for( std::map<std::string, std::string>::iterator iter = particlesAndGIDs.begin( ); iter != particlesAndGIDs.end( ); ++iter ) {
        if( ( iter->first == projectileIdArgument->value( ) ) || iter->first == targetIdArgument->value( ) ) {
            GIDI::Group *group = groups.get<GIDI::Group>( iter->second );
            GIDI::Transporting::Particle particle( iter->first, *group, *flux, mode );
            particles.add( particle );
        }
    }

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );

    GIDI::ProtareSingle *protare = GIDI::createMiniProtareSingle( construction, pops, projectileIdArgument->value( ), 
            targetIdArgument->value( ), interactionArgument->value( ), particles, crossSectionValue, XML_outputPath );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    std::string label( "" );
    if( useMultiGroup ) {
        label = temperatures[0].heatedMultiGroup( );
        particles.process( *protare, label );
    }
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.sampleNonTransportingParticles( true );
    if( useMultiGroup ) {
        MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );
    }
    MCGIDI::MultiGroupHash multiGroupHash( *protare, particles );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare;
    MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    MCGIDI::Vector<double> MC_temperatures = MCProtare->temperatures( 0 );
    std::cout << "MCGIDI temperatures:" << std::endl;
    for( auto iter = MC_temperatures.begin( ); iter != MC_temperatures.end( ); ++iter ) {
        std::cout << "    " << *iter << std::endl;
    }
    std::cout << std::endl;

    std::cout << "List of reactions" << std::endl;
    for( std::size_t i1 = 0; i1 < MCProtare->numberOfReactions( ); ++i1 ) {
        MCGIDI::Reaction const &reaction = *(MCProtare->reaction( i1 ));

        std::cout << "    reaction: " << std::left << std::setw( 40 ) << reaction.label( ).c_str( ) << ":  final Q = " << reaction.finalQ( 0 ) 
                << " threshold = " << LUPI::Misc::doubleToString3( "%.6g", reaction.crossSectionThreshold( ), true ) << std::endl;
    }
    std::cout << std::endl;

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );
    MCGIDI::Sampling::StdVectorProductHandler products;

    std::size_t hashIndex;
    int projectileIndex = static_cast<int>( pops[protare->projectile( ).ID( )] );
    int targetIndex = static_cast<int>( pops[protare->target( ).ID( )] );
    for( double temperature = 1e-8; temperature < 1e-3; temperature *= 100.1 ) {
        std::cout << "temperature = " << temperature << std::endl;
        for( double energy = 1e-12; energy < 1010; energy *= 1000 ) {
            if( useMultiGroup ) {
                hashIndex = multiGroupHash.index( energy ); }
            else {
                hashIndex = domainHash.index( energy );
            }
            input.setTemperatureAndEnergy( temperature, energy );
    
            double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, temperature, energy );
            std::cout << doubleToString2( " energy: %10.2e",    energy );
            std::cout << doubleToString2( " %14.6e",    crossSection );
            std::cout << doubleToString2( " %14.6e",    MCProtare->depositionEnergy( hashIndex, temperature, energy ) );
            std::cout << doubleToString2( " %14.6e", MCProtare->depositionMomentum( hashIndex, temperature, energy ) );
            std::size_t reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSection,
                    [&]() -> double { return float64RNG64( &rngState ); } );
            std::cout << " sampled reaction " << reactionIndex << std::endl;

            for( long count = 0; count < 1000 * 1000; ++count ) {
                reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSection,
                    [&]() -> double { return float64RNG64( &rngState ); } );
                if( reactionIndex != 0 ) {
                    std::cout << "Oops, reactionIndex != 0 but is " << reactionIndex << std::endl;
                    break;
                }
            }

            MCGIDI::Reaction const *reaction = MCProtare->reaction( 0 );
            products.clear( );
            reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                    [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
            double totalKE = 0.0;
            for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                MCGIDI::Sampling::Product const &product = products[i2];

                std::cout << "        productIndex " << std::setw( 4 ) << product.m_productIndex << " " << std::setw( 4 )
                        << product.m_userProductIndex;
                if( product.m_sampledType == MCGIDI::Sampling::SampledType::unspecified ) {
                    std::cout << " unspecified distribution" << std::endl; }
                else {
                    std::cout << " KE = " << product.m_kineticEnergy << std::endl;
                    totalKE += product.m_kineticEnergy;
                }
            }
            std::cout << "            total KE = " << totalKE << std::endl;

            double energy_out;
            reaction->angleBiasing( projectileIndex, temperature, energy, 0.1, energy_out,
                    [&]() -> double { return float64RNG64( &rngState2 ); } );
            reaction->angleBiasing( targetIndex,     temperature, energy, 0.1, energy_out,
                    [&]() -> double { return float64RNG64( &rngState2 ); } );
        }
    }

    delete protare;

    delete MCProtare;
}
