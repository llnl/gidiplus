/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>

/*
============================================================
=========================== Bins ===========================
============================================================
*/
class Bins {

    public:
        bool m_logDomainStep;
        double m_domainMin, m_domainMax, m_domainWidth;
        double m_logDomainFraction;
        long m_underFlows;
        long m_overFlows;
        std::vector<long> m_bins;
        double m_underFlowWeights;
        double m_overFlowWeights;
        std::vector<double> m_weightedBins;
        double m_averageValue;

        void merge( Bins const &a_bins ) {

            if( ( m_bins.size( ) != a_bins.m_bins.size( ) ) || ( m_logDomainStep != a_bins.m_logDomainStep ) 
                    || ( m_domainMin != a_bins.m_domainMin ) || ( m_domainMax != a_bins.m_domainMax ) )
                throw LUPI::Exception( "Error from Bins::combine: Bins do not have same parameters and cannot be combined." );

            m_underFlows += a_bins.m_underFlows;
            m_overFlows += a_bins.m_overFlows;
            m_underFlowWeights += a_bins.m_underFlowWeights;
            m_overFlowWeights += a_bins.m_overFlowWeights;
            for( std::size_t i1 = 0; i1 < m_bins.size( ); ++i1 ) {
                m_bins[i1] += a_bins.m_bins[i1];
                m_weightedBins[i1] += a_bins.m_weightedBins[i1];
            }
        }

        void setDomain( double a_domainMin, double a_domainMax ) {

            m_domainMin = a_domainMin;
            m_domainMax = a_domainMax;
            m_domainWidth = m_domainMax - m_domainMin;
            m_logDomainFraction = 2.0;
            if( m_logDomainStep ) m_logDomainFraction = log( pow( m_domainMax / m_domainMin, 1.0 / static_cast<double>( m_bins.size( ) ) ) );
        }

        Bins( std::size_t a_numberOfBins, double a_domainMin, double a_domainMax, bool a_logDomainStep = false ) :
                m_logDomainStep( a_logDomainStep ),
                m_underFlows( 0 ),
                m_overFlows( 0 ),
                m_bins( a_numberOfBins, 0 ),
                m_underFlowWeights( 0.0 ),
                m_overFlowWeights( 0.0 ),
                m_weightedBins( a_numberOfBins, 0.0 ),
                m_averageValue( 0.0 ) {

            setDomain( a_domainMin, a_domainMax );
        }

        void clear( ) {

            m_underFlows = 0;
            m_overFlows = 0;
            m_underFlowWeights = 0.0;
            m_overFlowWeights = 0.0;
            for( std::size_t i1 = 0; i1 < m_bins.size( ); ++i1 ) {
                m_bins[i1] = 0;
                m_weightedBins[i1] = 0.0;
            }

            m_averageValue = 0.0;
        }

        long accrue( double a_value, double a_weight = 1.0 ) {

            long longIndex;

            if( a_value == m_domainMax ) {
                longIndex = (long) m_bins.size( ) - 1; }
            else if( m_logDomainStep ) {
                longIndex = (long) ( log( a_value / m_domainMin ) / m_logDomainFraction ); }
            else {
                longIndex = (long) ( ( a_value - m_domainMin ) / m_domainWidth * static_cast<double>( m_bins.size( ) ) );
            }

            if( longIndex < 0 ) {
                ++m_underFlows;
                m_underFlowWeights += a_weight; }
            else if( longIndex >= (long) m_bins.size( ) ) {
                ++m_overFlows;
                m_overFlowWeights += a_weight; }
            else {
                std::size_t index = static_cast<std::size_t>( longIndex );
                ++m_bins[index];
                m_weightedBins[index] += a_weight;
            }

            m_averageValue += a_value;

            return( longIndex );
        }

        long total( bool a_includeOutOfBounds ) {
            long sum = 0;

            if( a_includeOutOfBounds ) sum += m_underFlows + m_overFlows;

            for( std::size_t i1 = 0; i1 < m_bins.size( ); ++i1 ) sum +=  m_bins[i1];

            return( sum );
        }

        double totalWeights( bool a_includeOutOfBounds ) {
            double sum = 0;

            if( a_includeOutOfBounds ) sum += m_underFlowWeights + m_overFlowWeights;

            for( std::size_t i1 = 0; i1 < m_weightedBins.size( ); ++i1 ) sum +=  m_weightedBins[i1];

            return( sum );
        }

        double meanX( ) {

            double _total = static_cast<double>( total( false ) );

            if( _total == 0 ) return( 0.0 );

            double mean_x = 0.0;
            for( std::size_t i1 = 0; i1 <  m_bins.size( ); ++i1 ) {
                double x1;

                if( m_logDomainStep ) {
                    x1 = m_domainMin * exp( m_logDomainFraction * ( static_cast<double>( i1 ) + 0.5 ) ); }
                else {
                    x1 = ( static_cast<double>( i1 ) + 0.5 ) / ( static_cast<double>( m_bins.size( ) ) ) * m_domainWidth + m_domainMin;
                }
                mean_x += static_cast<double>( m_bins[i1] ) * x1;
            }

            return( mean_x / _total );
        }

