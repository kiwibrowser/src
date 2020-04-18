# Web Bluetooth Fuzzer

The Web Bluetooth Fuzzer generates test pages that can be run as layout tests.
These pages consist of a sequence of calls to the [Web Bluetooth API](../..)
whose purpose is to stress test the API's implementation and catch any bugs
or regressions.

This document describes the overall design of the fuzzer.

[TOC]

## Overview
To generate test pages the fuzzer performs the following steps:

1. Generate a test page that consists of a series of random calls to the API
   with template parameters, calls to reload the page and calls to run garbage
   collection.
2. Replace the template parameters with random values.

These generated test pages can then be run as Layout Tests in content_shell.

## ClusterFuzz
This fuzzer is designed to be run by ClusterFuzz and therefore takes three
arguments, `--no_of_files`, `--input_dir`, and `--output_dir`.

## Setup
This fuzzer depends on files in:
* `//src/third_party/WebKit/LayoutTests/resources`
* `//src/testing/clusterfuzz/common`

To ease development a setup.py script is included to copy over the necessary
files to run the fuzzer locally. Additionally the script can be used to generate
a .tar.bz2 file that can be uploaded to ClusterFuzz. To see the available
options, run:
```sh
python setup.py -h
```
