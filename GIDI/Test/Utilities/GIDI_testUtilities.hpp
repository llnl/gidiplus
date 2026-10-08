/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#ifndef GIDI_testUtilities_hpp_included
#define GIDI_testUtilities_hpp_included 1

#include <GIDI.hpp>

class argvOption {

    public:
        std::string m_name;     // Must include all leading '-'s (E.g., "-v", "--target").
        std::size_t m_counter;
        bool m_needsValue;
        std::vector<int> m_indices;
        std::string m_descriptor;

        argvOption( std::string const &a_name, bool a_needsValue, std::string const &a_descriptor = "" );

        bool present( ) const { return( m_counter > 0 ); }
        std::string zeroOrOneOption( char **argv, std::string const &a_default = "" );
        int asInt( char **a_argv, int a_default = 0 );
        long asLong( char **a_argv, long a_default = 0 );
        double asDouble( char **a_argv, double a_default = 0.0 );
        void help( );
        void print( );
};

class argvOptions {

    public:
        std::string m_codeName;
        std::string m_descriptor;
        int m_minimumNonOptions;
        std::vector<argvOption> m_options;
        std::vector<std::size_t> m_arguments;

        argvOptions( std::string const &a_codeName, std::string const &a_descriptor = "", int a_minimumNonOptions = 0 );

        std::size_t size( ) { return( m_options.size( ) ); }
        void add( argvOption const &a_option ) { m_options.push_back( a_option ); }
        void parseArgv( int argc, char **argv );
        argvOption *find( std::string const &a_name );
        long asLong( char **argv, std::size_t argumentIndex );
        double asDouble( char **argv, std::size_t argumentIndex );
        void help( );
        void print( );
};

class ParseTestOptions {

    public :
        argvOptions &m_argvOptions;
        std::string m_description;

        int m_argc;
        char **m_argv;

        bool m_askPoPs;
        bool m_askMap;
        bool m_askPid;
        bool m_askTid;
        bool m_askPhotoAtomic;
        bool m_askPhotoNuclear;
        bool m_askOid;
        bool m_askDelayedFissionNeutrons;
        bool m_askGNDS_File;
        bool m_askLegendreOrder;
        bool m_askTemperature;
        bool m_askTracking;

        GIDI::Groups m_multiGroups;
        GIDI::Fluxes m_fluxes;
        std::map<std::string, std::string> m_particlesAndGIDs;

        ParseTestOptions( argvOptions &argvOptions, int a_argc, char **a_argv );

        void parse( );
        void pops( PoPI::Database &a_pops, std::string const &a_popsFileName ) const ;
        GIDI::Protare *protare( PoPI::Database &a_pops, std::string const &a_popsFileName, std::string const &a_mapFileName, 
                GIDI::Construction::Settings const &a_construction, std::string const &a_projectileID, std::string const &a_targetID ) const ;
        GIDI::Construction::PhotoMode photonMode( GIDI::Construction::PhotoMode a_photonMode = GIDI::Construction::PhotoMode::nuclearAndAtomic ) const ;
        void particles( GIDI::Transporting::Particles &a_particles );
};

enum class Justification { left, center, right };

std::string fillString( std::string const &a_string, std::size_t a_width, Justification a_justification, bool a_truncate );
int asInt( char const *a_chars );
long asLong( char const *a_chars );
double asDouble( std::string const &a_string );
std::string doubleToString( char const *format, double value );
std::size_t outputChannelStringMaximumLength( GIDI::Protare *protare );
std::string outputChannelString( GIDI::Reaction *reaction );
std::string outputChannelPrefix( std::size_t offset, std::size_t width, GIDI::Reaction *reaction );
long integerFromArgv( int iarg, int argc, char **argv );
void printVector( char const *prefix, GIDI::Vector &vector );
void printVector( std::string &prefix, GIDI::Vector &vector );
void writeVector( std::string const &a_fileName, std::string const &prefix, GIDI::Vector const &vector );
void printVectorOfDoubles( char const *a_prefix, std::vector<double> const &doubles );
void printVectorOfDoubles( std::string &a_prefix, std::vector<double> const &doubles );
void printIDs( char const *a_prefix, std::set<std::string> const &ids );
void printIDs( std::string &a_prefix, std::set<std::string> const &ids );
void printMatrix( char const *a_prefix, int maxOrder, GIDI::Matrix &matrix, std::string indent = "    " );
void printMatrix( std::string &prefix, int maxOrder, GIDI::Matrix &matrix, std::string indent = "    " );
std::string stripDirectoryBase( std::string const &a_path, std::string const &a_section = "/GIDI/Test/" );
std::string stripDotCPP( std::string const &a_path );
void printCodeArguments( std::string a_codeName, int a_argc, char **a_argv );
std::string longToString( char const *format, long value );
std::string intToString( char const *format, int value );

#endif          // GIDI_testUtilities_hpp_included
