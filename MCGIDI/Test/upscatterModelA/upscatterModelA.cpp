/*
# <<BEGIN-copyright>>
# Copyright 2019, Lawrence Livermore National Security, LLC.
# See the top-level COPYRIGHT file for details.
# 
# SPDX-License-Identifier: MIT
# <<END-copyright>>
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <MCGIDI.hpp>
#include <bins.hpp>
#include <MCGIDI_testUtilities.hpp>
#include <GIDI_testUtilities.hpp>

class ParticleInfo {

    public:
        int numberOfCollisions;
        int numberOfCollisionsInTimeStep;
        double energy;
        double initialEnergy;
        double velocity;
        double vx, vy, vz;
        double averageCollisionTime;
        double timeToCollision;
};

class FileInfo {

    public:
        std::string m_fileName;
        FILE *m_fOut;
        std::string m_suffix;

        FileInfo( std::string const &a_fileNamePrefix, char const *a_suffix );
        ~FileInfo( );
};

FileInfo::FileInfo( std::string const &a_fileNamePrefix, char const *a_suffix ) {

    m_suffix = a_suffix;
    m_fileName = a_fileNamePrefix + "." + a_suffix + ".dat";
    if( ( m_fOut = fopen( m_fileName.c_str( ), "w" ) ) == nullptr ) throw "error opening output file";
}

FileInfo::~FileInfo( ) {

    fclose( m_fOut );
}

static FileInfo *fileInfo[5];


#define nBins 501
static double neutronMass;
static double temperature_MeV = 1.0e-3;
static const int nParticles = 100000;
static double temp[nParticles];

static MCGIDI::URR_protareInfos URR_protare_infos;

void main2( int argc, char **argv );
void updateParticle( MCGIDI::Protare *protare, double energy, MCGIDI::DomainHash &domainHash, ParticleInfo *particle, 
                MCGIDI::Sampling::Product const *product );
void printBins( FILE *fOut, double time, ParticleInfo *particleInfos, long *bins, double velocityMax );
void printData( int index, std::string const &timeLabel );
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
}
/*
=========================================================
*/
void main2( int argc, char **argv ) {

    // Default values
    std::string mapFilename( "../upscatterModelB/Data/upscatterModelB.map" );
    mapFilename = "/usr/gapps/data/nuclear/development/GNDS_2.0/ENDL2009/ENDL2009.5-direct-rc5.1/all.map";
    std::string neutronID( PoPI::IDs::neutron );
    std::string targetID = "Li6";
    int nTimeSteps = 5001;
    double initialEnergy = 1e-1;
    long bins[nBins+1];
    double time = 0.0;
    std::set<int> reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    unsigned long long rngState = 1;

    // Command line argument parsing
    argvOptions argv_options( __FILE__, "Upscatter model A test program" );
    ParseTestOptions parseTestOptions( argv_options, argc, argv );
    parseTestOptions.m_askOid = true;

    parseTestOptions.m_askGNDS_File = true;

    argv_options.add( argvOption( "--map", true, "Map file path" ) );
    argv_options.add( argvOption( "--target", true, "Target ID (default: Li6)" ) );
    argv_options.add( argvOption( "--pops", true, "PoPs XML file path" ) );
    argv_options.add( argvOption( "--timeSteps", true, "Number of time steps (default: 5001)" ) );
    argv_options.add( argvOption( "--initialEnergy", true, "Initial energy in MeV (default: 0.1)" ) );
    argv_options.add( argvOption( "--multiGroup", false, "If present, use multi-group cross section lookup mode instead of continuous energy." ) );
    argv_options.add( argvOption( "--userGrid", false, "If present, use log-spaced group boundaries for model A upscatter (616 groups)." ) );

    parseTestOptions.parse( );

    // Apply parsed options
    targetID = argv_options.find( "--target" )->zeroOrOneOption( argv, "Li6" );
    std::string mapFile = argv_options.find( "--map" )->zeroOrOneOption( argv, mapFilename );
    std::string popsFile = argv_options.find( "--pops" )->zeroOrOneOption( argv, "../../../TestData/PoPs/pops.xml" );
    nTimeSteps = argv_options.find( "--timeSteps" )->asInt( argv, 5001 );
    initialEnergy = argv_options.find( "--initialEnergy" )->asDouble( argv, 1e-1 );
    bool multiGroup = argv_options.find( "--multiGroup" )->present();

    PoPI::Database pops( popsFile );
    GIDI::Map::Map map( mapFile, pops );
    int neutronIndex( pops[neutronID] );
    GIDI::Transporting::Particles particles;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Protare *protare = map.protare( construction, pops, neutronID, targetID );
    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    std::string label( temperatures[0].griddedCrossSection( ) );
    if( multiGroup ) {
        label = temperatures[0].heatedMultiGroup( );
    }
    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );
    MC.setUpscatterModelA( );
    if( argv_options.find( "--userGrid" )->present() ) {
        std::vector<double> energy_grid;
        const int num_energies = 616;
        const double start_energy = 1e-11;
        const double end_energy = 20.0;
        energy_grid.reserve(num_energies);
        double log_energy = log10(start_energy);
        double log_energy_step = (log10(end_energy) - log_energy) / (num_energies - 1);
        for (int i = 0; i < num_energies; ++i) {
            energy_grid.push_back(std::pow(10,log_energy + i * log_energy_step));
        }
        MC.setUpscatterModelAGroupBoundaries( energy_grid );
    }

    if( multiGroup ) {
        MC.crossSectionLookupMode( MCGIDI::Transporting::LookupMode::Data1d::multiGroup );
    }

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );
    if( multiGroup ) {
        particles.process( *protare, label );
    }

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );
    MCGIDI::ProtareSingle *MCProtareSingle = MCProtare->protare( 0 );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtareSingle;
    URR_protare_infos.setup( protares );

    PoPI::Base const &target = pops.get<PoPI::Base>( targetID );
    std::string fileNamePrefix = "relax." + target.ID( );
    std::string Str = LUPI::Misc::argumentsToString( "relax.%s.dat", target.ID( ).c_str( ) );
    FILE *fOut;
    if( ( fOut = fopen( Str.c_str( ), "w" ) ) == nullptr ) throw "error opening output file";

    fileInfo[0] = new FileInfo( fileNamePrefix, "E" );
    fileInfo[1] = new FileInfo( fileNamePrefix, "v" );
    fileInfo[2] = new FileInfo( fileNamePrefix, "vx" );
    fileInfo[3] = new FileInfo( fileNamePrefix, "vy" );
    fileInfo[4] = new FileInfo( fileNamePrefix, "vz" );

    neutronMass = MCProtareSingle->projectileMass( );
    double targetMass = MCProtareSingle->targetMass( );
    double velocityMax = MCGIDI_speedOfLight_cm_sec * MCGIDI_particleBeta( neutronMass, 100 * temperature_MeV );

    ParticleInfo *particleInfos = new ParticleInfo[nParticles];
    for( long i1 = 0; i1 < nParticles; ++i1 ) {
        particleInfos[i1].timeToCollision = 0;
        updateParticle( MCProtareSingle, initialEnergy, domainHash, &(particleInfos[i1]), nullptr );
    }
    printBins( fOut, time, particleInfos, bins, velocityMax );

    MCGIDI::Sampling::StdVectorProductHandler products;

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::A );

    MCGIDI::Reaction const *reaction = MCProtareSingle->reaction( 0 );

    long badNeutronIndex = 0;
    long badProductNumber = 0;
    for( long i1 = 0; i1 < nTimeSteps; ++i1 ) {
        double dTime = 0.0;

        for( long i2 = 0; i2 < nParticles; ++i2 ) dTime += particleInfos[i2].averageCollisionTime;
        dTime *= targetMass / neutronMass * 0.002 / (double) nParticles;
        time += dTime;

        for( long i2 = 0; i2 < nParticles; i2++ ) {
            ParticleInfo *particle = &(particleInfos[i2]);

            particle->timeToCollision -= dTime;
            while( particle->timeToCollision <= 0. ) {
                input.setTemperatureAndEnergy( temperature_MeV, particle->energy );
                MCProtareSingle->sampleTargetBetaForUpscatterModelA( input, [&]( ) -> double { return float64RNG64( &rngState ); } );
                reaction->sampleProducts( MCProtareSingle, input, [&]( ) -> double { return float64RNG64( &rngState ); }, 
                        [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                if( products.size( ) < 1 ) {
                    ++badProductNumber; }
                else {
                    MCGIDI::Sampling::Product const &product = products[0];
                    if( product.m_productIndex != neutronIndex ) {
                        ++badNeutronIndex; }
                    else {
                        updateParticle( MCProtareSingle, product.m_kineticEnergy, domainHash, particle, &product );
                    }
                }
                products.clear( );
            }
        }
        if( ( i1 % 100 ) == 0 ) printBins( fOut, time, particleInfos, bins, velocityMax );
    }

    fprintf( fOut, "# badNeutronIndex = %ld\n", badNeutronIndex );
    fprintf( fOut, "# badProductNumber = %ld\n", badProductNumber );
    fclose( fOut );

    for( std::size_t i1 = 0; i1 < sizeof( fileInfo ) / sizeof( fileInfo[0] ); ++i1 ) delete fileInfo[i1];

    delete[] particleInfos;
    delete protare;
    delete MCProtare;

    exit( EXIT_SUCCESS );
}
/*
==============================================================================
*/
void updateParticle( MCGIDI::Protare *protare, double energy, MCGIDI::DomainHash &domainHash, ParticleInfo *particle, 
                MCGIDI::Sampling::Product const *product ) {

    particle->energy = energy;
    if( product == nullptr ) {
        particle->initialEnergy = energy;
        particle->numberOfCollisions = -1;
        particle->numberOfCollisionsInTimeStep = -1;
    }
    particle->numberOfCollisions++;
    particle->numberOfCollisionsInTimeStep++;
    particle->velocity = MCGIDI_speedOfLight_cm_sec * MCGIDI_particleBeta( neutronMass, energy );

    if( product != nullptr ) {
        particle->vx = product->m_px_vx;
        particle->vy = product->m_py_vy;
        particle->vz = product->m_pz_vz;
    }

    double crossSection = protare->crossSection( URR_protare_infos, domainHash.index( energy ), temperature_MeV, energy );
    particle->averageCollisionTime = 1. / ( crossSection * particle->velocity );
    particle->timeToCollision += -log( drand48( ) ) * particle->averageCollisionTime;
}
/*
==============================================================================
*/
void printBins( FILE *fOut, double time, ParticleInfo *particleInfos, long *bins, double velocityMax ) {

    int sum = 0;
    int numberOfCollisionsInTimeStep = 0;
    double betaPerBin = velocityMax / MCGIDI_speedOfLight_cm_sec / nBins;

    for( int i1 = 0; i1 <= nBins; i1++ ) bins[i1] = 0;
    for( long i1 = 0; i1 < nParticles; i1++ ) {
        int bin = (int) ( nBins * particleInfos[i1].velocity / velocityMax );

        if( bin > nBins ) bin = nBins;
        bins[bin]++;

        numberOfCollisionsInTimeStep += particleInfos[i1].numberOfCollisionsInTimeStep;
        particleInfos[i1].numberOfCollisionsInTimeStep = 0;
    }

    std::string timeLabel = LUPI::Misc::argumentsToString( "# time = %e\n", time );

    fprintf( fOut, "\n\n" );
    fprintf( fOut, "# time = %e\n", time );
    fprintf( fOut, "# numberOfCollisionsInTimeStep = %d\n", numberOfCollisionsInTimeStep );
    fprintf( fOut, "#   energy        pdf       probability    count\n" );

    double energy1 = 0.0;
    for( int i1 = 0; i1 <= nBins; ++i1 ) {
        double probability = bins[i1] / (double) nParticles;
        double beta2 = ( i1 + 1 ) * betaPerBin;
        double energy2 = 0.5 * neutronMass * beta2 * beta2;
        double dEnergy = energy2 - energy1;
        double binEnergy = 0.5 * ( energy1 + energy2 );

        fprintf( fOut, "%e %e %e %9ld\n", binEnergy, probability / dEnergy, probability, bins[i1] );
        sum += bins[i1];
        energy1 = energy2;
    }

    fprintf( fOut, "# sum = %d\n", sum );

    for( long i1 = 0; i1 < nParticles; i1++ ) temp[i1] = particleInfos[i1].energy;
    printData( 0, timeLabel );

    for( long i1 = 0; i1 < nParticles; i1++ ) temp[i1] = particleInfos[i1].velocity;
    printData( 1, timeLabel );

    for( long i1 = 0; i1 < nParticles; i1++ ) temp[i1] = particleInfos[i1].vx;
    printData( 2, timeLabel );

    for( long i1 = 0; i1 < nParticles; i1++ ) temp[i1] = particleInfos[i1].vy;
    printData( 3, timeLabel );

    for( long i1 = 0; i1 < nParticles; i1++ ) temp[i1] = particleInfos[i1].vz;
    printData( 4, timeLabel );
}
/*
==============================================================================
*/
void printData( int index, std::string const &timeLabel ) {

    double dataMin = temp[0], dataMax = temp[0];

    for( long i1 = 0; i1 < nParticles; i1++ ) {
        if( temp[i1] < dataMin ) dataMin = temp[i1];
        if( temp[i1] > dataMax ) dataMax = temp[i1];
    }

    if( dataMin == dataMax ) {
        if( dataMin < 0.0 ) {
            dataMin =  10.0  * dataMin;
            dataMax =   0.1 * dataMax; }
        else if ( dataMin == 0.0 ) {
            dataMin = -1.0;
            dataMin =  1.0; }
        else {
            dataMin =  0.1  * dataMin;
            dataMax = 10.0 * dataMax;
        }
    }

    Bins bins( nBins, dataMin, dataMax, index < 2 );

    for( long i1 = 0; i1 < nParticles; i1++ ) bins.accrue( temp[i1] );
    bins.print( fileInfo[index]->m_fOut, timeLabel.c_str( ) );
}
