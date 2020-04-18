# The JSON Test Results Format

The JSON Test Results Format is a generic file format we use to record the
results of each individual test in test run (whether the test is run on a bot,
or run locally).

[TOC]

## Introduction

We use these files on the bots in order to determine whether a test step had
any failing tests (using a separate file means that we don't need to parse the
output of the test run, and hence the test can be tailored for human readability
as a result). We also upload the test results to dashboards like the
[Flakiness Dashboard](http://test-results.appspot.com).

The test format originated with the Blink layout tests, but has since been
adopted by GTest-based tests and Python unittest-based tests, so we've
standardized on it for anything related to tracking test flakiness.

### Example

Here's a very simple example for one Python test:

    % python mojo/tools/run_mojo_python_tests.py --write-full-results-to results.json mojom_tests.parse.ast_unittest.ASTTest.testNodeBase
    Running Python unit tests under mojo/public/tools/bindings/pylib ...
    .
    ----------------------------------------------------------------------
    Ran 1 test in 0.000s

    OK
    % cat results.json
    {
      "tests": {
        "mojom_tests": {
          "parse": {
            "ast_unittest": {
              "ASTTest": {
                "testNodeBase": {
                  "expected": "PASS",
                  "actual": "PASS",
                  "artifacts": {
                    "screenshot": ["screenshots/page.png"],
                  }
                }
              }
            }
          }
        }
      },
      "interrupted": false,
      "path_delimiter": ".",
      "version": 3,
      "seconds_since_epoch": 1406662283.764424,
      "num_failures_by_type": {
        "FAIL": 0,
        "PASS": 1
      },
      "artifact_types": {
        "screenshot": "image/png"
      }
    }



As you can see, the format consists of a one top level dictionary containing a
set of metadata fields describing the test run, plus a single `tests` key that
contains the results of every test run, structured in a hierarchical trie format
to reduce duplication of test suite names (as you can see from the deeply
hierarchical Python test name).

The file is strictly JSON-compliant. As a part of this, the order the name
appear in each object is unimportant.

## Top-level field names

| Field Name | Data Type | Description |
|------------|-----------|-------------|
| `interrupted` | boolean | **Required.** Whether the test run was interrupted and terminated early (either via the runner bailing out or the user hitting ctrl-C, etc.) If true, this indicates that not all of the tests in the suite were run and the results are at best incomplete and possibly totally invalid. |
| `num_failures_by_type` |  dict | **Required.** A summary of the totals of each result type. If a test was run more than once, only the first invocation's result is included in the totals. Each key is one of the result types listed below. A missing result type is the same as being present and set to zero (0). |
| `path_delimiter` | string | **Optional, will be mandatory.** The separator string to use in between components of a tests name; normally "." for GTest- and Python-based tests and "/" for layout tests; if not present, you should default to "/" for backwards-compatibility.  |
| `seconds_since_epoch` | float | **Required.** The start time of the test run expressed as a floating-point offset in seconds from the UNIX epoch. |
| `tests` | dict | **Required.** The actual trie of test results. Each directory or module component in the test name is a node in the trie, and the leaf contains the dict of per-test fields as described below. |
| `version` | integer | **Required.** Version of the file format. Current version is 3. |
| `artifact_types` | dict | **Optional. Required if any artifacts are present for any tests.** MIME Type information for artifacts in this json file. All artifacts with the same name must share the same MIME type.  |
| `artifact_permament_location` | string | **Optional.** The URI of the root location where the artifacts are stored. If present, any artifact locations are taken to be relative to this location. Currently only the `gs://` scheme is supported. |
| `build_number` | string | **Optional.** If this test run was produced on a bot, this should be the build number of the run, e.g., "1234". |
| `builder_name` | string | **Optional.** If this test run was produced on a bot, this should be the builder name of the bot, e.g., "Linux Tests". |
| `chromium_revision` | string | **Optional.** The revision of the current Chromium checkout, if relevant, e.g. "356123". |
| `has_pretty_patch` | bool | **Optional, layout test specific, deprecated.** Whether the layout tests' output contains PrettyDiff-formatted diffs for test failures. |
| `has_wdiff` | bool | **Optional, layout test specific, deprecated.** Whether the layout tests' output contains wdiff-formatted diffs for test failures. |
| `layout_tests_dir` | string | **Optional, layout test specific.** Path to the LayoutTests directory for the test run (used so that we can link to the tests used in the run). |
| `pixel_tests_enabled` | bool | **Optional, layout test specific.** Whether the layout tests' were run with the --pixel-tests flag.  |
| `fixable` | integer | **Optional, deprecated.** The number of tests that were run but were expected to fail. |
| `num_flaky` | integer | **Optional, deprecated.** The number of tests that were run more than once and produced different results each time. |
| `num_passes` | integer | **Optional, deprecated.** The number of successful tests; equivalent to `num_failures_by_type["Pass"]` |
| `num_regressions` | integer | **Optional, deprecated.** The number of tests that produced results that were unexpected failures. |
| `skips` | integer | **Optional, deprecated.** The number of tests that were found but not run (tests should be listed in the trie with "expected" and "actual" values of `SKIP`. |

## Per-test fields

Each leaf of the `tests` trie contains a dict containing the results of a
particular test name. If a test is run multiple times, the dict contains the
results for each invocation in the `actual` field.

|  Field Name | Data Type | Description |
|-------------|-----------|-------------|
|  `actual` | string | **Required.** An ordered space-separated list of the results the test actually produced. `FAIL PASS` means that a test was run twice, failed the first time, and then passed when it was retried. If a test produces multiple different results, then it was actually flaky during the run. |
|  `expected` | string | **Required.** An unordered space-separated list of the result types expected for the test, e.g. `FAIL PASS` means that a test is expected to either pass or fail. A test that contains multiple values is expected to be flaky. |
|  `artifacts` | dict | **Optional.** A dictionary describing test artifacts generated by the execution of the test. The dictionary maps the name of the artifact (`screenshot`, `crash_log`) to a list of relative locations of the artifact (`screenshot/page.png`, `logs/crash.txt`). Any '/' characters in the file paths are meant to be platform agnostic; tools will replace them with the appropriate per platform path separators. There is one entry in the list per test execution. If `artifact_permanent_location` is specified, then this location is relative to that path. Otherwise, the path is assumed to be relative to the location of the json file which contains this. |
|  `bugs` | string | **Optional.** A comma-separated list of URLs to bug database entries associated with each test. |
|  `is_unexpected` | bool | **Optional.** If present and true, the failure was unexpected (a regression). If false (or if the key is not present at all), the failure was expected and will be ignored. |
|  `time` | float | **Optional.** If present, the time it took in seconds to execute the first invocation of the test. |
|  `times` | array of floats | **Optional.** If present, the times in seconds of each invocation of the test. |
|  `has_repaint_overlay` | bool | **Optional, layout test specific.** If present and true, indicates that the test output contains the data needed to draw repaint overlays to help explain the results (only used in layout tests). |
|  `is_missing_audio` | bool | **Optional, layout test specific.** If present and true, the test was supposed to have an audio baseline to compare against, and we didn't find one. |
|  `is_missing_text` | bool | **Optional, layout test specific.** If present and true, the test was supposed to have a text baseline to compare against, and we didn't find one.  |
|  `is_missing_video` | bool | **Optional, layout test specific.** If present and true, the test was supposed to have an image baseline to compare against and we didn't find one. |
|  `is_testharness_test` | bool | **Optional, layout test specific.** If present, indicates that the layout test was written using the w3c's test harness and we don't necessarily have any baselines to compare against. |
|  `reftest_type` | string | **Optional, layout test specific.** If present, one of `==` or `!=` to indicate that the test is a "reference test" and the results were expected to match the reference or not match the reference, respectively (only used in layout tests). |

## Test result types

Any test may fail in one of several different ways. There are a few generic
types of failures, and the layout tests contain a few additional specialized
failure types.

|  Result type | Description |
|--------------|-------------|
|  `SKIP` | The test was not run. |
|  `PASS` | The test ran as expected. |
|  `FAIL` | The test did not run as expected. |
|  `CRASH` | The test runner crashed during the test. |
|  `TIMEOUT` | The test hung (did not complete) and was aborted. |
|  `MISSING` | **Layout test specific.** The test completed but we could not find an expected baseline to compare against |
|  `LEAK` | **Layout test specific.** Memory leaks were detected during the test execution. |
|  `SLOW` | **Layout test specific.** The test is expected to take longer than normal to run. |
|  `TEXT` | **Layout test specific, deprecated.** The test is expected to produce a text-only failure (the image, if present, will match). Normally you will see `FAIL` instead. |
|  `AUDIO` | **Layout test specific, deprecated.** The test is expected to produce audio output that doesn't match the expected result. Normally you will see `FAIL` instead. |
|  `IMAGE` | **Layout test specific.** The test produces image (and possibly text output). The image output doesn't match what we'd expect, but the text output, if present, does. |
|  `IMAGE+TEXT` | **Layout test specific, deprecated.** The test produces image and text output, both of which fail to match what we expect. Normally you will see `FAIL` instead. |
|  `REBASELINE`  | **Layout test specific.** The expected test result is out of date and will be ignored (any result other than a crash or timeout will be considered as passing). This test result should only ever show up on local test runs, not on bots (it is forbidden to check in a TestExpectations file with this expectation). This should never show up as an "actual" result. |
|  `NEEDSREBASELINE` | **Layout test specific.** The expected test result is out of date and will be ignored (as above); the auto-rebaseline-bot will look for tests of this type and automatically update them. This should never show up as an "actual" result. |
|  `NEEDSMANUALREBASELINE`  | **Layout test specific.** The expected test result is out of date and will be ignored (as above). This result may be checked in to the TestExpectations file, but the auto-rebasline-bot will ignore these entries. This should never show up as an "actual" result. |

## "full_results.json" and "failing_results.json"

The layout tests produce two different variants of the above file. The
`full_results.json` file matches the above definition and contains every test
executed in the run. The `failing_results.json` file contains just the tests
that produced unexpected results, so it is a subset of the `full_results.json`
data. The `failing_results.json` file is also in the JSONP format, so it can be
read via as a `<script>` tag from an html file run from the local filesystem
without falling prey to the same-origin restrictions for local files.  The
`failing_results.json` file is converted into JSONP by containing the JSON data
preceded by the string `ADD_RESULTS(` and followed by the string `);`, so you
can extract the JSON data by stripping off that prefix and suffix.
