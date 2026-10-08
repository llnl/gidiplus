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
// #include<bits/stdc++.h>
#include<limits.h>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "GIDI_testUtilities.hpp"

#define GIDI_PRINT_NAME_WIDTH 20

/*
=========================================================
*/
std::string fillString( std::string const &a_string, std::size_t a_width, Justification a_justification, bool a_truncate ) {

    std::size_t length = a_string.size( );
    std::string str;

    str.resize( a_width, ' ' );

    if( length >= a_width ) {
        if( a_truncate ) {
            str = a_string.substr( 0, a_width ); }
        else {
            str = a_string;
        } }
    else {
        std::size_t start = a_width - length;                      // Default is right justification.

        if( a_justification == Justification::center ) {
            start /= 2; }
        else if( a_justification == Justification::left ) {
            start = 0;
        }
        str.replace( start, a_string.size( ), a_string );
    }

    return( str );
}
/*
=========================================================
*/
int asInt( char const *a_chars ) {

    long value = asLong( a_chars );

    if( ( value < INT_MIN ) || ( value > INT_MAX ) ) throw std::runtime_error( "Value outside of range for a C++ int." );

    return( static_cast<int>( value ) );
}
/*
=========================================================
*/
long asLong( char const *a_chars ) {

    char *end_ptr;
    long value = strtol( a_chars, &end_ptr, 10 );

    while( isspace( *end_ptr  ) ) ++end_ptr;
    std::string msg( "ERROR: " );
    if( *end_ptr != 0 ) throw std::runtime_error( msg + a_chars + " is not a valid integer." );

    return( value );
}
/*
=========================================================
*/
double asDouble( std::string const &a_string ) {

    char *end_ptr;
    double value = strtod( a_string.c_str( ), &end_ptr );

    while( isspace( *end_ptr  ) ) ++end_ptr;
    std::string msg( "ERROR: " );
    if( *end_ptr != 0 ) throw std::runtime_error( msg + a_string + " is not a valid integer." );

    return( value );
}
/*
=========================================================
*/
std::string intToString( char const *format, int value ) {

    return( LUPI::Misc::argumentsToString( format, value ) );
}
/*
=========================================================
*/
std::string longToString( char const *format, long value ) {

    return( LUPI::Misc::argumentsToString( format, value ) );
}
/*
=========================================================
*/
std::string doubleToString( char const *format, double value ) {

    return( LUPI::Misc::argumentsToString( format, value ) );
}

/*
=========================================================
*/
argvOption::argvOption( std::string const &a_name, bool a_needsValue, std::string const &a_descriptor ) :
        m_name( a_name ),
        m_counter( 0 ),
        m_needsValue( a_needsValue ),
        m_descriptor( a_descriptor ) {

}
/*
=========================================================
*/
std::string argvOption::zeroOrOneOption( char **a_argv, std::string const &a_default ) {

    std::string msg( "ERROR: " );

    if( !m_needsValue ) throw std::runtime_error( msg + m_name + " does not have a value." );
    if( m_counter > 1 ) throw std::runtime_error( msg + m_name + " does not allow multiple arguments." );
    if( m_counter == 0 ) return( a_default );
    return( a_argv[m_indices[0]] );
}
/*
=========================================================
*/
int argvOption::asInt( char **a_argv, int a_default ) {

    long value = asLong( a_argv, a_default );

    if( ( value < INT_MIN ) || ( value > INT_MAX ) ) throw std::runtime_error( "Value outside of range for a C++ int." );

    return( static_cast<int>( value ) );
}
/*
=========================================================
*/
long argvOption::asLong( char **a_argv, long a_default ) {

    if( present( ) ) {
        std::string msg( "ERROR: " );
        char *end_ptr;
        std::string value_string = zeroOrOneOption( a_argv, "" );

        a_default = strtol( a_argv[m_indices[0]], &end_ptr, 10 );

        while( isspace( *end_ptr  ) ) ++end_ptr;
        if( *end_ptr != 0 ) throw std::runtime_error( msg + value_string + " is not a valid integer." );
    }

    return( a_default );
}
/*
=========================================================
*/
double argvOption::asDouble( char **a_argv, double a_default ) {

    if( present( ) ) {
        std::string msg( "ERROR: " );
        char *end_ptr;
        std::string value_string = zeroOrOneOption( a_argv, "" );

        a_default = strtod( a_argv[m_indices[0]], &end_ptr );

        while( isspace( *end_ptr  ) ) ++end_ptr;
        if( *end_ptr != 0 ) throw std::runtime_error( msg + value_string + " is not a valid double." );
    }

    return( a_default );
}
/*
=========================================================
*/
void argvOption::help( ) {

    std::cout << "    " << std::left << std::setw( GIDI_PRINT_NAME_WIDTH ) << m_name;
    if( m_needsValue ) {
        std::cout << " VALUE  "; }
    else {
        std::cout << "        ";
    }
    std::cout << m_descriptor << std::endl;
}
/*
=========================================================
*/
void argvOption::print( ) {

    std::cout << std::setw( GIDI_PRINT_NAME_WIDTH ) << m_name;
    for( std::vector<int>::iterator iter = m_indices.begin( ); iter != m_indices.end( ); ++iter ) std::cout << " " << *iter;
    std::cout << std::endl;
}

