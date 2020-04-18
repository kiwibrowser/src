# PDFium

## Prerequisites

Get the chromium depot tools via the instructions at
http://www.chromium.org/developers/how-tos/install-depot-tools (this provides
the gclient utility needed below).

Also install Python, Subversion, and Git and make sure they're in your path.


### Windows development

PDFium uses the same build tool as Chromium:

#### Open source contributors
Please refer to [Chromium's Visual Studio set up](https://chromium.googlesource.com/chromium/src/+/master/docs/windows_build_instructions.md#visual-studio)
for requirements and instructions on build environment configuration.

Run `set DEPOT_TOOLS_WIN_TOOLCHAIN=0`, or set that variable in your global
environment.

Compilation is done through ninja, **not** Visual Studio.

### CPU Architectures supported

The default architecture for Windows, Linux, and Mac is "`x64`". On Windows,
"`x86`" is also supported. GN parameter "`target_cpu = "x86"`" can be used to
override the default value. If you specify Android build, the default CPU
architecture will be "`arm`".

It is expected that there are still some places lurking in the code which will
not function properly on big-endian architectures. Bugs and/or patches are
welcome, however providing this support is **not** a priority at this time.

#### Google employees

Run: `download_from_google_storage --config` and follow the
authentication instructions. **Note that you must authenticate with your
@google.com credentials**. Enter "0" if asked for a project-id.

Once you've done this, the toolchain will be installed automatically for
you in the [Generate the build files](#GenBuild) step below.

The toolchain will be in `depot_tools\win_toolchain\vs_files\<hash>`, and windbg
can be found in `depot_tools\win_toolchain\vs_files\<hash>\win_sdk\Debuggers`.

If you want the IDE for debugging and editing, you will need to install
it separately, but this is optional and not needed for building PDFium.

## Get the code

The name of the top-level directory does not matter. In our examples, we use
"repo". This directory must not have been used before by `gclient config` as
each directory can only house a single gclient configuration.

```
mkdir repo
cd repo
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git
gclient sync
cd pdfium
```

Additional build dependencies need to be installed by running:

```
./build/install-build-deps.sh
```

## Generate the build files

We use GN to generate the build files and
[Ninja](http://martine.github.io/ninja/)
to execute the build files.  Both of these are included with the
depot\_tools checkout.

### Selecting build configuration

PDFium may be built either with or without JavaScript support, and with
or without XFA forms support.  Both of these features are enabled by
default. Also note that the XFA feature requires JavaScript.

Configuration is done by executing `gn args <directory>` to configure the build.
This will launch an editor in which you can set the following arguments.
A typical `<directory>` name is `out/Debug`.

```
use_goma = true  # Googlers only. Make sure goma is installed and running first.
is_debug = true  # Enable debugging features.

pdf_use_skia = false  # Set true to enable experimental skia backend.
pdf_use_skia_paths = false  # Set true to enable experimental skia backend (paths only).

pdf_enable_xfa = true  # Set false to remove XFA support (implies JS support).
pdf_enable_v8 = true  # Set false to remove Javascript support.
pdf_is_standalone = true  # Set for a non-embedded build.
is_component_build = false # Disable component build (must be false)

clang_use_chrome_plugins = false  # Currently must be false.
```

Note, you must set `pdf_is_standalone = true` if you want the sample
applications like `pdfium_test` to build.

When complete the arguments will be stored in `<directory>/args.gn`, and
GN will automatically use the new arguments to generate build files.
Should your files fail to generate, please double-check that you have set
use\_sysroot as indicated above.

## Building the code

You can build the sample program by running: `ninja -C <directory> pdfium_test`
You can build the entire product (which includes a few unit tests) by running:
`ninja -C <directory> pdfium_all`.

## Running the sample program

The pdfium\_test program supports reading, parsing, and rasterizing the pages of
a .pdf file to .ppm or .png output image files (windows supports two other
formats). For example: `<directory>/pdfium_test --ppm path/to/myfile.pdf`. Note
that this will write output images to `path/to/myfile.pdf.<n>.ppm`.

## Testing

There are currently several test suites that can be run:

 * pdfium\_unittests
 * pdfium\_embeddertests
 * testing/tools/run\_corpus\_tests.py
 * testing/tools/run\_javascript\_tests.py
 * testing/tools/run\_pixel\_tests.py

It is possible the tests in the `testing` directory can fail due to font
differences on the various platforms. These tests are reliable on the bots. If
you see failures, it can be a good idea to run the tests on the tip-of-tree
checkout to see if the same failures appear.

## Code Coverage

Code coverage reports for PDFium can be generated in Linux development
environments. Details can be found [here](/docs/code-coverage.md).

## Profiling

Valgrind and other profiling tools do not work correctly with the standard build
setup that PDFium uses. You will need to add
`ro_segment_workaround_for_valgrind=true` to `args.gn` to get symbols to
correctly appear.

## Waterfall

The current health of the source tree can be found at
http://build.chromium.org/p/client.pdfium/console

## Community

There are several mailing lists that are setup:

 * [PDFium](https://groups.google.com/forum/#!forum/pdfium)
 * [PDFium Reviews](https://groups.google.com/forum/#!forum/pdfium-reviews)
 * [PDFium Bugs](https://groups.google.com/forum/#!forum/pdfium-bugs)

Note, the Reviews and Bugs lists are typically read-only.

## Bugs

 We use this
[bug tracker](https://code.google.com/p/pdfium/issues/list), but for security
bugs, please use [Chromium's security bug template]
(https://code.google.com/p/chromium/issues/entry?template=Security%20Bug)
and add the "Cr-Internals-Plugins-PDF" label.

## Contributing code

For contributing code, we will follow
[Chromium's process](http://dev.chromium.org/developers/contributing-code)
as much as possible. The main exceptions are:

1. Code has to conform to the existing style and not Chromium/Google style.
2. PDFium uses a different tool for code reviews, and credentials for
the tool need to be generated before uploading a CL.
3. PDFium is currently holding at C++11 compatibility, rejecting features that
are only present in C++14 (onto which Chromium is now slowly moving).
