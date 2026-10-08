/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <LUPI.hpp>
#include <GIDI.hpp>

namespace GIDI {

#include <miniProtareSingleKeyed_xml.hpp>

static std::string valuesToString( std::vector<double> const &a_values );

/* *********************************************************************************************************//**
 * This function creates mini ProtareSingle for the specified projectile, target and interaction. Here mini
 * means a protare with one reaction which is elastic. Also, the cross section value **a_crossSection**
 * should be very small so that this protare is unlikely to be sampled in MCGIDI. A cross section value of 0.0
 * or 1e-40 is recommended. This protare was added so that if a map file does not have the requested protare (i.e.,
 * projectile + target for the interaction), then the caller can create one that should not effect the problem
 * but still all the code to run.
 *
 * @param a_construction    [in]    Used to pass user options to the constructor.
 * @param a_pops            [in]    A PoPs Database instance.
 * @param a_pids            [in]    The GNDS PoPs id of the projectile.
 * @param a_tids            [in]    The GNDS PoPs id of the target.
 * @param a_interaction     [in]    The interaction.
 * @param a_particles       [in]    The list of particles to be transported.
 * @param a_crossSection    [in]    The value of the cross section to be used at all energies.
 * @param a_outputPath      [in]    If not an empty string, the XML is written to the path specified by this parameter.
 *
 * @return                          An instance of *ProtareSingle*.
 ***********************************************************************************************************/

ProtareSingle *createMiniProtareSingle( GIDI::Construction::Settings const &a_construction, PoPI::Database const &a_pops, 
                std::string const &a_pid, std::string const &a_tid, std::string const &a_interaction, 
                GIDI::Transporting::Particles const &a_particles, double a_crossSection, std::string a_outputPath ) {

    PoPI::Particle const &projectilePoPs = a_pops.get<PoPI::Particle>( a_pid );
    double projectileMass = projectilePoPs.massValue( "amu" );
    PoPI::Particle const &targetPoPs = a_pops.get<PoPI::Particle>( a_tid );
    double targetMass = targetPoPs.massValue( "amu" );

    if( !a_particles.hasParticle( a_pid ) ) return( nullptr );
    if( ( a_interaction != "nuclear" ) && ( a_interaction != "atomic" ) ) return( nullptr );

    GIDI::Transporting::Particle const *projectile = a_particles.particle( a_pid );

    std::vector<double> PROJECTILE_MULTIGROUP_BOUNDARIES_values;
    std::vector<double> TARGET_MULTIGROUP_BOUNDARIES_values;
    if( projectile->mode( ) == GIDI::Transporting::Mode::MonteCarloContinuousEnergy ) {
        PROJECTILE_MULTIGROUP_BOUNDARIES_values.push_back( 1e-11 );
        PROJECTILE_MULTIGROUP_BOUNDARIES_values.push_back( 20 );
        TARGET_MULTIGROUP_BOUNDARIES_values = PROJECTILE_MULTIGROUP_BOUNDARIES_values; }
    else {
        GIDI::Transporting::MultiGroup multiGroup = projectile->multiGroup( );
        PROJECTILE_MULTIGROUP_BOUNDARIES_values = multiGroup.boundaries( );
        if( a_particles.hasParticle( a_tid ) ) {
            GIDI::Transporting::Particle const *target = a_particles.particle( a_tid );
            multiGroup = target->multiGroup( );
            TARGET_MULTIGROUP_BOUNDARIES_values = multiGroup.boundaries( ); }
        else {
            TARGET_MULTIGROUP_BOUNDARIES_values.push_back( 1e-11 );
            TARGET_MULTIGROUP_BOUNDARIES_values.push_back( 20 );
        }
    }

    std::vector<double> INVERSE_SPEEDS_values;
    std::vector<double> CROSSSECTION_DATA_values;
    for( std::size_t index = 0; index < PROJECTILE_MULTIGROUP_BOUNDARIES_values.size( ); ++index ) {
        INVERSE_SPEEDS_values.push_back( 1 );                       // This is wrong but currently not used.
        CROSSSECTION_DATA_values.push_back( a_crossSection );
    }

    std::string PROJECTILE_MULTIGROUP_BOUNDARIES = valuesToString( PROJECTILE_MULTIGROUP_BOUNDARIES_values );
    std::string TARGET_MULTIGROUP_BOUNDARIES = valuesToString( TARGET_MULTIGROUP_BOUNDARIES_values );
    std::string INVERSE_SPEEDS = valuesToString( INVERSE_SPEEDS_values );
    std::string CROSSSECTION_VALUE = LUPI::Misc::doubleToShortestString( a_crossSection, 8 );
    std::string CROSSSECTION_MULTIGROUP_DATA = valuesToString( CROSSSECTION_DATA_values );

    std::string protareTemplate( protareTemplateChars );
    std::string protareStr = LUPI::Misc::replaceString( protareTemplate, "PROJECTILE_ID", a_pid, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "TARGET_ID", a_tid, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "INTERACTION", a_interaction, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "PROJECTILE_MULTIGROUP_NUMBER_OF_GROUPS",
            std::to_string( PROJECTILE_MULTIGROUP_BOUNDARIES_values.size( ) ), true );
    protareStr = LUPI::Misc::replaceString( protareStr, "PROJECTILE_MULTIGROUP_BOUNDARIES", PROJECTILE_MULTIGROUP_BOUNDARIES, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "TARGET_MULTIGROUP_BOUNDARIES", TARGET_MULTIGROUP_BOUNDARIES, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "INVERSE_SPEEDS", INVERSE_SPEEDS, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "PROJECTILE_MASS", std::to_string( projectileMass ), true );
    protareStr = LUPI::Misc::replaceString( protareStr, "TARGET_MASS", std::to_string( targetMass ), true );
    protareStr = LUPI::Misc::replaceString( protareStr, "REACTION_LABEL", a_pid + " + " + a_tid, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "CROSSSECTION_VALUE", CROSSSECTION_VALUE, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "CROSSSECTION_MULTIGROUP_DATA", CROSSSECTION_MULTIGROUP_DATA, true );
    protareStr = LUPI::Misc::replaceString( protareStr, "MULTIGROUP_MULTIPLICITY", CROSSSECTION_MULTIGROUP_DATA, true );

    if( a_outputPath != "" ) {
        std::ofstream fileio;
        fileio.open( a_outputPath.c_str( ) );
        fileio << protareStr;
        fileio.close( );
    }

    HAPI::PugiXMLFile *doc = new HAPI::PugiXMLFile( "createMiniProtareSingle", protareStr, "createMiniProtareSingle" );
    if( doc == nullptr ) {
        throw std::runtime_error( "GIDI::createMiniProtareSingle: Failed to create HAPI::PugiXMLFile instance from string." );
    }
    HAPI::Node node = doc->first_child( );

    std::vector<std::string> libraries;
    GIDI::ParticleSubstitution particleSubstitution;
    GIDI::Construction::Settings construction( a_construction );
    construction.setLazyParsing( false );
    GIDI::ProtareSingle *protare = new GIDI::ProtareSingle( construction, node, a_pops, particleSubstitution,
            libraries, a_interaction, false, false );

    delete doc;

    return( protare );
}

/* *********************************************************************************************************//**
 * This method is for use by *createMiniProtareSingle*. This method takes a list of double values and returns
 * a space separated std::string of those values.
 *
 * @param a_values          [in]    The list or double values.
 *
 * @return                          A std::string instance.
 ***********************************************************************************************************/

static std::string valuesToString( std::vector<double> const &a_values ) {

    std::string valuesStr;
    std::string sep = "";

    for( auto valueIter = a_values.begin( ); valueIter != a_values.end( ); ++valueIter ) {
        valuesStr += sep + LUPI::Misc::doubleToShortestString( *valueIter, 8 );
        sep = " ";
    }

    return( valuesStr );
}

}           // End of namespace GIDI.
