/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include "PoPI.hpp"

namespace PoPI {

#define PoPI_generationChars "generation"

/*! \class Lepton
 * This class represents **PoPs** lepton instance.
 */

/* *********************************************************************************************************//**
 * Basic lepton constructor.
 *
 * @param a_id              [in]    The **PoPs** id for the lepton.
 * @param a_generation      [in]    The lepton's generation.
 * @param a_mass            [in]    The mass of the lepton.
 ***********************************************************************************************************/

Lepton::Lepton( std::string const &a_id, std::string const &a_generation, PhysicalQuantity *a_mass ):
        Particle( a_id, Particle_class::lepton, PoPI_leptonChars ),
        m_generation( a_generation ) {

    if( a_mass != nullptr ) m_mass.push_back( a_mass );
}

/* *********************************************************************************************************//**
 * Constructor that parses an **HAPI** instance to create a **PoPs** lepton node.
 *
 * @param a_node            [in]    The **HAPI::Node** to be parsed.
 * @param a_DB              [in]    The **PoPI::Database:: instance to add the constructed **Lepton** to.
 * @param a_parent          [in]    The parent suite that will contain *this*.
 ***********************************************************************************************************/

Lepton::Lepton( HAPI::Node const &a_node, Database *a_DB, LUPI_maybeUnused Database *a_parent ) :
        Particle( a_node, Particle_class::lepton, PoPI_leptonChars ),
        m_generation( a_node.attribute( PoPI_generationChars ).value( ) ) {

    if( ID( ).substr(0, 2) == IDs::electron ) setIntid( intidHelper( isAnti( ), Particle_class::lepton, 0 ) );

    addToDatabase( a_DB );
}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

Lepton::~Lepton( ) {

}

/* *********************************************************************************************************//**
 * Returns the generation attribute.
 ***********************************************************************************************************/

std::string Lepton::toXMLListExtraAttributes( void ) const {

    return( std::string( " generation=\"" + m_generation + "\"" ) );
}

}
