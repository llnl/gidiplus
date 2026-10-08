#! /usr/bin/env python3

import pathlib

benchmarks = pathlib.Path('Benchmarks')
if benchmarks.exists():
    floatDiffs = pathlib.Path('FloatDiffs')
    if floatDiffs.exists():
        for out in floatDiffs.glob('*.out'):
            benchmark = benchmarks / out.name
            benchmark.unlink(True)

    if len(list(benchmarks.glob('*'))) == 0:
        benchmarks.rmdir()
