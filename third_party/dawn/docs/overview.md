# Dawn repository overview

This repository contains the implementation of Dawn, which is itself composed of two main libraries (dawn_native and dawn_wire), along with support libraries, tests, and samples. Dawn makes heavy use of code-generation based on the `dawn.json` file that describes the native WebGPU API. It is used to generate the API headers, C++ wrapper, parts of the client-server implementation, and more!

## Directory structure

- [`dawn.json`](../dawn.json): contains a description of the native WebGPU in JSON form. It is the data model that's used by the code generators.
- [`dawn_wire.json`](../dawn_wire.json): contains additional information used to generate `dawn_wire` files, such as commands in addition to regular WebGPU commands.
- [`examples`](../examples): a small collection of samples using the native WebGPU API. They were mostly used when bringing up Dawn for the first time, and to test the `WGPUSwapChain` object.
- [`generator`](../generator): directory containg the code generators and their templates. Generators are based on Jinja2 and parse data-models from JSON files.
    - [`dawn_json_generator.py`](../generator/dawn_json_generator.py): the main code generator that outputs the WebGPU headers, C++ wrapper, client-server implementation, etc.
    - [`templates`](../generator/templates): Jinja2 templates for the generator, with subdirectories for groups of templates that are all used in the same library.
- [`infra`](../infra): configuration file for the commit-queue infrastructure.
- [`scripts`](../scripts): contains a grab-bag of files that are used for building Dawn, in testing, etc.
- [`src`](../src):
  - [`common`](../src/common): helper code that is allowed to be used by Dawn's core libraries, `dawn_native` and `dawn_wire`. Also allowed for use in all other Dawn targets.
  - [`dawn_native`](../src/dawn_native): code for the implementation of WebGPU on top of graphics APIs. Files in this folder are the "frontend" while subdirectories are "backends".
      - `<backend>`: code for the implementation of the backend on a specific graphics API, for example `d3d12`, `metal` or `vulkan`.
  - [`dawn_platform`](../src/dawn_platform): definition of interfaces for dependency injection in `dawn_native` or `dawn_wire`.
  - [`dawn_wire`](../src/dawn_wire): code for an implementation of WebGPU as a client-server architecture.
  - [`fuzzers`](../src/fuzzers): various fuzzers for Dawn that are running in [Clusterfuzz](https://google.github.io/clusterfuzz/).
  - [`include`](../src/include): public headers with subdirectories for each library. Note that some headers are auto-generated and not present directly in the directory.
  - [`tests`](../src/tests):
    - [`end2end`](../src/tests/end2end): tests for the execution of the WebGPU API and require a GPU to run.
    - [`perf_tests`](../src/tests/perf_tests): benchmarks for various aspects of Dawn.
    - [`unittests`](../src/tests/unittests): code unittests of internal classes, but also by extension WebGPU API tests that don't require a GPU to run.
      - [`validation`](../src/tests/unittests/validation): WebGPU validation tests not using the GPU (frontend tests)
    - [`white_box`](../src/tests/white_box): tests using the GPU that need to access the internals of `dawn_native` or `dawn_wire`.
  - [`utils`](../src/utils): helper code to use Dawn used by tests and samples but disallowed for `dawn_native` and `dawn_wire`.
- [`third_party`](../third_party): directory where dependencies live as well as their buildfiles.

## Dawn Native (`dawn_native`)

The largest library in Dawn is `dawn_native` which implements the WebGPU API by translating to native graphics APIs such as D3D12, Metal or Vulkan. It is composed of a frontend that does all the state-tracking and validation, and backends that do the actual translation to the native graphics APIs.

`dawn_native` hosts the [SPVC](https://github.com/google/shaderc/tree/master/spvc) shader translator that validates SPIR-V for WebGPU and converts it to an equivalent shader for use in the native graphics API (HLSL for D3D12, MSL for Metal or Vulkan SPIR-V for Vulkan).

## Dawn Wire (`dawn_wire`)

A second library that implements both a client that takes WebGPU commands and serializes them into a buffer, and a server that deserializes commands from a buffer, validates they are well-formed and calls the relevant WebGPU commands. Some server to client communication also happens so the API's callbacks work properly.

Note that `dawn_wire` is meant to do as little state-tracking as possible so that the client can be lean and defer most of the heavy processing to the server side where the server calls into `dawn_native`.

## Dawn Proc (`dawn_proc`)

Normally libraries implementing `webgpu.h` should implement function like `wgpuDeviceCreateBuffer` but instead `dawn_native` and `dawn_wire` implement the `dawnProcTable` which is a structure containing all the WebGPU functions Dawn implements. Then a `dawn_proc` library contains a static version of this `dawnProcTable` and for example forwards `wgpuDeviceCreateBuffer` to the `procTable.deviceCreateBuffer` function pointer. This is useful in two ways:

 - It allows deciding at runtime whether to use `dawn_native` and `dawn_wire`, which is useful to test boths paths with the same binary in our infrastructure.
 - It avoids applications that know they will only use Dawn to query all entrypoints at once instead of using `wgpuGetProcAddress` repeatedly.

## Code generation

When the WebGPU API evolves a lot of places in Dawn have to be updated, so to reduce efforts, Dawn relies heavily on code generation. The code generators are based on [Jinja2](https://jinja.palletsprojects.com/) and separate the model from the view like in Web development. The model is some JSON file, usually [`dawn.json`](../dawn.json) and the views are the Jinja2 templates in [`generator/templates`](../generator/templates). The generated files are not checked into the repository but instead are generated during the build.

Most of the code generation is done in [`dawn_json_generator.py`](../generator/dawn_json_generator.py) but other generators exist so common functionality to build code generators has been extracted into[`generator_lib.py`](../generator/generator_lib.py).