/*
=========================================================
*/
argvOptions::argvOptions( std::string const &a_codeName, std::string const &a_descriptor, int a_minimumNonOptions ) :
        m_codeName( a_codeName ),
        m_descriptor( a_descriptor ),
        m_minimumNonOptions( a_minimumNonOptions ) {

    add( argvOption( "-h", false, "Show this help message and exit." ) );
}
/*
=========================================================
*/
void argvOptions::parseArgv( int argc, char **argv ) {

    for( int iargc = 1; iargc < argc; ++iargc ) {
        std::string arg( argv[iargc] );

        if( arg == "-h" ) help( );
        if( arg[0] == '-' ) {
            std::size_t index = 0;

            for( ; index < size( ); ++index ) {
                argvOption &option = m_options[index];

                if( option.m_name == arg ) {
                    ++option.m_counter;
                    if( option.m_needsValue ) {
                        ++iargc;
                        if( iargc == argc ) {
                            std::string msg( "ERROR: option '" );

                            throw std::runtime_error( msg + arg + "' has no value." );
                        }
                        option.m_indices.push_back( iargc );
                    }
                    break;
                }
            }

            if( index == size( ) ) {

                try {
                    ::asDouble( arg.c_str( ) );
                    m_arguments.push_back( static_cast<std::size_t>( iargc ) ); }
                catch (std::runtime_error const &) {
                    throw std::runtime_error( std::string( "ERROR: invalid option '" ) + arg + "'." );
                }
            } }
        else {
            m_arguments.push_back( static_cast<std::size_t>( iargc ) );
        }
    }

    if( static_cast<int>( m_arguments.size( ) ) < m_minimumNonOptions ) throw std::runtime_error( "Insufficient non-option arguments." );
}
/*
=========================================================
*/
argvOption *argvOptions::find( std::string const &a_name ) {

    for( std::vector<argvOption>::iterator iter = m_options.begin( ); iter != m_options.end( ); ++iter ) {
        if( iter->m_name == a_name ) return( &(*iter) );
    }
    return( nullptr );
}
/*
=========================================================
*/
long argvOptions::asLong( char **argv, std::size_t argumentIndex ) {

    return( ::asLong( argv[m_arguments[argumentIndex]] ) );
}
/*
=========================================================
*/
double argvOptions::asDouble( char **argv, std::size_t argumentIndex ) {

    return( ::asDouble( argv[m_arguments[argumentIndex]] ) );
}
/*
=========================================================
*/
void argvOptions::help( ) {

    std::cout << std::endl << "Usage:" << std::endl;
    std::cout << "    " << stripDotCPP( m_codeName ) << std::endl;
    if( m_descriptor != "" ) {
        std::cout << std::endl << "Description:" << std::endl;
        std::cout << "    " << m_descriptor << std::endl;
    }

    if( m_options.size( ) > 0 ) {
        std::cout << std::endl << "Options:" << std::endl;
        for( std::vector<argvOption>::iterator iter = m_options.begin( ); iter != m_options.end( ); ++iter ) iter->help( );
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void argvOptions::print( ) {

    std::cout << "Arugment indices:";
    for( auto iter = m_arguments.begin( ); iter != m_arguments.end( ); ++iter ) std::cout << " " << *iter;
    std::cout << std::endl;
    for( std::vector<argvOption>::iterator iter = m_options.begin( ); iter != m_options.end( ); ++iter ) iter->print( );
}

/*
=========================================================
*/
std::size_t outputChannelStringMaximumLength( GIDI::Protare *protare ) {

    std::size_t maximumLength = 0;

    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction *reaction = protare->reaction( index );

        std::size_t length = outputChannelString( reaction ).size( );
        if( length > maximumLength ) maximumLength = length;
    }
    return( maximumLength );
}
/*
=========================================================
*/
std::string outputChannelString( GIDI::Reaction *reaction ) {

    if( reaction->hasFission( ) ) return( "fission" );
    return( reaction->label( ) );
}
/*
=========================================================
*/
std::string outputChannelPrefix( std::size_t a_offset, std::size_t a_width, GIDI::Reaction *reaction ) {

    std::string prefix( outputChannelString( reaction ) );

    prefix.insert( prefix.size( ), a_width - prefix.size( ), ' ' );
    prefix.insert( 0, a_offset, ' ' );
    return( prefix + ":" );
}
/*
=========================================================
*/
long integerFromArgv( int iarg, int argc, char **argv ) {

    char *endCharacter;

    if( iarg >= argc ) throw std::runtime_error( "integerFromArgv: iarg >= argc" );

    long value = strtol( argv[iarg], &endCharacter, 10 );

    if( *endCharacter != 0 ) throw std::runtime_error( "integerFromArgv: invalid integer string." );

    return( value );
}
/*
=========================================================
*/
void printVector( char const *a_prefix, GIDI::Vector &a_vector ) {

    std::string prefix( a_prefix );

    printVector( prefix, a_vector );
}
/*
=========================================================
*/
void printVector( std::string &a_prefix, GIDI::Vector &a_vector ) {

    a_vector.print( a_prefix );
}
/*
=========================================================
*/
void writeVector( std::string const &a_fileName, std::string const &a_prefix, GIDI::Vector const &a_vector ) {

    FILE *file = fopen( a_fileName.c_str( ), "w" );
    if( file == nullptr ) throw( "Could not open file '" + a_fileName + "'." );

    a_vector.write( file, a_prefix );
    fclose( file );    
}
/*
=========================================================
*/
void printVectorOfDoubles( char const *a_prefix, std::vector<double> const &doubles ) {

    GIDI::Vector vector( doubles );

    printVector( a_prefix, vector );
}
/*
=========================================================
*/
void printVectorOfDoubles( std::string &a_prefix, std::vector<double> const &doubles ) {

    GIDI::Vector vector( doubles );

    printVector( a_prefix, vector );
}
/*
=========================================================
*/
void printIDs( char const *a_prefix, std::set<std::string> const &ids ) {

    std::string prefix( a_prefix );

    printIDs( prefix, ids );
}
/*
=========================================================
*/
void printIDs( std::string &a_prefix, std::set<std::string> const &ids ) {

    std::cout << a_prefix;
    for( std::set<std::string>::const_iterator iter = ids.begin( ); iter != ids.end( ); ++iter ) std::cout << " " << *iter;
    std::cout << std::endl;
}
/*
=========================================================
*/
void printMatrix( char const *a_prefix, int maxOrder, GIDI::Matrix &matrix, std::string indent ) {

    std::string prefix( a_prefix );

    printMatrix( prefix, maxOrder, matrix, indent );
}
/*
=========================================================
*/
void printMatrix( std::string &prefix, int maxOrder, GIDI::Matrix &matrix, std::string indent ) {

    std::cout << std::endl << prefix << std::endl;
    if( maxOrder > -2 ) std::cout << "    max. Legendre order = " << maxOrder << std::endl;
    matrix.print( indent + "::  " );
}
/*
=========================================================
*/
std::string stripDirectoryBase( std::string const &a_path, std::string const &a_section ) {

    std::string path( a_path );
    std::size_t location( path.find( a_section ) );

    if( location != std::string::npos ) path = "/..." + path.substr( location );

    return( path );
}
/*
=========================================================
*/
std::string stripDotCPP( std::string const &a_path ) {

    std::string fileName( a_path );
    std::size_t location = fileName.find( ".cpp" );

    if( location == std::string::npos ) return( fileName );
    return( fileName.erase( location ) );
}
/*
=========================================================
*/
void printCodeArguments( std::string a_codeName, int a_argc, char **a_argv ) {

    std::cerr << "    " << stripDotCPP( a_codeName );
    for( int i1 = 1; i1 < a_argc; i1++ ) std::cerr << " " << a_argv[i1];
    std::cerr << std::endl;
}
/*
=========================================================
*/
ParseTestOptions::ParseTestOptions( argvOptions &a_argvOptions, int a_argc, char **a_argv ) :
        m_argvOptions( a_argvOptions ),
        m_argc( a_argc ),
        m_argv( a_argv ),
        m_askPoPs( true ),
        m_askMap( true ),
        m_askPid( true ),
        m_askTid( true ),
        m_askPhotoAtomic( true ),
        m_askPhotoNuclear( true ),
        m_askOid( false ),
        m_askDelayedFissionNeutrons( false ),
        m_askGNDS_File( false ),
        m_askLegendreOrder( false ),
        m_askTemperature( false ),
        m_askTracking( false ) {

    m_particlesAndGIDs[PoPI::IDs::neutron] = "LLNL_gid_4";
    m_particlesAndGIDs["H1"] = "LLNL_gid_71";
    m_particlesAndGIDs["H2"] = "LLNL_gid_71";
    m_particlesAndGIDs["H3"] = "LLNL_gid_71";
    m_particlesAndGIDs["He3"] = "LLNL_gid_71";
    m_particlesAndGIDs["He4"] = "LLNL_gid_71";
    m_particlesAndGIDs[PoPI::IDs::photon] = "LLNL_gid_70";
}
/*
=========================================================
*/
void ParseTestOptions::parse( ) {

    if( m_askPoPs ) m_argvOptions.add( argvOption( "--pops", true, "Add next argument to list of PoPs files to use." ) );
    if( m_askMap ) m_argvOptions.add( argvOption( "--map", true, "The map file to use." ) );
    if( m_askPid ) m_argvOptions.add( argvOption( "--pid", true, "The PoPs id of the projectile." ) );
    if( m_askTid ) m_argvOptions.add( argvOption( "--tid", true, "The PoPs id of the target." ) );
    if( m_askOid ) m_argvOptions.add( argvOption( "--oid", true, "The PoPs id of the product." ) );
    if( m_askDelayedFissionNeutrons ) m_argvOptions.add( argvOption( "--delayed", false, "If present, fission delayed neutrons are included with product sampling." ) );
    if( m_askPhotoAtomic ) m_argvOptions.add( argvOption( "--pa", false, "Include photo-atomic protare if relevant." ) );
    if( m_askPhotoNuclear ) m_argvOptions.add( argvOption( "--pn", false, "Include photo-nuclear protare if relevant." ) );
    if( m_askGNDS_File ) m_argvOptions.add( argvOption( "--gnds", true, "If present, specifies the GNDS file to read and the map file step is skipped." ) );
    if( m_askLegendreOrder ) m_argvOptions.add( argvOption( "--order", true, "Legendre order of the data to retrieve." ) );
    if( m_askTemperature ) m_argvOptions.add( argvOption( "--temperature", true, "The temperature of the target material." ) );

    if( m_askTracking ) {
        m_argvOptions.add( argvOption( "--tracking", true, "Add particle ID to list of tracking particles." ) );
        m_argvOptions.add( argvOption( "--multiGroupsFile", true, "Specifies the multi-groups file to use." ) );
        m_argvOptions.add( argvOption( "--fluxesFile", true, "Specifies the flux file to use." ) );
    }

    m_argvOptions.parseArgv( m_argc, m_argv );
    printCodeArguments( m_argvOptions.m_codeName, m_argc, m_argv );
}
/*
=========================================================
*/
void ParseTestOptions::pops( PoPI::Database &a_pops, std::string const &a_popsFileName ) const {

    if( m_askPoPs ) {
        argvOption *pops = m_argvOptions.find( "--pops" );

        if( pops->present( ) ) {
            for( std::size_t i1 = 0; i1 < pops->m_counter; ++i1 ) {
                a_pops.addFile( m_argv[pops->m_indices[i1]], false );
            } }
        else {
            a_pops.addFile( a_popsFileName, false );
        } }
    else {
        a_pops.addFile( a_popsFileName, false );
    }
}
/*
=========================================================
*/
GIDI::Protare *ParseTestOptions::protare( PoPI::Database &a_pops, std::string const &a_popsFileName, std::string const &a_mapFileName, 
                GIDI::Construction::Settings const &a_construction, std::string const &a_projectileID, std::string const &a_targetID ) const {

    GIDI::Protare *protare1 = nullptr;
    std::vector<std::string> libraries;

    pops( a_pops, a_popsFileName );

    if( m_askGNDS_File && m_argvOptions.find( "--gnds" )->present( ) ) {
        std::string GNDSFile = m_argvOptions.find( "--gnds" )->zeroOrOneOption( m_argv, "Oops" );
        GIDI::ParticleSubstitution particleSubstitution;

        protare1 = new GIDI::ProtareSingle( a_construction, GNDSFile, GIDI::FileType::XML, a_pops, particleSubstitution, libraries, 
                GIDI_MapInteractionNuclearChars, false, false ); }
    else {
        std::string mapFilename = a_mapFileName;
        if( m_askMap ) mapFilename = m_argvOptions.find( "--map" )->zeroOrOneOption( m_argv, a_mapFileName );
        GIDI::Map::Map map( mapFilename, a_pops );

        std::string projectileID = a_projectileID;
        if( m_askPid ) projectileID = m_argvOptions.find( "--pid" )->zeroOrOneOption( m_argv, a_projectileID );
        std::string targetID = a_targetID;
        if( m_askTid ) targetID = m_argvOptions.find( "--tid" )->zeroOrOneOption( m_argv, a_targetID );

        protare1 = map.protare( a_construction, a_pops, projectileID, targetID );
    }

    return( protare1 );    
}
/*
=========================================================
*/
GIDI::Construction::PhotoMode ParseTestOptions::photonMode( GIDI::Construction::PhotoMode a_photonMode ) const {

    if( m_askPhotoAtomic || m_askPhotoNuclear ) {
        bool doPhotonAtomic = false;
        bool doPhotonNuclear = false;

        if( m_askPhotoAtomic ) doPhotonAtomic = m_argvOptions.find( "--pa" )->present( );
        if( m_askPhotoNuclear ) doPhotonNuclear = m_argvOptions.find( "--pn" )->present( );

        if( doPhotonAtomic ) {
            a_photonMode = GIDI::Construction::PhotoMode::atomicOnly;
            if( doPhotonNuclear ) a_photonMode = GIDI::Construction::PhotoMode::nuclearAndAtomic; }
        else if( doPhotonNuclear ) {
            a_photonMode = GIDI::Construction::PhotoMode::nuclearOnly;
        }
    }

    return( a_photonMode );
}
/*
=========================================================
*/
void ParseTestOptions::particles( GIDI::Transporting::Particles &a_particles ) {

    if( m_askTracking ) {

        m_fluxes.addFile( m_argvOptions.find( "--fluxesFile" )->zeroOrOneOption( m_argv, "../fluxes.xml" ) );
        GIDI::Functions::Function3dForm const *function3d = m_fluxes.get<GIDI::Functions::Function3dForm>( 0 );
        std::vector<GIDI::Transporting::Flux> fluxes = GIDI::settingsFluxesFromFunction3d( *function3d );

        m_multiGroups.addFile( m_argvOptions.find( "--multiGroupsFile" )->zeroOrOneOption( m_argv, "../groups.xml" ) );

        argvOption *option = m_argvOptions.find( "--tracking" );
        for( std::size_t i1 = 0; i1 < option->m_counter; ++i1 ) {
            std::string particleID = m_argv[option->m_indices[i1]];

            if( m_particlesAndGIDs.find( particleID ) == m_particlesAndGIDs.end( ) ) throw std::runtime_error( "Tracking particle '" + particleID + "' not supported." );

            GIDI::Group const *multiGroup = m_multiGroups.get<GIDI::Group>( m_particlesAndGIDs[particleID] );
            GIDI::Transporting::Particle particle( particleID, *multiGroup );
            particle.appendFlux( fluxes[0] );
            a_particles.add( particle );
        }
    }
}
