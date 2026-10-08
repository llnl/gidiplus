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
#include <cmath>
#include <iomanip>

#include <GIDI_testUtilities.hpp>
#include <MCGIDI.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <bins.hpp>

static char const *description = "By default (see 'option -d') this code samples reactions for a photo-atomic protare "
        "using photo-atomic doppler broadening reaction if present and prints the sampling results.";

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

    std::string photonId( PoPI::IDs::photon );
    std::string electronId( PoPI::IDs::electron );
    std::string positronId( PoPI::IDs::electron + PoPI::IDs::anti );
    PoPI::Database pops;
    argvOption *option;
    long numberOfSamples = -1;
    double energy = 0.1;

    argvOptions argv_options( "photoAtomicDopplerBroadening", description );

    ParseTestOptions parseTestOptions( argv_options, argc, argv );
    parseTestOptions.m_askPid = false;
    parseTestOptions.m_askPhotoAtomic = false;
    parseTestOptions.m_askPhotoNuclear = false;

    argv_options.add( argvOption( "-n", true, "Number of samples. If negative, value multiplied by -1000,000." ) );
    argv_options.add( argvOption( "-d", false, "If present, disable using photo-atomic incoherent doppler data." ) );
    argv_options.add( argvOption( "--energy", true, "The incident photon energy for sampling reactions." ) );
    argv_options.add( argvOption( "--sampleProducts", false, "If present, sampleProducts is call for each photo-electric reaction." ) );
    argv_options.add( argvOption( "-v", false, "Verbosity flag." ) );

    parseTestOptions.parse( );

    option = argv_options.find( "-v" );
    int verbosity = option->m_counter;

    option = argv_options.find( "--energy" );
    energy = option->asDouble( argv, energy );

    option = argv_options.find( "-n" );
    numberOfSamples = option->asLong( argv, numberOfSamples );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    std::cout << "numberOfSamples = " << numberOfSamples << std::endl;

    option = argv_options.find( "--sampleProducts" );
    bool sampleProducts = option->m_counter > 0;

    std::cout << "GIDI:" << std::endl;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all,
                                                GIDI::Construction::PhotoMode::atomicOnly );
    option = argv_options.find( "-d" );
    construction.setUsePhotoAtomicIncoherentDoppler( option->m_counter == 0 );
    GIDI::Protare *protare = parseTestOptions.protare( pops, "../../../TestData/PoPs/pops.xml", 
            "../../../GIDI/Test/Data/MG_MC/photo-atomic/all.map", construction, photonId, "O" );
    if( protare == nullptr )
        throw LUPI::Exception( "Protare not found." );

    GIDI::Transporting::Particles particles;
    GIDI::Transporting::Particle projectile( photonId, GIDI::Transporting::Mode::MonteCarloContinuousEnergy );
    particles.add( projectile );
    GIDI::Transporting::Particle projectile2( electronId, GIDI::Transporting::Mode::MonteCarloContinuousEnergy );
    particles.add( projectile2 );

    auto monikers = protare->styles( ).findAllOfMoniker( GIDI_MonteCarlo_cdfStyleChars );
    std::string MonteCarlo_cdf = "";
    if( monikers.size( ) == 1 ) {
        MonteCarlo_cdf = monikers[0][0]->label( );
    }
    std::cout << "  Monte Carlo cdf label = '" << MonteCarlo_cdf << "'" << std::endl;

    std::cout << "  Number of GIDI reactions " << protare->numberOfReactions( ) << std::endl;
    for( std::size_t index = 0; index < protare->numberOfReactions( ); ++index ) {
        GIDI::Reaction *reaction = protare->reaction( index );
        GIDI::OutputChannel *outputChannel = reaction->outputChannel( );
        GIDI::Product *product = static_cast<GIDI::Product *>( outputChannel->products( ).get<GIDI::Product>( 0 ) );
        GIDI::Suite &distribution = product->distribution( );
        
        std::cout << "    " << reaction->label( ) << std::endl;

        for( std::size_t i2 = 0; i2 < distribution.size( ); ++i2 ) {
            GIDI::Distributions::Distribution *form = distribution.get<GIDI::Distributions::Distribution>( i2 );

            std::cout << "        distribution form moniker = " << form->moniker( ) << std::endl;

            if( form->moniker( ) == GIDI_incoherentBoundToFreePhotonScatteringChars ) {
                if( form->label( ) == MonteCarlo_cdf ) {
                    GIDI::Distributions::IncoherentBoundToFreePhotoAtomicScattering *incoherentBoundToFreePhotonScattering 
                            = static_cast<GIDI::Distributions::IncoherentBoundToFreePhotoAtomicScattering *>( form );
                    std::string Compton_href = incoherentBoundToFreePhotonScattering->href( );

                    std::cout << "          href = " << Compton_href << std::endl;
                    GUPI::Ancestry *link = incoherentBoundToFreePhotonScattering->findInAncestry( Compton_href );
                    GIDI::DoubleDifferentialCrossSection::IncoherentBoundToFreePhotoAtomicScattering const &dd 
                            = *static_cast<GIDI::DoubleDifferentialCrossSection::IncoherentBoundToFreePhotoAtomicScattering const *>( link );
                    GIDI::Functions::Function1dForm  const *ComptonProfile = dd.ComptonProfile( );
                    GIDI::Functions::Xs_pdf_cdf1d const *xpcCompton = static_cast<GIDI::Functions::Xs_pdf_cdf1d const *>( ComptonProfile );

                    std::vector<double> occupationNumbers = xpcCompton->cdf( );
                    std::vector<double> pz_grid = xpcCompton->Xs( );
                    if( verbosity > 1 ) {
                        for( std::size_t i3 = 0; i3 < occupationNumbers.size( ); ++i3 ) {
                            std::cout << LUPI::Misc::argumentsToString( " %14.6g", pz_grid[i3] ) << " " 
                                    << LUPI::Misc::argumentsToString( " %20.14g", occupationNumbers[i3] ) << std::endl;
                        }
                    }
                }
            }
        }
    }    

    std::cout << std::endl << "MCGIDI:" << std::endl;

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::set<size_t> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, photonId, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::off, 20.0 );
    MC.setDecayPositronium( false );
    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );

    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );
    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::none );
    int numberOfReactions = static_cast<int>( MCProtare->numberOfReactions( ) );


    /*
     * Basic consistency check for thick target Bremsstrahlung data.
     * This verifies that, when present, the grids and differential cross section sizes are self-consistent.
     */
    {
        const MCGIDI::ThickTargetBremsstrahlung &ttb_data = MCProtare->thickTargetBremsstrahlung( );

        if( ttb_data.m_hasData ) {
            std::cout << "Checking thick target Bremsstrahlung data:" << std::endl;
            std::cout << "  meanExcitationEnergy = " << ttb_data.meanExcitationEnergy( ) << std::endl;
            std::cout << "  electronGrid size    = " << ttb_data.electronGrid( ).size( ) << std::endl;
            std::cout << "  photonGrid size      = " << ttb_data.photonGrid( ).size( ) << std::endl;
            std::cout << "  dcs size             = " << ttb_data.differentialCrossSection( ).size( ) << std::endl;
        }
    }

    int electronIndex = pops[electronId];
    int photonIndex = pops[photonId];
    int positronIndex = pops[positronId];

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    MCGIDI::URR_protareInfos URR_protare_infos( protares );

    std::cout << "MCGIDI hasIncoherentDoppler: " << MCProtare->hasIncoherentDoppler( ) << std::endl;

    std::cout <<" Number of MCGIDI reactions: " << numberOfReactions << std::endl;

    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
        int MT = reaction->ENDF_MT( );

        std::cout << "    " << LUPI::Misc::argumentsToString( " %4d: %-60s", reactionIndex, reaction->label( ).c_str( ) )
                << ":  final Q = " << reaction->finalQ( 0 )
                << LUPI::Misc::doubleToString3( " threshold = %.6g", reaction->crossSectionThreshold( ), true )
                << "  c_num: " << reaction->ENDL_C( ) << " MT: " << MT <<  std::endl;

        // Incoherent scattering
        if( MT == 504 || ( ( MT >= 1534 ) && ( MT <= 1572 ) ) ) {
            if( verbosity > 1 ) {
                double temperature = 0;
                for( double energy2 = 1e-8; energy2 < 100; energy2 *= 2.0 ) {
                    int hashIndex = domainHash.index( energy2 );

                    double mscop_xs = MCProtare->reactionCrossSection( reactionIndex, URR_protare_infos, hashIndex, temperature, energy2 );
                    std::cout << "  energy = " << energy2 << " crossSection = " << mscop_xs << std::endl;
                }
            }

            if( sampleProducts) {
                int numberOfBins = 101;
                unsigned long long rngState = 1;

                MCGIDI::Sampling::StdVectorProductHandler products;
                double threshold = MCProtare->threshold( reactionIndex );

                std::cout << std::endl << "Sampling outgoing mu / E distributions" << std::endl;
                std::cout << "reaction (" << std::setw( 3 ) << reactionIndex << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
                if( threshold < 1e-13 ) threshold = 1e-13;

                std::string energyFilename = "energy" + std::to_string( reactionIndex ) + "_electron" + "_incoherent_" + std::to_string( MT ) + ".out";
                std::string muFilename = "mu" + std::to_string( reactionIndex ) + "_electron" + "_incoherent_" + std::to_string( MT ) + ".out";
                const char* energyFileCStr = energyFilename.c_str( );
                const char* muFileCStr = muFilename.c_str( );

                FILE *fOutEnergy = fopen( energyFileCStr, "w" );
                FILE *fOutMu = fopen( muFileCStr, "w" );

                Bins energyBins( numberOfBins, 0.0, 1.0 );
                Bins muBins( numberOfBins, -1.0, 1.0 );

                int energyIndex = 0;
                for( double energy2 = 1e-4; energy2 <= 1e3; energy2 *= 10, ++energyIndex ) {
                    std::cout << "  Incident energy: " << energy2 << std::endl;
                    energyBins.setDomain( energy2 * 1e-3, energy2 * 1.1 );
                    energyBins.clear( );
                    muBins.clear( );
                    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
                        products.clear( );
                        input.setTemperatureAndEnergy( 0.0, energy2 );
                        reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                                [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                        //std::cout << "products size " << products.size( ) << std::endl;
                        for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                            MCGIDI::Sampling::Product const &product = products[i2];
                            if( std::isnan( product.m_kineticEnergy ) ) {
                                std::cout << "nan at " << product.m_pz_vz / sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz ) << std::endl;
                            }

                            if( product.m_productIndex == photonIndex && false ) {
                                energyBins.accrue( product.m_kineticEnergy );
                                //std::cout << product.m_kineticEnergy << std::endl;
                                muBins.accrue( product.m_pz_vz / sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz ) );
                            }
                            if( product.m_productIndex == electronIndex ) {
                                //std::cout << "here " << product.m_kineticEnergy << std::endl;
                                double mass = PoPI_electronMass_MeV_c2 * MCGIDI_speedOfLight_cm_sec * MCGIDI_speedOfLight_cm_sec;
                                double pv = sqrt((mass * 2 + product.m_kineticEnergy) * product.m_kineticEnergy);
                                energyBins.accrue( product.m_kineticEnergy );
                                muBins.accrue( product.m_pz_vz / pv );
                            }
                            
                        }
                    }

                    std::string message = "# Product energy for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    energyBins.print( fOutEnergy, message.c_str( ) );

                    message = "# Product mu for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    muBins.print( fOutMu, message.c_str( ) );
                }
            }
        }
        // pair production
        if(MT == 515 || MT == 517){
            if( sampleProducts) {
                int numberOfBins = 101;
                unsigned long long rngState;
                if( MT == 517){
                    rngState = 1;
                } else {
                    rngState = 2;
                }

                MCGIDI::Sampling::StdVectorProductHandler products;
                double threshold = MCProtare->threshold( reactionIndex );

                std::cout << std::endl << "Sampling outgoing mu / E distributions" << std::endl;
                std::cout << "reaction (" << std::setw( 3 ) << reactionIndex << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
                if( threshold < 1e-13 ) threshold = 1e-13;

                std::string energyFilename = "";
                std::string energyFilename2 = "";
                std::string muFilename = "";
                std::string muFilename2 = "";

                if( MT == 517 ) { 
                    energyFilename = "energy" + std::to_string( reactionIndex ) + "_electron" + "_pair_production_" + std::to_string( MT ) + ".out";
                    muFilename = "mu" + std::to_string( reactionIndex ) + "_electron" + "_pair_production_" + std::to_string( MT ) + ".out";
                } else {
                    energyFilename = "energy" + std::to_string( reactionIndex ) + "_electron" + "_triplet_production_" + std::to_string( MT ) + ".out";
                    muFilename = "mu" + std::to_string( reactionIndex ) + "_electron" + "_triplet_production_" + std::to_string( MT ) + ".out";
                }
                const char* energyFileCStr = energyFilename.c_str( );
                const char* muFileCStr = muFilename.c_str( );

                FILE *fOutEnergy = fopen( energyFileCStr, "w" );
                FILE *fOutMu = fopen( muFileCStr, "w" );

                if( MT == 517 ) { 
                    energyFilename2 = "energy" + std::to_string( reactionIndex ) + "_positron" + "_pair_production_" + std::to_string( MT ) + ".out";
                    muFilename2 = "mu" + std::to_string( reactionIndex ) + "_positron" + "_pair_production_" + std::to_string( MT ) + ".out";
                } else {
                    energyFilename2 = "energy" + std::to_string( reactionIndex ) + "_positron" + "_triplet_production_" + std::to_string( MT ) + ".out";
                    muFilename2 = "mu" + std::to_string( reactionIndex ) + "_positron" + "_triplet_production_" + std::to_string( MT ) + ".out";
                }
                
                const char* energyFileCStr2 = energyFilename2.c_str( );
                const char* muFileCStr2 = muFilename2.c_str( );
                FILE *fOutEnergy2 = fopen( energyFileCStr2, "w" );
                FILE *fOutMu2 = fopen( muFileCStr2, "w" );


                Bins energyBins( numberOfBins, 1.0, 1e3 );
                Bins muBins( numberOfBins, -1.0, 1.0 );

                Bins energyBins2( numberOfBins, 1.0, 1e3 );
                Bins muBins2( numberOfBins, -1.0, 1.0 );

                int energyIndex = 0;
                for( double energy2 = 1e-4; energy2 <= 1e3; energy2 *= 10, ++energyIndex ) {
                    std::cout << "  Incident energy: " << energy2 << std::endl;
                    energyBins.setDomain( energy2 * 1e-3, energy2 * 1.1 );
                    energyBins.clear( );
                    energyBins2.setDomain( energy2 * 1e-3, energy2 * 1.1 );
                    energyBins2.clear( );
                    muBins.clear( );
                    muBins2.clear( );
                    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
                        products.clear( );
                        input.setTemperatureAndEnergy( 0.0, energy2 );
                        reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                                [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                        //std::cout << "products size " << products.size( ) << std::endl;
                        if (energy2 < 1.02) continue; // Some weird things happen when this gets called below 1.022 MeV
                        for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                            MCGIDI::Sampling::Product const &product = products[i2];
                            if( std::isnan( product.m_kineticEnergy ) ) {
                                std::cout << "nan at " << product.m_pz_vz / sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz ) << std::endl;
                            }
                            //std::cout << "pair production index: " << i2 << " : " << product.m_productIndex << " label:" << product.m_productIntid << std::endl;
                            if( product.m_productIndex == electronIndex) {
                                double mass = PoPI_electronMass_MeV_c2 * MCGIDI_speedOfLight_cm_sec * MCGIDI_speedOfLight_cm_sec;
                                double pv = sqrt((mass * 2 + product.m_kineticEnergy) * product.m_kineticEnergy);
                                energyBins.accrue( product.m_kineticEnergy );
                                //std::cout << product.m_kineticEnergy << std::endl;
                                muBins.accrue( product.m_pz_vz / pv );
                            }
                            if( product.m_productIndex == positronIndex) {
                                double mass = PoPI_electronMass_MeV_c2 * MCGIDI_speedOfLight_cm_sec * MCGIDI_speedOfLight_cm_sec;
                                double pv = sqrt((mass * 2 + product.m_kineticEnergy) * product.m_kineticEnergy);
                                energyBins2.accrue( product.m_kineticEnergy );
                                //std::cout << product.m_kineticEnergy << std::endl;
                                muBins2.accrue( product.m_pz_vz / pv );
                            }
                        }
                    }

                    std::string message = "# Product energy for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    energyBins.print( fOutEnergy, message.c_str( ) );

                    message = "# Product mu for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    muBins.print( fOutMu, message.c_str( ) );

                    message = "# Product energy for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    energyBins2.print( fOutEnergy2, message.c_str( ) );

                    message = "# Product mu for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    muBins2.print( fOutMu2, message.c_str( ) );
                }
            }

        }
        // photoelectric effect
        if(MT >= 534 && MT <= 572){
            if( sampleProducts ) {
                int numberOfBins = 101;
                unsigned long long rngState = 1;

                MCGIDI::Sampling::StdVectorProductHandler products;
                double threshold = MCProtare->threshold( reactionIndex );

                std::cout << std::endl << "Sampling outgoing mu / E distributions" << std::endl;
                std::cout << "reaction (" << std::setw( 3 ) << reactionIndex << ") = " << reaction->label( ).c_str( ) << "  threshold = " << threshold << std::endl;
                if( threshold < 1e-13 ) threshold = 1e-13;

                std::string energyFilename = "energy" + std::to_string( reactionIndex ) + "_electron" + "_photoelectric_" + std::to_string(MT) + ".out";
                std::string muFilename = "mu" + std::to_string( reactionIndex ) + "_electron" + "_photoelectric_" + std::to_string(MT) + ".out";
                const char* energyFileCStr = energyFilename.c_str( );
                const char* muFileCStr = muFilename.c_str( );

                FILE *fOutEnergy = fopen( energyFileCStr, "w" );
                FILE *fOutMu = fopen( muFileCStr, "w" );

                Bins energyBins( numberOfBins, 0.0, 1.0 );
                Bins muBins( numberOfBins, -1.0, 1.0 );

                int energyIndex = 0;
                for( double energy2 = 1e-4; energy2 <= 1e3; energy2 *= 10, ++energyIndex ) {
                    std::cout << "  Incident energy: " << energy2 << std::endl;
                    energyBins.setDomain( energy2 * 1e-3, energy2 * 1.1 );
                    energyBins.clear( );
                    muBins.clear( );
                    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
                        products.clear( );
                        input.setTemperatureAndEnergy( 0.0, energy2 );
                        reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                                [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                        //std::cout << "products size " << products.size( ) << std::endl;
                        for( std::size_t i2 = 0; i2 < products.size( ); ++i2 ) {
                            MCGIDI::Sampling::Product const &product = products[i2];
                            if( std::isnan( product.m_kineticEnergy ) ) {
                                std::cout << "nan at " << product.m_pz_vz / sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz ) << std::endl;
                            }
                            //std::cout << "here again " << product.m_productIndex << std::endl;

                            if( product.m_productIndex == electronIndex ) {
                                energyBins.accrue( product.m_kineticEnergy );
                                //std::cout << product.m_productMass << std::endl;
                                double mass = PoPI_electronMass_MeV_c2 * MCGIDI_speedOfLight_cm_sec * MCGIDI_speedOfLight_cm_sec;
                                double pv = sqrt((mass * 2 + product.m_kineticEnergy) * product.m_kineticEnergy);
                                muBins.accrue( product.m_pz_vz / pv);
                                //std::cout << "hello: " << product.m_px_vx << product.m_py_vy << product.m_pz_vz << std::endl;
                            }
                        }
                    }

                    std::string message = "# Product energy for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    energyBins.print( fOutEnergy, message.c_str( ) );

                    message = "# Product mu for projectile energy = " + LUPI::Misc::argumentsToString( "%g", energy2 );
                    muBins.print( fOutMu, message.c_str( ) );
                }
            }

        }
    }

    int hashIndex = domainHash.index( energy );
    unsigned long long rngState = 1;

    double crossSection = MCProtare->crossSection( URR_protare_infos, hashIndex, 0, energy );
    std::cout << std::endl << "Sampling reactions with " << LUPI::Misc::argumentsToString( "%ld", numberOfSamples ) << " samples:" << std::endl;
    std::cout << "energy = " << energy << ", " << "cross section = " << LUPI::Misc::argumentsToString( "%.6g", crossSection ) << std::endl;
    std::cout << "  Reaction index ->                 : ";
    for( std::size_t reactionIndex = 0; reactionIndex < MCProtare->numberOfReactions( ); ++reactionIndex ) {
        std::cout << LUPI::Misc::argumentsToString( " %10d", reactionIndex );
    }
    std::cout << std::endl;

    std::vector<double> reactionCrossSections( numberOfReactions );
    std::cout << LUPI::Misc::argumentsToString( "  %-34s: ", "reactionCrossSection/crossSection" );
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        double reactionCrossSection = MCProtare->reactionCrossSection( i1, URR_protare_infos, hashIndex, 0, energy );
        std::cout << LUPI::Misc::argumentsToString( " %10.7f", reactionCrossSection / crossSection );
    }
    std::cout << std::endl;

    input.setTemperatureAndEnergy( 0.0, energy );
    std::vector<long> counts( numberOfReactions + 1, 0 );
    for( long i1 = 0; i1 < numberOfSamples; ++i1 ) {
        std::size_t reactionIndex = MCProtare->sampleReaction( input, URR_protare_infos, hashIndex, crossSection,
                [&]() -> double { return float64RNG64( &rngState ); } );
        if( reactionIndex > numberOfReactions ) reactionIndex = numberOfReactions;
        ++counts[reactionIndex];
    }

    std::cout << LUPI::Misc::argumentsToString( "  %-34s: ", "ratio / numberOfSamples" );
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        double ratio = counts[i1];
        std::cout << LUPI::Misc::argumentsToString( " %10.7f", ratio / numberOfSamples );
    }
    std::cout << std::endl;

    std::cout << LUPI::Misc::argumentsToString( "  %-34s: ", "counts" );
    for( int i1 = 0; i1 < numberOfReactions; ++i1 ) {
        std::cout << LUPI::Misc::argumentsToString( " %10ld", counts[i1] );
    }
    std::cout << std::endl;

    delete MCProtare;
    delete protare;
}
