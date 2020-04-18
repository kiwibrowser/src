# Build Instructions

Instructions for building this repository on Linux, Windows, and MacOS.

## Index

1. [Contributing](#contributing-to-the-repository)
1. [Repository Content](#repository-content)
1. [Repository Set-Up](#repository-set-up)
1. [Windows Build](#building-on-windows)
1. [Linux Build](#building-on-linux)
1. [MacOS build](#building-on-macos)

## Contributing to the Repository

If you intend to contribute, the preferred work flow is for you to develop
your contribution in a fork of this repository in your GitHub account and then
submit a pull request. Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file
in this repository for more details.

## Repository Content

This repository contains the source code necessary to build the desktop Vulkan
loader and its tests.

### Installed Files

The `install` target installs the following files under the directory
indicated by *install_dir*:

- *install_dir*`/lib` : The Vulkan loader library
- *install_dir*`/bin` : The Vulkan loader library DLL (Windows)

The `uninstall` target can be used to remove the above files from the install
directory.

## Repository Set-Up

### Display Drivers

This repository does not contain a Vulkan-capable driver. You will need to
obtain and install a Vulkan driver from your graphics hardware vendor or from
some other suitable source if you intend to run Vulkan applications.

### Download the Repository

To create your local git repository:

    git clone https://github.com/KhronosGroup/Vulkan-Loader.git

### Repository Dependencies

This repository attempts to resolve some of its dependencies by using
components found from the following places, in this order:

1. CMake or Environment variable overrides (e.g., -DVULKAN_HEADERS_INSTALL_DIR)
1. LunarG Vulkan SDK, located by the `VULKAN_SDK` environment variable
1. System-installed packages, mostly applicable on Linux

Dependencies that cannot be resolved by the SDK or installed packages must be
resolved with the "install directory" override and are listed below. The
"install directory" override can also be used to force the use of a specific
version of that dependency.

#### Vulkan-Headers

This repository has a required dependency on the [Vulkan Headers repository](https://github.com/KhronosGroup/Vulkan-Headers).
You must clone the headers repository and build its `install` target before
building this repository. The Vulkan-Headers repository is required because it
contains the Vulkan API definition files (registry) that are required to build
the loader. You must also take note of the headers install directory and pass
it on the CMake command line for building this repository, as described below.

#### Google Test

The loader tests depend on the [Google Test](https://github.com/google/googletest)
framework and do not build unless this framework is downloaded into the
repository's `external` directory.

To obtain the framework, change your current directory to the top of your
Vulkan-Loader repository and run:

    git clone https://github.com/google/googletest.git external/googletest
    cd external/googletest
    git checkout tags/release-1.8.1

before configuring your build with CMake.

If you do not need the loader tests, there is no need to download this
framework.

### Build and Install Directories

A common convention is to place the `build` directory in the top directory of
the repository and place the `install` directory as a child of the `build`
directory. The remainder of these instructions follow this convention,
although you can place these directories in any location.

### Building Dependent Repositories with Known-Good Revisions

There is a Python utility script, `scripts/update_deps.py`, that you can use
to gather and build the dependent repositories mentioned above. This program
also uses information stored in the `scripts/known-good.json` file to checkout
dependent repository revisions that are known to be compatible with the
revision of this repository that you currently have checked out.

Here is a usage example for this repository:

    git clone git@github.com:KhronosGroup/Vulkan-Loader.git
    cd Vulkan-Loader
    mkdir build
    cd build
    ../scripts/update_deps.py
    cmake -C helper.cmake ..
    cmake --build .

#### Notes

- You may need to adjust some of the CMake options based on your platform. See
  the platform-specific sections later in this document.
- The `update_deps.py` script fetches and builds the dependent repositories in
  the current directory when it is invoked. In this case, they are built in
  the `build` directory.
- The `build` directory is also being used to build this
  (Vulkan-ValidationLayers) repository. But there shouldn't be any conflicts
  inside the `build` directory between the dependent repositories and the
  build files for this repository.
- The `--dir` option for `update_deps.py` can be used to relocate the
  dependent repositories to another arbitrary directory using an absolute or
  relative path.
- The `update_deps.py` script generates a file named `helper.cmake` and places
  it in the same directory as the dependent repositories (`build` in this
  case). This file contains CMake commands to set the CMake `*_INSTALL_DIR`
  variables that are used to point to the install artifacts of the dependent
  repositories. You can use this file with the `cmake -C` option to set these
  variables when you generate your build files with CMake. This lets you avoid
  entering several `*_INSTALL_DIR` variable settings on the CMake command line.
- If using "MINGW" (Git For Windows), you may wish to run
  `winpty update_deps.py` in order to avoid buffering all of the script's
  "print" output until the end and to retain the ability to interrupt script
  execution.
- Please use `update_deps.py --help` to list additional options and read the
  internal documentation in `update_deps.py` for further information.

### Build Options

When generating native platform build files through CMake, several options can
be specified to customize the build. Some of the options are binary on/off
options, while others take a string as input. The following is a table of all
on/off options currently supported by this repository:

| Option | Platform | Default | Description |
| ------ | -------- | ------- | ----------- |
| BUILD_LOADER | All | `ON` | Controls whether or not the loader is built. Setting this to `OFF` will allow building the tests against a loader that is installed to the system. |
| BUILD_TESTS | All | `???` | Controls whether or not the loader tests are built. The default is `ON` when the Google Test repository is cloned into the `external` directory.  Otherwise, the default is `OFF`. |
| BUILD_WSI_XCB_SUPPORT | Linux | `ON` | Build the loader with the XCB entry points enabled. Without this, the XCB headers should not be needed, but the extension `VK_KHR_xcb_surface` won't be available. |
| BUILD_WSI_XLIB_SUPPORT | Linux | `ON` | Build the loader with the Xlib entry points enabled. Without this, the X11 headers should not be needed, but the extension `VK_KHR_xlib_surface` won't be available. |
| BUILD_WSI_WAYLAND_SUPPORT | Linux | `ON` | Build the loader with the Wayland entry points enabled. Without this, the Wayland headers should not be needed, but the extension `VK_KHR_wayland_surface` won't be available. |
| ENABLE_STATIC_LOADER | Windows | `OFF` | By default, the loader is built as a dynamic library. This allows it to be built as a static library, instead. |
| ENABLE_WIN10_ONECORE | Windows | `OFF` | Link the loader to the [OneCore](https://msdn.microsoft.com/en-us/library/windows/desktop/mt654039.aspx) umbrella library, instead of the standard Win32 ones. |
| USE_CCACHE | Linux | `OFF` | Enable caching with the CCache program. |

The following is a table of all string options currently supported by this repository:

| Option | Platform | Default | Description |
| ------ | -------- | ------- | ----------- |
| CMAKE_OSX_DEPLOYMENT_TARGET | MacOS | `10.12` | The minimum version of MacOS for loader deployment. |
| FALLBACK_CONFIG_DIRS | Linux/MacOS | `/etc/xdg` | Configuration path(s) to use instead of `XDG_CONFIG_DIRS` if that environment variable is unavailable. The default setting is freedesktop compliant. |
| FALLBACK_DATA_DIRS | Linux/MacOS | `/usr/local/share:/usr/share` | Configuration path(s) to use instead of `XDG_DATA_DIRS` if that environment variable is unavailable. The default setting is freedesktop compliant. |

These variables should be set using the `-D` option when invoking
CMake to generate the native platform files.

## Building On Windows

### Windows Development Environment Requirements

- Windows
  - Any Personal Computer version supported by Microsoft
- Microsoft [Visual Studio](https://www.visualstudio.com/)
  - Versions
    - [2013 (update 4)](https://www.visualstudio.com/vs/older-downloads/)
    - [2015](https://www.visualstudio.com/vs/older-downloads/)
    - [2017](https://www.visualstudio.com/vs/downloads/)
  - The Community Edition of each of the above versions is sufficient, as
    well as any more capable edition.
- [CMake](http://www.cmake.org/download/) (Version 2.8.11 or better)
  - Use the installer option to add CMake to the system PATH
- Git Client Support
  - [Git for Windows](http://git-scm.com/download/win) is a popular solution
    for Windows
  - Some IDEs (e.g., [Visual Studio](https://www.visualstudio.com/),
    [GitHub Desktop](https://desktop.github.com/)) have integrated
    Git client support

### Windows Build - Microsoft Visual Studio

The general approach is to run CMake to generate the Visual Studio project
files. Then either run CMake with the `--build` option to build from the
command line or use the Visual Studio IDE to open the generated solution and
work with the solution interactively.

#### Windows Quick Start

    cd Vulkan-Loader
    mkdir build
    cd build
    cmake -A x64 -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir ..
    cmake --build .

The above commands instruct CMake to find and use the default Visual Studio
installation to generate a Visual Studio solution and projects for the x64
architecture. The second CMake command builds the Debug (default)
configuration of the solution.

See below for the details.

#### Use `CMake` to Create the Visual Studio Project Files

Change your current directory to the top of the cloned repository directory,
create a build directory and generate the Visual Studio project files:

    cd Vulkan-Loader
    mkdir build
    cd build
    cmake -A x64 -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir ..

> Note: The `..` parameter tells `cmake` the location of the top of the
> repository. If you place your build directory someplace else, you'll need to
> specify the location of the repository top differently.

The `-A` option is used to select either the "Win32" or "x64" architecture.

If a generator for a specific version of Visual Studio is required, you can
specify it for Visual Studio 2015, for example, with:

    64-bit: -G "Visual Studio 14 2015 Win64"
    32-bit: -G "Visual Studio 14 2015"

See this [list](#cmake-visual-studio-generators) of other possible generators
for Visual Studio.

When generating the project files, the absolute path to a Vulkan-Headers
install directory must be provided. This can be done by setting the
`VULKAN_HEADERS_INSTALL_DIR` environment variable or by setting the
`VULKAN_HEADERS_INSTALL_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the installation directory of a
Vulkan-Headers repository built with the install target.

The above steps create a Windows solution file named `Vulkan-Loader.sln` in
the build directory.

At this point, you can build the solution from the command line or open the
generated solution with Visual Studio.

#### Build the Solution From the Command Line

While still in the build directory:

    cmake --build .

to build the Debug configuration (the default), or:

    cmake --build . --config Release

to make a Release build.

#### Build the Solution With Visual Studio

Launch Visual Studio and open the "Vulkan-Loader.sln" solution file in the
build folder. You may select "Debug" or "Release" from the Solution
Configurations drop-down list. Start a build by selecting the Build->Build
Solution menu item.

#### Windows Install Target

The CMake project also generates an "install" target that you can use to copy
the primary build artifacts to a specific location using a "bin, include, lib"
style directory structure. This may be useful for collecting the artifacts and
providing them to another project that is dependent on them.

The default location is `$CMAKE_BINARY_DIR\install`, but can be changed with
the `CMAKE_INSTALL_PREFIX` variable when first generating the project build
files with CMake.

You can build the install target from the command line with:

    cmake --build . --config Release --target install

or build the `INSTALL` target from the Visual Studio solution explorer.

### Windows Tests

The Vulkan-Loader repository contains some simple unit tests for the loader
but no other test clients.

To run the loader test script, open a Powershell Console, change to the
`build\tests` directory, and run:

For Release builds:

    .\run_all_tests.ps1

For Debug builds:

    .\run_all_tests.ps1 -Debug

This script will run the following tests:

- `vk_loader_validation_tests`:
  Vulkan loader handle wrapping, allocation callback, and loader/layer interface tests

You can also change to either `build\tests\Debug` or `build\tests\Release`
(depending on which one you built) and run the executable tests (`*.exe`)
files from there.

### Windows Notes

#### CMake Visual Studio Generators

The chosen generator should match one of the Visual Studio versions that you
have installed. Generator strings that correspond to versions of Visual Studio
include:

| Build Platform               | 64-bit Generator              | 32-bit Generator        |
|------------------------------|-------------------------------|-------------------------|
| Microsoft Visual Studio 2013 | "Visual Studio 12 2013 Win64" | "Visual Studio 12 2013" |
| Microsoft Visual Studio 2015 | "Visual Studio 14 2015 Win64" | "Visual Studio 14 2015" |
| Microsoft Visual Studio 2017 | "Visual Studio 15 2017 Win64" | "Visual Studio 15 2017" |

#### Using The Vulkan Loader Library in this Repository on Windows

Vulkan programs must be able to find and use the Vulkan loader
(`vulkan-1.dll`) library as well as any other libraries the program requires.
One convenient way to do this is to copy the required libraries into the same
directory as the program. The projects in this solution copy the Vulkan loader
library and the "googletest" libraries to the `build\tests\Debug` or the
`build\tests\Release` directory, which is where the
`vk_loader_validation_test.exe` executable is found, depending on what
configuration you built. (The loader validation tests use the "googletest"
testing framework.)

Other techniques include placing the library in a system folder
(C:\Windows\System32) or in a directory that appears in the `PATH` environment
variable.

See the `LoaderAndLayerInterface` document in the `loader` folder in this
repository for more information on how the loader finds driver libraries and
layer libraries. The document also describes both how ICDs and layers should
be packaged, and how developers can point to ICDs and layers within their
builds.

## Building On Linux

### Linux Development Environment Requirements

This repository has been built and tested on the two most recent Ubuntu LTS
versions. Currently, the oldest supported version is Ubuntu 14.04, meaning
that the minimum supported compiler versions are GCC 4.8.2 and Clang 3.4,
although earlier versions may work. It should be straightforward to adapt this
repository to other Linux distributions.

#### Required Package List

    sudo apt-get install git cmake build-essential libx11-xcb-dev \
        libxkbcommon-dev libwayland-dev libxrandr-dev

### Linux Build

The general approach is to run CMake to generate make files. Then either run
CMake with the `--build` option or `make` to build from the command line.

#### Linux Quick Start

    cd Vulkan-Loader
    mkdir build
    cd build
    cmake -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir ..
    make

See below for the details.

#### Use CMake to Create the Make Files

Change your current directory to the top of the cloned repository directory,
create a build directory and generate the make files.

    cd Vulkan-Loader
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir \
          -DCMAKE_INSTALL_PREFIX=install ..

> Note: The `..` parameter tells `cmake` the location of the top of the
> repository. If you place your `build` directory someplace else, you'll need
> to specify the location of the repository top differently.

Use `-DCMAKE_BUILD_TYPE` to specify a Debug or Release build.

When generating the project files, the absolute path to a Vulkan-Headers
install directory must be provided. This can be done by setting the
`VULKAN_HEADERS_INSTALL_DIR` environment variable or by setting the
`VULKAN_HEADERS_INSTALL_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the installation directory of a
Vulkan-Headers repository built with the install target.

> Note: For Linux, the default value for `CMAKE_INSTALL_PREFIX` is
> `/usr/local`, which would be used if you do not specify
> `CMAKE_INSTALL_PREFIX`. In this case, you may need to use `sudo` to install
> to system directories later when you run `make install`.

#### Build the Project

You can just run `make` to begin the build.

To speed up the build on a multi-core machine, use the `-j` option for `make`
to specify the number of cores to use for the build. For example:

    make -j4

You can also use

    cmake --build .

If your build system supports ccache, you can enable that via CMake option
`-DUSE_CCACHE=On`

### Linux Notes

#### Using The Vulkan Loader Library in this Repository on Linux

The `vk_loader_validation_tests` executable is linked with an RPATH setting to
allow it to find the Vulkan loader library in the repository's build
directory. This allows the test executable to run and find this Vulkan loader
library without installing the loader library to a directory searched by the
system loader or in the `LD_LIBRARY_PATH`.

If you want to test a Vulkan application that is not built within this
repository with the loader you just built from this repository, you can direct
the application to load it from your build directory:

    export LD_LIBRARY_PATH=<path to your repository root>/build/loader

#### WSI Support Build Options

By default, the Vulkan Loader is built with support for the Vulkan-defined WSI
display servers: Xcb, Xlib, and Wayland. It is recommended to build the
repository components with support for these display servers to maximize their
usability across Linux platforms. If it is necessary to build these modules
without support for one of the display servers, the appropriate CMake option
of the form `BUILD_WSI_xxx_SUPPORT` can be set to `OFF`.

#### Linux Install to System Directories

Installing the files resulting from your build to the systems directories is
optional since environment variables can usually be used instead to locate the
binaries. There are also risks with interfering with binaries installed by
packages. If you are certain that you would like to install your binaries to
system directories, you can proceed with these instructions.

Assuming that you've built the code as described above and the current
directory is still `build`, you can execute:

    sudo make install

This command installs files to `/usr/local` if no `CMAKE_INSTALL_PREFIX` is
specified when creating the build files with CMake:

- `/usr/local/lib`:  Vulkan loader library and package config files

You may need to run `ldconfig` in order to refresh the system loader search
cache on some Linux systems.

You can further customize the installation location by setting additional
CMake variables to override their defaults. For example, if you would like to
install to `/tmp/build` instead of `/usr/local`, on your CMake command line
specify:

    -DCMAKE_INSTALL_PREFIX=/tmp/build

Then run `make install` as before. The install step places the files in
`/tmp/build`. This may be useful for collecting the artifacts and providing
them to another project that is dependent on them.

Using the `CMAKE_INSTALL_PREFIX` to customize the install location also
modifies the loader search paths to include searching for layers in the
specified install location. In this example, setting `CMAKE_INSTALL_PREFIX` to
`/tmp/build` causes the loader to search
`/tmp/build/etc/vulkan/explicit_layer.d` and
`/tmp/build/share/vulkan/explicit_layer.d` for the layer JSON files. The
loader also searches the "standard" system locations of
`/etc/vulkan/explicit_layer.d` and `/usr/share/vulkan/explicit_layer.d` after
searching the two locations under `/tmp/build`.

You can further customize the installation directories by using the CMake
variables `CMAKE_INSTALL_SYSCONFDIR` to rename the `etc` directory and
`CMAKE_INSTALL_DATADIR` to rename the `share` directory.

See the CMake documentation for more details on using these variables to
further customize your installation.

Also see the `LoaderAndLayerInterface` document in the `loader` folder in this
repository for more information about loader operation.

Note that some executables in this repository (e.g.,
`vk_loader_validation_tests`) use the RPATH linker directive to load the
Vulkan loader from the build directory, `build` in this example. This means
that even after installing the loader to the system directories, these
executables still use the loader from the build directory.

#### Linux Uninstall

To uninstall the files from the system directories, you can execute:

    sudo make uninstall

#### Linux Tests

The Vulkan-Loader repository contains some simple unit tests for the loader
but no other test clients.

To run the loader test script, change to the `build/tests` directory, and run:

    ./run_all_tests.sh

This script will run the following tests:

- `vk_loader_validation_tests`: Vulkan loader handle wrapping, allocation
  callback, and loader/layer interface tests

#### Linux 32-bit support

Usage of this repository's contents in 32-bit Linux environments is not
officially supported. However, since this repository is supported on 32-bit
Windows, these modules should generally work on 32-bit Linux.

Here are some notes for building 32-bit targets on a 64-bit Ubuntu "reference"
platform:

If not already installed, install the following 32-bit development libraries:

`gcc-multilib g++-multilib libx11-dev:i386`

This list may vary depending on your distribution and which windowing systems
you are building for.

Set up your environment for building 32-bit targets:

    export ASFLAGS=--32
    export CFLAGS=-m32
    export CXXFLAGS=-m32
    export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu

Again, your PKG_CONFIG configuration may be different, depending on your
distribution.

Finally, rebuild the repository using `cmake` and `make`, as explained above.

## Building on MacOS

### MacOS Development Environment Requirements

Tested on OSX version 10.12.6

Setup Homebrew and components

- Follow instructions on [brew.sh](http://brew.sh) to get Homebrew installed.

      /usr/bin/ruby -e "$(curl -fsSL \
          https://raw.githubusercontent.com/Homebrew/install/master/install)"

- Ensure Homebrew is at the beginning of your PATH:

      export PATH=/usr/local/bin:$PATH

- Add packages with the following (may need refinement)

      brew install cmake python python3 git

### Clone the Repository

Clone the Vulkan-ValidationLayers repository:

    git clone https://github.com/KhronosGroup/Vulkan-ValidationLayers.git

### MacOS build

#### CMake Generators

This repository uses CMake to generate build or project files that are then
used to build the repository. The CMake generators explicitly supported in
this repository are:

- Unix Makefiles
- Xcode

#### Building with the Unix Makefiles Generator

This generator is the default generator.

When generating the project files, the absolute path to a Vulkan-Headers
install directory must be provided. This can be done by setting the
`VULKAN_HEADERS_INSTALL_DIR` environment variable or by setting the
`VULKAN_HEADERS_INSTALL_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the installation directory of a
Vulkan-Headers repository built with the install target.

    mkdir build
    cd build
    cmake -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir -DCMAKE_BUILD_TYPE=Debug ..
    make

To speed up the build on a multi-core machine, use the `-j` option for `make`
to specify the number of cores to use for the build. For example:

    make -j4

#### Building with the Xcode Generator

To create and open an Xcode project:

    mkdir build-xcode
    cd build-xcode
    cmake -GXcode ..
    open Vulkan-Loader.xcodeproj

Within Xcode, you can select Debug or Release builds in the project's Build
Settings.

### Using the new macOS loader

If you want to test a Vulkan application with the loader you just built, you
can direct the application to load it from your build directory:

    export DYLD_LIBRARY_PATH=<path to your repository>/build/loader

### MacOS Tests

The Vulkan-Loader repository contains some simple unit tests for the loader
but no other test clients.

Before you run these tests, you will need to clone and build the
[MoltenVK](https://github.com/KhronosGroup/MoltenVK) repository.

You will also need to direct your new loader to the MoltenVK ICD:

    export VK_ICD_FILENAMES=<path to MoltenVK repository>/Package/Latest/MoltenVK/macOS/MoltenVK_icd.json

To run the loader test script, change to the `build/tests` directory in your
Vulkan-Loader repository, and run:

    ./vk_loader_validation_tests
