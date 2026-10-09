# GIDI+

**GIDI+** is a collection of C++ libraries for accessing evaluated and processed nuclear data stored in the 
Generalized Nuclear Database Structure (**GNDS**).  In addition to reading **GNDS** files, **GIDI+** has functions to 
sum and collapse multi-group data as needed by deterministic transport codes, and to sample **GNDS** data as needed by
Monte Carlo transport codes.

# Dependency

**GIDI+** requires the third party library pugixml version 1.15.  If pugixml is not already present in the **GIDI+ Misc**
directory, download it from https://pugixml.org/2025/01/10/pugixml-1.15-release.html and place it in the 'Misc' folder.

# Installation

To clone the **GIDI+** Git repository one of the following commands is recommended:
```
git clone https://github.com/llnl/gidiplus.git
# or using SSH (requires creating a github account and registering an ssh key):
git clone git@github.com:llnl/gidiplus.git
```

Currently, **GIDI+** uses the **unix make** command to build and puts needed header and library files into the *include* and
*lib* directories, respecitively. Important targets in the Makefile are:

| Target     | Description
|------------|------------
| default:   | Builds libgidiplus.a (and all other libraries) and puts them into the 'lib' directory. Puts all needed header files into the 'include' directory.
| install:   | Copies the header files from the 'include' directory to '$(PREFIX)/include'. Copies all '.a' from the 'lib' directory to '$(PREFIX)/lib.'
| bin:       | Compiles binary utilities for testing processed GNDS files in PoPI/bin, GIDI/bin and MCGIDI/bin.
| check:     | Runs the tests in all the sub-libraries. This target requires that the test data have been installed.
| realclean: | Returns **GIDI+** back to its initial state (i.e., removes all files created by the other targets).

**GIDI+** uses features from the 2011 C++ standard, so it must be compiled with *-std=c++11* or newer. The Makefile selects c++11 by default,
or if desired a newer version of the standard can be selected by setting the CXXFLAGS when calling make.
**GIDI+** supports parallel compilation using the '-j' flag:
```
make -s -j   # compile with default C++ 2011
# or
make -s -j CXXFLAGS="-std=c++17"
```

For building executables and running the tests, this would look like:
```
make -s -j bin
make -s -j check
```

One may specify the CC and CXX compilers and their flags when building. For example,
```
make default CC=g++ CXX=g++ CXXFLAGS="-std=c++11 -g -O3" CFLAGS="-std=gnu11 -g -O3"
```

To put the results into the path */path/to/my/builds*, execute
```
make install CC=gcc CXX=g++ CXXFLAGS="-std=c++11 -g -O3" CFLAGS="-std=gnu11 -g -O3" PREFIX=/path/to/my/builds
```

If one is using mixed **XML/HDF5 GNDS** files, **HDF5** must be included when building and linking. This is done by specifying the location
of **HDF5** with the *HDF5_PATH* macro. For example, if **HDF5** is installed on the system, them the following may work:
```
make -s CXXFLAGS="-std=c++11" HDF5_PATH=/usr
```

On some systems, the default **HDF5** library was compiled for 32-bit execution. To link in with a 64-bit library one also needs
to specify the path to the 64-bit libraries. For example,
```
make -s CXXFLAGS="-std=c++11" HDF5_PATH=/usr HDF5_LIB=/usr/lib64
```

Comments for compiling on LLNL LC systems:

A set of bash scripts for building on LC systems can be found in the *Scripts* directory.
These scripts first define some environment varibles before executing **make**.

# License

**GIDI+** is distributed under the terms of the MIT license.

All new contributions must be made under the MIT license.

See LICENSE, COPYRIGHT and NOTICE for details.

SPDX-License-Identifier: MIT

This package includes several components: \
LLNL-CODE-778320	(GIDI+)

LLNL-CODE-770917	(GIDI) \
LLNL-CODE-790397	(MCGIDI) \
LLNL-CODE-771182	(statusMessageReporting) \
LLNL-CODE-770377	(PoPI) \
LLNL-CODE-770134	(numericalFunctions)

**FUDGE** is a product of the Nuclear Data and Theory Group at Lawrence Livermore National Laboratory (LLNL).

This work was performed under the auspices of the U.S. Department of Energy by Lawrence Livermore National
Laboratory under Contract DE-AC52-07NA27344.
