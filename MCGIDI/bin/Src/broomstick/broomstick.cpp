/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>

#include <statusMessageReporting.h>

#include <MCGIDI.hpp>

#include <GIDI_testUtilities.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <bins.hpp>
#include <GIDIP_python.hpp>

static char const *description = "For a protare, samples specified reaction (or all if specified reaction index is negative "
    "via option '-r') many times (see option '-n') at the specified projectile energy or energies (see options '--energyMode'),"
    "and creates an energy and angular spectrum for the specified outgoing particle (see option '--oid'). To see a list of "
    "reactions and their indices, make the value for the '-r' option large (larger than the number of reaction for the protare).\n"
    "\n"
    "The next table shows that allowed values for the '--energyMode' option.\n"
    "\n"
    "   --energyMode    | value\n"
    "       value       |\n"
    "   ================+==========================================================\n"
    "    mono           | Mono-energetic at the specified energy (default)\n"
    "    range          | Uniformly between the two specified energies\n"
    "    crossSection   | Cross section sampled between the two specified energies\n"
    "    python         | Via a python module.\n"
    "\n"
    "For '--energyMode' option of 'python', the first argument names a python module to load and that module must have a\n"
    "function named 'sampleEnergy'. The function 'sampleEnergy' takes no arguments and must return a float. For each\n"
    "event, the sampleEnergy function is called to sample that event's projectile energy.\n"
    "For the first argument, do not include the extension of the module (e.g., for the module named 'test.py'\n"
    "enter just '--energyMode test'.\n"
    "\n"
    "Upscatter input and models:\n"
    "   --upscatter | model \n"
    "      value    |\n"
    "   ============+===================\n"
    "          0    | none (default)\n"
    "          1    | B\n"
    "          2    | BSnLimits\n"
    "          3    | DBRC\n"
    "Anything else causes a throw to be executed.\n"
    "\n"
    "Example for a projectile with energy 1.2 (in file units):\n"
    "    broomstick --map path/to/all.map --tid O16 -r 13 1.2\n"
    "\n"
    "Example for a projectile with energy in the range 1.2 to 3.2 (in file units):\n"
    "    broomstick --map path/to/all.map --tid O16 -r 13 --energyMode range 1.2 3.2";

enum class EnergyMode { mono, range, crossSection, python };

