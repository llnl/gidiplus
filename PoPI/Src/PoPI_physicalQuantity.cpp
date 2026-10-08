/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "PoPI.hpp"

namespace PoPI {

#define PoPI_valueChars "value"
#define PoPI_unitChars "unit"

/*! \class PhysicalQuantity
 * The base class for all PhysicalQuantity classes.
 */

/* *********************************************************************************************************//**
 * @param a_class                   [in]    The class of the physical quantity.
 * @param a_tag                     [in]    The tag (i.e., moniker) for ths physical quantity.
 * @param a_label                   [in]    The label for the physical quantity.
 * @param a_valueString             [in]    The string value for the physical quantity.
 * @param a_unit                    [in]    The units of the value.
 ***********************************************************************************************************/

PhysicalQuantity::PhysicalQuantity( PQ_class a_class, std::string const &a_tag, std::string const &a_label, 
                std::string const &a_valueString, std::string const &a_unit ) :
        m_class( a_class ),
        m_tag( a_tag ),
        m_label( a_label ),
        m_valueString( a_valueString ),
        m_unit( a_unit ) {

}

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 * @param a_class                   [in]    The class of the physical quantity.
 ***********************************************************************************************************/

PhysicalQuantity::PhysicalQuantity( HAPI::Node const &a_node, PQ_class a_class ) :
        m_class( a_class ),
        m_tag( a_node.name( ) ),
        m_label( a_node.attribute( PoPI_labelChars ).value( ) ),
        m_valueString( a_node.attribute( PoPI_valueChars ).value( ) ),
        m_unit( a_node.attribute( PoPI_unitChars ).value( ) ) {

}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

PhysicalQuantity::~PhysicalQuantity( ) {

}

/* *********************************************************************************************************//**
 * Adds the contents of *this* to *a_XMLList* where each item in *a_XMLList* is one line (without linefeeds) to output as an XML representation of *this*.
 *
 * @param a_XMLList                 [in]    The list to add an XML output representation of *this* to.
 * @param a_indent1                 [in]    The amount of indentation to added to each line added to *a_XMLList*.
 ***********************************************************************************************************/

void PhysicalQuantity::toXMLList( std::vector<std::string> &a_XMLList, std::string const &a_indent1 ) const {

    std::string _unit;

    if( m_unit.size( ) > 0 ) _unit = "\" unit=\"" + m_unit;

    std::string header = a_indent1 + "<" + m_tag + " label=\"" + m_label + "\" value=\"" + valueToString( ) + _unit + "\"/>";
    a_XMLList.push_back( std::move( header ) );
}

/*! \class PQ_double
 * The physical quantity class representing a double.
 */

/* *********************************************************************************************************//**
 * @param a_label                   [in]    The label for the physical quantity.
 * @param a_value                   [in]    The value for the physical quantity.
 * @param a_unit                    [in]    The units of the value.
 ***********************************************************************************************************/

PQ_double::PQ_double( std::string const &a_label, double a_value, std::string const &a_unit ) :
        PhysicalQuantity( PQ_class::Double, "double", a_label, "", a_unit ),
        m_value( a_value ) {

    m_valueString = valueToString( );
}

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 ***********************************************************************************************************/

PQ_double::PQ_double( HAPI::Node const &a_node ) :
        PhysicalQuantity( a_node, PQ_class::Double ),
        m_value( 0.0 ) {

        initialize( );
}

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 * @param a_class                   [in]    The class of the physical quantity.
 ***********************************************************************************************************/

PQ_double::PQ_double( HAPI::Node const &a_node, PQ_class a_class ) :
        PhysicalQuantity( a_node, a_class ),
        m_value( 0.0 ) {

        initialize( );
}

/* *********************************************************************************************************//**
 * This method is called my the constructors to do the common stuff.
 ***********************************************************************************************************/

void PQ_double::initialize( ) {

    char *last;

    if( valueString( ) != "" ) m_value = strtod( valueString( ).c_str( ), &last );
}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

PQ_double::~PQ_double( ) {

}

/* *********************************************************************************************************//**
 * Returns the value of *this* in units of *a_unit*. Currently, unit conversion is not supported.
 *
 * @param a_unit                    [in]    The requested unit to return the value in.
 *
 * @return                                  The value of *this* in units of *a_unit*.
 ***********************************************************************************************************/

double PQ_double::value( LUPI_maybeUnused char const *a_unit ) const {

    return( m_value );
}

/* *********************************************************************************************************//**
 * Converts the double value of self to a string.
 *
 * @return                                  The string value of *this*.
 ***********************************************************************************************************/

std::string PQ_double::valueToString( void ) const {

    std::string sValue = LUPI::Misc::argumentsToString( "%.12g", m_value );
    if( fabs( m_value ) < 1e10 ) {
        if( sValue.find( '.' ) == std::string::npos ) sValue = LUPI::Misc::argumentsToString( "%.1f", m_value );
    }

    return( sValue );
}

/*! \class PQ_integer
 * The physical quantity class representing an integer.
 */

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 ***********************************************************************************************************/

PQ_integer::PQ_integer( HAPI::Node const &a_node ) :
        PhysicalQuantity( a_node, PQ_class::integer ),
        m_value( a_node.attribute( PoPI_valueChars ).as_int( ) ) {
}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

PQ_integer::~PQ_integer( ) {

}

/* *********************************************************************************************************//**
 * Returns the value of *this* in units of *a_unit*. Currently, unit conversion is not supported.
 *
 * @param a_unit                    [in]    The requested unit to return the value in.
 *
 * @return                                  The value of *this* in units of *a_unit*.
 ***********************************************************************************************************/

int PQ_integer::value( LUPI_maybeUnused char const *a_unit ) const {

    return( m_value );
}

/* *********************************************************************************************************//**
 * Convert the integer value of *this* to a string.
 *
 * @return                                  The string value of *this*.
 ***********************************************************************************************************/

std::string PQ_integer::valueToString( void ) const {

    return( LUPI::Misc::argumentsToString( "%d", m_value ) );
}

/*! \class PQ_fraction
 * The physical quantity class representing a fraction.
 */

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 ***********************************************************************************************************/

PQ_fraction::PQ_fraction( HAPI::Node const &a_node ) :
        PhysicalQuantity( a_node, PQ_class::fraction ) {

}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

PQ_fraction::~PQ_fraction( ) {

}

/* *********************************************************************************************************//**
 * Returns the value of *this* in units of *a_unit*. Currently, unit conversion is not supported.
 *
 * @param a_unit                    [in]    The requested unit to return the value in.
 *
 * @return                                  The value of *this* in units of *a_unit*.
 ***********************************************************************************************************/

std::string PQ_fraction::value( LUPI_maybeUnused char const *a_unit ) const {

    return( valueString( ) );
}

/* *********************************************************************************************************//**
 * Returns the value as a string.
 *
 * @return                                  The string value of *this*.
 ***********************************************************************************************************/

std::string PQ_fraction::valueToString( void ) const {

    return( valueString( ) );
}

/*! \class PQ_string
 * The physical quantity class represented as a string.
 */

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 ***********************************************************************************************************/

PQ_string::PQ_string( HAPI::Node const &a_node ) :
        PhysicalQuantity( a_node, PQ_class::string ) {

}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

PQ_string::~PQ_string( ) {

}

/* *********************************************************************************************************//**
 * Returns the value of *this* in units of *a_unit*. Currently, unit conversion is not supported.
 *
 * @param a_unit                    [in]    The requested unit to return the value in.
 *
 * @return                                  The value of *this* in units of *a_unit*.
 ***********************************************************************************************************/

std::string PQ_string::value( LUPI_maybeUnused char const *a_unit ) const {

    return( valueString( ) );
}

/* *********************************************************************************************************//**
 * Returns the string value of *this*.
 *
 * @return                                  The string value of *this*.
 ***********************************************************************************************************/

std::string PQ_string::valueToString( void ) const {

    return( valueString( ) );
}

/*! \class PQ_shell
 * The physical quantity that represents the probability as a double that a process like internal conversion or pair production occurs.
 */

/* *********************************************************************************************************//**
 * @param a_node                    [in]    The **HAPI::Node** node to be parsed.
 ***********************************************************************************************************/

PQ_shell::PQ_shell( HAPI::Node const &a_node ) :
        PQ_double( a_node, PQ_class::shell ) {

}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

PQ_shell::~PQ_shell( ) {

}

}
