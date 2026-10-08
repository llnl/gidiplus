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
#include <MCGIDI_testUtilities.hpp>

class ParticleInfo {

    public:
        int numberOfCollisions;
        int numberOfCollisionsInTimeStep;
        double energy;
        double initialEnergy;
        double velocity;
        double averageCollisionTime;
        double timeToCollision;
};

#define nBins 501
static double neutronMass;
static double temperature_MeV = 1.0e-3;
static int nParticles = 100000;

static MCGIDI::URR_protareInfos URR_protare_infos;

void main2( int argc, char **argv );
void updateParticle( MCGIDI::Protare *protare, double energy, MCGIDI::DomainHash &domainHash, ParticleInfo *particle, bool firstTime );
void printBins( FILE *fOut, double time, ParticleInfo *particleInfos, long *bins, double velocityMax );

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

    std::string mapFilename( "Data/upscatterModelB.map" );
    PoPI::Database pops( "../../../TestData/PoPs/pops.xml" );
    GIDI::Map::Map map( mapFilename, pops );
    std::string neutronID( PoPI::IDs::neutron );
    std::string targetID = "O16";
    int neutronIndex( pops[neutronID] );
    int nTimeSteps = 5001;
    GIDI::Transporting::Particles particles;
    double initialEnergy = 1e-1;
    long bins[nBins+1];
    double time = 0.0;
    GIDI::ExcludeReactionsSet reactionsToExclude;
    LUPI::StatusMessageReporting smr1;
    unsigned long long rngState = 1;

    std::cerr << "    " << __FILE__;
    for( int i1 = 1; i1 < argc; i1++ ) std::cerr << " " << argv[i1];
    std::cerr << std::endl;

    if( argc > 1 ) targetID = argv[1];
    std::string protareFilename( map.protareFilename( neutronID, targetID ) );

    GIDI::Construction::Settings construction( GIDI::Construction::ParseMode::all, GIDI::Construction::PhotoMode::nuclearAndAtomic );
    GIDI::Protare *protare = map.protare( construction, pops, neutronID, targetID );

    GIDI::Styles::TemperatureInfos temperatures = protare->temperatures( );

    std::string label( temperatures[0].griddedCrossSection( ) );
    MCGIDI::Transporting::MC MC( pops, neutronID, &protare->styles( ), label, GIDI::Transporting::DelayedNeutrons::on, 20.0 );

    GIDI::Transporting::Groups_from_bdfls groups_from_bdfls( "../../../GIDI/Test/bdfls" );
    GIDI::Transporting::Fluxes_from_bdfls fluxes_from_bdfls( "../../../GIDI/Test/bdfls", 0 );

    GIDI::Transporting::Particle neutron( PoPI::IDs::neutron, groups_from_bdfls.getViaGID( 4 ) );
    neutron.appendFlux( fluxes_from_bdfls.getViaFID( 1 ) );
    particles.add( neutron );

    MCGIDI::DomainHash domainHash( 4000, 1e-8, 10 );
    MCGIDI::Protare *MCProtare = MCGIDI::protareFromGIDIProtare( smr1, *protare, pops, MC, particles, domainHash, temperatures, reactionsToExclude );

    MCGIDI::Vector<MCGIDI::Protare *> protares( 1 );
    protares[0] = MCProtare;
    URR_protare_infos.setup( protares );

    PoPI::Base const &target = pops.get<PoPI::Base>( targetID );
    std::string Str = LUPI::Misc::argumentsToString( "relax.%s.dat", target.ID( ).c_str( ) );
    FILE *fOut;
    if( ( fOut = fopen( Str.c_str( ), "w" ) ) == nullptr ) throw "error opening output file";

    neutronMass = MCProtare->projectileMass( );
    double targetMass = MCProtare->targetMass( );
    double velocityMax = MCGIDI_speedOfLight_cm_sec * MCGIDI_particleBeta( neutronMass, 100 * temperature_MeV );

    ParticleInfo *particleInfos = new ParticleInfo[nParticles];
    for( long i1 = 0; i1 < nParticles; ++i1 ) {
        particleInfos[i1].timeToCollision = 0;
        updateParticle( MCProtare, initialEnergy, domainHash, &(particleInfos[i1]), true );
    }
    printBins( fOut, time, particleInfos, bins, velocityMax );

    MCGIDI::Sampling::StdVectorProductHandler products;

    MCGIDI::Sampling::Input input( true, MCGIDI::Sampling::Upscatter::Model::B );

    MCGIDI::Reaction const *reaction = MCProtare->reaction( 0 );
    long badNeutronIndex = 0;
    long badProductNumber = 0;
    for( long i1 = 0; i1 < nTimeSteps; ++i1 ) {
        double dTime = 0.0;

        for( long i2 = 0; i2 < nParticles; ++i2 ) dTime += particleInfos[i2].averageCollisionTime;
        dTime *= targetMass / neutronMass * 0.004 / (double) nParticles;
        time += dTime;

        for( long i2 = 0; i2 < nParticles; i2++ ) {
            ParticleInfo *particle = &(particleInfos[i2]);

            particle->timeToCollision -= dTime;
            while( particle->timeToCollision <= 0. ) {
                input.setTemperatureAndEnergy( temperature_MeV, particle->energy );
                reaction->sampleProducts( MCProtare, input, [&]( ) -> double { return float64RNG64( &rngState ); },
                        [&]( MCGIDI::Sampling::Product &a_product ) -> void { products.push_back( a_product ); }, products );
                if( products.size( ) < 1 ) {
                    ++badProductNumber; }
                else {
                    MCGIDI::Sampling::Product const &product = products[0];
                    if( product.m_productIndex != neutronIndex ) {
                        ++badNeutronIndex; }
                    else {
                        updateParticle( MCProtare, product.m_kineticEnergy, domainHash, particle, false );
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

    delete[] particleInfos;
    delete protare;
    delete MCProtare;

    exit( EXIT_SUCCESS );
}
/*
==============================================================================
*/
void updateParticle( MCGIDI::Protare *protare, double energy, MCGIDI::DomainHash &domainHash, ParticleInfo *particle, bool firstTime ) {

    particle->energy = energy;
    if( firstTime ) {
        particle->initialEnergy = energy;
        particle->numberOfCollisions = -1;
        particle->numberOfCollisionsInTimeStep = -1;
    }
    particle->numberOfCollisions++;
    particle->numberOfCollisionsInTimeStep++;
    particle->velocity = MCGIDI_speedOfLight_cm_sec * MCGIDI_particleBeta( neutronMass, energy );

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

    fprintf( fOut, "\n\n" );
    fprintf( fOut, "# time = %e\n", time );
    fprintf( fOut, "# numberOfCollisionsInTimeStep = %d\n", numberOfCollisionsInTimeStep );
    fprintf( fOut, "#   energy        pdf       velocity    probability    count\n" );

    double energy1 = 0.0;
    for( int i1 = 0; i1 <= nBins; ++i1 ) {
        double binVelocity = ( i1 + 0.5 ) * betaPerBin;
        double probability = bins[i1] / (double) nParticles;
        double beta2 = ( i1 + 1 ) * betaPerBin;
        double energy2 = 0.5 * neutronMass * beta2 * beta2;
        double dEnergy = energy2 - energy1;
        double binEnergy = 0.5 * ( energy1 + energy2 );

        fprintf( fOut, "%e %e %e %e %9ld\n", binEnergy, probability / dEnergy, MCGIDI_speedOfLight_cm_sec * binVelocity, probability, bins[i1] );
        sum += bins[i1];
        energy1 = energy2;
    }

    fprintf( fOut, "# sum = %d\n", sum );
}
