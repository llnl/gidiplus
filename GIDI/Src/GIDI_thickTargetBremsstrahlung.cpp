/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include "GIDI.hpp"

namespace GIDI {

/* *********************************************************************************************************//**
 * Helper to extract a grid from a *gridded2d* axis, with basic type checking.
 *
 * @param a_axis            [in]    Pointer to the axis to extract grid values from.
 *
 * @return                          Extracted grid values as a std::vector<double>.
 ***********************************************************************************************************/

static std::vector<double> gridFromAxis( GIDI::Axis const *a_axis ) {

    std::vector<double> gridValues;

    GIDI::Grid const *grid = dynamic_cast<GIDI::Grid const *>( a_axis );
    if( grid == nullptr ) throw std::runtime_error( "ThickTargetBremsstrahlung::parse: expected Grid axis for differentialCrossSection, axis name: " + a_axis->moniker() );

    nf_Buffer<double> const &values = grid->values( );
    gridValues.resize( values.size( ) );
    for( std::size_t index = 0; index < values.size( ); ++index ) gridValues[index] = values[index];

    return( gridValues );
}

/* *********************************************************************************************************//**
 * Helper to fill subshell occupancy and binding energies from the Protare's internal PoPs database.
 *
 * @param a_setupInfo                [in]    Helper information created by the Protare constructor.
 * @param a_electronsPerSubshell     [out]   Electrons per subshell, in PoPs configurations order.
 * @param a_subshellIonizationEnergies [out] Binding energies in eV, in PoPs configurations order.
 *
 * @return                                  True if populated, false otherwise.
 ***********************************************************************************************************/

static bool fillSubshellDataFromPoPs( SetupInfo const &a_setupInfo, std::vector<double> &a_electronsPerSubshell,
                std::vector<double> &a_subshellIonizationEnergies ) {

    if( a_setupInfo.m_protare == nullptr ) return( false );

    PoPI::Database const &pops = a_setupInfo.m_protare->internalPoPs( );
    std::string symbol = pops.chemicalElementSymbol( a_setupInfo.m_protare->GNDS_target( ).ID( ) );
    if( symbol == "" ) symbol = a_setupInfo.m_protare->GNDS_target( ).ID( );

    auto const &chemicalElements = pops.chemicalElements( );
    for( std::size_t i1 = 0; i1 < chemicalElements.size( ); ++i1 ) {
        PoPI::ChemicalElement const &chemicalElement = chemicalElements[i1];
        if( chemicalElement.symbol( ) != symbol ) continue;

        auto const &configs = chemicalElement.atomicConfigurations( );
        if( configs.size( ) == 0 ) return( false );

        a_electronsPerSubshell.clear( );
        a_subshellIonizationEnergies.clear( );
        a_electronsPerSubshell.reserve( configs.size( ) );
        a_subshellIonizationEnergies.reserve( configs.size( ) );

        for( std::size_t i2 = 0; i2 < configs.size( ); ++i2 ) {
            PoPI::AtomicConfiguration const &config = configs[i2];
            a_electronsPerSubshell.push_back( config.electronNumber( ) );
            a_subshellIonizationEnergies.push_back( config.bindingEnergy_eV( ) );
        }

        return( true );
    }

    return( false );
}

/* *********************************************************************************************************//**
 * Default constructor.
 ***********************************************************************************************************/

ThickTargetBremsstrahlung::ThickTargetBremsstrahlung( ) :
        Form( FormType::thickTargetBremsstrahlung ),
        m_meanExcitationEnergy( 0.0 ),
        m_screeningRadius( 0.0 ),
        m_hasData( false ) {
}

/* *********************************************************************************************************//**
 * Constructor that parses a GNDS *thickTargetBremsstrahlung* applicationData node.
 *
 * @param a_construction    [in]    Used to pass user options to the constructor.
 * @param a_node            [in]    The HAPI::Node for the *thickTargetBremsstrahlung* data.
 * @param a_setupInfo       [in]    Helper information created by the Protare constructor.
 * @param a_parent          [in]    The parent Suite for this form.
 ***********************************************************************************************************/

ThickTargetBremsstrahlung::ThickTargetBremsstrahlung( Construction::Settings const &a_construction, HAPI::Node const &a_node, SetupInfo &a_setupInfo,
                Suite *a_parent ) :
        Form( a_node, a_setupInfo, FormType::thickTargetBremsstrahlung, a_parent ),
        m_meanExcitationEnergy( 0.0 ),
        m_screeningRadius( 0.0 ),
        m_hasData( false ) {

    parse( a_construction, a_node, a_setupInfo, a_parent );
}

/* *********************************************************************************************************//**
 * Parses the GNDS *thickTargetBremsstrahlung* applicationData node into this instance.
 *
 * @param a_construction    [in]    Used to pass user options to the constructor.
 * @param a_node            [in]    The HAPI::Node for the *thickTargetBremsstrahlung* data.
 * @param a_setupInfo       [in]    Helper information created by the Protare constructor.
 * @param a_parent          [in]    The parent Suite for this form.
 ***********************************************************************************************************/

void ThickTargetBremsstrahlung::parse( Construction::Settings const &a_construction, HAPI::Node const &a_node, SetupInfo &a_setupInfo,
                Suite *a_parent ) {

    m_meanExcitationEnergy = 0.0;
    m_screeningRadius = 0.0;
    m_electronsPerSubshell.clear( );
    m_subshellIonizationEnergies.clear( );
    m_srad.clear( );
    m_egrid.clear( );
    m_pgrid.clear( );
    m_dcs.clear( );

    HAPI::Node const model = a_node.child( "thickTargetBremsstrahlungModel" );
    if( model.empty( ) ) throw std::runtime_error( "ThickTargetBremsstrahlung::parse: expected 'thickTargetBremsstrahlungModel' node." );

    for( HAPI::Node subchild = model.first_child( ); !subchild.empty( ); subchild.to_next_sibling( ) ) {
        std::string const subName( subchild.name( ) );

        if( subName == "meanExcitationEnergy" ) {
            HAPI::Node const pqNode = subchild.child( "physicalQuantity" );
            if( !pqNode.empty( ) ) {
                PhysicalQuantity const meanExcitationEnergy( pqNode, a_setupInfo );
                m_meanExcitationEnergy = meanExcitationEnergy.value( );
            }
        }
        else if( subName == "screeningRadius" ) {
            HAPI::Node const pqNode = subchild.child( "physicalQuantity" );
            if( !pqNode.empty( ) ) {
                PhysicalQuantity const screeningRadius( pqNode, a_setupInfo );
                m_screeningRadius = screeningRadius.value( );
            }
        }
        else if( subName == "radiativeStoppingPower" ) {
            HAPI::Node const xysNode = subchild.child( "XYs1d" );
            if( !xysNode.empty( ) ) {
                Functions::XYs1d const radiativeStoppingPower( a_construction, xysNode, a_setupInfo, a_parent );
                m_srad = radiativeStoppingPower.ys( );
            }
        }
        else if( subName == "differentialCrossSection" ) {
            HAPI::Node const dcsChild = subchild.child( "gridded2d" );
            if( !dcsChild.empty( ) ) {
                Form const *form = data2dParse( a_construction, dcsChild, a_setupInfo, a_parent );
                if( form == nullptr ) throw std::runtime_error( "ThickTargetBremsstrahlung::parse: failed to parse differentialCrossSection data" );
                Functions::Gridded2d const *gridded2d = dynamic_cast<Functions::Gridded2d const *>( form );
                if( gridded2d == nullptr ) throw std::runtime_error( "ThickTargetBremsstrahlung::parse: expected gridded2d for differentialCrossSection" );

                GIDI::Axes const &axes = gridded2d->axes( );

                // Scaled outgoing photon energy (first axis) and incident electron energy (second axis).
                m_pgrid = gridFromAxis( axes[0] );
                m_egrid = gridFromAxis( axes[1] );

                // Differential cross section values.
                GIDI::Array::FullArray const fullArray = gridded2d->array( ).constructArray( );
                m_dcs = fullArray.m_flattenedValues;
            }
        }
    }

    if( !fillSubshellDataFromPoPs( a_setupInfo, m_electronsPerSubshell, m_subshellIonizationEnergies ) ) {
        throw std::runtime_error( "ThickTargetBremsstrahlung::parse: expected subshell configuration data in PoPs (chemicalElements/chemicalElement/atomic/configurations)." );
    }

    if( m_electronsPerSubshell.size( ) != m_subshellIonizationEnergies.size( ) ) {
        throw std::runtime_error( "ThickTargetBremsstrahlung::parse: subshell electron counts and binding energies have different sizes." );
    }

    m_hasData = !m_electronsPerSubshell.empty() && !m_subshellIonizationEnergies.empty() && 
                !m_srad.empty() && !m_egrid.empty() && !m_pgrid.empty() && !m_dcs.empty();
}

/* *********************************************************************************************************//**
 * Destructor.
 ***********************************************************************************************************/

ThickTargetBremsstrahlung::~ThickTargetBremsstrahlung( ) {
}

}       // end namespace GIDI
