# GPU Testing

This set of pages documents the setup and operation of the GPU bots and try
servers, which verify the correctness of Chrome's graphically accelerated
rendering pipeline.

[TOC]

## Overview

The GPU bots run a different set of tests than the majority of the Chromium
test machines. The GPU bots specifically focus on tests which exercise the
graphics processor, and whose results are likely to vary between graphics card
vendors.

Most of the tests on the GPU bots are run via the [Telemetry framework].
Telemetry was originally conceived as a performance testing framework, but has
proven valuable for correctness testing as well. Telemetry directs the browser
to perform various operations, like page navigation and test execution, from
external scripts written in Python. The GPU bots launch the full Chromium
browser via Telemetry for the majority of the tests. Using the full browser to
execute tests, rather than smaller test harnesses, has yielded several
advantages: testing what is shipped, improved reliability, and improved
performance.

[Telemetry framework]: https://github.com/catapult-project/catapult/tree/master/telemetry

A subset of the tests, called "pixel tests", grab screen snapshots of the web
page in order to validate Chromium's rendering architecture end-to-end. Where
necessary, GPU-specific results are maintained for these tests. Some of these
tests verify just a few pixels, using handwritten code, in order to use the
same validation for all brands of GPUs.

The GPU bots use the Chrome infrastructure team's [recipe framework], and
specifically the [`chromium`][recipes/chromium] and
[`chromium_trybot`][recipes/chromium_trybot] recipes, to describe what tests to
execute. Compared to the legacy master-side buildbot scripts, recipes make it
easy to add new steps to the bots, change the bots' configuration, and run the
tests locally in the same way that they are run on the bots. Additionally, the
`chromium` and `chromium_trybot` recipes make it possible to send try jobs which
add new steps to the bots. This single capability is a huge step forward from
the previous configuration where new steps were added blindly, and could cause
failures on the tryservers. For more details about the configuration of the
bots, see the [GPU bot details].

[recipe framework]: https://chromium.googlesource.com/external/github.com/luci/recipes-py/+/master/doc/user_guide.md
[recipes/chromium]:        https://chromium.googlesource.com/chromium/tools/build/+/master/scripts/slave/recipes/chromium.py
[recipes/chromium_trybot]: https://chromium.googlesource.com/chromium/tools/build/+/master/scripts/slave/recipes/chromium_trybot.py
[GPU bot details]: gpu_testing_bot_details.md

The physical hardware for the GPU bots lives in the Swarming pool\*. The
Swarming infrastructure ([new docs][new-testing-infra], [older but currently
more complete docs][isolated-testing-infra]) provides many benefits:

*   Increased parallelism for the tests; all steps for a given tryjob or
    waterfall build run in parallel.
*   Simpler scaling: just add more hardware in order to get more capacity. No
    manual configuration or distribution of hardware needed.
*   Easier to run certain tests only on certain operating systems or types of
    GPUs.
*   Easier to add new operating systems or types of GPUs.
*   Clearer description of the binary and data dependencies of the tests. If
    they run successfully locally, they'll run successfully on the bots.

(\* All but a few one-off GPU bots are in the swarming pool. The exceptions to
the rule are described in the [GPU bot details].)

The bots on the [chromium.gpu.fyi] waterfall are configured to always test
top-of-tree ANGLE. This setup is done with a few lines of code in the
[tools/build workspace]; search the code for "angle".

These aspects of the bots are described in more detail below, and in linked
pages. There is a [presentation][bots-presentation] which gives a brief
overview of this documentation and links back to various portions.

<!-- XXX: broken link -->
[new-testing-infra]: https://github.com/luci/luci-py/wiki
[isolated-testing-infra]: https://www.chromium.org/developers/testing/isolated-testing/infrastructure
[chromium.gpu]: https://build.chromium.org/p/chromium.gpu/console
[chromium.gpu.fyi]: https://build.chromium.org/p/chromium.gpu.fyi/console
[tools/build workspace]: https://code.google.com/p/chromium/codesearch#chromium/build/scripts/slave/recipe_modules/chromium_tests/chromium_gpu_fyi.py
[bots-presentation]: https://docs.google.com/presentation/d/1BC6T7pndSqPFnituR7ceG7fMY7WaGqYHhx5i9ECa8EI/edit?usp=sharing

