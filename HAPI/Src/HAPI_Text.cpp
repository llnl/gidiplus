/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include "HAPI.hpp"

namespace HAPI {

/*
=========================================================
 *
 * @return
 */
Text::Text() :
        m_text( "" ) {

}
/*
=========================================================
 *
 * @param a_text text string
 * @return
 */
Text::Text( std::string const &a_text ) :
        m_text( a_text ) {

}
/*
=========================================================
*/
Text::~Text( ) {

}

}

