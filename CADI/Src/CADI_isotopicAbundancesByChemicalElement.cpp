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

static GUPI::Entry *parseChemicalElement( LUPI_maybeUnused GUPI::Suite *a_parent, HAPI::Node const &a_node );

/*! \class IsotopicAbundancesByChemicalElement
 * The class that stores the atom fraction and its uncertainty for a chemical element's isotope.
 */

/* *********************************************************************************************************//**
 * IsotopicAbundancesByChemicalElement constructor.
 ***********************************************************************************************************/

IsotopicAbundancesByChemicalElement::IsotopicAbundancesByChemicalElement( ) :
        GUPI::Ancestry( CADI_isotopicAbundancesByChemicalElementChars ),
        m_chemicalElements( CADI_chemicalElementsChars, CADI_symbolChars ) {

}

/* *********************************************************************************************************//**
 * IsotopicAbundancesByChemicalElement constructor.
 *
 * @param a_evaluation          The evalation for the database.
 * @param a_format              The format the data are represented in.
 ***********************************************************************************************************/

IsotopicAbundancesByChemicalElement::IsotopicAbundancesByChemicalElement( std::string const &a_format, std::string const &a_evaluation ) :
        GUPI::Ancestry( CADI_isotopicAbundancesByChemicalElementChars ),
        m_format( a_format ),
        m_evaluation( a_evaluation ),
        m_chemicalElements( CADI_chemicalElementsChars, CADI_symbolChars ) {

    m_chemicalElements.setAncestor( this );
}

/* *********************************************************************************************************//**
 * IsotopicAbundancesByChemicalElement constructor.
 *
 * @param a_fileName                    [in]    The file containing the **isotopicAbundancesByChemicalElement** node to read.
 ***********************************************************************************************************/

IsotopicAbundancesByChemicalElement::IsotopicAbundancesByChemicalElement( std::string const &a_fileName ) :
        GUPI::Ancestry( CADI_isotopicAbundancesByChemicalElementChars ),
        m_chemicalElements( CADI_chemicalElementsChars, CADI_symbolChars ) {

    m_chemicalElements.setAncestor( this );

    HAPI::File *doc = nullptr;

    doc = new HAPI::PugiXMLFile( a_fileName.c_str( ), "IsotopicAbundancesByChemicalElement::IsotopicAbundancesByChemicalElement" );
    if( doc == nullptr ) {
        throw std::runtime_error( "Only XML/HDF file types supported." );
    }

    HAPI::Node isotopicAbundancesByChemicalElement = doc->first_child( );

    if( isotopicAbundancesByChemicalElement.name( ) != moniker( ) )
        throw LUPI::Exception( "Invalid IsotopicAbundancesByChemicalElement node with moniker (name) '" + 
            isotopicAbundancesByChemicalElement.name( ) + "'." );

    m_format = isotopicAbundancesByChemicalElement.attribute_as_string( CADI_formatChars );
    m_evaluation = isotopicAbundancesByChemicalElement.attribute_as_string( CADI_evaluationChars );
    m_chemicalElements.parse( isotopicAbundancesByChemicalElement.child( CADI_chemicalElementsChars ), parseChemicalElement );

    delete doc;
}

/* *********************************************************************************************************//**
 ***********************************************************************************************************/

IsotopicAbundancesByChemicalElement::~IsotopicAbundancesByChemicalElement( ) {

}

/* *********************************************************************************************************//**
 * Returns the **ChemicalElement** with symbol *a_symbol* if it exists; otherwise, **nullptr** is returned.
 *
 * @param a_symbol      [in]    The symbol for the chemical element whose isotopic abundance data are being requested.
 ***********************************************************************************************************/

ChemicalElement const *IsotopicAbundancesByChemicalElement::operator[]( std::string const &a_symbol ) const {

    for( auto iter = m_chemicalElements.begin( ); iter != m_chemicalElements.end( ); ++iter ) {
        ChemicalElement const *chemicalElement = static_cast<ChemicalElement const *>( *iter );
        if( chemicalElement->symbol( ) == a_symbol ) return( chemicalElement );
    }

    return( nullptr );
}

