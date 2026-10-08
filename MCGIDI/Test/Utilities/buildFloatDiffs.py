#! /usr/bin/env python3

"""
This module creates diff patch files for the difference in some MCGIDI tests between MCGIDI build with doubles (the 
MACRO MCGIDI_USE_DOUBLES defined during make) versus build with with floats for many MCGIDI variables.

To use this module go into the GIDI+ directory and,
    -) run "make realclean"
    -) run "make" with other needed options.
    -) run "cd MCGIDI"
    -) run "make realclean"
    -) run "make -DMCGIDI_USE_DOUBLES" with other needed options
    -) cd into a specifid MCGIDI test and run "make -DMCGIDI_USE_DOUBLES check" with other needed options
    -) run "mv Output D"
    -) run "cd ../../"                             # You should be back at the top of the MCGIDI directory
    -) run "make realclean"
    -) run "make" with needed options and without MCGIDI_USE_DOUBLES defined
    -) cd into a specifid MCGIDI test and run "make check" with other needed options and 
            without -DMCGIDI_USE_DOUBLES defined
    -) run "rm -rf Benchmarks"
    -) run "mv D Benchmarks"
    -) run "../Utilities/buildFloatDiffs.py"

Where differences exists, a ".patch" file will be created in the 'FloatDiffs' directory for that test directory.
"""

import os
import shutil
import pathlib

benchmarks = pathlib.Path('Benchmarks').glob('*.out')
floatDiffs = pathlib.Path('FloatDiffs')
if floatDiffs.exists():
    shutil.rmtree(floatDiffs)
floatDiffs.mkdir()

for benchmark in benchmarks:
    output = pathlib.Path('Outputs') / benchmark.name
    with open(benchmark) as fIn:
        lines1 = fIn.readlines()
    with open(output) as fIn:
        lines2 = fIn.readlines()
    if lines1 != lines2:
        path = floatDiffs / benchmark.name
        patchPath = path.with_suffix('.patch')
        os.system('diff %s %s > %s' % (benchmark, output, patchPath))
        shutil.move(benchmark, path)