std::string double2String( double a_value );
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

    exit( EXIT_SUCCESS );
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    PoPI::Database pops;
    unsigned long long rngState = 1;
    std::set<int> reactionsToExclude;
    GIDI::Transporting::Particles particles, particlesEmpty;
    LUPI::StatusMessageReporting smr1;
    std::set<int> deactivateIndices;

    std::map<std::string, std::string> particlesAndGIDs;

    particlesAndGIDs[PoPI::IDs::neutron] = "LLNL_gid_4";
    particlesAndGIDs["H1"] = "LLNL_gid_71";
    particlesAndGIDs["H2"] = "LLNL_gid_71";
    particlesAndGIDs["H3"] = "LLNL_gid_71";
    particlesAndGIDs["He3"] = "LLNL_gid_71";
    particlesAndGIDs["He4"] = "LLNL_gid_71";
    particlesAndGIDs[PoPI::IDs::photon] = "LLNL_gid_70";

    argvOptions argv_options( __FILE__, description );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );
    parseTestOptions.m_askOid = true;

    parseTestOptions.m_askGNDS_File = true;

    argv_options.add( argvOption( "-n", true, "Number of samples. If value is negative, it is multiplied by -1000000." ) );
    argv_options.add( argvOption( "--energyMode", true, "Specifies how the projectile energies are sampled." ) );
    argv_options.add( argvOption( "--histograms", false, "If present, the energy and angular spectra bins are written as histograms; otherwise, the center of each bin is written." ) );
    argv_options.add( argvOption( "--logEnergySpacing", false, "If present, the energy spectrum bins are logarithmically spaced." ) );
    argv_options.add( argvOption( "--numberOfEBins", true, "Number of outgoing energy bins. Default is 1000." ) );
    argv_options.add( argvOption( "--numberOfMuBins", true, "Number of mu bins. Default is 1000." ) );
    argv_options.add( argvOption( "--energyMin", true, "The minimum energy for binning. Default is 1e-11." ) );
    argv_options.add( argvOption( "--energyMax", true, "The maximum energy for binning. Default is 20." ) );
    argv_options.add( argvOption( "--temperature", true, "The temperature of the material in MeV/k. Default is 2.53e-8 MeV/k (293.6 K)." ) );
    argv_options.add( argvOption( "--upscatter", true, "The upscatter model to use. Default is none." ) );
    argv_options.add( argvOption( "-r", true, 
            "Specifies a reaction index to sample. If negative, all reactions are sampled using the protare's sampleReaction method." ) );
    argv_options.add( argvOption( "--recordPath", true,
            "If present, each sampled event's outgoing particle data are written to the specified file." ) );
    argv_options.add( argvOption( "--recordVelocities", false, "If present, velocities are also written to the --recordPath file." ) );
    argv_options.add( argvOption( "--hist2dPath", true,
            "If present, the outgoing energy/angle 2-d histogram is written to the specified file in csv format." ) );
    argv_options.add( argvOption( "--GRIN", false, "If present, use GRIN continuum gamma data if present." ) );
    argv_options.add( argvOption( "--makePhotonEmissionProbabilitiesOne", false, "If present, all photon emission probabilities are set to 1.0." ) );
    argv_options.add( argvOption( "--zeroNuclearLevelEnergyWidth", false, "If present, the GRIN continuum nuclear level energy width are set to 0.0." ) );
    argv_options.add( argvOption( "-d", true, "Disable reaction at index. May be entered multiple times, one for each reaction to be disabled." ) );

    parseTestOptions.parse( );

    if( ( argv_options.m_arguments.size( ) < 1 ) || ( argv_options.m_arguments.size( ) > 2 ) )
        throw "Need projectile energy or energy range.";

    std::string recordPath = argv_options.find( "--recordPath" )->zeroOrOneOption( argv, "" );
    bool recordVelocities = argv_options.find( "--recordVelocities" )->present( );
    std::string hist2dPath = argv_options.find( "--hist2dPath" )->zeroOrOneOption( argv, "" ) ;
    std::ofstream recordStream;
    if( recordPath != "" ) {
        recordStream.open( recordPath, std::ios::out );
    }

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, parseTestOptions.photonMode( ) );
    construction.setGRIN_continuumGammas( argv_options.find( "--GRIN" )->present( ) );

    GIDI::Protare *protare = parseTestOptions.protare( pops, "/usr/gapps/data/nuclear/common/PoPs/pops.xml", "../../../../GIDI/Test/Data/MG_MC/all_maps.map", construction, PoPI::IDs::neutron, "O16" );

    GIDI::Transporting::Settings incompleteParticlesSetting( protare->projectile( ).ID( ), GIDI::Transporting::DelayedNeutrons::on );
    std::set<std::string> incompleteParticles;
    protare->incompleteParticles( incompleteParticlesSetting, incompleteParticles );
    std::cout << "# List of incomplete particles:";
    for( auto iter = incompleteParticles.begin( ); iter != incompleteParticles.end( ); ++iter ) {
        std::cout << " " << *iter;
    }
    std::cout << std::endl;

    std::string productID = argv_options.find( "--oid" )->zeroOrOneOption( argv, protare->projectile( ).ID( ) );
    long numberOfSamples = argv_options.find( "-n" )->asLong( argv, -1 );
    if( numberOfSamples < 0 ) numberOfSamples *= -1000000;
    long numberOfEBins = argv_options.find( "--numberOfEBins" )->asLong( argv, 1000 );
    long numberOfMuBins = argv_options.find( "--numberOfMuBins" )->asLong( argv, 1000 );

    int reactionIndex = static_cast<int>( argv_options.find( "-r" )->asLong( argv, 999999 ) );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );
    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, protare->projectile( ).ID( ), &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setThrowOnError( false );
    MC.setSampleNonTransportingParticles( true );
    MC.setMakePhotonEmissionProbabilitiesOne( argv_options.find( "--makePhotonEmissionProbabilitiesOne" )->present( ) );
    MC.setZeroNuclearLevelEnergyWidth( argv_options.find( "--zeroNuclearLevelEnergyWidth" )->present( ) );

    double temperature_MeV_k = argv_options.find( "--temperature" )->asDouble( argv, 2.53e-8 );

    MCGIDI::Sampling::Upscatter::Model upscatterModel = MCGIDI::Sampling::Upscatter::Model::none;
    int upscatter = argv_options.find( "--upscatter" )->asInt( argv, 0 );
    switch( upscatter ) {
    case 0:
        break;
    case 1:
        upscatterModel = MCGIDI::Sampling::Upscatter::Model::B;
        MC.setUpscatterModelB( );
        break;
    case 2:
        upscatterModel = MCGIDI::Sampling::Upscatter::Model::BSnLimits;
        MC.setUpscatterModelBSnLimits( );
        break;
    case 3:
        upscatterModel = MCGIDI::Sampling::Upscatter::Model::DBRC;
        MC.setUpscatterModelDBRC( );
        break;
    default:
        throw "Invalid upscatter option.";
    }

    for( std::map<std::string, std::string>::iterator iter = particlesAndGIDs.begin( ); iter != particlesAndGIDs.end( ); ++iter ) {
        GIDI::Transporting::Particle particle( iter->first );
        particles.add( particle );
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particlesEmpty, domainHash, temperatures, reactionsToExclude );

    EnergyMode energyMode = EnergyMode::mono;
    std::string energyModeStr = argv_options.find( "--energyMode" )->zeroOrOneOption( argv, "mono" );
    if( energyModeStr == "mono" ) {
        energyMode = EnergyMode::mono; }
    else if( energyModeStr == "range" ) {
        energyMode = EnergyMode::range; }
    else if( energyModeStr == "crossSection" ) {
        energyMode = EnergyMode::crossSection; }
    else if( energyModeStr == "python" ) {
        energyMode = EnergyMode::python; }
    else {
        throw LUPI::Exception( "Unsupported energyMode." );
    }

    double energyInMin = 0, energyInMax = 0;
    void *py_function = nullptr;
    if( energyMode != EnergyMode::python ) {
        energyInMin = argv_options.asDouble( argv, 0 );
        energyInMax = energyInMin;
        if( energyMode != EnergyMode::mono ) {
            energyInMax = argv_options.asDouble( argv, 1 );
        } }
    else {
        std::string pythonModuleName = argv[argv_options.m_arguments[0]];
        py_function = GIDIP::Python::loadFunctionInModule( pythonModuleName, "sampleEnergy" );
        if( py_function == nullptr ) throw LUPI::Exception( "Function 'sampleEnergy' not found in python module." );
    }
    MCGIDI::URR_protareInfos URR_protareInfos;

    argvOption *deactivateIndicesOption = argv_options.find( "-d" );
    for( int optionCounter = 0; optionCounter < deactivateIndicesOption->m_counter; ++optionCounter ) {
        deactivateIndices.insert( asInt( argv[deactivateIndicesOption->m_indices[optionCounter]] ) );
    }

    std::cout << std::endl;
    std::cout << "# List of reaction:" << std::endl;
    std::cout << "# index       threshold     label" << std::endl;
    std::cout << "# -----------------------------------------------------------" << std::endl;

    bool reactionsDeativated = false;
    for( int reactionIndex2 = 0; reactionIndex2 < static_cast<int>( MCProtare->numberOfReactions( ) ); ++reactionIndex2 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex2 );
        std::string deactiveFlag = "   ";

        if( deactivateIndices.count( reactionIndex2 ) ) {
            reactionsDeativated = true;
            deactiveFlag = " * ";
            GIDI::Reaction *reactionGIDI = protare->reaction( reaction->GIDI_reactionIndex( ) );
            reactionGIDI->setActive( false );
        }

        std::cout << "# " << deactiveFlag << std::setw( 5 ) << reactionIndex2 << "  " << doubleToString( "%14.6e", reaction->crossSectionThreshold( ) ) 
                << "  " << reaction->label( ).c_str( ) << std::endl;
    }

    if( reactionIndex > static_cast<int>( MCProtare->numberOfReactions( ) ) ) {
        delete protare;
        delete MCProtare;
        exit( EXIT_SUCCESS );
    }

    if( reactionsDeativated ) {
        delete MCProtare;
        MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particlesEmpty, domainHash, temperatures, reactionsToExclude );
    }

    int oidIndex = -1;
    int maxProductIndex = 0;
    std::cout << std::endl;
    for( auto particleIter = particles.particles( ).begin( ); particleIter != particles.particles( ).end( );  ++particleIter, ++maxProductIndex ) {
        MCProtare->setUserParticleIndex( pops[(*particleIter).first], maxProductIndex );
        if( (*particleIter).first == productID ) oidIndex = maxProductIndex;
        std::cout << "# particle ID/user defined index " << (*particleIter).first << " " << maxProductIndex << std::endl;
    }
    if( oidIndex == -1 ) {
        std::cout << "\nError: could not find outgoing particle with ID '" << productID << "'!" << std::endl;
        exit( EXIT_FAILURE );
    }

    std::cout << std::endl;
    std::cout << "# path is " << protare->realFileName( ) << std::endl;
    std::cout << "# projectile is " << MCProtare->projectileID( ).c_str( ) << std::endl;
    std::cout << "# target is " << MCProtare->targetID( ).c_str( ) << std::endl;
    std::cout << "# product is " << productID << std::endl;
    std::cout << "# projectile energy minimum " << energyInMin << " MeV" << std::endl;
    std::cout << "# projectile energy maximum " << energyInMax << " MeV" << std::endl;
    if( reactionIndex >= 0 ) {
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
        std::cout << "# Reaction Info:" << std::endl;
        std::cout << "#     index: " << reactionIndex << std::endl;
        std::cout << "#     label: " << reaction->label( ).c_str( ) << std::endl;
        std::cout << "#     threshold: " << doubleToString( "%14.6e MeV", reaction->crossSectionThreshold( ) ) << std::endl;
    }

    bool histograms = argv_options.find( "--histograms" )->present( );
    bool logEnergySpacing = argv_options.find( "--logEnergySpacing" )->present( );
    double energyMin = argv_options.find( "--energyMin" )->asDouble( argv, 1e-11 );
    double energyMax = argv_options.find( "--energyMax" )->asDouble( argv, 20.0 );

    long numberOfEBins2 = numberOfEBins;
    if( numberOfEBins2 < 1 ) numberOfEBins2 = 1;
    Bins energyBins( numberOfEBins2, energyMin, energyMax, logEnergySpacing );

    long numberOfMuBins2 = numberOfMuBins;
    if( numberOfMuBins2 < 1 ) numberOfMuBins2 = 1;
    Bins muBins( numberOfMuBins2, -1.0, 1.0 );

    std::vector<std::vector<long>> *hist2d = nullptr;
    if( ( numberOfEBins > 0 ) && ( numberOfMuBins > 0 ) ) {
        if( argv_options.find( "--hist2dPath" )->present( ) ) {
            hist2d = new std::vector<std::vector<long>>( );
            hist2d->resize( numberOfEBins );
            for( long i1 = 0; i1 < numberOfEBins; ++i1 ) (*hist2d)[i1].resize( numberOfMuBins );
        }
    }

    MCGIDI::Sampling::Input input( true, upscatterModel );
    MCGIDI::Sampling::StdVectorProductHandler products;
    if( ( reactionIndex >= 0 ) && ( energyMode != EnergyMode::python ) ){
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
        if( reaction->crossSectionThreshold( ) > energyInMin ) energyInMin = reaction->crossSectionThreshold( );
        if( reaction->crossSectionThreshold( ) > energyInMax ) {
            delete protare;
            delete MCProtare;

            std::cout << std::endl << "    **** Reaction threshold is above maximum projectile energy. ****" << std::endl << std::endl;
            exit( EXIT_SUCCESS );
        }
    }

    long eBinIndex, muBinIndex;
    long numberOfDBRC_rejections = 0;
    long numberOfDBRC_samples = 0;
    if( recordPath != "" ) recordStream << "#                user id       intid      energy           v_x             v_y             v_z" << std::endl;

    MCGIDI::Probabilities::Xs_pdf_cdf1d *crossSectionXs_pdf_cdf1d = nullptr;
    if( energyMode == EnergyMode::crossSection ) {
        GIDI::Functions::XYs1d modeCrossSection;
        if( reactionIndex < 0 ) {
            for( std::size_t protareIndex = 0; protareIndex < MCProtare->numberOfProtares( ); ++protareIndex ) {
                MCGIDI::ProtareSingle *protareSingle = MCProtare->protare( protareIndex );
                modeCrossSection += protareSingle->heatedCrossSections( ).crossSectionAsGIDI_XYs1d( temperature_MeV_k );
            } }
        else {
            MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex );
            modeCrossSection = reaction->crossSectionAsGIDI_XYs1d( temperature_MeV_k );
        }
        modeCrossSection = modeCrossSection.domainSlice( energyInMin, energyInMax, true );
        double integral = modeCrossSection.normalize( );
        if( integral == 0 ) {
            std::cout << "Cross section integral is 0." << std::endl;
            exit( EXIT_SUCCESS );
        }
        crossSectionXs_pdf_cdf1d = new MCGIDI::Probabilities::Xs_pdf_cdf1d( modeCrossSection.toXs_pdf_cdf1d( ) );
    }

    for( long sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex ) {
        double energy_in = energyInMin;
        if( energyMode == EnergyMode::range ) {
            energy_in = energyInMin + float64RNG64( &rngState ) * ( energyInMax - energyInMin ); }
        else if( energyMode == EnergyMode::crossSection ) {
            energy_in = crossSectionXs_pdf_cdf1d->sample( float64RNG64( &rngState ), [&]( ) -> double { return float64RNG64( &rngState ); } ); }
        else if( energyMode == EnergyMode::python ) {
            energy_in = GIDIP::Python::callFunctionReturnDouble( py_function, nullptr );
        }

        int hashIndex = domainHash.index( energy_in );
        input.setTemperatureAndEnergy( temperature_MeV_k, energy_in );

        int reactionIndex2 = reactionIndex;
        if( reactionIndex2 < 0 ) {
            double totalCrossSection = MCProtare->crossSection( URR_protareInfos, hashIndex, temperature_MeV_k, energy_in );
            reactionIndex2 = MCProtare->sampleReaction( input, URR_protareInfos, hashIndex, totalCrossSection, 
                    [&]( ) -> double { return float64RNG64( &rngState ); } );
        }
        MCGIDI::Reaction const *reaction = MCProtare->reaction( reactionIndex2 );

        double crossSection = MCProtare->reactionCrossSection( reactionIndex2, URR_protareInfos, hashIndex, temperature_MeV_k, energy_in, false );

        if( recordPath != "" ) recordStream << "Event: " << LUPI::Misc::argumentsToString( "%12d:", sampleIndex ) 
                << " energy " << LUPI::Misc::argumentsToString( "%15.8e:", energy_in )
                << " cross section " << LUPI::Misc::argumentsToString( "%15.8e:", crossSection )
                << " reaction " << reactionIndex2 << ": " << reaction->label( ).c_str( ) << std::endl;

        products.clear( );
        reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); }, 
                [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );

        for( std::size_t productIndex = 0; productIndex < products.size( ); ++productIndex ) {
            MCGIDI::Sampling::Product const &product = products[productIndex];
            int userProductIndex = product.m_userProductIndex;
            if( recordPath != "" ) {
                recordStream << "    product: " << std::setw( 11 ) << userProductIndex << std::setw( 12 ) << product.m_productIntid << " " 
                        << double2String( product.m_kineticEnergy );
                if( recordVelocities ) recordStream << " " << double2String( product.m_px_vx ) << " " << double2String( product.m_py_vy ) 
                        << " " << double2String( product.m_pz_vz );
                recordStream << std::endl;
            }
            if( userProductIndex != oidIndex ) continue;

            if( product.m_numberOfDBRC_rejections >= 0 ) {
                numberOfDBRC_rejections += product.m_numberOfDBRC_rejections;
                ++numberOfDBRC_samples;
            }

            eBinIndex = energyBins.accrue( product.m_kineticEnergy, 1.0 );
            double speed = sqrt( product.m_px_vx * product.m_px_vx + product.m_py_vy * product.m_py_vy + product.m_pz_vz * product.m_pz_vz );
            double mu = 0.0;
            if( speed != 0.0 ) mu = product.m_pz_vz / speed;
            muBinIndex = muBins.accrue( mu, 1.0 );

            if( hist2d != nullptr ) {
                if( ( 0 <= eBinIndex )  && ( eBinIndex <= numberOfEBins ) &&
                    ( 0 <= muBinIndex ) && ( muBinIndex <= numberOfMuBins ) ) {
                    (*hist2d)[eBinIndex][muBinIndex] += 1;
                }
            }
        }

        if( input.m_GRIN_intermediateResidual >= 0 ) {
            PoPI::ParseIntidInfo parseIntidInfo( input.m_GRIN_intermediateResidual, true );
            recordStream << "    GRIN intermediate residual: " << parseIntidInfo.id( ) << std::endl;
        }
    }

    if( numberOfDBRC_samples > 0 ) {
        double averageDBRC = numberOfDBRC_rejections / (double) numberOfDBRC_samples;
        std::cout << "# Number of DBRC samples = " << numberOfDBRC_samples << std::endl;
        std::cout << "# Total number of DBRC rejections = " << numberOfDBRC_rejections << std::endl;
        std::cout << "# Average number of DBRC rejections per sample = " << LUPI::Misc::doubleToString3( "%.3e", averageDBRC ) << std::endl;
    }

    std::string header = "# energy spectrum P(E') for " + productID + ":";
    if( numberOfEBins > 0 ) {
        energyBins.print( stdout, header.c_str( ), true, histograms );
    }

    if( numberOfMuBins > 0 ) {
        header = "\n# angular spectrum P(mu) for " + productID + ":";
        muBins.print( stdout, header.c_str( ), true, histograms );
    }

    if( hist2d != nullptr ) {
        std::ofstream fout;
        fout.open( hist2dPath );
        fout << "# Projectile: " << protare->projectile( ).ID( );
        fout << ", target: " << protare->target( ).ID( );
        fout << ", projectile energy minimum : " << energyInMin << " MeV, product: " << productID;
        fout << ", projectile energy maximum : " << energyInMax << " MeV, product: " << productID;
        std::vector<double> edges = energyBins.edges( );
        fout << "\n# energy bin boundaries (MeV): " << edges[0];
        for( size_t i1 = 1; i1 < edges.size( ); ++i1 )
            fout << "," << edges[i1];
        edges = muBins.edges( );
        fout << "\n# angle bin boundaries (mu): " << edges[0];
        for( size_t i1 = 1; i1 < edges.size( ); ++i1 )
            fout << "," << edges[i1];
        fout << "\n";

        for( long i1 = 0; i1 < numberOfEBins; ++i1 ) {
            fout << (*hist2d)[i1][0];
            for( long i2 = 1; i2 < numberOfMuBins; ++i2 ) {
                fout << "," << (*hist2d)[i1][i2];
            }
            fout << "\n";
        }
        fout.close( );
        delete hist2d;
    }

    delete protare;
    delete MCProtare;
    if( recordPath != "" ) {
        recordStream.close( );
    }
    delete crossSectionXs_pdf_cdf1d;
    if( py_function != nullptr ) GIDIP::Python::decrementRef( py_function );

    exit( EXIT_SUCCESS );
}

/*
=========================================================
*/
std::string double2String( double a_value ) {

    return( LUPI::Misc::doubleToString3( "%15.8e", a_value ) );
}
