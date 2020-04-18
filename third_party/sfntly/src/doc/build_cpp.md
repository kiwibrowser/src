# How to build sfntly C++ port

## Build Environment Requirements

*   cmake 2.6 or above
*   C++ compiler requirement
    *   Windows: Visual C++ 2008, Visual C++ 2010
    *   Linux: g++ 4.3 or above.
        g++ must support built-in atomic ops and has companion libstd++.
    *   Mac: Apple XCode 3.2.5 or above.

## External Dependencies

sfntly is dependent on several external packages.
 
*   [Google C++ Testing Framework](https://github.com/google/googletest)

    You can download the package yourself or extract the one from `ext/redist`.
    The package needs to be extracted to ext and rename/symbolic link to `gtest`.

    sfntly C++ port had been tested with gTest 1.6.0.
 
*   ICU

    For Linux, default ICU headers in system will be used.
    Linux users please make sure you have dev packages for ICU.
    For example, you can run `sudo apt-get install libicu-dev` in Ubuntu and see if the required library is installed.
    
    For Windows, download from http://site.icu-project.org/download or extract the one from `ext/redist`.
    You can also provide your own ICU package.
    However, you need to alter the include path, library path, and provide `icudt.dll`.
    
    Tested with ICU 4.6.1 binary release.

    For Mac users, please download ICU source tarball from http://site.icu-project.org/download
    and install according to ICU documents.

## Getting the Source

Clone the Git repository from https://github.com/googlei18n/sfntly.
 
## Building on Windows

Let's assume your folder for sfntly is `d:\src\sfntly`.

1.  If you don't have cmake installed, extract the cmake-XXX.zip
    into `d:\src\sfntly\cpp\ext\cmake` removing the "-XXX" part.
    The extracted binary should be in `d:\src\sfntly\cpp\ext\cmake\bin\cmake.exe`.
2.  Extract gtest-XXX.zip into `d:\src\sfntly\cpp\ext\gtest`
    removing the "-XXX" part.
3.  Extract icu4c-XXX.zip into `d:\src\sfntly\cpp\ext\icu`
    removing the "-XXX" part.
4.  Run the following commands to create the Visual Studio solution files:

    ```
    d:
    cd d:\src\sfntly\cpp
    md build
    cd build
    ..\ext\cmake\bin\cmake ..
    ```

You should see `sfntly.sln` in `d:\src\sfntly\cpp\build`.
 
5.  Until the test is modified to access the fonts in the `ext\data` directory:
    copy the test fonts from `d:\src\sfntly\cpp\data\ext\` to `d:\src\sfntly\cpp\build\bin\Debug`.
6.  Open sfntly.sln.
    Since sfntly use STL extensively, please patch your Visual Studio for any STL-related hotfixes/service packs.
7.  Build the solution (if the icuuc dll is not found,
    you may need to add `d:\src\sfntly\cpp\ext\icu\bin` to the system path).

### Building on Windows via Command Line

Visual Studio 2008 and 2010 support command line building,
therefore you dont need the IDE to build the project.

For Visual Studio 2008 (assume its installed at `c:\vs08`)

    cd d:\src\sfntly\cpp\build
    ..\ext\cmake\bin\cmake .. -G "Visual Studio 9 2008"
    c:\vs08\common7\tools\vsvars32.bat
    vcbuild sfntly.sln

We invoke the cmake with `-G` to make sure
Visual Studio 2008 solution/project files are generated.
You can also use `devenv sfntly.sln /build`
to build the solution instead of using `vcbuild`.

There are subtle differences between `devenv` and `vcbuild`.
Please refer to your Visual Studio manual for more details.

For Visual Studio 2010 (assume its installed at `c:\vs10`)


    cd d:\src\sfntly\cpp\build
    ..\ext\cmake\bin\cmake .. -G "Visual Studio 10"
    c:\vs10\common7\tools\vsvars32.bat
    msbuild sfntly.sln

If you install both Visual Studio 2008 and 2010 on your system,
you cant run the scripts above in the same Command Prompt window.
`vsvars32.bat` assumes that it is run from a clean Command Prompt.

## Building on Linux/Mac

### Recommended Out-of-Source Building

1.  `cd` *&lt;sfntly dir&gt;*
2.  `mkdir build`
3.  `cd build`
4.  `cmake ..`
5.  `make`

### Default In-Source Building

> This is not recommended.
> Please use out-of-source whenever possible.

1.  `cd` *&lt;sfntly dir&gt;*
2.  `cmake .`
3.  `make`

### Using clang Instead

Change the `cmake` command line to

    CC=clang CXX=clang++ cmake ..

The generated Makefile will use clang.
Please note that sfntly uses a lot of advanced C++ semantics that
might not be understood or compiled correctly by earlier versions
of clang (2.8 and before).
   
sfntly is tested to compile and run correctly on clang 3.0 (trunk 135314).
clang 2.9 might work but unfortunately we dont have the resource to test it.

### Debug and Release Builds

Currently Debug builds are set as default.
To build Release builds, you can either modify the `CMakeList.txt`,
or set environment variable `CMAKE_BUILD_TYPE` to `Release`
before invoking `cmake`.
 
Windows users can just switch the configuration in Visual Studio.
 
## Running Unit Test

A program named `unit_test` will be generated after a full compilation.
It expects fonts in `data/ext` to be in the same directory it resides to execute the unit tests.
Windows users also needs to copy `icudt.dll` and `icuuc.dll` to that directory.
