/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <algorithm>

#include <HAPI.hpp>
#include <CADI.hpp>

namespace CADI {

static GUPI::Entry *parseIsotope( GUPI::Suite *a_parent, HAPI::Node const &a_node );

/*! \class ChemicalElement
 * The class that stores the atom fraction and its uncertainty for a chemical element's isotope.
 */

/* *********************************************************************************************************//**
 * ChemicalElement constructor.
 *
 * @param a_symbol              The **PoPs** symbol for the chemical element.
 ***********************************************************************************************************/

ChemicalElement::ChemicalElement( std::string const &a_symbol ) :
        GUPI::Entry( CADI_chemicalElementChars, CADI_symbolChars, a_symbol ),
        m_isotopes( CADI_isotopesChars, CADI_idChars ) {

    m_isotopes.setAncestor( this );
}

/* *********************************************************************************************************//**
 * ChemicalElement constructor.
 *
 * @param a_node                        [in]    HAPI node to be parsed and used to construct an *this* **ChemicalElement**.
 ***********************************************************************************************************/

ChemicalElement::ChemicalElement( HAPI::Node const &a_node ) :
        GUPI::Entry( a_node, CADI_symbolChars ),
        m_isotopes( a_node.child( CADI_isotopesChars ), CADI_idChars, parseIsotope ) {

    m_isotopes.setAncestor( this );
}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

ChemicalElement::~ChemicalElement( ) {

}

/* *********************************************************************************************************//**
 * Returns the **ChemicalElement** with id *a_id* if it exists; otherwise, **nullptr** is returned.
 *
 * @param a_id      [in]    The id for the chemical element whose isotopic abundance data are being requested.
 ***********************************************************************************************************/

Isotope const *ChemicalElement::operator[]( std::string const &a_id ) const {

    for( auto isotopeIter = m_isotopes.begin( ); isotopeIter != m_isotopes.end( ); ++isotopeIter ) {
        Isotope const *isotope = static_cast<Isotope *>( *isotopeIter );
        if( isotope->id( ) == a_id ) return( isotope );
    }

    return( nullptr );
}

/* *********************************************************************************************************//**
 * This method serializes *this* for broadcasting as needed for MPI and GPUs. The method can count the number of required
 * bytes, pack *this* or unpack *this* depending on *a_mode*.
 *
 * @param a_buffer              [in]    The buffer to read or write data to depending on *a_mode*.
 * @param a_mode                [in]    Specifies the action of this method.
 ***********************************************************************************************************/

double ChemicalElement::mass( PoPI::Database const &a_pops, std::string const &a_unit ) const {

    double mass = 0.0;

    for( auto isotopeIter = m_isotopes.begin( ); isotopeIter != m_isotopes.end( ); ++isotopeIter ) {
        Isotope const *isotope = static_cast<Isotope *>( *isotopeIter );
        PoPI::Particle const &popsIsotope = a_pops.get<PoPI::Particle>( isotope->id( ) );

        mass += isotope->atomFraction( ) * popsIsotope.massValue( a_unit );
    }

    return( mass );
}

/* *********************************************************************************************************//**
 * This method serializes *this* for broadcasting as needed for MPI and GPUs. The method can count the number of required
 * bytes, pack *this* or unpack *this* depending on *a_mode*.
 *
 * @param a_buffer              [in]    The buffer to read or write data to depending on *a_mode*.
 * @param a_mode                [in]    Specifies the action of this method.
 ***********************************************************************************************************/

void ChemicalElement::serialize( LUPI::DataBuffer &a_buffer, LUPI::DataBuffer::Mode a_mode ) {

    GUPI::Entry::serialize( a_buffer, a_mode );

    std::size_t vectorSize = m_isotopes.size( );
    int vectorSizeInt = (int) vectorSize;
    DATA_MEMBER_INT( vectorSizeInt, a_buffer, a_mode );
    vectorSize = (std::size_t) vectorSizeInt;

    if( a_mode == LUPI::DataBuffer::Mode::Unpack ) {
        for( std::size_t vectorIndex = 0; vectorIndex < vectorSize; ++vectorIndex ) {
                Isotope *isotope = new Isotope( "", 0.0, 0.0 );
                isotope->serialize( a_buffer, a_mode );
                m_isotopes.add( isotope );
        } }
    else {
        for( std::size_t vectorIndex = 0; vectorIndex < vectorSize; ++vectorIndex ) {
            m_isotopes.get<Isotope>( vectorIndex )->serialize( a_buffer, a_mode );
        }
    }
}

/* *********************************************************************************************************//**
 * Fills the argument *a_writeInfo* with the XML lines that represent *this*. Recursively enters each sub-node.
 *
 * @param       a_writeInfo         [in/out]    Instance containing incremental indentation and other information and stores the appended lines.
 * @param       a_indent            [in]        The amount to indent *this* node.
 ***********************************************************************************************************/

void ChemicalElement::toXMLList( GUPI::WriteInfo &a_writeInfo, std::string const &a_indent ) const {

    std::string indent2 = a_writeInfo.incrementalIndent( a_indent );
    std::string attributes;

    attributes  = a_writeInfo.addAttribute( CADI_symbolChars, symbol( ) );
    a_writeInfo.addNodeStarter( a_indent, moniker( ), attributes );
    m_isotopes.toXMLList( a_writeInfo, indent2 );
    a_writeInfo.addNodeEnder( moniker( ) );
}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

static GUPI::Entry *parseIsotope( LUPI_maybeUnused GUPI::Suite *a_parent, HAPI::Node const &a_node ) {

    return new Isotope( a_node );
}

}               // End of namespace CADI.