/* *********************************************************************************************************//**
 * If *a_evaluation* is equal to *m_evaluation*, returns the **ChemicalElement** with symbol *a_symbol* if it exists; otherwise, **nullptr** is returned.
 *
 * @param a_symbol      [in]    The symbol for the chemical element whose isotopic abundance data are being requested.
 * @param a_evaluation  [in]    The evalation for the database.
 *
 * @return                      Pointer to matching **ChemicalElement** or **nullptr** if no match is found.
 ***********************************************************************************************************/

ChemicalElement const *IsotopicAbundancesByChemicalElement::find( std::string const &a_symbol, std::string const &a_evaluation ) const {

    if( ( a_evaluation != "" ) && ( m_evaluation != a_evaluation ) ) return( nullptr );

    return( (*this)[a_symbol] );
}

/* *********************************************************************************************************//**
 * This method serializes *this* for broadcasting as needed for MPI and GPUs. The method can count the number of required
 * bytes, pack *this* or unpack *this* depending on *a_mode*.
 *
 * @param a_buffer              [in]    The buffer to read or write data to depending on *a_mode*.
 * @param a_mode                [in]    Specifies the action of this method.
 ***********************************************************************************************************/

LUPI_HOST void IsotopicAbundancesByChemicalElement::serialize( LUPI::DataBuffer &a_buffer, LUPI::DataBuffer::Mode a_mode ) {

    GUPI::Ancestry::serialize( a_buffer, a_mode );
    DATA_MEMBER_STD_STRING( m_format, a_buffer, a_mode );
    DATA_MEMBER_STD_STRING( m_evaluation, a_buffer, a_mode );

    std::size_t vectorSize = m_chemicalElements.size( );
    int vectorSizeInt = (int) vectorSize;
    DATA_MEMBER_INT( vectorSizeInt, a_buffer, a_mode );
    vectorSize = (std::size_t) vectorSizeInt;

    if( a_mode == LUPI::DataBuffer::Mode::Unpack ) {
        for( std::size_t vectorIndex = 0; vectorIndex < vectorSize; ++vectorIndex ) {
                ChemicalElement *chemicalElement = new ChemicalElement( "" );
                chemicalElement->serialize( a_buffer, a_mode );
                m_chemicalElements.add( chemicalElement );
        } }
    else {
        for( std::size_t vectorIndex = 0; vectorIndex < vectorSize; ++vectorIndex ) {
            m_chemicalElements.get<ChemicalElement>( vectorIndex )->serialize( a_buffer, a_mode );
        }
    }
}

/* *********************************************************************************************************//**
 * Fills the argument *a_writeInfo* with the XML lines that represent *this*. Recursively enters each sub-node.
 *
 * @param       a_writeInfo         [in/out]    Instance containing incremental indentation and other information and stores the appended lines.
 * @param       a_indent            [in]        The amount to indent *this* node.
 ***********************************************************************************************************/

void IsotopicAbundancesByChemicalElement::toXMLList( GUPI::WriteInfo &a_writeInfo, std::string const &a_indent ) const {

    std::string indent2 = a_writeInfo.incrementalIndent( a_indent );
    std::string attributes;

    attributes  = a_writeInfo.addAttribute( CADI_evaluationChars, m_evaluation );
    attributes += a_writeInfo.addAttribute( CADI_formatChars, m_format );
    a_writeInfo.addNodeStarter( a_indent, moniker( ), attributes );
    m_chemicalElements.toXMLList( a_writeInfo, indent2 );
    a_writeInfo.addNodeEnder( moniker( ) );
}

/* *********************************************************************************************************//**
 * Fills the argument *a_writeInfo* with the XML lines that represent *this*. Recursively enters each sub-node.
 *
 * @param a_node                        [in]    HAPI node to be parsed and used to construct a **ChemicalElement**.
 *
 * @return                                      Returns a parsed **ChemicalElement** node.
 ***********************************************************************************************************/

static GUPI::Entry *parseChemicalElement( LUPI_maybeUnused GUPI::Suite *a_parent, HAPI::Node const &a_node ) {

    return new ChemicalElement( a_node );
}

}               // End of namespace CADI.
