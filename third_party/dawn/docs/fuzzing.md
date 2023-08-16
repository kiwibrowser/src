# Fuzzing Dawn

## `dawn_wire_server_and_frontend_fuzzer`

The `dawn_wire_server_and_frontend_fuzzer` sets up Dawn using the Null backend, and passes inputs to the wire server. This fuzzes the `dawn_wire` deserialization, as well as Dawn's frontend validation.

## `dawn_wire_server_and_vulkan_backend_fuzzer`

The `dawn_wire_server_and_vulkan_backend_fuzzer` is like `dawn_wire_server_and_frontend_fuzzer` but it runs using a Vulkan CPU backend such as Swiftshader. This fuzzer supports error injection by using the first bytes of the fuzzing input as a Vulkan call index for which to mock a failure.

## Updating the Seed Corpus

Using a seed corpus significantly improves the efficiency of fuzzing. Dawn's fuzzers use interesting testcases discovered in previous fuzzing runs to seed future runs. Fuzzing can be further improved by using Dawn tests as a example of API usage which allows the fuzzer to quickly discover and use new API entrypoints and usage patterns.

The script [update_fuzzer_seed_corpus.sh](../scripts/update_fuzzer_seed_corpus.sh) can be used to capture a trace while running Dawn tests, and upload it to the existing fuzzer seed corpus. It does the following steps:
1. Builds the provided test and fuzzer targets.
2. Runs the provided test target with `--use-wire --wire-trace-dir=tmp_dir1 [additional_test_args]` to dump traces of the tests.
3. Generates one variant of each trace for every possible error index, by running the fuzzer target with `--injected-error-testcase-dir=tmp_dir2 ...`.
4. Minimizes all testcases by running the fuzzer target with `-merge=1 tmp_dir3 tmp_dir1 tmp_dir2`.

To run the script:
1. You must be in a Chromium checkout using the GN arg `use_libfuzzer=true`
2. Run `./third_party/dawn/scripts/update_fuzzer_seed_corpus.sh <out_dir> <fuzzer> <test> [additional_test_args]`.

   Example: `./third_party/dawn/scripts/update_fuzzer_seed_corpus.sh out/fuzz dawn_wire_server_and_vulkan_backend_fuzzer dawn_end2end_tests --gtest_filter=*Vulkan`
3. The script will print instructions for testing, and then uploading new inputs. Please, only upload inputs after testing the fuzzer with new inputs, and verifying there is a meaningful change in coverage. Uploading requires [gcloud](https://g3doc.corp.google.com/cloud/sdk/g3doc/index.md?cl=head) to be logged in with @google.com credentials: `gcloud auth login`.
