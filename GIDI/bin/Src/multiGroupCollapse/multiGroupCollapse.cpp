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

#include <GIDI.hpp>

static char const *description = "";
static std::string productId;
static std::string projectileId;

void main2( int argc, char **argv );
void printVector( std::string const &a_label, GIDI::Vector const &a_vector, GIDI::Transporting::Settings const &a_settings, 
                GIDI::Transporting::Particles const &a_particles, double a_temperature );
void printMatrix( std::string const &a_label, GIDI::Matrix const &a_matrix, GIDI::Transporting::Settings const &a_settings,
                GIDI::Transporting::Particles const &a_particles, double a_temperature );

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

    LUPI::StatusMessageReporting smr;
    PoPI::Database pops;
    double temperature = 0.0;
    std::size_t lOrder = 0;

    LUPI::ArgumentParser argumentParser( __FILE__, description );
    LUPI::Positional *popsPositional = argumentParser.add<LUPI::Positional>( "pops", "The path to the pops file to use.", 1, 1 );
    LUPI::Positional *mapPositional = argumentParser.add<LUPI::Positional>( "map", "The path to the map file to use.", 1, 1 );
    LUPI::Positional *projectilePositional = argumentParser.add<LUPI::Positional>( "pid", "The protare's projectile id.", 1, 1 );
    LUPI::Positional *targetPositional = argumentParser.add<LUPI::Positional>( "tid", "The protare's target id.", 1, 1 );
    LUPI::Positional *productPositional = argumentParser.add<LUPI::Positional>( "oid", "The product's projectile id.", 1, 1 );
    LUPI::Positional *gidFileArgument = argumentParser.add<LUPI::Positional>( "groupsFile", "The path to the groups file.", 1, 1 );
    LUPI::Positional *gid1Argument = argumentParser.add<LUPI::Positional>( "projectileGID", "The projectile's collapse group id.", 1, 1 );
    LUPI::Positional *gid2Argument = argumentParser.add<LUPI::Positional>( "productGID", "The product's collapse group id.", 1, 1 );
    LUPI::Positional *fluxFileArgument = argumentParser.add<LUPI::Positional>( "fluxesFile", "The path to the fluxes file.", 1, 1 );
    LUPI::Positional *fidArgument = argumentParser.add<LUPI::Positional>( "fluxId", "The flux id for collapsing.", 1, 1 );

    LUPI::OptionStore *lOrderOption = argumentParser.add<LUPI::OptionStore>( "-l", "The Legendre order of the transport correction and matrices printed.", -1, 1 );
    LUPI::OptionStore *transportCorrectionOption = argumentParser.add<LUPI::OptionStore>( "-t", "The transport correction type (0: None, 1: Pendlebury/Underhill.", -1, 1 );

    argumentParser.parse( argc, argv, false );

    projectileId = projectilePositional->value( );
    std::string targetId( targetPositional->value( ) );
    productId = productPositional->value( );

    pops.addFile( popsPositional->value( ), false );

    GIDI::Map::Map map( mapPositional->value( ), pops );

    if( lOrderOption->counts( ) > 0 ) {
        LUPI::Misc::stringToSize_t( lOrderOption->value( ), lOrder );
    }

    int transportCorrection = 0;
    GIDI::TransportCorrectionType transportCorrectionType( GIDI::TransportCorrectionType::None );
    if( transportCorrectionOption->counts( ) > 0 ) {
        LUPI::Misc::stringToInt( transportCorrectionOption->value( ), transportCorrection );
    }
    switch( transportCorrection ) {
    case 0:
        break;
    case 1:
        transportCorrectionType = GIDI::TransportCorrectionType::Pendlebury;
        break;
    default:
        throw "Invalid transjport correction option.";
    }

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Protare *protare = map.protare(construction, pops, projectileId, targetId, "", "", false, false );

    GIDI::Groups groups( gidFileArgument->value( ) );

    GIDI::Group group1 = *groups.get<GIDI::Group>( gid1Argument->value( ) );
    GIDI::Group group2 = *groups.get<GIDI::Group>( gid2Argument->value( ) );

    GIDI::Fluxes fluxes( fluxFileArgument->value( ) );
    GIDI::Functions::Function3dForm const *fluxFunction3d = fluxes.get<GIDI::Functions::Function3dForm>( fidArgument->value( ) );
    auto fluxVsTemperature( settingsFluxesFromFunction3d( *fluxFunction3d ) );

    GIDI::Transporting::Particles particles;

    GIDI::Transporting::Particle projectile( projectileId, group1 );
    projectile.appendFlux( fluxVsTemperature[0] );
    particles.add( projectile );

    GIDI::Transporting::Particle product( productId, group2 );
    particles.add( product );

    GIDI::Styles::TemperatureInfos temperatureInfos = protare->temperatures( );
    GIDI::Styles::TemperatureInfo &temperatureInfo = temperatureInfos[0];
    particles.process( *protare, temperatureInfo.heatedMultiGroup( ) );

    std::cout << "# projectile = " << projectileId << std::endl;
    std::cout << "# target = " << targetId << std::endl;
    std::cout << "# product = " << productId << std::endl;
    std::cout << "# GNDS file = " << protare->realFileName( ) << std::endl;

    std::cout << "#" << std::endl;
    std::cout << "# projectile file gid = " << particles.particle( projectileId )->fineMultiGroup( ).label( ) << std::endl;
    std::cout << "# product file gid = " << particles.particle( productId )->fineMultiGroup( ).label( ) << std::endl;
    std::cout << "# Groups file = " <<  LUPI::FileInfo::realPath( gidFileArgument->value( ) ) << std::endl;
    std::cout << "# projectile collapse gid = " << gid1Argument->value( ) << std::endl;
    std::cout << "# product collapse gid = " << gid2Argument->value( ) << std::endl;

    std::cout << "#" << std::endl;
    std::cout << "# Flux file = " << LUPI::FileInfo::realPath( fluxFileArgument->value( ) ) << std::endl;
    std::cout << "# Flux id = " << fidArgument->value( ) << std::endl;

    GIDI::Transporting::MG settings( protare->projectile( ).ID( ), GIDI::Transporting::Mode::multiGroup, GIDI::Transporting::DelayedNeutrons::off );

    printVector( "Inverse speed", protare->multiGroupInverseSpeed( smr, settings, temperatureInfo ), settings, particles, temperature );
    printVector( "Cross section", protare->multiGroupCrossSection( smr, settings, temperatureInfo ), settings, particles, temperature );
    printVector( "Q", protare->multiGroupQ(                        smr, settings, temperatureInfo, true ), settings, particles, temperature );
    printVector( "multiplicity", protare->multiGroupMultiplicity( smr, settings, temperatureInfo, productId ), settings, particles, temperature );
    printVector( "fission neutron multiplicity", protare->multiGroupFissionNeutronMultiplicity( smr, settings, temperatureInfo ), settings, particles, temperature );
    printVector( "fission photon multiplicity", protare->multiGroupFissionGammaMultiplicity( smr, settings, temperatureInfo ), settings, particles, temperature );

    printVector( "Available energy", protare->multiGroupAvailableEnergy( smr, settings, temperatureInfo ), settings, particles, temperature );
    printVector( "Average energy", protare->multiGroupAverageEnergy( smr, settings, temperatureInfo, productId ), settings, particles, temperature );
    printVector( "Deposition energy", protare->multiGroupDepositionEnergy( smr, settings, temperatureInfo, particles ), settings, particles, temperature );

    printVector( "Available momentum", protare->multiGroupAvailableMomentum( smr, settings, temperatureInfo ), settings, particles, temperature );
    printVector( "Average momentum", protare->multiGroupAverageMomentum( smr, settings, temperatureInfo, productId ), settings, particles, temperature );
    printVector( "Deposition momentum", protare->multiGroupDepositionMomentum( smr, settings, temperatureInfo, particles ), settings, particles, temperature );

    printVector( "Gain", protare->multiGroupGain( smr, settings, temperatureInfo, productId ), settings, particles, temperature );

    printMatrix( "Product matrix", protare->multiGroupProductMatrix( smr, settings, temperatureInfo, particles, productId, lOrder ), settings, particles, temperature );
    printMatrix( "Fission matrix", protare->multiGroupFissionMatrix( smr, settings, temperatureInfo, particles, lOrder ), settings, particles, temperature );

    printVector( "Transport correction", protare->multiGroupTransportCorrection( smr, settings, temperatureInfo, particles, lOrder, 
            transportCorrectionType, temperature ), settings, particles, temperature );

    delete protare;
}

