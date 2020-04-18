
# Loader Tests

This directory contains a test suite for the Vulkan loader.
These tests are not exhaustive &mdash; they are expected to be supplemented with other tests, such as CTS.

## Running Tests

To run the tests, your environment needs to be configured so that the test layers will be found.
This can be done by setting the `VK_LAYER_PATH` environment variable to point at the built layers.
Depending on the platform build tool you use, this location will either be `${CMAKE_BINARY_DIR}/tests/layers` or `${CMAKE_BINARY_DIR}/tests/layers/${CONFIGURATION}`.
When using Visual Studio, a the generated project will already be set up to set the environment as needed.
Running the tests through the `run_loader_tests.sh` script on Linux will also set up the environment properly.
With any other toolchain, the user will have to set up the environment manually.
