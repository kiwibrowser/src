# Build Instructions

Instructions for building this repository on Linux, Windows, Android, and MacOS.

## Index

1. [Contributing](#contributing-to-the-repository)
1. [Repository Content](#repository-content)
1. [Repository Set-Up](#repository-set-up)
1. [Windows Build](#building-on-windows)
1. [Linux Build](#building-on-linux)
1. [Android Build](#building-on-android)
1. [MacOS build](#building-on-macos)

## Contributing to the Repository

If you intend to contribute, the preferred work flow is for you to develop
your contribution in a fork of this repository in your GitHub account and then
submit a pull request. Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file
in this repository for more details.

## Repository Content

This repository contains the source code necessary to build the following components:

- vulkaninfo
- vkcube and vkcubepp demos
- mock ICD

### Installed Files

The `install` target installs the following files under the directory
indicated by *install_dir*:

- *install_dir*`/bin` : The vulkaninfo, vkcube and vkcubepp executables
- *install_dir*`/lib` : The mock ICD library and JSON (Windows) (If INSTALL_ICD=ON)
- *install_dir*`/share/vulkan/icd.d` : mock ICD JSON (Linux/MacOS) (If INSTALL_ICD=ON)

The `uninstall` target can be used to remove the above files from the install
directory.

## Repository Set-Up

### Display Drivers

This repository does not contain a Vulkan-capable driver. You will need to
obtain and install a Vulkan driver from your graphics hardware vendor or from
some other suitable source if you intend to run Vulkan applications.

### Download the Repository

To create your local git repository:

    git clone https://github.com/KhronosGroup/Vulkan-Tools.git

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

This repository has a required dependency on the
[Vulkan Headers repository](https://github.com/KhronosGroup/Vulkan-Headers).
You must clone the headers repository and build its `install` target before
building this repository. The Vulkan-Headers repository is required because it
contains the Vulkan API definition files (registry) that are required to build
the mock ICD. You must also take note of the headers install directory and
pass it on the CMake command line for building this repository, as described
below.

Note that this dependency can be ignored if not building the mock ICD
(CMake option: `-DBUILD_ICD=OFF`).

#### glslang

This repository has a required dependency on the `glslangValidator` (shader
compiler) for compiling the shader programs for the vkcube demos.

The CMake code in this repository downloads release binaries of glslang if a
build glslang repository is not provided. The glslangValidator is obtained
from this set of release binaries.

If you don't wish the CMake code to download these binaries, then you must
clone the [glslang repository](https://github.com/KhronosGroup/glslang) and
build its `install` target. Follow the build instructions in the glslang
[README.md](https://github.com/KhronosGroup/glslang/blob/master/README.md)
file. Ensure that the `update_glslang_sources.py` script has been run as part
of building glslang. You must also take note of the glslang install directory
and pass it on the CMake command line for building this repository, as
described below.

Note that this dependency can be ignored if not building the vkcube demo
(CMake option: `-DBUILD_CUBE=OFF`).

### Build and Install Directories

A common convention is to place the build directory in the top directory of
the repository with a name of `build` and place the install directory as a
child of the build directory with the name `install`. The remainder of these
instructions follow this convention, although you can use any name for these
directories and place them in any location.

### Building Dependent Repositories with Known-Good Revisions

There is a Python utility script, `scripts/update_deps.py`, that you can use
to gather and build the dependent repositories mentioned above. This program
also uses information stored in the `scripts/known-good.json` file to checkout
dependent repository revisions that are known to be compatible with the
revision of this repository that you currently have checked out.

Here is a usage example for this repository:

    git clone git@github.com:KhronosGroup/Vulkan-Tools.git
    cd Vulkan-Tools
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
  (Vulkan-Tools) repository. But there shouldn't be any conflicts
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
| BUILD_CUBE | All | `ON` | Controls whether or not the vkcube demo is built. |
| BUILD_VULKANINFO | All | `ON` | Controls whether or not the vulkaninfo utility is built. |
| BUILD_ICD | All | `ON` | Controls whether or not the mock ICD is built. |
| INSTALL_ICD | All | `OFF` | Controls whether or not the mock ICD is installed as part of the install target. |
| BUILD_WSI_XCB_SUPPORT | Linux | `ON` | Build the components with XCB support. |
| BUILD_WSI_XLIB_SUPPORT | Linux | `ON` | Build the components with Xlib support. |
| BUILD_WSI_WAYLAND_SUPPORT | Linux | `ON` | Build the components with Wayland support. |
| USE_CCACHE | Linux | `OFF` | Enable caching with the CCache program. |

The following is a table of all string options currently supported by this repository:

| Option | Platform | Default | Description |
| ------ | -------- | ------- | ----------- |
| CMAKE_OSX_DEPLOYMENT_TARGET | MacOS | `10.12` | The minimum version of MacOS for loader deployment. |

These variables should be set using the `-D` option when invoking CMake to
generate the native platform files.

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

    cd Vulkan-Tools
    mkdir build
    cd build
    cmake -A x64 -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir
    cmake --build .

The above commands instruct CMake to find and use the default Visual Studio
installation to generate a Visual Studio solution and projects for the x64
architecture. The second CMake command builds the Debug (default)
configuration of the solution.

See below for the details.

#### Use `CMake` to Create the Visual Studio Project Files

Change your current directory to the top of the cloned repository directory,
create a build directory and generate the Visual Studio project files:

    cd Vulkan-Tools
    mkdir build
    cd build
    cmake -A x64 -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir

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

The above steps create a Windows solution file named
`Vulkan-Tools.sln` in the build directory.

At this point, you can build the solution from the command line or open the
generated solution with Visual Studio.

#### Build the Solution From the Command Line

While still in the build directory:

    cmake --build .

to build the Debug configuration (the default), or:

    cmake --build . --config Release

to make a Release build.

#### Build the Solution With Visual Studio

Launch Visual Studio and open the "Vulkan-Tools.sln" solution file in the
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

#### Using a Loader Built from a Repository

If you do need to build and use your own loader, build the Vulkan-Loader
repository with the install target and modify your CMake invocation to add the
location of the loader's install directory:

    cmake -A x64 -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir \
                 -DVULKAN_LOADER_INSTALL_DIR=absolute_path_to_install_dir ..

#### Using glslang Built from a Repository

If you do need to build and use your own glslang, build the glslang repository
with the install target and modify your CMake invocation to add the location
of the glslang's install directory:

    cmake -A x64 -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir \
                 -DGLSLANG_INSTALL_DIR=absolute_path_to_install_dir ..

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

## Building On Linux

### Linux Build Requirements

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

    cd Vulkan-Tools
    mkdir build
    cd build
    cmake -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_install_dir ..
    make

See below for the details.

#### Use CMake to Create the Make Files

Change your current directory to the top of the cloned repository directory,
create a build directory and generate the make files.

    cd Vulkan-Tools
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

If your build system supports ccache, you can enable that via CMake option `-DUSE_CCACHE=On`

### Linux Notes

#### WSI Support Build Options

By default, the repository components are built with support for the
Vulkan-defined WSI display servers: Xcb, Xlib, and Wayland. It is recommended
to build the repository components with support for these display servers to
maximize their usability across Linux platforms. If it is necessary to build
these modules without support for one of the display servers, the appropriate
CMake option of the form `BUILD_WSI_xxx_SUPPORT` can be set to `OFF`.

Note vulkaninfo currently only supports Xcb and Xlib WSI display servers. See
the CMakeLists.txt file in `Vulkan-Tools/vulkaninfo` for more info.

You can select which WSI subsystem is used to execute the vkcube applications
using a CMake option called DEMOS_WSI_SELECTION. Supported options are XCB
(default), XLIB, and WAYLAND. Note that you must build using the corresponding
BUILD_WSI_*_SUPPORT enabled at the base repository level. For instance,
creating a build that will use Xlib when running the vkcube demos, your CMake
command line might look like:

    cmake -DCMAKE_BUILD_TYPE=Debug -DDEMOS_WSI_SELECTION=XLIB ..

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
specified when creating the build files with CMake.

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

Note: The Mock ICD is not installed by default since it is a "null" driver
that does not render anything and is used for testing purposes. Installing it
to system directories may cause some applications to discover and use this
driver instead of other full drivers installed on the system. If you really
want to install this null driver, use:

    -DINSTALL_ICD=ON

See the CMake documentation for more details on using these variables to
further customize your installation.

Also see the `LoaderAndLayerInterface` document in the `loader` folder of the
Vulkan-Loader repository for more information about loader and layer
operation.

#### Linux Uninstall

To uninstall the files from the system directories, you can execute:

    sudo make uninstall

### Linux Tests

After making any changes to the repository, you should perform some quick
sanity tests, such as running the vkcube demo with validation enabled.

To run the **vkcube application** with validation, in a terminal change to the
`build/cube` directory and run:

    VK_LAYER_PATH=../path/to/validation/layers ./vkcube --validate

If you have an SDK installed and have run the setup script to set the
`VULKAN_SDK` environment variable, it may be unnecessary to specify a
`VK_LAYER_PATH`.

#### Linux 32-bit support

Usage of the contents of this repository in 32-bit Linux environments is not
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

## Building On Android

Install the required tools for Linux and Windows covered above, then add the
following.

### Android Build Requirements

- Install [Android Studio 2.3](https://developer.android.com/studio/index.html) or later.
- From the "Welcome to Android Studio" splash screen, add the following components using
  Configure > SDK Manager:
  - SDK Platforms > Android 6.0 and newer
  - SDK Tools > Android SDK Build-Tools
  - SDK Tools > Android SDK Platform-Tools
  - SDK Tools > Android SDK Tools
  - SDK Tools > NDK

#### Add Android specifics to environment

For each of the below, you may need to specify a different build-tools
version, as Android Studio will roll it forward fairly regularly.

On Linux:

    export ANDROID_SDK_HOME=$HOME/Android/sdk
    export ANDROID_NDK_HOME=$HOME/Android/sdk/ndk-bundle
    export PATH=$ANDROID_SDK_HOME:$PATH
    export PATH=$ANDROID_NDK_HOME:$PATH
    export PATH=$ANDROID_SDK_HOME/build-tools/23.0.3:$PATH

On Windows:

    set ANDROID_SDK_HOME=%LOCALAPPDATA%\Android\sdk
    set ANDROID_NDK_HOME=%LOCALAPPDATA%\Android\sdk\ndk-bundle
    set PATH=%LOCALAPPDATA%\Android\sdk\ndk-bundle;%PATH%

On OSX:

    export ANDROID_SDK_HOME=$HOME/Library/Android/sdk
    export ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk-bundle
    export PATH=$ANDROID_NDK_PATH:$PATH
    export PATH=$ANDROID_SDK_HOME/build-tools/23.0.3:$PATH

Note: If `jarsigner` is missing from your platform, you can find it in the
Android Studio install or in your Java installation. If you do not have Java,
you can get it with something like the following:

  sudo apt-get install openjdk-8-jdk

#### Additional OSX System Requirements

Tested on OSX version 10.13.3

Setup Homebrew and components

- Follow instructions on [brew.sh](http://brew.sh) to get Homebrew installed.

      /usr/bin/ruby -e "$(curl -fsSL \
          https://raw.githubusercontent.com/Homebrew/install/master/install)"

- Ensure Homebrew is at the beginning of your PATH:

      export PATH=/usr/local/bin:$PATH

- Add packages with the following:

      brew install cmake python

### Android Build

There are two options for building the Android tools. Either using the SPIRV
tools provided as part of the Android NDK, or using upstream sources. To build
with SPIRV tools from the NDK, remove the build-android/third_party directory
created by running update_external_sources_android.sh, (or avoid running
update_external_sources_android.sh). Use the following script to build
everything in the repository for Android, including validation layers, tests,
demos, and APK packaging: This script does retrieve and use the upstream SPRIV
tools.

    cd build-android
    ./build_all.sh

Test and application APKs can be installed on production devices with:

    ./install_all.sh [-s <serial number>]

Note that there are no equivalent scripts on Windows yet, that work needs to
be completed. The following per platform commands can be used for layer only
builds:

#### Linux and OSX

Follow the setup steps for Linux or OSX above, then from your terminal:

    cd build-android
    ./update_external_sources_android.sh --no-build
    ./android-generate.sh
    ndk-build -j4

#### Windows

Follow the setup steps for Windows above, then from Developer Command Prompt
for VS2013:

    cd build-android
    update_external_sources_android.bat
    android-generate.bat
    ndk-build

### Android Tests and Demos

After making any changes to the repository you should perform some quick
sanity tests, including the layer validation tests and the vkcube 
demo with validation enabled.

#### Run Layer Validation Tests

Use the following steps to build, install, and run the layer validation tests
for Android:

    cd build-android
    ./build_all.sh
    adb install -r bin/VulkanLayerValidationTests.apk
    adb shell am start com.example.VulkanLayerValidationTests/android.app.NativeActivity

Alternatively, you can use the test_APK script to install and run the layer
validation tests:

    test_APK.sh -s <serial number> -p <platform name> -f <gtest_filter>

#### Run vkcube with Validation

TODO: This must be reworked to pull in layers from the ValidationLayers repo

Use the following steps to build, install, and run vkcube for Android:

    cd build-android
    ./build_all.sh
    adb install -r ../demos/android/cube/bin/vkcube.apk
    adb shell am start com.example.Cube/android.app.NativeActivity

To build, install, and run Cube with validation layers,
first build layers using steps above, then run:

    cd build-android
    ./build_all.sh
    adb install -r ../demos/android/cube-with-layers/bin/cube-with-layers.apk

##### Run without validation enabled

    adb shell am start com.example.CubeWithLayers/android.app.NativeActivity

##### Run with validation enabled

    adb shell am start -a android.intent.action.MAIN -c android-intent.category.LAUNCH -n com.example.CubeWithLayers/android.app.NativeActivity --es args "--validate"

## Building on MacOS

### MacOS Build Requirements

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

Clone the Vulkan-Tools repository as defined above in the [Download the Repository](#download-the-repository)
section.

### Get the External Libraries

[MoltenVK](https://github.com/KhronosGroup/MoltenVK) Library

- Building the vkcube and vulkaninfo applications require linking to the
  MoltenVK Library (libMoltenVK.dylib)
  - The following option should be used on the cmake command line to specify a
    vulkan loader library: MOLTENVK_REPO_ROOT=/absolute_path_to/MoltenVK
    making sure to specify an absolute path, like so: cmake
    -DMOLTENVK_REPO_ROOT=/absolute_path_to/MoltenVK ....

Vulkan Loader Library

- Building the vkcube and vulkaninfo applications require linking to the Vulkan
  Loader Library (libvulkan.1.dylib)
  - The following option should be used on the cmake command line to specify a
    vulkan loader library:
    VULKAN_LOADER_INSTALL_DIR=/absolute_path_to/Vulkan-Loader_install_dir
    making sure to specify an absolute path.

### MacOS build

#### CMake Generators

This repository uses CMake to generate build or project files that are then
used to build the repository. The CMake generators explicitly supported in
this repository are:

- Unix Makefiles
- Xcode

#### Building with the Unix Makefiles Generator

This generator is the default generator, so all that is needed for a debug
build is:

        mkdir build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Debug \
              -DVULKAN_LOADER_INSTALL_DIR=/absolute_path_to/Vulkan-Loader_install_dir \
              -DMOLTENVK_REPO_ROOT=/absolute_path_to/MoltenVK \
              -DCMAKE_INSTALL_PREFIX=install ..
        make

To speed up the build on a multi-core machine, use the `-j` option for `make`
to specify the number of cores to use for the build. For example:

    make -j4

You can now run the demo applications from the command line:

    open cube/vkcube.app
    open cube/vkcubepp.app
    open vulkaninfo/vulkaninfo.app

Or you can locate them from `Finder` and launch them from there.

##### The Install Target and RPATH

The applications you just built are "bundled applications", but the
executables are using the `RPATH` mechanism to locate runtime dependencies
that are still in your build tree.

To see this, run this command from your `build` directory:

    otool -l cube/cube.app/Contents/MacOS/vkcube

and note that the `vkcube` executable contains loader commands:

- `LC_LOAD_DYLIB` to load `libvulkan.1.dylib` via an `@rpath`
- `LC_RPATH` that contains an absolute path to the build location of the Vulkan loader

This makes the bundled application "non-transportable", meaning that it won't
run unless the Vulkan loader is on that specific absolute path. This is useful
for debugging the loader or other components built in this repository, but not
if you want to move the application to another machine or remove your build
tree.

To address this problem, run:

    make install

This step copies the bundled applications to the location specified by
CMAKE_INSTALL_PREFIX and "cleans up" the `RPATH` to remove any external
references and performs other bundle fix-ups. After running `make install`,
run the `otool` command again from the `build/install` directory and note:

- `LC_LOAD_DYLIB` is now `@executable_path/../MacOS/libvulkan.1.dylib`
- `LC_RPATH` is no longer present

The "bundle fix-up" operation also puts a copy of the Vulkan loader into the
bundle, making the bundle completely self-contained and self-referencing.

##### The Non-bundled vulkaninfo Application

There is also a non-bundled version of the `vulkaninfo` application that you
can run from the command line:

    vulkaninfo/vulkaninfo

If you run this from the build directory, vulkaninfo's RPATH is already
set to point to the Vulkan loader in the build tree, so it has no trouble
finding it. But the loader will not find the MoltenVK driver and you'll see a
message about an incompatible driver. To remedy this:

    VK_ICD_FILENAMES=<path-to>/MoltenVK/Package/Latest/MoltenVK/macOS/MoltenVK_icd.json vulkaninfo/vulkaninfo

If you run `vulkaninfo` from the install directory, the `RPATH` in the
`vulkaninfo` application got removed and the OS needs extra help to locate
the Vulkan loader:

    DYLD_LIBRARY_PATH=<path-to>/Vulkan-Loader/loader VK_ICD_FILENAMES=<path-to>/MoltenVK/Package/Latest/MoltenVK/macOS/MoltenVK_icd.json vulkaninfo/vulkaninfo

#### Building with the Xcode Generator

To create and open an Xcode project:

        mkdir build-xcode
        cd build-xcode
        cmake -DVULKAN_LOADER_INSTALL_DIR=/absolute_path_to/Vulkan-Loader_install_dir -DMOLTENVK_REPO_ROOT=/absolute_path_to/MoltenVK -GXcode ..
        open VULKAN.xcodeproj

Within Xcode, you can select Debug or Release builds in the project's Build
Settings. You can also select individual schemes for working with specific
applications like `vkcube`.
