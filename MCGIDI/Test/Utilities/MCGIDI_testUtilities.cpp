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

#include <iostream>
#include <iomanip>

#include "MCGIDI_testUtilities.hpp"

#define PRINT_NAME_WIDTH 20

/*
=========================================================
*/
long asLong2( char const *a_chars ) {

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
double asDouble2( char const *a_chars ) {

    char *end_ptr;
    double value = strtod( a_chars, &end_ptr );

    while( isspace( *end_ptr  ) ) ++end_ptr;
    std::string msg( "ERROR: " );
    if( *end_ptr != 0 ) throw std::runtime_error( msg + a_chars + " is not a valid integer." );

    return( value );
}
/*
=========================================================
*/
std::string longToString2( char const *format, long value ) {

    return( LUPI::Misc::argumentsToString( format, value ) );
}
/*
=========================================================
*/
std::string doubleToString2( char const *format, double value ) {

    return( LUPI::Misc::argumentsToString( format, value ) );
}

/*
=========================================================
*/
argvOption2::argvOption2( std::string const &a_name, bool a_needsValue, std::string const &a_descriptor ) :
        m_name( a_name ),
        m_counter( 0 ),
        m_needsValue( a_needsValue ),
        m_descriptor( a_descriptor ) {

}
/*
=========================================================
*/
std::string argvOption2::zeroOrOneOption( char **a_argv, std::string const &a_default ) {

    std::string msg( "ERROR: " );

    if( !m_needsValue ) throw std::runtime_error( msg + m_name + " does not have a value." );
    if( m_counter > 1 ) throw std::runtime_error( msg + m_name + " does not allow multiple arguments." );
    if( m_counter == 0 ) return( a_default );
    return( a_argv[m_indices[0]] );
}
/*
=========================================================
*/
long argvOption2::asLong( char **a_argv, long a_default ) {

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
double argvOption2::asDouble( char **a_argv, double a_default ) {

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
void argvOption2::help( ) {

    std::cout << "    " << std::left << std::setw( PRINT_NAME_WIDTH ) << m_name;
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
void argvOption2::print( ) {

    std::cout << std::setw( PRINT_NAME_WIDTH ) << m_name;
    for( std::vector<int>::iterator iter = m_indices.begin( ); iter != m_indices.end( ); ++iter ) std::cout << " " << *iter;
    std::cout << std::endl;
}

/*
=========================================================
*/
argvOptions2::argvOptions2( std::string const &a_codeName, std::string const &a_descriptor ) :
        m_codeName( a_codeName ),
        m_descriptor( a_descriptor ) {

    add( argvOption2( "-h", false, "Show this help message and exit." ) );
}
/*
=========================================================
*/
void argvOptions2::parseArgv( int argc, char **argv ) {

    for( int iargc = 1; iargc < argc; ++iargc ) {
        std::string arg( argv[iargc] );

        if( arg == "-h" ) help( );
        if( arg[0] == '-' ) {
            size_t index = 0;

            for( ; index < size( ); ++index ) {
                argvOption2 &option = m_options[index];

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
                std::string msg( "ERROR: invalid option '" );
                throw std::runtime_error( msg + arg + "'." );
            } }
        else {
            m_arguments.push_back( iargc );
        }
    }
}
/*
=========================================================
*/
argvOption2 *argvOptions2::find( std::string const &a_name ) {

    for( std::vector<argvOption2>::iterator iter = m_options.begin( ); iter != m_options.end( ); ++iter ) {
        if( iter->m_name == a_name ) return( &(*iter) );
    }
    return( nullptr );
}
/*
=========================================================
*/
long argvOptions2::asLong( char **argv, std::size_t argumentIndex ) {

    return( ::asLong2( argv[m_arguments[argumentIndex]] ) );
}
/*
=========================================================
*/
double argvOptions2::asDouble( char **argv, std::size_t argumentIndex ) {

    return( ::asDouble2( argv[m_arguments[argumentIndex]] ) );
}
/*
=========================================================
*/
void argvOptions2::help( ) {

    std::cout << std::endl << "Usage:" << std::endl;
    std::cout << "    " << m_codeName << std::endl;
    if( m_descriptor != "" ) {
        std::cout << std::endl << "Description:" << std::endl;
        std::cout << "    " << m_descriptor << std::endl;
    }

    if( m_options.size( ) > 0 ) {
        std::cout << std::endl << "Options:" << std::endl;
        for( std::vector<argvOption2>::iterator iter = m_options.begin( ); iter != m_options.end( ); ++iter ) iter->help( );
    }

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void argvOptions2::print( ) {

    std::cout << "Arugment indices:";
    for( std::vector<int>::iterator iter = m_arguments.begin( ); iter != m_arguments.end( ); ++iter ) std::cout << " " << *iter;
    std::cout << std::endl;
    for( std::vector<argvOption2>::iterator iter = m_options.begin( ); iter != m_options.end( ); ++iter ) iter->print( );
}
