#! /usr/bin/env python3

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
