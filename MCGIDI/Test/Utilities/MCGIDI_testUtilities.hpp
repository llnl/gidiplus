/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#ifndef MCGIDI_testUtilities_hpp_included
#define MCGIDI_testUtilities_hpp_included 1

#include <stdarg.h>

#include <string>
#include <vector>
#include <stdexcept>

#include <LUPI.hpp>
#include <LUPI_declareMacro.hpp>

class argvOption2 {

    public:
        std::string m_name;     // Must include all leading '-'s (E.g., "-v", "--target").
        int m_counter;
        bool m_needsValue;
        std::vector<int> m_indices;
        std::string m_descriptor;

        argvOption2( std::string const &a_name, bool a_needsValue, std::string const &a_descriptor = "" );

        bool present( ) const { return( m_counter > 0 ); }
        std::string zeroOrOneOption( char **argv, std::string const &a_default = "" );
        long asLong( char **a_argv, long a_default = 0 );
        double asDouble( char **a_argv, double a_default = 0.0 );
        void help( );
        void print( );
};

class argvOptions2 {

    public:
        std::string m_codeName;
        std::string m_descriptor;
        std::vector<argvOption2> m_options;
        std::vector<int> m_arguments;

        argvOptions2( std::string const &a_codeName, std::string const &a_descriptor = "" );

        std::size_t size( ) { return( m_options.size( ) ); }
        void add( argvOption2 const &a_option ) { m_options.push_back( a_option ); }
        void parseArgv( int argc, char **argv );
        argvOption2 *find( std::string const &a_name );
        long asLong( char **argv, std::size_t argumentIndex );
        double asDouble( char **argv, std::size_t argumentIndex );
        void help( );
        void print( );
};

long asLong2( char const *a_chars );
double asDouble2( char const *a_chars );
std::string doubleToString2( char const *format, double value );
std::string longToString2( char const *format, long value );
/*
=========================================================
*/
LUPI_HOST_DEVICE inline double float64RNG64( unsigned long long *a_state ) {

    *a_state = 0x27bb2ee687b0b0fd * *a_state + 0xb504f32d;
    return( 5.42101086242752157e-20 * static_cast<double>( *a_state ) );
}
#endif          // MCGIDI_testUtilities_hpp_included
