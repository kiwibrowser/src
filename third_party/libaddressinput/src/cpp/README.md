# Intro

The C++ version of libaddressinput library provides UI layout information and
validation for address input forms.

The library does not provide a UI. The user of the library must provide the user
interface that uses libaddressinput. The user of the library must also provide a
way to store data on disk and download data from the internet.

The first client of the library is Chrome web browser. This motivates not
providing UI or networking capabilities. Chrome will provide those.

When including the library in your project, you can override the dependencies
and include directories in libaddressinput.gypi to link with your own
third-party libraries.

# Dependencies

The library depends on these tools and libraries:

GYP: Generates the build files.
Ninja: Executes the build files.
GTest: Used for unit tests.
Python: Used by GRIT, which generates localization files.
RE2: Used for validating postal code format.

Most of these packages are available on Debian-like distributions. You can
install them with this command:

$ sudo apt-get install gyp ninja-build libgtest-dev python libre2-dev

Make sure that your version of GYP is at least 0.1~svn1395. Older versions of
GYP do not generate the Ninja build files correctly. You can download a
new-enough version from http://packages.ubuntu.com/saucy/gyp.

Make sure that your version of RE2 is at least 20140111+dfsg-1. Older versions
of RE2 don't support set_never_capture() and the packages don't provide shared
libraries.

If your distribution does not include the binary packages for the dependencies,
you can download them from these locations:

http://packages.ubuntu.com/saucy/gyp
http://packages.ubuntu.com/saucy/ninja-build
http://packages.ubuntu.com/saucy/libgtest-dev
http://packages.ubuntu.com/saucy/python
http://packages.ubuntu.com/utopic/libre2-1
http://packages.ubuntu.com/utopic/libre2-dev

Alternatively, you can download, build, and install these tools and libraries
from source code. Their home pages contain information on how to accomplish
that.

https://code.google.com/p/gyp/
http://martine.github.io/ninja/
https://code.google.com/p/googletest/
http://python.org/
https://code.google.com/p/re2/

# Build

Building the library involves generating an out/Default/build.ninja file and
running ninja:

$ export GYP_GENERATORS='ninja'
$ gyp --depth .
$ ninja -C out/Default

Overriding paths defined in the *.gyp files can be done by setting the
GYP_DEFINES environment variable before running gyp:

$ export GYP_DEFINES="gtest_dir='/xxx/include' gtest_src_dir='/xxx'"

# Test

This command will execute the unit tests for the library:

$ out/Default/unit_tests