## Fleet Status

Please see the [GPU Pixel Wrangling instructions] for links to dashboards
showing the status of various bots in the GPU fleet.

[GPU Pixel Wrangling instructions]: pixel_wrangling.md#Fleet-Status

## Using the GPU Bots

Most Chromium developers interact with the GPU bots in two ways:

1.  Observing the bots on the waterfalls.
2.  Sending try jobs to them.

The GPU bots are grouped on the [chromium.gpu] and [chromium.gpu.fyi]
waterfalls. Their current status can be easily observed there.

To send try jobs, you must first upload your CL to the codereview server. Then,
either clicking the "CQ dry run" link or running from the command line:

```sh
git cl try
```

Sends your job to the default set of try servers.

The GPU tests are part of the default set for Chromium CLs, and are run as part
of the following tryservers' jobs:

*   [linux_chromium_rel_ng] on the [tryserver.chromium.linux] waterfall
*   [mac_chromium_rel_ng]   on the [tryserver.chromium.mac]   waterfall
*   [win_chromium_rel_ng]   on the [tryserver.chromium.win]   waterfall

[linux_chromium_rel_ng]:    http://build.chromium.org/p/tryserver.chromium.linux/builders/linux_chromium_rel_ng?numbuilds=100
[mac_chromium_rel_ng]:      http://build.chromium.org/p/tryserver.chromium.mac/builders/mac_chromium_rel_ng?numbuilds=100
[win_chromium_rel_ng]:      http://build.chromium.org/p/tryserver.chromium.win/builders/win_chromium_rel_ng?numbuilds=100
[tryserver.chromium.linux]: http://build.chromium.org/p/tryserver.chromium.linux/waterfall?numbuilds=100
[tryserver.chromium.mac]:   http://build.chromium.org/p/tryserver.chromium.mac/waterfall?numbuilds=100
[tryserver.chromium.win]:   http://build.chromium.org/p/tryserver.chromium.win/waterfall?numbuilds=100

Scan down through the steps looking for the text "GPU"; that identifies those
tests run on the GPU bots. For each test the "trigger" step can be ignored; the
step further down for the test of the same name contains the results.

It's usually not necessary to explicitly send try jobs just for verifying GPU
tests. If you want to, you must invoke "git cl try" separately for each
tryserver master you want to reference, for example:

```sh
git cl try -b linux_chromium_rel_ng
git cl try -b mac_chromium_rel_ng
git cl try -b win_chromium_rel_ng
```

Alternatively, the Gerrit UI can be used to send a patch set to these try
servers.

Three optional tryservers are also available which run additional tests. As of
this writing, they ran longer-running tests that can't run against all Chromium
CLs due to lack of hardware capacity. They are added as part of the included
tryservers for code changes to certain sub-directories.

*   [linux_optional_gpu_tests_rel] on the [luci.chromium.try] waterfall
*   [mac_optional_gpu_tests_rel]   on the [luci.chromium.try]   waterfall
*   [win_optional_gpu_tests_rel]   on the [luci.chromium.try]   waterfall

[linux_optional_gpu_tests_rel]: https://ci.chromium.org/p/chromium/builders/luci.chromium.try/linux_optional_gpu_tests_rel
[mac_optional_gpu_tests_rel]:   https://ci.chromium.org/p/chromium/builders/luci.chromium.try/mac_optional_gpu_tests_rel
[win_optional_gpu_tests_rel]:   https://ci.chromium.org/p/chromium/builders/luci.chromium.try/win_optional_gpu_tests_rel

Tryservers for the [ANGLE project] are also present on the
[tryserver.chromium.angle] waterfall. These are invoked from the Gerrit user
interface. They are configured similarly to the tryservers for regular Chromium
patches, and run the same tests that are run on the [chromium.gpu.fyi]
waterfall, in the same way (e.g., against ToT ANGLE).

