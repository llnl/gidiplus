/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include "HAPI.hpp"

#ifdef HAPI_USE_PUGIXML
namespace HAPI {

/*
=========================================================
 *
 * @return
 */
PugiXMLFile::PugiXMLFile() :
        m_name( "" ) {

}
/*
=========================================================
 *
 * @param filename
 * @return
 */
PugiXMLFile::PugiXMLFile(char const *filename, std::string const &a_callingFunctionName) :
        m_name( filename ) {

    pugi::xml_parse_result result = m_doc.load_file(filename);
    if (result.status != pugi::status_ok) {
        throw std::runtime_error( "ERROR from PugiXMLFile::PugiXMLFile via " + a_callingFunctionName + " for file '" + filename + "': " + result.description() );
    }

}

/* *********************************************************************************************************//**
 * Constuctor for XML data in a std::string.
 *
 * @param a_name                        [in]    The user defined name of the XML string.
 * @param a_xmlString                   [in]    The XML data to parse.
 * @param a_callingFunctionName         [in]    Name of the calling function.
 ***********************************************************************************************************/

PugiXMLFile::PugiXMLFile( std::string const &a_name, std::string const &a_xmlString, std::string const &a_callingFunctionName ) :
        m_name( a_name ) {

    pugi::xml_parse_result result = m_doc.load_buffer( a_xmlString.c_str( ), a_xmlString.size( ) );
    if( result.status != pugi::status_ok ) {
        throw std::runtime_error( "ERROR from PugiXMLFile::PugiXMLFile via " + a_callingFunctionName + " for string named '" + a_name + "': " + result.description( ) );
    }
}

/*
=========================================================
*/
PugiXMLFile::~PugiXMLFile( ) {

}
/*
============================================================
===================== get child element ====================
============================================================
 *
 * @return
 */
Node PugiXMLFile::child(char const *a_name) {

    // Only one child element allowed in XML file
    if (a_name == m_doc.first_child( ).name())
        return Node(new PugiXMLNode( m_doc.first_child( ) ));
    else
        return Node(new PugiXMLNode( ));
}
/*
=========================================================
*/
Node PugiXMLFile::first_child() {

    return Node(new PugiXMLNode( m_doc.first_child( ) ));

}
/*
=========================================================
*/
std::string PugiXMLFile::name() const {

    return m_name;

}

}
#endif
