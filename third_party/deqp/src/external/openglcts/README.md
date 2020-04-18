OpenGL and OpenGL ES 2.0/3.X Conformance Test Instructions
=================

This document describes how to build, port, and run the OpenGL and OpenGL ES
2.0/3.X conformance tests, and how to verify and submit test results.

The Conformance Tests are built on dEQP framework. dEQP documentation is
available at http://source.android.com/devices/graphics/testing.html


Contents
------------------------
 - [Test History](#test-history)
 - [Introduction](#introduction)
 - [Test Environment Requirements](#test-environment-requirements)
 - [Configuring and Building the Tests](#configuring-and-building-the-tests)
    - [Configuration](#configuration)
    - [Building the Tests](#building-the-tests)
       - [Windows](#windows)
       - [Linux](#linux)
       - [Android](#android)
 - [Porting](#porting)
    - [Common Porting Changes](#common-porting-changes)
    - [Other Allowable Porting Changes](#other-allowable-porting-changes)
 - [Running the Tests](#running-the-tests)
    - [Conformance runs](#conformance-runs)
       - [Linux and Windows](#linux-and-windows)
       - [Android](#android-1)
    - [Running Subsets](#running-subsets)
       - [Command line options](#command-line-options)
    - [Understanding the Results](#understanding-the-results)
    - [Test Logs](#test-logs)
 - [Debugging Test Failures](#debugging-test-failures)
 - [Waivers](#waivers)
 - [Creating a Submission Package](#creating-a-submission-package)
 - [Submission Update Package](#submission-update-package)
 - [Passing Criteria](#passing-criteria)
 - [Troubleshooting](#troubleshooting)
    - [Crashes early on in the run](#crashes-early-on-in-the-run)
    - [Build fails](#build-fails)
 - [Adding new tests](#adding-new-tests)
 - [Acknowledgments](#acknowledgments)
 - [Revision History](#revision-history)

Test History
------------------------
The OpenGL and OpenGL ES Conformance Tests are expanded versions of the
OpenGL ES 2.x Conformance Test. Much of the development was done by Symbio, Inc.
under a contract with The Khronos Group. drawElements donated a considerable
number of new tests and a new execution framework for version 1.1.
The tests are built from the same source code base, although some individual
feature tests are specific to OpenGL or OpenGL ES and their specification
versions, and compilation options differing between OpenGL and OpenGL ES affect
how the tests are compiled and executed in some cases.

Introduction
------------------------

This document contains instructions for certifying conformance of implementations
of the OpenGL and OpenGL ES APIs. The steps of the process are as follows:

1. Configure the conformance tests and port them to your platform.
2. Build a test executable and run it against your implementation to produce
result logs.
3. Debug any test failures and modify your implementation as needed until it
passes the test.
4. Create a Submission Package containing your final result logs and other
documents describing the tested platform.
5. Submit the results to the appropriate Review Committee via the
Khronos Adopters web page. The Committee will examine your submission and will
notify you within thirty days if they find any issues requiring action on your part.

This document describes each of these steps in detail. It also provides advice
on reproducing, understanding, and debugging test failures, and discusses how
to extend or modify the tests and the test framework.

The reader is assumed to be a fluent programmer experienced with command line
utilities and build tools, such as CMake or Make.

Test Environment Requirements
------------------------

The conformance tests require a file system. The file system requires support
for long file names (i.e. > 8.3 name format). Source files in the conformance
tests use mixed case file names. When the `--verbose` option is used, rendered
images and test case shaders are copied to the log files. This can lead to quite
large log files, up to hundreds of megabytes on disk.

Each execution of the conformance test writes a text-format results log to a disk.
You will need to include this log as part of your conformance submission package.

The conformance test executable can be large. Compiler options and CPU instruction
sets can cause substantial variation. The disk space required for the build
including all the temporary files can be up to 400MB.

The build environment is expected to support C++ with exceptions and
the Standard Template Library (STL).

Configuring and Building the Tests
------------------------
The CTS is built via CMake build system. The requirements for the build are as follows:
- CMake 2.8 or newer, 2.8.8 or newer recommended
- C++ compiler with STL and exceptions support
- Unix: Make + GCC / Clang
- Windows: Visual Studio or Windows SDK (available free-of-charge)
- Android: Android SDK and NDK for host platform

The build is controlled by the file CMakeLists.txt found at the root of
the CTS source.

If the platform and compiler tools you use are not supported, you may be able to
add support for that platform and tools to the build system. If you do this,
please submit your changes back to Khronos for inclusion in the official tests
going forward.

Otherwise, if you choose not to use the supplied Makefiles, you must construct
an equivalent build system for the chosen development environment(s).

### Configuration

The build is configured by using `CMakeLists.txt` files in the build target
directory (`targets/`).  They specify platform-specific configuration, including
include paths and link libraries.

The main `CMakeLists.txt` includes the target file based on the `DEQP_TARGET`
variable. For example `-DDEQP_TARGET=my_target` will use the target description
file `targets/my_target/my_target.cmake`.

See the main `CMakeLists.txt` file for the description of the variables that
the target file can set.

Porting to a new platform includes either creating a new target file, or
modifying an existing target description.

**NOTE**: All paths, except `TCUTIL_PLATFORM_SRCS` are relative to root source
directory. `TCUTIL_PLATFORM_SRCS` is relative to `framework/platform` directory.

Following target files are provided with the package:

| Name | Description  |
|:---------|-----------------|
|android | Used in Android build. Requires use of suitable toolchain file (see `cmake/` directory) |
|default| Checks for presence of GL, ES2, ES3, and EGL libraries and headers in default search paths and configures build accordingly|
|null | Null build target |
|nullws | NullWS build target |
|x11_egl| X11 build for platforms with native EGL support|
|x11_glx| X11 build for platforms with native GLX support|
|x11_egl_glx| X11 build for platforms with native EGL/GLX support|

**Example target file (targets/null/null.cmake):**
```
message("*** Using null context target")

set(DEQP_TARGET_NAME "Null")

set(TCUTIL_PLATFORM_SRCS
	null/tcuNullPlatform.cpp
	null/tcuNullPlatform.hpp
	null/tcuNullRenderContext.cpp
	null/tcuNullRenderContext.hpp
	null/tcuNullContextFactory.cpp
	null/tcuNullContextFactory.hpp
	)
```

**Common configuration variables and their default values in CMake syntax:**

- Target name
```
set(DEQP_TARGET_NAME "UNKNOWN")
```

- List of link libraries per API.  If no libraries are specified, entry points
are loaded at run-time by default for OpenGL ES APIs. EGL always requires link
libraries.  OpenGL always uses run-time loading.
```
set(DEQP_GLES2_LIBRARIES   )
set(DEQP_GLES3_LIBRARIES   )
set(DEQP_GLES31_LIBRARIES  )
set(DEQP_GLES32_LIBRARIES  )
set(DEQP_EGL_LIBRARIES     )
set(DEQP_OPENGL_LIBRARIES  )
```

- Generic platform libraries required to link a working OpenGL (ES) Application
(e.g. X11 libraries on Unix/X11)
```
set(DEQP_PLATFORM_LIBRARIES )
```

- Libraries / binaries that need to be copied to the build target dir
```
set(DEQP_PLATFORM_COPY_LIBRARIES )
```

- If running on Linux using X11 for creating windows etc., enable this.
```
set(DEQP_USE_X11 OFF)
```

- Embed the test files in the test Before building with this set (if GTF module is present), run these commands:
```
cd external/kc-cts/src/GTF_ES/glsl/GTF
perl mergeTestFilesToCSource.pl
```

 In your target `.cmake` file add
```
set(DEQP_EMBED_TESTS ON)
add_definitions(-DHKEMBEDDEDFILESYSTEM)
```

### Building the Tests

To build the framework, you need first to download sources for zlib, libpng.

To download sources, run:

	python external/fetch_sources.py

For OpenGL CTS releases, and OpenGL ES CTS releases prior to opengl-es-cts-3.2.4.0
download Khronos Confidential Conformance Test Suite:

	python external/fetch_kc_cts.py

For OpenGL CTS releases, and OpenGL ES CTS releases prior to opengl-es-cts-3.2.4.0
the results for the tests included in this suite must be included in a
conformance submission.

**NOTE**: You need to be a Khronos Adopter and have an active account
at [Khronos Gitlab](https://gitlab.khronos.org/) to be able to download
Khronos Confidential CTS.
It is possible to run and build the CTS without the Khronos Confidential CTS.
For OpenGL CTS releases, and OpenGL ES CTS releases prior to opengl-es-cts-3.2.4.0
Khronos Confidential CTS is mandatory if you plan to make a
conformance submission (see [Creating a Submission Package](#creating-a-submission-package)).
For opengl-es-cts-3.2.4.0 and later OpenGL ES CTS releases Khronos Confidential CTS
results must not be included in a submission package.


With CMake out-of-source builds are always recommended. Create a build directory
of your choosing, and in that directory generate Makefiles or IDE project
using Cmake.

#### Windows

Requirements:
- Visual Studio (2010 or newer recommended) or Windows SDK
- CMake 2.8.x Windows native version (i.e. not Cygwin version)
- For GL/ES2/ES3.x tests: OpengGL, OpenGL ES 2 or ES 3.x libraries and headers

To choose the backend build system for CMake, choose one of the following Generator Names for the
command line examples in the next steps:
- VS2010: "Visual Studio 10"
- VS2012: "Visual Studio 11"
- NMake (must be run in VS or SDK command prompt): "NMake Makefiles"

Building GL, ES2, or ES3.x conformance tests:

	cmake <path to openglcts> -DDEQP_TARGET=default -G"<Generator Name>"
	cmake --build .

Khronos Confidential CTS doesn't support run-time selection of API context.
If you intend to run it you need to additionally supply `GLCTS_GTF_TARGET`
option to you cmake command, e.g.:

	cmake <path to openglcts> -DDEQP_TARGET=default -DGLCTS_GTF_TARGET=<target> -G"<Generator Name>"

Available `<target>`s are `gles2`, `gles3`, `gles31`, `gles32`, and `gl`.
The default `<target>` is `gles32`.

It's also possible to build `GL-CTS.sln` in Visual Studio instead of running
the `cmake --build .` command.

**NOTE**: Do not create the build directory under the source directory
(i.e anywhere under `<path to openglcts>`) on Windows, since it causes
random build failures when copying data files around.

**NOTE**: You can use the CMake for Windows GUI to do configuration and project
file generation.

**NOTE**: If using cygwin, you must install and ensure you use the Windows
version of cmake. The cygwin vesion does not contain the Visual Studio
generators. Here is a shell function you can put in your cygwin `.bash_profile`
to use it easily. With this you can simply type `wcmake` to run the Windows version.

```
function wcmake () {
    (TMP=$tmp TEMP=$temp; unset tmp; unset temp; "C:/Program Files (x86)/CMake 2.8/bin/cmake" "$@")
}
```

#### Linux

Required tools:
- Standard build utilities (make, gcc, etc.)
- CMake 2.8.x
- Necessary API libraries (OpenGL, GLES, EGL depending on configuration)

Building ES2 or ES3.x conformance tests:

	cmake <path to openglcts> -DDEQP_TARGET=null -DGLCTS_GTF_TARGET=gles32
	cmake --build .

Building OpenGL conformance tests:

	cmake <path to openglcts> -DDEQP_TARGET=null -DGLCTS_GTF_TARGET=gl
	cmake --build .

Khronos Confidential CTS doesn't support run-time selection of API context.
If you intend to run it then the `GLCTS_GTF_TARGET` option is necessary.

Available values for `GLCTS_GTF_TARGET` are `gles2`, `gles3`, `gles31`, `gles32`, and `gl`.
The default value is `gles32`.

CMake chooses to generate Makefiles by default. Other generators can be used
as well. See CMake help for more details.

#### Android

The conformance tests come with native Android support. The following packages
are needed in order to build an Android binary:
- Python 2.7.x
- Android NDK r11c
- Android SDK with API 24 packages and tools installed
- Apache Ant

An Android binary (for ES 3.2) can be built using command:

	python scripts/android/build_apk.py --target=openglcts --sdk <path to Android SDK> --ndk <path to Android NDK>

If Khronos Confidential CTS is present then the script will set `GLCTS_GTF_TARGET`
to `gles32` by default.
It is possible to specify a different `GLCTS_GTF_TARGET` target by invoking the script
with the `--kc-cts-target` option, e.g.:

	python scripts/android/build_apk.py --target=openglcts --kc-cts-target=gles31 --sdk <path to Android SDK> --ndk <path to Android NDK>

Available values for `--kc-cts-target` are `gles32`, `gles31`, `gles3`, `gles2` and `gl`.

The package can be installed by either running:

	python scripts/android/install_apk.py --target=openglcts

By default the CTS package will contain libdeqp.so built for `armeabi-v7a`, `arm64-v8a`,
`x86`, and `x86_64` ABIs, but that can be changed with `--abis` command line option.

To pick which ABI to use at install time, following commands must be used
instead:

	adb install --abi <ABI name> <build root>/Khronos-CTS.apk /data/local/tmp/Khronos-CTS.apk

Porting
------------------------
The Conformance Tests have been designed to be relatively platform-, OS-, and
compiler-independent. Adopters are responsible for final changes needed to allow
the Test to run on the platform they wish to
certify as conformant.

### Common Porting Changes

Porting the dEQP framework requires implementation of either `glu::Platform` or,
on platforms supporting EGL, the `tcu::EglPlatform` interface. The porting layer
API is described in detail in following files:

	framework/common/tcuPlatform.hpp
	framework/opengl/gluPlatform.hpp
	framework/egl/egluPlatform.hpp
	framework/platform/tcuMain.cpp

This version of the dEQP framework includes ports for Windows (both EGL and WGL),
X11 (EGL and XGL), and Android.

Base portability libraries in `framework/delibs` seldom need changes. However,
introducing support for a new compiler or a new processor family may require
some changes to correctly detect and parameterize the environment.

Porting typically involves three types of changes:
1. Changes to the make system used to generate the test executable.
2. Changes needed to adapt the test executable to the operating system used on the platform.
3. Changes to the platform specific GL and EGL header files.

Changes should normally be confined to build files (CMake or Python) or source
files (.c, .h, .cpp, and .h files) in the following directories or their
subdirectories:
- `framework/platform`
- `targets`

If you find that you must change other source (.c, .cpp, .h, or .hpp) files,
you will need to file a waiver as described below.

Note that the conformance tests assume that the implementation supports EGL.
However EGL is not required for OpenGL or OpenGL ES conformance.

Most of the tests require at least 256x256 pixels resolution in order to run properly
and produce stable results. It is, therefore, important to ensure that a port to a
new platform can support surfaces that fulfill width and height requirements.

### Other Allowable Porting Changes

Other than changes needed for porting, the only changes that are permitted are
changes to fix bugs in the conformance test. A bug in the conformance test is
a behavior which causes clearly incorrect execution (e.g., hanging, crashing,
or memory corruption), OR which requires behavior which contradicts or exceeds
the requirements of the relevant OpenGL or OpenGL ES Specification. Changes
required to address either of these issues typically require [waivers](#waivers).

Running the Tests
------------------------
All the following commands need to be run in the CTS build directory. If you
need to move the binaries from the build directory, remember to copy the
data directories named `gl_cts`, `gles2`, `gles3`, and `gles31` and its subdirectories
from the build directory to the test target in the same relative locations.

If the build instructions have been followed as-is, the correct path is:

	cd <builddir>/external/openglcts/modules

### Conformance runs
A conformance run can be launched either by running the `cts-runner` binary with
appropriate options on Linux/Windows or by running an Android application.

### Linux and Windows
Conformance run for OpenGL ES 3.2 on Windows:

	Debug/cts-runner.exe --type=es32
	  [For ES 3.1 use --type=es31; ES 3.0 use --type=es3; for ES 2.0, use --type=es2]

Conformance run for OpenGL 3.0 - 4.6 on Windows:

	Debug/cts-runner.exe --type=glxy
	  [x and y are the major and minor specifiction versions]

Full list of parameters for the `cts-runner` binary:
```
--type=[esN[M]|glNM] Conformance test run type. Choose from
					 ES: es2, es3, es31, es32
					 GL: gl30, gl31, gl32, gl33, gl40, gl41, gl42, gl43, gl44, gl45, gl46
--logdir=[path]      Destination directory for log files
--summary            Print summary without running the tests
--verbose            Print out and log more information
```

The conformance run will create one or more `.qpa` files per tested config, a
summary `.qpa` file containing run results and a summary `.xml` file containing
command line options for each run, all of which should be included in your
conformance submission package. The final verdict will be printed out at
the end of run.

Sometimes it is useful to know the command line options used for the conformance
before the run completed. Full conformance run configuration is written
to `cts-run-summary.xml` and this file can be generated by adding `--summary`
parameter.

By default the `cts-runner` does not include result images or shaders used in
the logs. Adding parameter `--verbose` will cause them to be included in
the logs. Images will be embedded as PNG data into the`.qpa` log files.
See Section [Test Logs](#test-logs) for instructions on how to view the images.

To direct logs to a directory, add `--logdir=[path]` parameter.

**NOTE**: Due to the lack of support for run-time selection of API context in the
Khronos Confidential CTS, a conformance run may fail if it is executed for an API
version that doesn't match the `GLCTS_GTF_TARGET` value used during the build step.

#### Android

Once the CTS binary is built and installed on the device, a new application
called `ES3.2 CTS`, `ES3.1 CTS`, `ES3 CTS`, `ES2 CTS`, `GL4.5 CTS`, or `GL4.6 CTS`
(depending on the test version you built) should appear in the launcher.
Conformance test runs can be done by launching the applications.

Alternatively it is possible to start a conformance run from the command line,
for example to launch a GLES 3.2 conformance run use:

	am start -n org.khronos.gl_cts/org.khronos.cts.ES32Activity -e logdir "/sdcard/logs"

For GLES 2.0, GLES 3.0, GLES 3.1, GL 4.5, or GL 4.6 conformance runs, substitute
the following activity name (respectively) ES2Activity, ES3Activity, ES31Activity,
GL45Activity, or GL46Activity.

Test logs will be written to `/sdcard` by default. The log path can be
customized by supplying a `logdir` string extra in launch intent. Verbose mode
can be enabled by supplying a `verbose` = `"true"` string extra. See
the following example:

	am start -n org.khronos.gl_cts/org.khronos.cts.ES32Activity -e logdir "/sdcard/logs" -e verbose "true"

Conformance run configuration can be generated by supplying a `summary` = `"true"`
string extra. See the following example:

	am start -n org.khronos.gl_cts/org.khronos.cts.ES32Activity -e logdir "/sdcard/logs" -e summary "true"

**NOTE**: Supplying a `summary` = `"true"` string extra will result in the `cts-run-summary.xml` file
being written out but no tests will be executed.

Individual tests can be launched as well by targeting
`org.khronos.gl_cts/android.app.NativeActivity` activity. Command line
arguments must be supplied in a `cmdLine` string extra. See following example:

	am start -n org.khronos.gl_cts/android.app.NativeActivity -e cmdLine "cts --deqp-case=KHR-GLES32.info.version --deqp-gl-config-id=1 --deqp-log-filename=/sdcard/ES32-egl-config-1.qpa --deqp-surface-width=128 --deqp-surface-height=128"

In addition to the detailed `*.qpa` output files, the Android port of the CTS
logs a summary of the test run, including the pass/fail status of each test.
This summary can be viewed using the Android *logcat* utility.

See Section [Running Subsets](#running-subsets) above for details on command
line parameters.

### Running Subsets

Run shader compiler loop test cases from the OpenGL ES 3.0 CTS using EGL config with ID 3:

	Debug/glcts.exe --deqp-case=KHR-GLES3.shaders.loops.* --deqp-gl-config-id=3

Note that the GL context version is determined by the case name. `KHR-GLES3` in
the example above selects OpenGL ES 3.0. The command to run the same test
against OpenGL version 4.1 is:

	Debug/glcts.exe --deqp-case=GL41-CTS.shaders.loops.* --deqp-gl-config-id=3

To list available test cases (writes out `*-cases.txt` files per module), run:

	Debug/glcts.exe --deqp-runmode=txt-caselist

The type of the run for cts-runner chooses a specific list of test cases to
be run. The selected tests can be checked from the summary logs. To run
the same tests, just give equivalent test selection parameters to the `glcts`.

#### Command line options

Full list of parameters for the `glcts` binary:
```
  -h, --help
    Show this help

  -n, --deqp-case=<value>
    Test case(s) to run, supports wildcards (e.g. dEQP-GLES2.info.*)

  --deqp-caselist=<value>
    Case list to run in trie format (e.g. {dEQP-GLES2{info{version,renderer}}})

  --deqp-caselist-file=<value>
    Read case list (in trie format) from given file

  --deqp-stdin-caselist
    Read case list (in trie format) from stdin

  --deqp-log-filename=<value>
    Write test results to given file
    default: 'TestResults.qpa'

  --deqp-runmode=[execute|xml-caselist|txt-caselist|stdout-caselist]
    Execute tests, or write list of test cases into a file
    default: 'execute'

  --deqp-caselist-export-file=<value>
    Set the target file name pattern for caselist export
    default: '${packageName}-cases.${typeExtension}'

  --deqp-watchdog=[enable|disable]
    Enable test watchdog
    default: 'disable'

  --deqp-crashhandler=[enable|disable]
    Enable crash handling
    default: 'disable'

  --deqp-base-seed=<value>
    Base seed for test cases that use randomization
    default: '0'

  --deqp-test-iteration-count=<value>
    Iteration count for cases that support variable number of iterations
    default: '0'

  --deqp-visibility=[windowed|fullscreen|hidden]
    Default test window visibility
    default: 'windowed'

  --deqp-surface-width=<value>
    Use given surface width if possible
    default: '-1'

  --deqp-surface-height=<value>
    Use given surface height if possible
    default: '-1'

  --deqp-surface-type=[window|pixmap|pbuffer|fbo]
    Use given surface type
    default: 'window'

  --deqp-screen-rotation=[unspecified|0|90|180|270]
    Screen rotation for platforms that support it
    default: '0'

  --deqp-gl-context-type=<value>
    OpenGL context type for platforms that support multiple

  --deqp-gl-config-id=<value>
    OpenGL (ES) render config ID (EGL config id on EGL platforms)
    default: '-1'

  --deqp-gl-config-name=<value>
    Symbolic OpenGL (ES) render config name

  --deqp-gl-context-flags=<value>
    OpenGL context flags (comma-separated, supports debug and robust)

  --deqp-cl-platform-id=<value>
    Execute tests on given OpenCL platform (IDs start from 1)
    default: '1'

  --deqp-cl-device-ids=<value>
    Execute tests on given CL devices (comma-separated, IDs start from 1)
    default: ''

  --deqp-cl-build-options=<value>
    Extra build options for OpenCL compiler

  --deqp-egl-display-type=<value>
    EGL native display type

  --deqp-egl-window-type=<value>
    EGL native window type

  --deqp-egl-pixmap-type=<value>
    EGL native pixmap type

  --deqp-log-images=[enable|disable]
    Enable or disable logging of result images
    default: 'enable'

  --deqp-log-shaders=[enable|disable]
    Enable or disable logging of shaders
    default: 'enable'

  --deqp-test-oom=[enable|disable]
    Run tests that exhaust memory on purpose
    default: 'disable'

  --deqp-egl-config-id=<value>
    Legacy name for --deqp-gl-config-id
    default: '-1'

  --deqp-egl-config-name=<value>
    Legacy name for --deqp-gl-config-name
```

### Understanding the Results

At the end of a completed test run, a file called `cts-run-summary.xml` is
generated. It will contain summaries per configuration and the full command
lines for the `glcts` application
(See Section [Running Subsets](#running-subsets)) for debugging purposes.
Additionally, a summary string similar to one below is printed:
```
4/4 sessions passed, conformance test PASSED
```

If the run fails, the message will say `FAILED` instead of `PASSED`. Under
Linux or Windows, this string is printed to stdout if available. Under Android,
it is emitted to the Android logging system for access via *logcat*.

Each test case will be logged into the `.qpa` files in XML. Below is a minimal
example of a test case log. The Result element contains the final verdict in
the `StatusCode` attribute. Passing cases will have `Pass` and failing cases
`Fail`. Other results such as `QualityWarning`, `CompatibilityWarning`,
`NotSupported` or `ResourceError` are possible. Only `Fail` status will count
as failure for conformance purposes.
```
<TestCaseResult Version="0.3.2" CasePath="ES2-CTS.info.vendor" CaseType="SelfValidate">
    <Text>Vendor A</Text>
    <Result StatusCode="Pass">Pass</Result>
</TestCaseResult>
```

If the failure count is zero for all config sequences, the implementation
passes the test. Note that in addition to a successful test result,
a Submission Package must satisfy the conditions specified below under
[Passing Criteria](#passing-criteria) in order to achieve conformance certification.

### Test Logs

The CTS writes test logs in XML encapsulated in a simple plain-text container
format. Each tested configuration listed in `cts-run-summary.xml`

To analyse and process the log files, run the following scripts
- `external/openglcts/scripts/verify_submission.py`: Script that verifies logs based on `cts-run-summary.xml` file.
- `scripts/log/log_to_csv.py`: This utility converts `.qpa` log into CSV format. This is
useful for importing results into other systems.
- `scripts/log/log_to_xml.py`: Converts `.qpa` into well-formed XML document. The document
can be then viewed in browser using the testlog.{xsl,css} files.

Some browsers, like Chrome, limit local file access. In such case, the files
must be accessed over HTTP. Python comes with a simple HTTP server suitable
for the purpose. Run `python -m SimpleHTTPServer` in the directory containing
the generated XML files and point the browser to `127.0.0.1:8000`.

Parser for the `.qpa` log file format in python is provided in
`scripts/log/log_parser.py`.

Python scripts require python 2.7 or newer in 2.x series. They are not
compatible with python 3.x.

Debugging Test Failures
------------------------
The best first step is to run the failing test cases via `glcts` executable to
get the more verbose logs. Use, for example, the `log_to_xml.py` script
detailed in Section [Test Logs](#test-logs), to view the generated logs.
If the visual inspection of the logs does not give sufficient hints on the
nature of the issue, inspecting the test code and stepping through it in
debugger should help.

Waivers
------------------------
The procedure for requesting a waiver is to report the issue by filing a bug
report in the Gitlab VK GL CTS project
(https://gitlab.khronos.org/Tracker/vk-gl-cts). When you create your submission
package, include references to the waivers as described in the adopters' agreement.
[Fully-qualified links](https://en.wikipedia.org/wiki/Fully_qualified_domain_name)
to bug reports are highly recommended.
Including as much information as possible in your bug report will ensure the issue
can be progressed as speedily as possible. Such bug report must
include a link to suggested file changes. Issues must be labeled `Waiver` and `OpenGL-ES`
(for OpenGL ES submissions) or `Waiver` and `OpenGL` (for OpenGL submissions) and
identify the CTS release tag and affected tests.

Creating a Submission Package
------------------------
Please see the [Creating a Submission Package page](https://github.com/KhronosGroup/VK-GL-CTS/wiki/Creating-a-OpenGL-and-OpenGL-ES-Submission-Package).

Submission Update Package
------------------------
Please see the [Submission Update Package page](https://github.com/KhronosGroup/VK-GL-CTS/wiki/Submission-Update-Package).

Passing Criteria
------------------------
Please see the [Conformance Submission Passing Criteria page](https://github.com/KhronosGroup/VK-GL-CTS/wiki/OpenGL-and-OpenGL-ES-Conformance-Submission-Passing-Criteria).

Troubleshooting
------------------------
### Crashes early on in the run
If using run-time entry point loading, it is possible that not all required
entry points are available. This will result in `NULL` pointer dereferencing.

### Build fails
First try re-running the build. If that does not help and you have used the
same build directory with different version of the CTS, remove the build
directory and run the CMake again.

Adding new tests
------------------------

See the [Contribution Guide](CONTRIBUTING.md)

Acknowledgments
------------------------
The Khronos Group gratefully acknowledges the support of drawElements Oy,
who donated a large number of GLSL tests and a new test framework and build system.

The Khronos Group also gratefully acknowledges the support of 3DLabs Inc.,
who gave permission to use the 3DLabs Graphics Test Framework (GTF).

The first internal version of the test was created by Bruno Schwander of
Hooked Wireless, under a development contract with the Khronos Group.

Symbio added tests specific to OpenGL and OpenGL ES 3.0.

drawElements added their donated language tests and build system.

The CTS results from these efforts, together with additional hard work by
volunteers from the OpenGL ES Working Group, the OpenGL ARB Working Group,
and their member companies, including:

- Sumit Agarwal, Imagination Technologies
- Eric Anholt, Intel
- Oleksiy Avramchenko, Sony
- Anthony Berent, ARM
- Joseph Blankenship, AMD
- Jeff Bolz, NVIDIA
- Pierre Boudier, AMD
- Benji Bowman, Imagination Technologies
- Pat Brown, NVIDIA
- David Cairns, Apple
- Mark Callow, ArtSpark
- Antoine Chauveau, NVIDIA
- Aske Simon Christensen, ARM
- Lin Chen, Qualcomm
- Mathieu Comeau, QNX
- Graham Connor, Imagination Technologies
- Slawomir Cygan, Intel
- Piotr Czubak, Intel
- Piers Daniell, NVIDIA
- Matthias Dejaegher, ZiiLabs
- Chris Dodd, NVIDIA
- David Donohoe, Movidius
- Alex Eddy, Apple
- Sean Ellis, ARM
- Bryan Eyler, NVIDIA
- Erik Faye-Lund, ARM
- Nicholas FitzRoy-Dale, Broadcom
- Michael Frydrych, NVIDIA
- Toshiki Fujimori, Takumi
- David Garcia, Qualcomm
- Frido Garritsen, Vivante
- Klaus Gerlicher, NVIDIA
- Slawomir Grajewski, Intel
- Jonas Gustavsson, Sony
- Nick Haemel, NVIDIA
- Matthew Harrison, Imagination Technologies
- Pyry Haulos, drawElements
- Jim Hauxwell, Broadcom
- Valtteri Heikkil, Symbio
- Tsachi Herman, AMD
- Mathias Heyer, NVIDIA
- Atsuko Hirose, Fujitsu
- Ari Hirvonen, NVIDIA
- Rune Holm, ARM
- Jaakko Huovinen, Nokia
- James Jones, Imagination Technologies
- Norbert Juffa, NVIDIA
- Jordan Justen, Intel
- Sandeep Kakarlapudi, ARM
- Anssi Kalliolahti, NVIDIA
- Philip Kamenarsky, NVIDIA
- Krzysztof Kaminski, Intel
- Daniel Kartch, NVIDIA
- Maxim Kazakov, DMP
- Jon Kennedy, 3DLabs
- John Kessenich
- Daniel Koch, NVIDIA
- Benjamin Kohler-Crowe, NVIDIA
- Georg Kolling, Imagination Technologies
- Misa Komuro, DMP
- Boguslaw Kowalik, Intel
- Aleksandra Krstic, Qualcomm
- Karol Kurach, NVIDIA
- VP Kutti
- Sami Kyostila, Google
- Teemu Laakso, Symbio
- Antoine Labour, Sony
- Alexandre Laurent, Imagination Technologies
- Jon Leech, Khronos
- Graeme Leese, Broadcom
- I-Gene Leong, Intel
- Radoslava Leseva, Imagination Technologies
- Jake Lever, NVIDIA
- Fred Liao, MediaTek
- Bill Licea-Kane, Qualcomm
- Benj Lipchak, Apple
- Wayne Lister, Imagination Technologies
- Isaac Liu, NVIDIA
- Weiwan Liu, NVIDIA
- Zhifang Long, Marvell
- Toni L&#246;nnberg, AMD
- Erik Lovlie
- Christer Lunde, ARM
- Zong-Hong Lyu, DMP
- Daniel Mahashin, NVIDIA
- Rob Matthesen, NVIDIA
- Tom McReynolds, NVIDIA (CTS TSG Chair, ES 1.1)
- Bruce Merry, ARM
- Assif Mirza, Imagination Technologies
- Zhenyao Mo, Google
- Kazuhiro Mochizuki, Fujitsu
- Affie Munshi, Apple
- Yeshwant Muthusamy, Samsung
- Mirela Nicolescu, Broadcom
- Glenn Nissen, Broadcom
- Michael O'Hara, AMD
- Eisaku Ohbuchi, DMP
- Tom Olson, ARM
- Tapani Palli, Intel
- Brian Paul, VMWare
- Remi Pedersen, ARM
- Adrian Peirson, ARM
- Russell Pflughaupt, NVIDIA
- Anuj Phogat, Intel
- Tero Pihlajakoski, Nokia
- Peter Pipkorn, NVIDIA
- Acorn Pooley, NVIDIA
- Guillaume Portier, ArtSpark
- Greg Prisament, Lychee Software
- Jonathan Putsman, Imagination Technologies
- Mike Quinlan, AMD
- Tarik Rahman, CodePlay
- Kalle Raita, drawElements
- Daniel Rakos, AMD
- Manjunatha Ramachandra
- John Recker, NVIDIA
- Maurice Ribble, Qualcomm (CTS TSG Chair, ES 2.0)
- James Riordon, Khronos
- Lane Roberts, Samsung
- Ian Romanick, Intel
- Greg Roth, NVIDIA
- Kenneth Russell, Google
- Matteo Salardi, Imagination Technologies
- Jeremy Sandmel, Apple
- Shusaku Sawato, DMP
- Chris Scholtes, Fujitsu
- Mathias Schott, NVIDIA
- Bruno Schwander, Hooked Wireless
- Graham Sellers, AMD
- Shereef Shehata, Texas Instruments
- Benjamin Shen, Vivante
- Robert Simpson, Qualcomm
- Stuart Smith, Imagination Technologies
- Janusz Sobczak, Mobica
- Jacob Strom, Ericsson
- Timo Suoranta, Broadcom
- Jan Svarovsky, Ideaworks3D
- Anthony Tai, Apple
- Payal Talati, Imagination Technologies
- Gregg Tavares, Google
- Ross Thompson, NVIDIA
- Jeremy Thorne, Broadcom
- Jani Tikkanen, Symbio
- Antti Tirronen, Qualcomm (CTS TSG Chair, ES 3.0/3.1)
- Robert Tray, NVIDIA
- Matt Turner, Intel
- Eben Upton, Broadcom
- Jani Vaarala, Nokia
- Dmitriy Vasilev, NVIDIA
- Chad Versace, Intel
- Holger Waechtler, Broadcom
- Joerg Wagner, ARM
- Jun Wang, Imagination Technologies
- Yuan Wang, Imagination Technologies
- Hans-Martin Will
- Ewa Wisniewska, Mobica
- Dominik Witczak, Mobica
- Oliver Wohlmuth, Fujitsu
- Yanjun Zhang, Vivante
- Lefan Zhong, Vivante
- Jill Zhou
- Marek Zylak, NVIDIA
- Iliyan Dinev, Imagination Technologies
- James Glanville, Imagination Technologies
- Mark Adams, NVIDIA
- Alexander Galazin, ARM
- Riccardo Capra, ARM
- Lars-Ivar Simonsen, ARM
- Fei Yang, ARM

Revision History
------------------------
- 0.0 - Tom Olson

  Initial version cloned from `ES2_Readme`, plus feedback from Mark Callow.

- 0.2 - Tom Olson

  Modified to incorporate feedback in bug 8534.

- 0.3 - Jon Leech

  Added details for OpenGL Conformance.

- 0.4 - Jon Leech 2012/10/31

  Add configuration & build section, and table of contents

- 0.5 - Jon Leech 2012/10/31

  Fix typos noted by Mark Callow in bug 8534.

- 0.6 - Jon Leech 2012/11/13

  Discuss automatic version selection and document support for OpenGL 3.3-4.3.

- 0.7 - Jon Leech 2012/11/14

  Minor cleanup for GL version numbers per Bug 8534 comment #41.

- 0.8 - Tom Olson 2013/1/25

  Updated GL status in preparation for ES 3.0 release, removed display
  parameters from product description, and removed mention of sample submission.

- 0.9 - Jon Leech 2013/07/17

  Restore GL-specific details in preparation for initial GL CTS release.

- 1.0 - Jon Leech 2013/07/17

  Change references to Visual Studio 11 to Visual Studio 2012 per bug 9862.
  Reset change tracking to reduce clutter.

- 1.1 - Kalle Raita 2013/10/30

  Updated documentation after the integration of the drawElements framework and
  language tests.

- 1.2 - Kalle Raita 2013/12/03

  Removed TODOs, added some notes on further development, and notes on file
  dependencies. Exact list of directory sub-trees that can be modified during porting.

- 1.3 - Tom Olson 2014/05/27

  Updates for ES CTS 3.1.1.0 . Added Passing Criteria, updated examples to
  include 3.1 versioning, and updated Acknowledgements.

- 1.4 - Alexander Galazin 2016/05/12

  Updates for ES CTS 3.2.1.0.

- 2.0 - Alexander Galazin 2016/09/23

  Moved the contents to README.md.
  Updated to reflect new CTS structure and build instructions.

- 2.1 - Alexander Galazin 2016/12/15

  Updates in preparation for the new release.
  Document restructuring, more detailed process of creating a submission package.
  Incorporated OpenGL/CTS issue 39 and 40 in the Passing Criteria.
