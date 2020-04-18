# Code Coverage Support for PDFium

[TOC]

This guide explains how to generate code coverage information for the PDFium
library on a local computer.

## Prerequisites

You will need the PDFium source code on your computer. You can see
the [README](/README.md) for instructions on checking out PDFium's source.

The tools used for code coverage are known to work on Ubuntu 14.04. They should
work correctly on newer versions of Ubuntu and related Linux distros. They have
not been tested on Windows and Mac.

### lcov

The code coverage scripts depend on having a version of `lcov` of 1.11 or
greater available, which is enforced by the script. Unfortunately the default
version of `lcov` for Ubuntu 14.04 is 1.10, thus you will need to install a
newer version.

You can build a newer version of `lcov` from source, which is
available [here](http://ltp.sourceforge.net/coverage/lcov.php).

If you don't want to build from source and use an RPM based Linux, not
Ubuntu/Debian, then there are pre-built RPMs
available [here](http://downloads.sourceforge.net/ltp/lcov-1.13-1.noarch.rpm).

For Ubuntu/Debian users these RPMs can be converted to .deb using `alien`. More
information about how to do this can be found in `man alien`.

### llvm-cov

The other external dependency for generating code coverage information is having
a version of `llvm-cov` that supports the `gcov` command. This should be all
versions of 3.5.0 or greater.

Again, unfortunately, the default llvm-cov that comes with Ubuntu 14.04, 3.4, is
lower then what is needed. The 14.04 repositories do support having multiple
versions of the `llvm` package, and thus `llvm-cov`. Through your favourite
package manager you should be able to install any version of `llvm` of 3.5 or
greater and the coverage scripts should find it.

## Generating Code Coverage

### Setup

This step assumes that you have already checked out the PDFium source code and
installed the proper versions of the external tools. If you have not, please
consult the above Prerequisites section.

Before generating code coverage information, you will need to have a build
directory with coverage enabled. This can be done by running the `gn args`
command and adding `use_coverage = true` in the editor that is opened. If not
using the default directory, `out/Coverage`, then replace it with the correct
location in the following command.

```shell
gn args out/Coverage
```

If you already have a build directory, you can append the coverage flag to the
existing `args.gn` as follows. If not using the default directory,
`out/Coverage`, then replace it with the correct location in the following
command.

```shell
echo "use_coverage = true" >> out/Coverage/args.gn
```


### Usage

Generating code coverage information is done via the
`testing/tools/coverage/coverage_report.py` script. This script will build any binaries
that it needs, perform test runs, collect coverage data, and finally generate a
nice HTML coverage report.

Running the script with no arguments, as below, will assume that you are
currently at the root of your PDFium checkout, the build directory to use is
`./out/Coverage/` and that HTML should be outputted to `./coverage_report/`. By
default, it will also only run `pdfium_unittests` and `pdfium_embeddertests` for
coverage data. This is because the other tests are known to take a long time to
run, so they are not included in the defaults.

```shell
testing/tools/coverage/coverage_report.py
```

If the current working directory is not the root of your PDFium checkout, then
you will need to pass in `--source-directory` with the appropriate directory. If
you are using a different build directory, then `--build-directory` will need to
be passed in. Finally, if you want the HTML report in a different location then
you will need to pass in `--output-directory`.

An example of all these flags being used:

```shell
testing/tools/coverage/coverage_report.py \
    --source-directory ~/pdfium/pdfium \
    --build-directory ~/pdfium/pdfium/out/Debug_with_Coverage \
    --output-directory ~/Documents/PDFium_coverage
```

To run different tests then the default set, there are two ways to achieve
this. If you want to run everything, including tests that are known to take a
long time, then you just need to add the `--slow` flag.

```shell
testing/tools/coverage/coverage_report.py --slow
```

If you want more fine grained control, including running just a single test, you
can specify the test names on the command line. The `--slow` flag is not needed
if you are explicitly invoking tests. The list of supported tests can be found
by running the script with `--help`.

An example running the default tests explicitly:

```shell
testing/tools/coverage/coverage_report.py pdfium_unittests pdfium_embeddertests
```

NOTE:
At the present time, there is no mechanism for combining data from different
invocations of `coverage_report.py`. Instead you must specify all of the tests
to be included in the report in a single invocation.

There are additional developer debugging flags available, `--dry-run` and
`--verbose`. `--dry-run` will output a trace of commands that would have been
run, but doesn't actually execute them. `--verbose` turns on outputting
additional logging information.

### Viewing

Once the script has run, the output directory should contain a set of HTML files
containing the coverage report.

These files are static HTML, so you can point your browser at them directly on
your local file system and they should render fine. You can also serve them via a
web server if you want, but how to achieve that is beyond the scope of this
documentation.

## Issues

For help with using the code coverage tools please contact the PDFium
maintainers via the PDFium
mailing [list](https://groups.google.com/forum/#!forum/pdfium).

Please file bugs against the code coverage
support [here](https://bugs.chromium.org/p/pdfium/issues/list).
