/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <iostream>
#include <iomanip>

#include <RISI.hpp>

static char const *description = "Lists all reactions for a given projectile and target, showing products and their multiplicities.";

void main2( int argc, char **argv );

/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv );
        exit( EXIT_SUCCESS ); }
    catch (std::exception &exception) {
        std::cerr << exception.what( ) << std::endl; }
    catch (char const *str) {
        std::cout << str << std::endl; }
    catch (std::string &str) {
        std::cout << str << std::endl;
    }

    exit( EXIT_FAILURE );
}

/*
=========================================================
*/
void main2( int argc, char **argv ) {

    LUPI::ArgumentParser argumentParser( __FILE__, description );
    LUPI::Positional *risPathArgument = argumentParser.add<LUPI::Positional>( "risPath", "The path to the .ris file to read.", 1, 1 );
    LUPI::Positional *targetArgument = argumentParser.add<LUPI::Positional>( "target", "Target particle ID (e.g., O16, U238).", 1, 1 );
    LUPI::OptionStore *projectileArgument = argumentParser.add<LUPI::OptionStore>( "--projectile", "Projectile ID (default: n).", 0, 1 );

    argumentParser.parse( argc, argv );

    std::string risFilePath = risPathArgument->value( );
    std::string targetId = targetArgument->value( );

    std::string projectileId = "n";
    if( projectileArgument->counts( ) > 0 ) projectileId = projectileArgument->value( );

    std::cout << "Reading RIS file: " << risFilePath << std::endl;
    std::cout << "Projectile: " << projectileId << std::endl;
    std::cout << "Target: " << targetId << std::endl;
    std::cout << std::endl;

    // Read the RIS file
    GIDI::RISI::Projectiles projectiles;
    try {
        GIDI::RISI::readRIS( risFilePath, "MeV", projectiles );
    }
    catch( ... ) {
        std::cerr << "Error reading RIS file: " << risFilePath << std::endl;
        exit( EXIT_FAILURE );
    }

    // Find the projectile
    GIDI::RISI::Projectile const *projectile = projectiles.projectile( projectileId );
    if( projectile == nullptr ) {
        std::cerr << "Projectile '" << projectileId << "' not found in RIS file." << std::endl;
        exit( EXIT_FAILURE );
    }

    // Find the target
    GIDI::RISI::Target const *target = projectile->target( targetId );
    if( target == nullptr ) {
        std::cerr << "Target '" << targetId << "' not found for projectile '" << projectileId << "'." << std::endl;
        exit( EXIT_FAILURE );
    }

    // Get the reactions for this target
    std::vector<GIDI::RISI::Reaction *> const &reactions = target->reactions( );

    if( reactions.empty( ) ) {
        std::cout << "No reactions found for " << projectileId << " + " << targetId << std::endl;
        exit( EXIT_SUCCESS );
    }

    std::cout << "Found " << reactions.size( ) << " reactions for " << projectileId << " + " << targetId << ":" << std::endl;
    std::cout << std::endl;

    // Iterate through reactions and print products with multiplicities
    for( std::size_t reactionIndex = 0; reactionIndex < reactions.size( ); ++reactionIndex ) {
        GIDI::RISI::Reaction const *reaction = reactions[reactionIndex];

        std::cout << reaction->m_reactionLabel << ": "
                  << reaction->m_process << " (threshold: "
                  << std::scientific << std::setprecision( 3 ) << reaction->m_effectiveThreshold
                  << " MeV)" << std::endl;

        // Print products and their multiplicities
        if( reaction->m_products.empty( ) ) {
            std::cout << "    (no products)" << std::endl;
        } else {
            for( std::size_t productIndex = 0; productIndex < reaction->m_products.size( ); ++productIndex ) {

                std::string const &productId = reaction->m_products[productIndex];
                int multiplicity = reaction->multiplicity( productId );

                std::cout << "    " << productId << " * " << multiplicity << std::endl;
            }
        }

        std::cout << std::endl;
    }
}