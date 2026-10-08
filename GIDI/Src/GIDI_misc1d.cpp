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

namespace Functions {

/*! \class CoulombPlusNuclearElastic1d
 * Class for the GNDS <***CoulombPlusNuclearElastic*> node for the <**crossSecton**> suite.
 */

/* *********************************************************************************************************//**
 *
 * @param a_construction    [in]    Used to pass user options to the constructor.
 * @param a_node            [in]    The **HAPI::Node** to be parsed and used to construct the XYs2d.
 * @param a_setupInfo       [in]    Information create my the Protare constructor to help in parsing.
 * @param a_parent          [in]    The parent GIDI::Suite.
 ***********************************************************************************************************/

CoulombPlusNuclearElastic1d::CoulombPlusNuclearElastic1d( Construction::Settings const &a_construction, HAPI::Node const &a_node,
		        SetupInfo &a_setupInfo, Suite *a_parent ) :
        Function1dForm( a_construction, a_node, a_setupInfo, FormType::CoulombPlusNuclearElastic1d, a_parent ),
        m_href( a_node.attribute_as_string( GIDI_hrefChars ) ) {

}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

CoulombPlusNuclearElastic1d::~CoulombPlusNuclearElastic1d( ) {

}

/* *********************************************************************************************************//**
 * Returns the domain minimum for the instance.
 *
 * @return          The domain minimum for the instance.
 ***********************************************************************************************************/

double CoulombPlusNuclearElastic1d::domainMin( ) const {

    if( moniker( ) != "" ) // This will always be false but without an if statement some compilers will complain.
        throw Exception( "CoulombPlusNuclearElastic1d::domainMin: not implemented" );

    return( 0.0 );
}

/* *********************************************************************************************************//**
 * Returns the domain maximum for the instance.
 *
 * @return              The domain maximum for the instance.
 ***********************************************************************************************************/

double CoulombPlusNuclearElastic1d::domainMax( ) const {

    if( moniker( ) != "" ) // This will always be false but without an if statement some compilers will complain.
        throw Exception( "CoulombPlusNuclearElastic1d::domainMax: not implemented" );  // This will also be false.

    return( 1.0 );
}

/* *********************************************************************************************************//**
 * Returns the value of the function *f(x1)* at the specified point and *a_x1*.
 * **This is currently not implemented**.
 *
 * @param a_x1              [in]    The value of the **x1** axis.
 * @return                          The value of the function evaluated at *a_x1*.
 ***********************************************************************************************************/

double CoulombPlusNuclearElastic1d::evaluate( LUPI_maybeUnused double a_x1 ) const {

    throw Exception( "CoulombPlusNuclearElastic1d::evaluate: not implemented" );
}

/*! \class ThermalNeutronScatteringLaw1d
 * Class for the GNDS <**thermalNeutronScatteringLaw**> node.
 */

/* *********************************************************************************************************//**
 *
 * @param a_construction    [in]    Used to pass user options to the constructor.
 * @param a_node            [in]    The **HAPI::Node** to be parsed and used to construct the XYs2d.
 * @param a_setupInfo       [in]    Information create my the Protare constructor to help in parsing.
 * @param a_parent          [in]    The parent GIDI::Suite.
 ***********************************************************************************************************/

ThermalNeutronScatteringLaw1d::ThermalNeutronScatteringLaw1d( Construction::Settings const &a_construction, HAPI::Node const &a_node,
		        SetupInfo &a_setupInfo, Suite *a_parent ) :
        Function1dForm( a_construction, a_node, a_setupInfo, FormType::thermalNeutronScatteringLaw1d, a_parent ),
        m_href( a_node.attribute_as_string( GIDI_hrefChars ) ) {

}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

ThermalNeutronScatteringLaw1d::~ThermalNeutronScatteringLaw1d( ) {

}

/* *********************************************************************************************************//**
 * Returns the domain minimum for the instance.
 *
 * @return          The domain minimum for the instance.
 ***********************************************************************************************************/

double ThermalNeutronScatteringLaw1d::domainMin( ) const {

    if( moniker( ) != "" ) // This will always be false but without an if statement some compilers will complain.
        throw Exception( "ThermalNeutronScatteringLaw1d::domainMin: not implemented" );  // This will also be false.

    return( 0.0 );
}

/* *********************************************************************************************************//**
 * Returns the domain maximum for the instance.
 *
 * @return              The domain maximum for the instance.
 ***********************************************************************************************************/

double ThermalNeutronScatteringLaw1d::domainMax( ) const {

    if( moniker( ) != "" ) // This will always be false but without an if statement some compilers will complain.
        throw Exception( "ThermalNeutronScatteringLaw1d::domainMax: not implemented" );  // This will also be false.

    return( 1.0 );
}

/* *********************************************************************************************************//**
 * Returns the value of the function *f(x1)* at the specified point and *a_x1*.
 * **This is currently not implemented**.
 *
 * @param a_x1              [in]    The value of the **x1** axis.
 * @return                          The value of the function evaluated at *a_x1*.
 ***********************************************************************************************************/

double ThermalNeutronScatteringLaw1d::evaluate( LUPI_maybeUnused double a_x1 ) const {

    throw Exception( "ThermalNeutronScatteringLaw1d::evaluate: not implemented" );
}

}               // End namespace Functions.

}               // End namespace GIDI.
