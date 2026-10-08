/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <RISI.hpp>
#include <PoPI.hpp>

static char const *description = "Loads a PoPs database and .ris file, then computes products for specified targets using RISI::Projectile::products() method.";

void main2( int argc, char **argv );
void parseEnergyRange( const std::string& energyRangeString, double& minEnergy, double& maxEnergy );
GIDI::RISI::ReactionTypes parseReactionType( std::string reactionTypeString );

/*
=========================================================
*/
int main( int argc, char **argv ) {

    try {
        main2( argc, argv ); }
     catch (std::exception &exception) {
        std::cerr << exception.what( ) << std::endl;
        exit( EXIT_FAILURE ); }
    catch (char const *str) {
        std::cerr << str << std::endl;
        exit( EXIT_FAILURE ); }
    catch (std::string &str) {
        std::cerr << str << std::endl;
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}

/*
=========================================================
*/
void main2( int argc, char **argv ) {

    LUPI::ArgumentParser argumentParser( __FILE__, description );
    LUPI::Positional *risPathArgument = argumentParser.add<LUPI::Positional>( "risPath", "The path to the .ris file to read.", 1, 1 );
    LUPI::Positional *targetsArgument = argumentParser.add<LUPI::Positional>( "targets", "List of initial target IDs (e.g., H1 He4 C12).", 1, -1 );
    LUPI::OptionStore *maxEnergyArgument = argumentParser.add<LUPI::OptionStore>( "--max-energy", "Maximum projectile energy in MeV (default: 20).", 0, 1 );
    LUPI::OptionStore *maxLevelArgument = argumentParser.add<LUPI::OptionStore>( "--max-level", "Maximum number of reaction steps to search (default: 5).", 0, 1 );
    LUPI::OptionStore *popsPathArgument = argumentParser.add<LUPI::OptionStore>( "--pops", "Path to PoPs database(s) (default: data/nuclear/common/pops.xml).", 0, -1 );
    LUPI::OptionStore *projectileArgument = argumentParser.add<LUPI::OptionStore>( "--projectile", "Projectile ID (default: n).", 0, 1 );
    LUPI::OptionStore *reactionTypeArgument = argumentParser.add<LUPI::OptionStore>( "--reaction-type",
            "Reaction type filter: All, AllButScatter, NeutronEmitting, NeutronAbsorbing, or FissionOnly (default: All).", 0, 1 );
    LUPI::OptionTrue *includeSumOfRemainingArg = argumentParser.add<LUPI::OptionTrue>( "--includeSumOfRemaining", "Include MT=5 reaction products (default: false).", 0, -1 );

    argumentParser.parse( argc, argv );

    std::string risPath = risPathArgument->value( );
    std::vector<std::string> targets = targetsArgument->values( );
    
    // Load PoPs database(s)
    PoPI::Database pops;
    std::vector<std::string> popsFiles = popsPathArgument->values();
    if (popsFiles.size() == 0) {
        popsFiles.push_back("/usr/gapps/data/nuclear/common/pops.xml");
    }
    for ( const auto& popsPath : popsFiles ) {
        std::cout << "Loading PoPs database from: " << popsPath << std::endl;
        pops.addFile(popsPath.c_str(), true); // true: warn about duplicate particles
    }
    
    std::string projectileId = "n";
    if( projectileArgument->counts( ) > 0 ) projectileId = projectileArgument->value( );
    
    double maxEnergy = 20.0;
    if( maxEnergyArgument->counts( ) > 0 ) {
        maxEnergy = std::stod( maxEnergyArgument->value( ) );
    }
    
    int maxLevel = 5;
    if( maxLevelArgument->counts( ) > 0 ) {
        maxLevel = std::stoi( maxLevelArgument->value( ) );
    }

    GIDI::RISI::ReactionTypes reactionType = GIDI::RISI::ReactionTypes::All;
    if( reactionTypeArgument->counts( ) > 0 ) {
        reactionType = parseReactionType( reactionTypeArgument->value( ) );
    }

    bool includeSumOfRemainingOutputChannels = includeSumOfRemainingArg->isTrue( );

    // Load RIS file
    GIDI::RISI::Projectiles projectiles;
    GIDI::RISI::readRIS( risPath, "MeV", projectiles );

    // Get the specified projectile
    const GIDI::RISI::Projectile *projectile = projectiles.projectile( projectileId );
    if( projectile == nullptr ) {
        std::cerr << "Error: Projectile '" << projectileId << "' not found in RIS file." << std::endl;
        std::cerr << "Available projectiles: ";
        std::vector<std::string> availableProjectiles = projectiles.projectileIds();
        for( size_t i = 0; i < availableProjectiles.size(); ++i ) {
            std::cerr << availableProjectiles[i];
            if( i < availableProjectiles.size() - 1 ) std::cerr << ", ";
        }
        std::cerr << std::endl;
        exit( EXIT_FAILURE );
    }

    // Compute products for each target
    std::cout << "=== Products Analysis ===" << std::endl;
    std::cout << std::left << std::setw(15) << "Target" << std::setw(15) << "Target ZA" << std::setw(20) << "Product" << std::setw(15) << "Product ZA" << std::setw(10) << "Steps" << std::endl;
    std::cout << std::string(75, '=') << std::endl;

    for( const auto& targetId : targets ) {
        std::cout << std::endl << "--- Target: " << targetId << " ---" << std::endl;
        
        // Get target ZA if available
        std::string targetZAStr = "N/A";
        try {
            if( pops.exists( targetId ) ) {
                int targetZA = PoPI::particleZA( pops, targetId );
                targetZAStr = std::to_string( targetZA );
            }
        } catch( const std::exception& e ) {
            targetZAStr = "ERROR";
        }

        // Get products for this target
        std::map<std::string, int> products;
        try {
            projectile->products( targetId, 0, maxLevel, maxEnergy, products, reactionType, includeSumOfRemainingOutputChannels );
            
            if( products.empty() ) {
                std::cout << std::left << std::setw(15) << targetId 
                         << std::setw(15) << targetZAStr 
                         << std::setw(20) << "No products found"
                         << std::setw(15) << "-" 
                         << std::setw(10) << 0 << std::endl;
            } else {
                bool firstProduct = true;
                for( const auto& productPair : products ) {
                    const std::string& productId = productPair.first;
                    
                    // Get product ZA if available
                    std::string productZAStr = "N/A";
                    try {
                        if( pops.exists( productId ) ) {
                            int productZA = PoPI::particleZA( pops, productId );
                            productZAStr = std::to_string( productZA );
                        }
                    } catch( const std::exception& e ) {
                        productZAStr = "ERROR";
                    }
                    
                    if( firstProduct ) {
                        std::cout << std::left << std::setw(15) << targetId 
                                 << std::setw(15) << targetZAStr 
                                 << std::setw(20) << productId
                                 << std::setw(15) << productZAStr 
                                 << std::setw(15) << productPair.second << std::endl;
                        firstProduct = false;
                    } else {
                        std::cout << std::left << std::setw(15) << "" 
                                 << std::setw(15) << "" 
                                 << std::setw(20) << productId
                                 << std::setw(15) << productZAStr 
                                 << std::setw(15) << productPair.second << std::endl;
                    }
                }
            }
        } catch( const std::exception& e ) {
            std::cout << std::left << std::setw(15) << targetId 
                     << std::setw(15) << targetZAStr 
                     << std::setw(20) << "EXCEPTION"
                     << std::setw(15) << "-" 
                     << std::setw(10) << "ERROR" << std::endl;
            std::cout << "  Exception: " << e.what() << std::endl;
        }
    }

    // Compute the list of products for *all* seed targets and check whether any are fissionable:
    std::vector<std::string> productIds = projectiles.products( projectileId, targets, maxLevel, maxEnergy, true, reactionType, includeSumOfRemainingOutputChannels );
    std::string fission = (projectile->fissionPresent(productIds) ? " and fission" : "");

    std::cout << std::endl << "The final product list contains " << productIds.size() << " products" << fission << "." << std::endl;

    std::cout << std::endl << "Products analysis complete." << std::endl;

}

/*
=========================================================
*/
GIDI::RISI::ReactionTypes parseReactionType( std::string reactionTypeString ) {

    std::string originalReactionTypeString = reactionTypeString;
    std::transform( reactionTypeString.begin( ), reactionTypeString.end( ), reactionTypeString.begin( ),
            []( unsigned char character ) { return static_cast<char>( std::tolower( character ) ); } );

    if( ( reactionTypeString == "all" ) || ( reactionTypeString == "0" ) ) {
        return GIDI::RISI::ReactionTypes::All;
    }
    if( ( reactionTypeString == "allbutscatter" ) || ( reactionTypeString == "1" ) ) {
        return GIDI::RISI::ReactionTypes::AllButScatter;
    }
    if( ( reactionTypeString == "neutronemitting" ) || ( reactionTypeString == "2" ) ) {
        return GIDI::RISI::ReactionTypes::NeutronEmitting;
    }
    if( ( reactionTypeString == "neutronabsorbing" ) || ( reactionTypeString == "3" ) ) {
        return GIDI::RISI::ReactionTypes::NeutronAbsorbing;
    }
    if( ( reactionTypeString == "fissiononly" ) || ( reactionTypeString == "4" ) ) {
        return GIDI::RISI::ReactionTypes::FissionOnly;
    }
    throw std::runtime_error( "Invalid reaction type '" + originalReactionTypeString
            + "'. Expected All, AllButScatter, NeutronEmitting, NeutronAbsorbing, FissionOnly, 0, 1, 2, 3 or 4." );
}
