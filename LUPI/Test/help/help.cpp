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

#include <LUPI.hpp>

static char const *description = "The test1 checker.";

static char const *fourScore = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, " \
    "and dedicated to the proposition that all men are created equal.  Now we are engaged in a great civil war, testing whether that nation, or any " \
    "nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of " \
    "that field, as a final resting place for those who here gave their lives that the nation might live. It is altogether fitting and proper that we " \
    "should do this.  But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living " \
    "and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what " \
    "we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they " \
    "who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these " \
    "honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that " \
    "these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by " \
    "the people, for the people, shall not perish from the earth.";

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

    LUPI::Positional *positional1 = argumentParser.add<LUPI::Positional>( "fourScore", fourScore, 1, 1 );

    LUPI::OptionTrue *optionTrue = argumentParser.add<LUPI::OptionTrue>( "--true", "Testing the 'OptionTrue' class." );
    argumentParser.addAlias( optionTrue, "-t" );
    argumentParser.addAlias( optionTrue, "-y" );
    argumentParser.addAlias( optionTrue, "-yes" );
    argumentParser.addAlias( optionTrue, "--yeap" );
    LUPI::OptionFalse *optionFalse = argumentParser.add<LUPI::OptionFalse>( "--false", "Testing the 'OptionFalse' class." );

    LUPI::OptionCounter *optionCounter = argumentParser.add<LUPI::OptionCounter>( "--veryVeryLongCounterNameVeryVeryLong", 
            "A very long counter name, very long." );
    LUPI::OptionStore *optionStore = argumentParser.add<LUPI::OptionStore>( "--store", "The path to a pops file to load." );
    LUPI::OptionAppend *optionAppend = argumentParser.add<LUPI::OptionAppend>( "--pops", "The path to a pops file to load.", 0, -1 );
    argumentParser.addAlias( "--pops", "-p" );
    argumentParser.addAlias( optionAppend, "-pops" );
    LUPI::Positional *positional2 = argumentParser.add<LUPI::Positional>( "GNDS", 
            "The path to a GNDS file to load. This is a very very long discriptor. It should cause line wrapping. Ok, may it needs to longer and longer and longer." );
    LUPI::OptionAppend *p24 = argumentParser.add<LUPI::OptionAppend>( "--p24", "Option that requires 2 to 4 instances.", 2, 4 );
    LUPI::Positional *positional3 = argumentParser.add<LUPI::Positional>( "name", "Short discriptor.", 2, 2 );

    argumentParser.parse( argc, argv );

    std::cout << "    " << optionTrue->name( )  << " option: number entered " << optionTrue->counts( ) << ",   value = " << optionTrue->value( ) << std::endl;
    std::cout << "    " << optionFalse->name( ) << " option: number entered " << optionFalse->counts( ) << ",   value = " << optionFalse->value( ) << std::endl;
    std::cout << "    " << optionCounter->name( ) << " option: number entered " << optionCounter->counts( ) << ",   counts = " << optionCounter->counts( ) << std::endl;
    std::cout << "    " << optionStore->name( ) << " option: number entered " << optionStore->counts( ) << ",   value = '" << optionStore->value( ) << "'" << std::endl;

    std::cout << "    " << optionAppend->name( ) << " option: number entered " << optionAppend->counts( ) << std::endl;
    std::cout << "         ";
    for( std::size_t index = 0; index < optionAppend->counts( ); ++index ) {
        std::cout << " '" << optionAppend->value( index ) << "'";
    }
    std::cout << std::endl;

    std::cout << "    " << p24->name( ) << " option: number entered " << p24->counts( ) << std::endl;
    std::cout << "         ";
    for( std::size_t index = 0; index < p24->counts( ); ++index ) {
        std::cout << " '" << p24->value( index ) << "'";
    }
    std::cout << std::endl;

    std::cout << "    " << positional1->name( ) << positional1->counts( ) << std::endl;
    std::cout << "         ";
    for( std::size_t index = 0; index < positional1->counts( ); ++index ) {
        std::cout << " '" << positional1->value( index ) << "'";
    }
    std::cout << std::endl;

    std::cout << "    " << positional2->name( ) << positional2->counts( ) << std::endl;
    std::cout << "         ";
    for( std::size_t index = 0; index < positional2->counts( ); ++index ) {
        std::cout << " '" << positional2->value( index ) << "'";
    }
    std::cout << std::endl;

    std::cout << "    " << positional3->name( ) << positional3->counts( ) << std::endl;
    std::cout << "         ";
    for( std::size_t index = 0; index < positional3->counts( ); ++index ) {
        std::cout << " '" << positional3->value( index ) << "'";
    }
    std::cout << std::endl;
}