        double averageValue( ) {

            double averageValue = 0.0;
            long counts = total( true );
            if( counts > 0 ) averageValue = m_averageValue / static_cast<double>( counts );

            return( averageValue );
        }

        std::vector<double> edges() {

            std::vector<double> edges;
            edges.resize( m_bins.size() + 1 );
            edges[0] = m_domainMin;

            for( std::size_t i1 = 1; i1 <  edges.size( ); ++i1 ) {
                if( m_logDomainStep ) {
                    edges[i1] = m_domainMin * exp( m_logDomainFraction * static_cast<double>( i1 ) );
                }
                else {
                    edges[i1] = static_cast<double>( i1 ) / static_cast<double>( m_bins.size( ) ) * m_domainWidth + m_domainMin;
                }
            }
            edges.back( ) = m_domainMax;

            return edges;
        }

        void print( FILE *a_fOut, char const *a_label, bool a_includeWeights = false, bool a_histogram = false ) {

            long _total = total( false );
            double weightedTotal = totalWeights( false );

            fprintf( a_fOut, "\n\n" );
            if( strlen( a_label ) > 0 ) fprintf( a_fOut, "%s\n", a_label );
            fprintf( a_fOut, "# total number of inflows = %ld\n", _total );
            fprintf( a_fOut, "# number of underflows = %ld\n", m_underFlows );
            fprintf( a_fOut, "# number of overflows = %ld\n", m_overFlows );
            fprintf( a_fOut, "# number of Bins = %lu\n", m_bins.size( ) );
            fprintf( a_fOut, "# domain min = %g\n", m_domainMin );
            fprintf( a_fOut, "# domain max = %g\n", m_domainMax );
            fprintf( a_fOut, "# Average value = %g\n", averageValue( ) );
            if( a_includeWeights ) {
                fprintf( a_fOut, "# total weight = %15.7e\n", weightedTotal );
                fprintf( a_fOut, "# underflow weight = %15.7e\n", m_underFlowWeights );
                fprintf( a_fOut, "# overflow weight = %15.7e\n", m_overFlowWeights );
            }
            fprintf( a_fOut, "# x-values                      pdf         counts       fraction" );
            if( a_includeWeights ) fprintf( a_fOut, "       weighted pdf       weights       weighted fraction" );
            fprintf( a_fOut, "\n" );

            double offset = a_histogram ? 0.0 : 0.5;
            if( _total == 0 ) _total = 1;
            if( weightedTotal == 0.0 ) weightedTotal = 1.0;
            double norm = m_domainWidth / static_cast<double>( m_bins.size( ) + 1 );
            double priorPdf = 0.0, priorFraction = 0.0, priorWeightedPdf = 0.0, priorWeights = 0.0, priorWeightedFraction = 0.0;
            long priorCount = 0;
            for( std::size_t i1 = 0; i1 <  m_bins.size( ); ++i1 ) {
                double x1;

                if( m_logDomainStep ) {
                    x1 = m_domainMin * exp( m_logDomainFraction * ( static_cast<double>( i1 ) + offset ) );
                    norm = x1 * ( exp( m_logDomainFraction ) - 1 ); }
                else {
                    x1 = ( static_cast<double>( i1 ) + offset ) / ( (double) m_bins.size( ) ) * m_domainWidth + m_domainMin;
                }

                if( a_histogram ) {
                    fprintf( a_fOut, "%22.14e  %15.7e  %8ld  %15.7e", x1, priorPdf, priorCount, priorFraction );
                    if( a_includeWeights ) {
                        fprintf( a_fOut, "  %15.7e  %15.7e  %15.7e",  priorWeightedPdf, priorWeights, priorWeightedFraction );
                    }
                    fprintf( a_fOut, "\n" );
                }

                priorCount = m_bins[i1];
                priorFraction = static_cast<double>( m_bins[i1] ) / static_cast<double>( _total );
                priorPdf = priorFraction / norm;
                fprintf( a_fOut, "%22.14e  %15.7e  %8ld  %15.7e", x1, priorPdf, priorCount, priorFraction );

                if( a_includeWeights ) {
                    priorWeights = m_weightedBins[i1];
                    priorWeightedFraction = m_weightedBins[i1] / weightedTotal;
                    priorWeightedPdf = priorWeightedFraction / norm;

                    fprintf( a_fOut, "  %15.7e  %15.7e  %15.7e",  priorWeightedPdf, priorWeights, priorWeightedFraction );
                }
                fprintf( a_fOut, "\n" );
            }

            if( a_histogram ) {
                fprintf( a_fOut, "%22.14e  %15.7e  %8ld  %15.7e", m_domainMax, priorPdf, priorCount, priorFraction );
                if( a_includeWeights ) {
                    fprintf( a_fOut, "  %15.7e  %15.7e  %15.7e",  priorWeightedPdf, priorWeights, priorWeightedFraction );
                }
                fprintf( a_fOut, "\n" );

                long zero = 0;
                fprintf( a_fOut, "%22.14e  %15.7e  %8ld  %15.7e", m_domainMax, 0.0, zero, 0.0 );
                if( a_includeWeights ) {
                    fprintf( a_fOut, "  %15.7e  %15.7e  %15.7e",  0.0, 0.0, 0.0 );
                }
                fprintf( a_fOut, "\n" );
            }
        }
};
