# SafetyNet - Performance regression detection for PDFium

[TOC]

This document explains how to use SafetyNet to detect performance regressions
in PDFium.

## Comparing performance of two versions of PDFium

safetynet_compare.py is a script that compares the performance between two
versions of pdfium. This can be used to verify if a given change has caused
or will cause any positive or negative changes in performance for a set of test
cases.

The supported profilers are exclusive to Linux, so for now this can only be run
on Linux.

An illustrative example is below, comparing the local code version to an older
version. Positive % changes mean an increase in time/instructions to run the
test - a regression, while negative % changes mean a decrease in
time/instructions, therefore an improvement.

```
$ testing/tools/safetynet_compare.py ~/test_pdfs --branch-before beef5e4
================================================================================
   % Change      Time after  Test case
--------------------------------------------------------------------------------
   -0.1980%  45,703,820,326  ~/test_pdfs/PDF Reference 1-7.pdf
   -0.5678%      42,038,814  ~/test_pdfs/Page 24 - PDF Reference 1-7.pdf
   +0.2666%  10,983,158,809  ~/test_pdfs/Rival.pdf
   +0.0447%  10,413,890,748  ~/test_pdfs/dynamic.pdf
   -7.7228%      26,161,171  ~/test_pdfs/encrypted1234.pdf
   -0.2763%     102,084,398  ~/test_pdfs/ghost.pdf
   -3.7005%  10,800,642,262  ~/test_pdfs/musician.pdf
   -0.2266%  45,691,618,789  ~/test_pdfs/no_metadata.pdf
   +1.4440%  38,442,606,162  ~/test_pdfs/test7.pdf
   +0.0335%       9,286,083  ~/test_pdfs/testbulletpoint.pdf
================================================================================
Test cases run: 10
Failed to measure: 0
Regressions: 0
Improvements: 2
```

### Usage

Run the safetynet_compare.py script in testing/tools to perform a comparison.
Pass one or more paths with test cases - each path can be either a .pdf file or
a directory containing .pdf files. Other files in those directories are
ignored.

The following comparison modes are supported:

1. Compare uncommitted changes against clean branch:
```shell
$ testing/tools/safetynet_compare.py path/to/pdfs
```

2. Compare current branch with another branch or commit:
```shell
$ testing/tools/safetynet_compare.py path/to/pdfs --branch-before another_branch
$ testing/tools/safetynet_compare.py path/to/pdfs --branch-before 1a3c5e7
```

3. Compare two other branches or commits:
```shell
$ testing/tools/safetynet_compare.py path/to/pdfs --branch-after another_branch --branch-before yet_another_branch
$ testing/tools/safetynet_compare.py path/to/pdfs --branch-after 1a3c5e7 --branch-before 0b2d4f6
$ testing/tools/safetynet_compare.py path/to/pdfs --branch-after another_branch --branch-before 0b2d4f6
```

4. Compare two build flag configurations:
```shell
$ gn args out/BuildConfig1
$ gn args out/BuildConfig2
$ testing/tools/safetynet_compare.py path/to/pdfs --build-dir out/BuildConfig2 --build-dir-before out/BuildConfig1
```

safetynet_compare.py takes care of checking out the appropriate branch, building
it, running the test cases and comparing results.

### Profilers

safetynet_compare.py uses callgrind as a profiler by default. Use --profiler
to specify another one. The supported ones are:

#### perfstat

Only works on Linux.
Make sure you have perf by typing in the terminal:
```shell
$ perf
```

This is a fast profiler, but uses sampling so it's slightly inaccurate.
Expect variations of up to 1%, which is below the cutoff to consider a
change significant.

Use this when running over large test sets to get good enough results.

#### callgrind

Only works on Linux.
Make sure valgrind is installed:
```shell
$ valgrind
```

Add `ro_segment_workaround_for_valgrind=true` to `args.gn` for symbols to appear
correctly.

This is a slow and accurate profiler. Expect variations of around 100
instructions. However, this takes about 50 times longer to run than perf stat.

Use this when looking for small variations (< 1%).

One advantage is that callgrind can generate `callgrind.out` files (by passing
--output-dir to safetynet_compare.py), which contain profiling information that
can be analyzed to find the cause of a regression. KCachegrind is a good
visualizer for these files.

### Common Options

Arguments commonly passed to safetynet_compare.py.

* --profiler: described above.
* --build-dir: this specified the build config with a relative path from the
pdfium src directory to the build directory. Defaults to out/Release.
* --output-dir: where to place the profiling output files. These are
callgrind.out.[test_case] files for callgrind, perfstat does not produce them.
By default they are not written.
* --case-order: sort test case results according to this metric. Can be "after",
"before", "ratio" and "rating". If not specified, sort by path.
* --this-repo: use the repository where the script is instead of checking out a
temporary one. This is faster and does not require downloads. Although it
restores the state of the local repo, if the script is killed or crashes the
uncommitted changes can remain stashed and you may be on another branch.

### Other Options

Most of the time these don't need to be used.

* --build-dir-before: if comparing different build dirs (say, to test what a
flag flip does), specify the build dir for the “before” branch here and the
build dir for the “after” branch with --build-dir.
* --interesting-section: only the interesting section should be measured instead
of all the execution of the test harness. This only works in debug, since in
release the delimiters are stripped out. This does not work to compare branches
that don’t have the callgrind delimiters, as it would otherwise be unfair to
compare a whole run vs the interesting section of another run.
* --machine-readable: output a json with the results that is easier to read by
code.
* --num-workers: how many workers to use to parallelize test case runs. Defaults
to # of CPUs in the machine.
* --threshold-significant: highlight differences that exceed this value.
Defaults to 0.02.
* --tmp-dir: directory in which temporary repos will be cloned and downloads
will be cached, if --this-repo is not enabled. Defaults to /tmp.

## Setup a nightly job

TODO: Complete with safetynet_job.py setup and usage.