If you find it necessary to try patches against other sub-repositories than
Chromium (`src/`) and ANGLE (`src/third_party/angle/`), please
[file a bug](http://crbug.com/new) with component Internals\>GPU\>Testing.

[ANGLE project]: https://chromium.googlesource.com/angle/angle/+/master/README.md
[tryserver.chromium.angle]: https://build.chromium.org/p/tryserver.chromium.angle/waterfall
[file a bug]: http://crbug.com/new

## Running the GPU Tests Locally

All of the GPU tests running on the bots can be run locally from a Chromium
build. Many of the tests are simple executables:

*   `angle_unittests`
*   `content_gl_tests`
*   `gl_tests`
*   `gl_unittests`
*   `tab_capture_end2end_tests`

Some run only on the chromium.gpu.fyi waterfall, either because there isn't
enough machine capacity at the moment, or because they're closed-source tests
which aren't allowed to run on the regular Chromium waterfalls:

*   `angle_deqp_gles2_tests`
*   `angle_deqp_gles3_tests`
*   `angle_end2end_tests`
*   `audio_unittests`

The remaining GPU tests are run via Telemetry.  In order to run them, just
build the `chrome` target and then
invoke `src/content/test/gpu/run_gpu_integration_test.py` with the appropriate
argument. The tests this script can invoke are
in `src/content/test/gpu/gpu_tests/`. For example:

*   `run_gpu_integration_test.py context_lost --browser=release`
*   `run_gpu_integration_test.py pixel --browser=release`
*   `run_gpu_integration_test.py webgl_conformance --browser=release --webgl-conformance-version=1.0.2`
*   `run_gpu_integration_test.py maps --browser=release`
*   `run_gpu_integration_test.py screenshot_sync --browser=release`
*   `run_gpu_integration_test.py trace_test --browser=release`

**Note:** If you are on Linux and see this test harness exit immediately with
`**Non zero exit code**`, it's probably because of some incompatible Python
packages being installed. Please uninstall the `python-egenix-mxdatetime` and
`python-logilab-common` packages in this case; see
[Issue 716241](http://crbug.com/716241).

You can also run a subset of tests with this harness:

*   `run_gpu_integration_test.py webgl_conformance --browser=release
    --test-filter=conformance_attribs`

Figuring out the exact command line that was used to invoke the test on the
bots can be a little tricky. The bots all\* run their tests via Swarming and
isolates, meaning that the invocation of a step like `[trigger]
webgl_conformance_tests on NVIDIA GPU...` will look like:

*   `python -u
    'E:\b\build\slave\Win7_Release__NVIDIA_\build\src\tools\swarming_client\swarming.py'
    trigger --swarming https://chromium-swarm.appspot.com
    --isolate-server https://isolateserver.appspot.com
    --priority 25 --shards 1 --task-name 'webgl_conformance_tests on NVIDIA GPU...'`

You can figure out the additional command line arguments that were passed to
each test on the bots by examining the trigger step and searching for the
argument separator (<code> -- </code>). For a recent invocation of
`webgl_conformance_tests`, this looked like:

*   `webgl_conformance --show-stdout '--browser=release' -v
    '--extra-browser-args=--enable-logging=stderr --js-flags=--expose-gc'
    '--isolated-script-test-output=${ISOLATED_OUTDIR}/output.json'`

You can leave off the --isolated-script-test-output argument, so this would
leave a full command line of:

*   `run_gpu_integration_test.py
    webgl_conformance --show-stdout '--browser=release' -v
    '--extra-browser-args=--enable-logging=stderr --js-flags=--expose-gc'`

The Maps test requires you to authenticate to cloud storage in order to access
the Web Page Reply archive containing the test. See [Cloud Storage Credentials]
for documentation on setting this up.

[Cloud Storage Credentials]: gpu_testing_bot_details.md#Cloud-storage-credentials

Pixel tests use reference images from cloud storage, bots pass
`--upload-refimg-to-cloud-storage` argument, but to run locally you need to pass
`--download-refimg-from-cloud-storage` argument, as well as other arguments bot
uses, like `--refimg-cloud-storage-bucket` and `--os-type`.

Sample command line for Android:

*   `run_gpu_integration_test.py pixel --show-stdout --browser=android-chromium
    -v --passthrough --extra-browser-args='--enable-logging=stderr
    --js-flags=--expose-gc' --refimg-cloud-storage-bucket
    chromium-gpu-archive/reference-images --os-type android
    --download-refimg-from-cloud-storage`

<!-- XXX: update this section; these isolates don't exist anymore -->
You can find the isolates for the various tests in
[src/chrome/](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/):

*   [angle_unittests.isolate](https://chromium.googlesource.com/chromium/src/+/master/chrome/angle_unittests.isolate)
*   [content_gl_tests.isolate](https://chromium.googlesource.com/chromium/src/+/master/content/content_gl_tests.isolate)
*   [gl_tests.isolate](https://chromium.googlesource.com/chromium/src/+/master/chrome/gl_tests.isolate)
*   [gles2_conform_test.isolate](https://chromium.googlesource.com/chromium/src/+/master/chrome/gles2_conform_test.isolate)
*   [tab_capture_end2end_tests.isolate](https://chromium.googlesource.com/chromium/src/+/master/chrome/tab_capture_end2end_tests.isolate)
*   [telemetry_gpu_test.isolate](https://chromium.googlesource.com/chromium/src/+/master/chrome/telemetry_gpu_test.isolate)

The isolates contain the full or partial command line for invoking the target.
The complete command line for any test can be deduced from the contents of the
isolate plus the stdio output from the test's run on the bot.

Note that for the GN build, the isolates are simply described by build targets,
and [gn_isolate_map.pyl] describes the mapping between isolate name and build
target, as well as the command line used to invoke the isolate. Once all
platforms have switched to GN, the .isolate files will be obsolete and be
removed.

(\* A few of the one-off GPU configurations on the chromium.gpu.fyi waterfall
run their tests locally rather than via swarming, in order to decrease the
number of physical machines needed.)

[gn_isolate_map.pyl]: https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/gn_isolate_map.pyl

## Running Binaries from the Bots Locally

Any binary run remotely on a bot can also be run locally, assuming the local
machine loosely matches the architecture and OS of the bot.

The easiest way to do this is to find the ID of the swarming task and use
"swarming.py reproduce" to re-run it:

*   `./src/tools/swarming_client/swarming.py reproduce -S https://chromium-swarm.appspot.com [task ID]`

The task ID can be found in the stdio for the "trigger" step for the test. For
example, look at a recent build from the [Mac Release (Intel)] bot, and
look at the `gl_unittests` step. You will see something like:

[Mac Release (Intel)]: https://ci.chromium.org/buildbot/chromium.gpu/Mac%20Release%20%28Intel%29/

```
Triggered task: gl_unittests on Intel GPU on Mac/Mac-10.12.6/[TRUNCATED_ISOLATE_HASH]/Mac Release (Intel)/83664
To collect results, use:
  swarming.py collect -S https://chromium-swarm.appspot.com --json /var/folders/[PATH_TO_TEMP_FILE].json
Or visit:
  https://chromium-swarm.appspot.com/user/task/[TASK_ID]
```

There is a difference between the isolate's hash and Swarming's task ID. Make
sure you use the task ID and not the isolate's hash.

As of this writing, there seems to be a
[bug](https://github.com/luci/luci-py/issues/250)
when attempting to re-run the Telemetry based GPU tests in this way. For the
time being, this can be worked around by instead downloading the contents of
the isolate. To do so, look more deeply into the trigger step's log:

*   <code>python -u
    /b/build/slave/Mac_10_10_Release__Intel_/build/src/tools/swarming_client/swarming.py
    trigger [...more args...] --tag data:[ISOLATE_HASH] [...more args...]
    [ISOLATE_HASH] -- **[...TEST_ARGS...]**</code>

As of this writing, the isolate hash appears twice in the command line. To
download the isolate's contents into directory `foo` (note, this is in the
"Help" section associated with the page for the isolate's task, but I'm not
sure whether that's accessible only to Google employees or all members of the
chromium.org organization):

*   `python isolateserver.py download -I https://isolateserver.appspot.com
    --namespace default-gzip -s [ISOLATE_HASH] --target foo`

`isolateserver.py` will tell you the approximate command line to use. You
should concatenate the `TEST_ARGS` highlighted in red above with
`isolateserver.py`'s recommendation. The `ISOLATED_OUTDIR` variable can be
safely replaced with `/tmp`.

Note that `isolateserver.py` downloads a large number of files (everything
needed to run the test) and may take a while. There is a way to use
`run_isolated.py` to achieve the same result, but as of this writing, there
were problems doing so, so this procedure is not documented at this time.

Before attempting to download an isolate, you must ensure you have permission
to access the isolate server. Full instructions can be [found
here][isolate-server-credentials]. For most cases, you can simply run:

*   `./src/tools/swarming_client/auth.py login
    --service=https://isolateserver.appspot.com`

The above link requires that you log in with your @google.com credentials. It's
not known at the present time whether this works with @chromium.org accounts.
Email kbr@ if you try this and find it doesn't work.

[isolate-server-credentials]: gpu_testing_bot_details.md#Isolate-server-credentials

## Running Locally Built Binaries on the GPU Bots

See the [Swarming documentation] for instructions on how to upload your binaries to the isolate server and trigger execution on Swarming.

[Swarming documentation]: https://www.chromium.org/developers/testing/isolated-testing/for-swes#TOC-Run-a-test-built-locally-on-Swarming

## Adding New Tests to the GPU Bots

The goal of the GPU bots is to avoid regressions in Chrome's rendering stack.
To that end, let's add as many tests as possible that will help catch
regressions in the product. If you see a crazy bug in Chrome's rendering which
would be easy to catch with a pixel test running in Chrome and hard to catch in
any of the other test harnesses, please, invest the time to add a test!

There are a couple of different ways to add new tests to the bots:

1.  Adding a new test to one of the existing harnesses.
2.  Adding an entire new test step to the bots.

### Adding a new test to one of the existing test harnesses

Adding new tests to the GTest-based harnesses is straightforward and
essentially requires no explanation.

As of this writing it isn't as easy as desired to add a new test to one of the
Telemetry based harnesses. See [Issue 352807](http://crbug.com/352807). Let's
collectively work to address that issue. It would be great to reduce the number
of steps on the GPU bots, or at least to avoid significantly increasing the
number of steps on the bots. The WebGL conformance tests should probably remain
a separate step, but some of the smaller Telemetry based tests
(`context_lost_tests`, `memory_test`, etc.) should probably be combined into a
single step.

If you are adding a new test to one of the existing tests (e.g., `pixel_test`),
all you need to do is make sure that your new test runs correctly via isolates.
See the documentation from the GPU bot details on [adding new isolated
tests][new-isolates] for the `GYP_DEFINES` and authentication needed to upload
isolates to the isolate server. Most likely the new test will be Telemetry
based, and included in the `telemetry_gpu_test_run` isolate. You can then
invoke it via:

*   `./src/tools/swarming_client/run_isolated.py -s [HASH]
    -I https://isolateserver.appspot.com -- [TEST_NAME] [TEST_ARGUMENTS]`

[new-isolates]: gpu_testing_bot_details.md#Adding-a-new-isolated-test-to-the-bots

o## Adding new steps to the GPU Bots

The tests that are run by the GPU bots are described by a couple of JSON files
in the Chromium workspace:

*   [`chromium.gpu.json`](https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/chromium.gpu.json)
*   [`chromium.gpu.fyi.json`](https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/chromium.gpu.fyi.json)

These files are autogenerated by the following script:

*   [`generate_buildbot_json.py`](https://chromium.googlesource.com/chromium/src/+/master/content/test/gpu/generate_buildbot_json.py)

This script is completely self-contained and should hopefully be
self-explanatory. The JSON files are parsed by the chromium and chromium_trybot
recipes, and describe two types of tests:

*   GTests: those which use the Googletest and Chromium's `base/test/launcher/`
    frameworks.
*   Telemetry based tests: those which are built on the Telemetry framework and
    launch the entire browser.

A prerequisite of adding a new test to the bots is that that test [run via
isolates][new-isolates]. Once that is done, modify `generate_buildbot_json.py` to add the
test to the appropriate set of bots. Be careful when adding large new test
steps to all of the bots, because the GPU bots are a limited resource and do
not currently have the capacity to absorb large new test suites. It is safer to
get new tests running on the chromium.gpu.fyi waterfall first, and expand from
there to the chromium.gpu waterfall (which will also make them run against
every Chromium CL by virtue of the `linux_chromium_rel_ng`,
`mac_chromium_rel_ng` and `win_chromium_rel_ng` tryservers' mirroring of the
bots on this waterfall – so be careful!).

Tryjobs which add new test steps to the chromium.gpu.json file will run those
new steps during the tryjob, which helps ensure that the new test won't break
once it starts running on the waterfall.

Tryjobs which modify chromium.gpu.fyi.json can be sent to the
`win_optional_gpu_tests_rel`, `mac_optional_gpu_tests_rel` and
`linux_optional_gpu_tests_rel` tryservers to help ensure that they won't
break the FYI bots.

## Updating and Adding New Pixel Tests to the GPU Bots

Adding new pixel tests which require reference images is a slightly more
complex process than adding other kinds of tests which can validate their own
correctness. There are a few reasons for this.

*   Reference image based pixel tests require different golden images for
    different combinations of operating system, GPU, driver version, OS
    version, and occasionally other variables.
*   The reference images must be generated by the main waterfall. The try
    servers are not allowed to produce new reference images, only consume them.
    The reason for this is that a patch sent to the try servers might cause an
    incorrect reference image to be generated. For this reason, the main
    waterfall bots upload reference images to cloud storage, and the try
    servers download them and verify their results against them.
*   The try servers will fail if they run a pixel test requiring a reference
    image that doesn't exist in cloud storage. This is deliberate, but needs
    more thought; see [Issue 349262](http://crbug.com/349262).

If a reference image based pixel test's result is going to change because of a
change in ANGLE or Blink (for example), updating the reference images is a
slightly tricky process. Here's how to do it:

*   Mark the pixel test as failing in the [pixel tests]' [test expectations]
*   Commit the change to ANGLE, Blink, etc. which will change the test's
    results
*   Note that without the failure expectation, this commit would turn some bots
    red; a Blink change will turn the GPU bots on the chromium.webkit waterfall
    red, and an ANGLE change will turn the chromium.gpu.fyi bots red
*   Wait for Blink/ANGLE/etc. to roll
*   Commit a change incrementing the revision number associated with the test
    in the [test pages]
*   Commit a second change removing the failure expectation, once all of the
    bots on the main waterfall have generated new reference images. This change
    should go through the commit queue cleanly.

[pixel tests]: https://chromium.googlesource.com/chromium/src/+/master/content/test/gpu/gpu_tests/pixel_test_pages.py
[test expectations]: https://chromium.googlesource.com/chromium/src/+/master/content/test/gpu/gpu_tests/pixel_expectations.py
[test pages]: https://chromium.googlesource.com/chromium/src/+/master/content/test/gpu/gpu_tests/pixel_test_pages.py

When adding a brand new pixel test that uses a reference image, the steps are
similar, but simpler:

*   Mark the test as failing in the same commit which introduces the new test
*   Wait for the reference images to be produced by all of the GPU bots on the
    waterfalls (see [chromium-gpu-archive/reference-images])
*   Commit a change un-marking the test as failing

When making a Chromium-side change which changes the pixel tests' results:

*   In your CL, both mark the pixel test as failing in the pixel test's test
    expectations and increment the test's version number in the page set (see
    above)
*   After your CL lands, land another CL removing the failure expectations. If
    this second CL goes through the commit queue cleanly, you know reference
    images were generated properly.

In general, when adding a new pixel test, it's better to spot check a few
pixels in the rendered image rather than using a reference image per platform.
The [GPU rasterization test] is a good example of a recently added test which
performs such spot checks.

[cloud storage bucket]: https://console.developers.google.com/storage/chromium-gpu-archive/reference-images
<!-- XXX: old link -->
[GPU rasterization test]: http://src.chromium.org/viewvc/chrome/trunk/src/content/test/gpu/gpu_tests/gpu_rasterization.py

## Stamping out Flakiness

It's critically important to aggressively investigate and eliminate the root
cause of any flakiness seen on the GPU bots. The bots have been known to run
reliably for days at a time, and any flaky failures that are tolerated on the
bots translate directly into instability of the browser experienced by
customers. Critical bugs in subsystems like WebGL, affecting high-profile
products like Google Maps, have escaped notice in the past because the bots
were unreliable. After much re-work, the GPU bots are now among the most
reliable automated test machines in the Chromium project. Let's keep them that
way.

Flakiness affecting the GPU tests can come in from highly unexpected sources.
Here are some examples:

*   Intermittent pixel_test failures on Linux where the captured pixels were
    black, caused by the Display Power Management System (DPMS) kicking in.
    Disabled the X server's built-in screen saver on the GPU bots in response.
*   GNOME dbus-related deadlocks causing intermittent timeouts ([Issue
    309093](http://crbug.com/309093) and related bugs).
*   Windows Audio system changes causing intermittent assertion failures in the
    browser ([Issue 310838](http://crbug.com/310838)).
*   Enabling assertion failures in the C++ standard library on Linux causing
    random assertion failures ([Issue 328249](http://crbug.com/328249)).
*   V8 bugs causing random crashes of the Maps pixel test (V8 issues
    [3022](https://code.google.com/p/v8/issues/detail?id=3022),
    [3174](https://code.google.com/p/v8/issues/detail?id=3174)).
*   TLS changes causing random browser process crashes ([Issue
    264406](http://crbug.com/264406)).
*   Isolated test execution flakiness caused by failures to reliably clean up
    temporary directories ([Issue 340415](http://crbug.com/340415)).
*   The Telemetry-based WebGL conformance suite caught a bug in the memory
    allocator on Android not caught by any other bot ([Issue
    347919](http://crbug.com/347919)).
*   context_lost test failures caused by the compositor's retry logic ([Issue
    356453](http://crbug.com/356453)).
*   Multiple bugs in Chromium's support for lost contexts causing flakiness of
    the context_lost tests ([Issue 365904](http://crbug.com/365904)).
*   Maps test timeouts caused by Content Security Policy changes in Blink
    ([Issue 395914](http://crbug.com/395914)).
*   Weak pointer assertion failures in various webgl\_conformance\_tests caused
    by changes to the media pipeline ([Issue 399417](http://crbug.com/399417)).
*   A change to a default WebSocket timeout in Telemetry causing intermittent
    failures to run all WebGL conformance tests on the Mac bots ([Issue
    403981](http://crbug.com/403981)).
*   Chrome leaking suspended sub-processes on Windows, apparently a preexisting
    race condition that suddenly showed up ([Issue
    424024](http://crbug.com/424024)).
*   Changes to Chrome's cross-context synchronization primitives causing the
    wrong tiles to be rendered ([Issue 584381](http://crbug.com/584381)).
*   A bug in V8's handling of array literals causing flaky failures of
    texture-related WebGL 2.0 tests ([Issue 606021](http://crbug.com/606021)).
*   Assertion failures in sync point management related to lost contexts that
    exposed a real correctness bug ([Issue 606112](http://crbug.com/606112)).
*   A bug in glibc's `sem_post`/`sem_wait` primitives breaking V8's parallel
    garbage collection ([Issue 609249](http://crbug.com/609249)).
*   A change to Blink's memory purging primitive which caused intermittent
    timeouts of WebGL conformance tests on all platforms ([Issue
    840988](http://crbug.com/840988)).

If you notice flaky test failures either on the GPU waterfalls or try servers,
please file bugs right away with the component Internals>GPU>Testing and
include links to the failing builds and copies of the logs, since the logs
expire after a few days. [GPU pixel wranglers] should give the highest priority
to eliminating flakiness on the tree.

[GPU pixel wranglers]: pixel_wrangling.md
