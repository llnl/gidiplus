#! /usr/bin/env python3

import sys
import os
import shutil
import pathlib

doPatch = not 'MCGIDI_USE_DOUBLES' in sys.argv[1]
benchmarks = pathlib.Path('Benchmarks')

floatDiffs = pathlib.Path('FloatDiffs')
if floatDiffs.exists():
    for out in floatDiffs.glob('*.out'):
        benchmark = benchmarks / out.name
        shutil.copy(out, benchmark)
        if doPatch:
            patch = out.with_suffix('.patch')
            os.system('patch -s %s %s' % (benchmark, patch))