/*
=========================================================
*/
void printVector( std::string const &a_label, GIDI::Vector const &a_vector, GIDI::Transporting::Settings const &a_settings, 
                GIDI::Transporting::Particles const &a_particles, double a_temperature ) {

    GIDI::Transporting::Particle const *projectile = a_particles.particle( projectileId );
    auto boundaries = projectile->multiGroup( ).boundaries( );

    GIDI::Vector collapsed = GIDI::collapse( a_vector, a_settings, a_particles, a_temperature );

    std::cout << std::endl << std::endl;
    std::cout << "#   " << a_label << ":" << std::endl;
    std::cout << "## vector::: " << boundaries.size( ) << std::endl;
    std::vector<double> data = collapsed.data( );
    data.push_back( data.back( ) );
    for( std::size_t index = 0; index < data.size( ); ++index ) {
        std::cout << "        " <<  LUPI::Misc::argumentsToString( "%16.9e %16.9e", boundaries[index], data[index] ) << std::endl;
    }
}

/*
=========================================================
*/
void printMatrix( std::string const &a_label, GIDI::Matrix const &a_matrix, GIDI::Transporting::Settings const &a_settings,
                GIDI::Transporting::Particles const &a_particles, double a_temperature ) {

    GIDI::Transporting::Particle const *projectile = a_particles.particle( projectileId );
    auto boundaries1 = projectile->multiGroup( ).boundaries( );
    GIDI::Transporting::Particle const *product = a_particles.particle( productId );
    auto boundaries2 = product->multiGroup( ).boundaries( );

    GIDI::Matrix collapsed = GIDI::collapse( a_matrix, a_settings, a_particles, a_temperature, productId );

    std::cout << std::endl << std::endl;
    std::cout << "#   " << a_label << ":" << std::endl;
    std::cout << "## matrix::: " << boundaries1.size( ) << " " << boundaries2.size( ) << std::endl;

    std::cout << "## matrix-column-header:       ";
    for( auto iter = boundaries2.begin( ); iter != boundaries2.end( ); ++iter ) 
        std::cout << LUPI::Misc::argumentsToString( " %16.9e", *iter );
    std::cout << std::endl;
    GIDI::Vector row;
    for( std::size_t index = 0; index < collapsed.size( ); ++index ) {
        row = collapsed[index];
        auto data = row.data( );
        data.push_back( data.back( ) );
        std::cout << "## matrix-row: " << LUPI::Misc::argumentsToString( "%16.9e", boundaries1[index] );
        for( auto iter = data.begin( ); iter != data.end( ); ++iter ) {
            std::cout << LUPI::Misc::argumentsToString( " %16.9e", *iter );
        }
        std::cout << std::endl;
    }
    std::cout << "## matrix-row: " << LUPI::Misc::argumentsToString( "%16.9e", boundaries1.back( ) );
    auto data = row.data( );
        data.push_back( data.back( ) );
    for( auto iter = data.begin( ); iter != data.end( ); ++iter ) {
        std::cout << LUPI::Misc::argumentsToString( " %16.9e", *iter );
    }
    std::cout << std::endl;
}
