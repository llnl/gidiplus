# GIDI+

**GIDI+** is a collection of C++ libraries for accessing evaluated and processed nuclear data stored in the 
Generalized Nuclear Database Structure (**GNDS**).  In addition to reading **GNDS** files, **GIDI+** has functions to 
sum and collapse multi-group data as needed by deterministic transport codes, and to sample **GNDS** data as needed by
Monte Carlo transport codes.

# Dependency

**GIDI+** requires the third party library pugixml version 1.13.  If pugixml is not already present in the **GIDI+ Misc**
directory, download it from https://pugixml.org/2022/11/02/pugixml-1.13-release.html and place it in the 'Misc' folder.

# Installation

To clone the **GIDI+** Git repository the following command is recommended:
```
git clone --recurse-submodules https://github.com/LLNL/gidiplus.git
```

**NOTE:**
If you have an older version of git (2.2 or lower), use `git lfs clone` instead of `git clone`.
This is unnecessary on newer versions of git since they handle lfs (Large File Storage) automatically.

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

# Working with Git submodules

**GIDI+** is composed of submodules that are hosted in their own repositories and Git keeps a record of this in the following files:

- .gitmodules which contains the local, relative path to the submodules and the URL to the corresponding 
        remote repositories; and

- files, containing the commit hash for the version used in **GIDI+**, for each submodule.

Consequently, **GIDI+** points to a specific version of each submodule while code updates in the individual submodules continue independently. 
The submodules may also be in a state called *detached HEAD* which indicates that **GIDI+** is not associated with a submodule's local branch name. 
This may be observed via the output from the following command:
```
git submodule foreach 'git status'
```

The output for a given submodule may contain the string *HEAD detached* (indicating a submodule in the *detached HEAD* state) or 
*Your branch is up to date with 'origin/master'*. If no code updates are to be done in a submodule in the *HEAD detached* state, no further 
action is required. If a submodule is to be updated, it will need to be associated with a branch that contains the commit to which **GIDI+**
points. For example, to associate **GIDI** with the master branch use the command:
```
cd GIDI; git checkout master
```

or to associated all submodules with master, use
```
git submodule foreach 'git checkout master'
```

These commands will associate the submodule(s) with the latest commit in that branch and this may not correspond to the submodule commit to which 
**GIDI+** is pointing. The following command provides the commit hash identifier that **GIDI+** points at for each of the submodules:
```
git ls-tree -r HEAD | grep commit
```

To list the commit identifiers for the currently checked-out submodules, the following command may be used:
```
git submodule foreach 'git rev-parse HEAD'
```

A difference in commit hash identifiers will indicate a difference between the currently checked-out version of the submodule and the version to 
which **GIDI+** is pointing. The command `git status` in the **GIDI+** folder will also indicate if the file associated with the submodule is in 
the modified state.

It is obviously important that any code updates to the submodule be `git push` before the corresponding **GIDI+** `git push` command. This prevents 
future **GIDI+** repository checkouts that point to a non-existent submodule commit hash identifier.

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
