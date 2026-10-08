/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include <RISI.hpp>
#include <PoPI.hpp>

static char const *description = "Loads .ris file and PoPs database(s), prints a summary of all available targets with their ZA values.";

void main2( int argc, char **argv );

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
    LUPI::Positional *risPathArgument = argumentParser.add<LUPI::Positional>( "risPath", "Path to a .ris file.", 0, 1 );
    LUPI::OptionStore *popsPathArgument = argumentParser.add<LUPI::OptionStore>( "--pops", "Path to PoPs database file(s).", 0, -1 );
    LUPI::OptionStore *projectileArgument = argumentParser.add<LUPI::OptionStore>( "-p", "Only print targets for the specified projectile.", 0, 1 );

    argumentParser.parse( argc, argv );

    // Load PoPs database(s)
    PoPI::Database pops;
    std::vector<std::string> popsPaths = popsPathArgument->values();
    if( popsPathArgument->counts( ) == 0 ) {
        popsPaths.push_back("../../../GIDI/Test/pops.xml");
    }
    for ( const auto& popsPath : popsPaths ) {
        std::cout << "Loading PoPs database from: " << popsPath << std::endl;
        pops.addFile(popsPath.c_str(), true); // true: warn about duplicate particles
    }

    // Load RIS file
    std::string risPath = "../../../GIDI/Test/Data/MG_MC/all.ris";
    if( risPathArgument->counts( ) > 0 ) risPath = risPathArgument->value();
    std::cout << "Loading RIS file from: " << risPath << std::endl << std::endl;
    GIDI::RISI::Projectiles projectiles;
    GIDI::RISI::readRIS( risPath, "MeV", projectiles );

    // Check if a specific projectile was requested
    std::vector<std::string> projectileIds = projectiles.projectileIds();
    if( projectileArgument->counts() > 0 ) {
        std::string projectileId = projectileArgument->value();
        if (std::find(projectileIds.begin(), projectileIds.end(), projectileId) != projectileIds.end()) {
            projectileIds.clear();
            projectileIds.push_back(projectileId);
        }
        else {
            std::cerr << "Projectile " << projectileId << " not found" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Loop over projectiles and corresponding targets
    std::string fissileFlag;
    for( const auto& projectileId : projectileIds ) {
        const GIDI::RISI::Projectile *proj = projectiles.projectile( projectileId );
        std::vector<std::string> targets = proj->targetIds();

        if( !targets.empty() ) {
            std::cout << "Projectile " << projectileId << " has " << targets.size() << " targets" << std::endl << std::endl;
            std::cout << "  Target" << std::right << std::setw(21) << "ZA" << std::setw(12) << "Note" << std::endl;
            std::cout << std::string(60, '=') << std::endl;

            // Add all targets to the unique set
            for( const auto& target : targets ) {
                if( pops.exists( target ) ) {
                    int za = PoPI::particleZA( pops, target );
                    fissileFlag = (proj->target( target )->fissionPresent() ? "Fissile" : "");
                    std::cout << "  " << std::left << std::setw(25) << target << std::setw(10) << za << fissileFlag << std::endl;
                } else {
                    std::cout << "  " << std::left << std::setw(25) << target
                            << " N/A      (not in PoPs)" << std::endl;
                }
            }
        } else {
            std::cout << "Projectile " << projectileId << " has no targets" << std::endl;
        }
        std::cout << std::endl;
    }
}
