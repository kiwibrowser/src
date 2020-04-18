# GPU Bot Details

This page describes in detail how the GPU bots are set up, which files affect
their configuration, and how to both modify their behavior and add new bots.

[TOC]

## Overview of the GPU bots' setup

Chromium's GPU bots, compared to the majority of the project's test machines,
are physical pieces of hardware. When end users run the Chrome browser, they
are almost surely running it on a physical piece of hardware with a real
graphics processor. There are some portions of the code base which simply can
not be exercised by running the browser in a virtual machine, or on a software
implementation of the underlying graphics libraries. The GPU bots were
developed and deployed in order to cover these code paths, and avoid
regressions that are otherwise inevitable in a project the size of the Chromium
browser.

The GPU bots are utilized on the [chromium.gpu] and [chromium.gpu.fyi]
waterfalls, and various tryservers, as described in [Using the GPU Bots].

[chromium.gpu]: https://ci.chromium.org/p/chromium/g/chromium.gpu/console
[chromium.gpu.fyi]: https://ci.chromium.org/p/chromium/g/chromium.gpu.fyi/console
[Using the GPU Bots]: gpu_testing.md#Using-the-GPU-Bots

All of the physical hardware for the bots lives in the Swarming pool, and most
of it in the Chrome-GPU Swarming pool. The waterfall bots are simply virtual
machines which spawn Swarming tasks with the appropriate tags to get them to run
on the desired GPU and operating system type. So, for example, the [Win10
Release (NVIDIA)] bot is actually a virtual machine which spawns all of its jobs
with the Swarming parameters:

[Win10 Release (NVIDIA)]: https://ci.chromium.org/buildbot/chromium.gpu/Win10%20Release%20%28NVIDIA%29/?limit=200

```json
{
    "gpu": "10de:1cb3-23.21.13.8816",
    "os": "Windows-10",
    "pool": "Chrome-GPU"
}
```

Since the GPUs in the Swarming pool are mostly homogeneous, this is sufficient
to target the pool of Windows 10-like NVIDIA machines. (There are a few Windows
7-like NVIDIA bots in the pool, which necessitates the OS specifier.)

Details about the bots can be found on [chromium-swarm.appspot.com] and by
using `src/tools/swarming_client/swarming.py`, for example `swarming.py bots`.
If you are authenticated with @google.com credentials you will be able to make
queries of the bots and see, for example, which GPUs are available.

[chromium-swarm.appspot.com]: https://chromium-swarm.appspot.com/

The waterfall bots run tests on a single GPU type in order to make it easier to
see regressions or flakiness that affect only a certain type of GPU.

The tryservers like `win_chromium_rel_ng` which include GPU tests, on the other
hand, run tests on more than one GPU type. As of this writing, the Windows
tryservers ran tests on NVIDIA and AMD GPUs; the Mac tryservers ran tests on
Intel and NVIDIA GPUs. The way these tryservers' tests are specified is simply
by *mirroring* how one or more waterfall bots work. This is an inherent
property of the [`chromium_trybot` recipe][chromium_trybot.py], which was designed to eliminate
differences in behavior between the tryservers and waterfall bots. Since the
tryservers mirror waterfall bots, if the waterfall bot is working, the
tryserver must almost inherently be working as well.

[chromium_trybot.py]: https://chromium.googlesource.com/chromium/tools/build/+/master/scripts/slave/recipes/chromium_trybot.py

There are a few one-off GPU configurations on the waterfall where the tests are
run locally on physical hardware, rather than via Swarming. A few examples are:

<!-- XXX: update this list -->
*   [Mac Pro Release (AMD)](https://luci-milo.appspot.com/buildbot/chromium.gpu.fyi/Mac%20Pro%20Release%20%28AMD%29/)
*   [Mac Pro Debug (AMD)](https://luci-milo.appspot.com/buildbot/chromium.gpu.fyi/Mac%20Pro%20Debug%20%28AMD%29/)
*   [Linux Release (Intel HD 630)](https://luci-milo.appspot.com/buildbot/chromium.gpu.fyi/Linux%20Release%20%28Intel%20HD%20630%29/)
*   [Linux Release (AMD R7 240)](https://luci-milo.appspot.com/buildbot/chromium.gpu.fyi/Linux%20Release%20%28AMD%20R7%20240%29/)

There are a couple of reasons to continue to support running tests on a
specific machine: it might be too expensive to deploy the required multiple
copies of said hardware, or the configuration might not be reliable enough to
begin scaling it up.

## Adding a new isolated test to the bots

Adding a new test step to the bots requires that the test run via an isolate.
Isolates describe both the binary and data dependencies of an executable, and
are the underpinning of how the Swarming system works. See the [LUCI wiki] for
background on Isolates and Swarming.

<!-- XXX: broken link -->
[LUCI wiki]: https://github.com/luci/luci-py/wiki

### Adding a new isolate

1.  Define your target using the `template("test")` template in
    [`src/testing/test.gni`][testing/test.gni]. See `test("gl_tests")` in
    [`src/gpu/BUILD.gn`][gpu/BUILD.gn] for an example. For a more complex
    example which invokes a series of scripts which finally launches the
    browser, see [`src/chrome/telemetry_gpu_test.isolate`][telemetry_gpu_test.isolate].
2.  Add an entry to [`src/testing/buildbot/gn_isolate_map.pyl`][gn_isolate_map.pyl] that refers to
    your target. Find a similar target to yours in order to determine the
    `type`. The type is referenced in [`src/tools/mb/mb_config.pyl`][mb_config.pyl].

[testing/test.gni]:           https://chromium.googlesource.com/chromium/src/+/master/testing/test.gni
[gpu/BUILD.gn]:               https://chromium.googlesource.com/chromium/src/+/master/gpu/BUILD.gn
<!-- XXX: broken link -->
[telemetry_gpu_test.isolate]: https://chromium.googlesource.com/chromium/src/+/master/chrome/telemetry_gpu_test.isolate
[gn_isolate_map.pyl]:         https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/gn_isolate_map.pyl
[mb_config.pyl]:              https://chromium.googlesource.com/chromium/src/+/master/tools/mb/mb_config.pyl

At this point you can build and upload your isolate to the isolate server.

See [Isolated Testing for SWEs] for the most up-to-date instructions. These
instructions are a copy which show how to run an isolate that's been uploaded
to the isolate server on your local machine rather than on Swarming.

[Isolated Testing for SWEs]: https://www.chromium.org/developers/testing/isolated-testing/for-swes

If `cd`'d into `src/`:

1.  `./tools/mb/mb.py isolate //out/Release [target name]`
    *   For example: `./tools/mb/mb.py isolate //out/Release angle_end2end_tests`
1.  `python tools/swarming_client/isolate.py batcharchive -I https://isolateserver.appspot.com out/Release/[target name].isolated.gen.json`
    *   For example: `python tools/swarming_client/isolate.py batcharchive -I https://isolateserver.appspot.com out/Release/angle_end2end_tests.isolated.gen.json`
1.  This will write a hash to stdout. You can run it via:
    `python tools/swarming_client/run_isolated.py -I https://isolateserver.appspot.com -s [HASH] -- [any additional args for the isolate]`

See the section below on [isolate server credentials](#Isolate-server-credentials).

### Adding your new isolate to the tests that are run on the bots

See [Adding new steps to the GPU bots] for details on this process.

[Adding new steps to the GPU bots]: gpu_testing.md#Adding-new-steps-to-the-GPU-Bots

## Relevant files that control the operation of the GPU bots

In the [tools/build] workspace:

*   [masters/master.chromium.gpu] and [masters/master.chromium.gpu.fyi]:
    *   builders.pyl in these two directories defines the bots that show up on
        the waterfall. If you are adding a new bot, you need to add it to
        builders.pyl and use go/bug-a-trooper to request a restart of either
        master.chromium.gpu or master.chromium.gpu.fyi.
    *   Only changes under masters/ require a waterfall restart. All other
        changes – for example, to scripts/slave/ in this workspace, or the
        Chromium workspace – do not require a master restart (and go live the
        minute they are committed).
*   `scripts/slave/recipe_modules/chromium_tests/`:
    *   <code>[chromium_gpu.py]</code> and
        <code>[chromium_gpu_fyi.py]</code> define the following for
        each builder and tester:
        *   How the workspace is checked out (e.g., this is where top-of-tree
            ANGLE is specified)
        *   The build configuration (e.g., this is where 32-bit vs. 64-bit is
            specified)
        *   Various gclient defines (like compiling in the hardware-accelerated
            video codecs, and enabling compilation of certain tests, like the
            dEQP tests, that can't be built on all of the Chromium builders)
        *   Note that the GN configuration of the bots is also controlled by
            <code>[mb_config.pyl]</code> in the Chromium workspace; see below.
    *   <code>[trybots.py]</code> defines how try bots *mirror* one or more
        waterfall bots.
        *   The concept of try bots mirroring waterfall bots ensures there are
            no differences in behavior between the waterfall bots and the try
            bots. This helps ensure that a CL will not pass the commit queue
            and then break on the waterfall.
        *   This file defines the behavior of the following GPU-related try
            bots:
            *   `linux_chromium_rel_ng`, `mac_chromium_rel_ng`, and
                `win_chromium_rel_ng`, which run against every Chromium CL, and
                which mirror the behavior of bots on the chromium.gpu
                waterfall.
            *   The ANGLE try bots, which run against ANGLE CLs, and mirror the
                behavior of the chromium.gpu.fyi waterfall (including using
                top-of-tree ANGLE, and running additional tests not run by the
                regular Chromium try bots)
           *   The optional GPU try servers `linux_optional_gpu_tests_rel`,
               `mac_optional_gpu_tests_rel` and
               `win_optional_gpu_tests_rel`, which are triggered manually and
               run some tests which can't be run on the regular Chromium try
               servers mainly due to lack of hardware capacity.

[tools/build]:                     https://chromium.googlesource.com/chromium/tools/build/
[masters/master.chromium.gpu]:     https://chromium.googlesource.com/chromium/tools/build/+/master/masters/master.chromium.gpu/
[masters/master.chromium.gpu.fyi]: https://chromium.googlesource.com/chromium/tools/build/+/master/masters/master.chromium.gpu.fyi/
[chromium_gpu.py]:                 https://chromium.googlesource.com/chromium/tools/build/+/master/scripts/slave/recipe_modules/chromium_tests/chromium_gpu.py
[chromium_gpu_fyi.py]:             https://chromium.googlesource.com/chromium/tools/build/+/master/scripts/slave/recipe_modules/chromium_tests/chromium_gpu_fyi.py
[trybots.py]:                      https://chromium.googlesource.com/chromium/tools/build/+/master/scripts/slave/recipe_modules/chromium_tests/trybots.py

In the [chromium/src] workspace:

*   [src/testing/buildbot]:
    *   <code>[chromium.gpu.json]</code> and
        <code>[chromium.gpu.fyi.json]</code> define which steps are run on
        which bots. These files are autogenerated. Don't modify them directly!
    *   <code>[gn_isolate_map.pyl]</code> defines all of the isolates' behavior in the GN
        build.
*   [`src/tools/mb/mb_config.pyl`][mb_config.pyl]
    *   Defines the GN arguments for all of the bots.
*   [`src/content/test/gpu/generate_buildbot_json.py`][generate_buildbot_json.py]
    *   The generator script for `chromium.gpu.json` and
        `chromium.gpu.fyi.json`. It defines on which GPUs various tests run.
    *   It's completely self-contained and should hopefully be fairly
        comprehensible.
    *   When modifying this script, don't forget to also run it, to regenerate
        the JSON files.
    *   See [Adding new steps to the GPU bots] for more details.

[chromium/src]:              https://chromium.googlesource.com/chromium/src/
[src/testing/buildbot]:      https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot
[chromium.gpu.json]:         https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/chromium.gpu.json
[chromium.gpu.fyi.json]:     https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/chromium.gpu.fyi.json
[gn_isolate_map.pyl]:        https://chromium.googlesource.com/chromium/src/+/master/testing/buildbot/gn_isolate_map.pyl
[mb_config.pyl]:             https://chromium.googlesource.com/chromium/src/+/master/tools/mb/mb_config.pyl
[generate_buildbot_json.py]: https://chromium.googlesource.com/chromium/src/+/master/content/test/gpu/generate_buildbot_json.py

In the [infradata/config] workspace (Google internal only, sorry):

*   [configs/chromium-swarm/bots.cfg]
    *   Defines a `Chrome-GPU` Swarming pool which contains most of the
        specialized hardware: as of this writing, the Windows and Linux NVIDIA
        bots, the Windows AMD bots, and the MacBook Pros with NVIDIA and AMD
        GPUs. New GPU hardware should be added to this pool.

[infradata/config]:                https://chrome-internal.googlesource.com/infradata/config
[configs/chromium-swarm/bots.cfg]: https://chrome-internal.googlesource.com/infradata/config/+/master/configs/chromium-swarm/bots.cfg

## Walkthroughs of various maintenance scenarios

This section describes various common scenarios that might arise when
maintaining the GPU bots, and how they'd be addressed.

### How to add a new test or an entire new step to the bots

This is described in [Adding new tests to the GPU bots].

[Adding new tests to the GPU bots]: https://www.chromium.org/developers/testing/gpu-testing/#TOC-Adding-New-Tests-to-the-GPU-Bots

### How to add a new tester bot to the chromium.gpu.fyi waterfall

When deploying a new GPU configuration, it should be added to the
chromium.gpu.fyi waterfall first. The chromium.gpu waterfall should be reserved
for those GPUs which are tested on the commit queue. (Some of the bots violate
this rule – namely, the Debug bots – though we should strive to eliminate these
differences.) Once the new configuration is ready to be fully deployed on
tryservers, bots can be added to the chromium.gpu waterfall, and the tryservers
changed to mirror them.

In order to add Release and Debug waterfall bots for a new configuration,
experience has shown that at least 4 physical machines are needed in the
swarming pool. The reason is that the tests all run in parallel on the Swarming
cluster, so the load induced on the swarming bots is higher than it would be
if the tests were run strictly serially.

With these prerequisites, these are the steps to add a new (swarmed) tester bot.
(Actually, pair of bots -- Release and Debug. If deploying just one or the
other, ignore the other configuration.) These instructions assume that you are
reusing one of the existing builders, like [`GPU FYI Win Builder`][GPU FYI Win
Builder].

1.  Work with the Chrome Infrastructure Labs team to get the (minimum 4)
    physical machines added to the Swarming pool. Use
    [chromium-swarm.appspot.com] or `src/tools/swarming_client/swarming.py bots`
    to determine the PCI IDs of the GPUs in the bots. (These instructions will
    need to be updated for Android bots which don't have PCI buses.)

    1.  Make sure to add these new machines to the Chrome-GPU Swarming pool by
        creating a CL against [`configs/chromium-swarm/bots.cfg`][bots.cfg] in
        the [infradata/config] (Google internal) workspace. Git configure your
        user.email to @google.com if necessary. Here is an [example
        CL](https://chrome-internal-review.googlesource.com/524420).

1.  File a Chrome Infrastructure Labs ticket requesting 2 virtual machines for
    the testers. These need to match the OS of the physical machines and
    builders. For example, if you're adding a "Windows 7 CoolNewGPUType" tester,
    you'll need 2 Windows VMs. See this [example
    ticket](http://crbug.com/838975).

1.  Once the VMs are ready, create a CL in the
    [`infradata/config`][infradata/config] (Google internal) workspace which
    does the following. Git configure your user.email to @google.com if
    necessary. Here's an [example
    CL](https://chrome-internal-review.googlesource.com/619497).
    1.  Adds two new "bot_group" blocks in the Chromium GPU FYI section of
        [`configs/chromium-swarm/bots.cfg`][bots.cfg], one for the Release bot
        and one for the Debug bot. Copy the closest configuration you can find
        -- for example, Windows, Android, etc.
    1.  Get this reviewed and landed. This step associates the VM with the bot's
        name on the waterfall.

1.  Create a CL in the Chromium workspace which does the following. Here's an
    [example CL](https://chromium-review.googlesource.com/1041164).
    1.  Adds the new machines to
        `src/content/test/gpu/generate_buildbot_json.py`.
        1.  The swarming dimensions are crucial. These must match the GPU and
            OS type of the physical hardware in the Swarming pool. This is what
            causes the VMs to spawn their tests on the correct hardware. Make
            sure to use the Chrome-GPU pool, and that the new machines were
            specifically added to that pool.
        1.  Make sure to set the `swarming` property to `True` for both the
            Release and Debug bots.
        1.  Make triply sure that there are no collisions between the new
            hardware you're adding and hardware already in the Swarming pool.
            For example, it used to be the case that all of the Windows NVIDIA
            bots ran the same OS version. Later, the Windows 8 flavor bots were
            added. In order to avoid accidentally running tests on Windows 8
            when Windows 7 was intended, the OS in the swarming dimensions of
            the Win7 bots had to be changed from `win` to
            `Windows-2008ServerR2-SP1` (the Win7-like flavor running in our
            data center). Similarly, the Win8 bots had to have a very precise
            OS description (`Windows-2012ServerR2-SP0`).
        1.  If you're deploying a new bot that's similar to another existing
            configuration, please search around in the file for references to
            the other bot's name and see if your new bot needs to be added to
            any exclusion lists. For example, some of the tests don't run on
            certain Win bots because of missing OpenGL extensions.
        1.  Run this script to regenerate
            `src/testing/buildbot/chromium.gpu.fyi.json`.
    1. Updates [`cr-buildbucket.cfg`][cr-buildbucket.cfg]:
        *   Add the two new machines (Release and Debug) inside the
            luci.chromium.ci bucket. This sets up storage for the builds in the
            system. Use the appropriate mixin; for example, "win-gpu-fyi-ci" has
            already been set up for Windows GPU FYI bots on the waterfall.
    1. Updates [`luci-scheduler.cfg`][luci-scheduler.cfg]:
        *   Add new "job" blocks for your new Release and Debug test bots. They
            should go underneath the builder which triggers them (like "GPU Win
            FYI Builder"), in alphabetical order. Make sure the "id" and
            "builer" entries match. This job block should use the acl_sets
            "triggered-by-parent-builders", because it's triggered by the
            builder, and not by changes to the git repository.
    1. Updates [`luci-milo.cfg`][luci-milo.cfg]:
        *   Add new "builders" blocks for your new testers (Release and Debug)
            on the [`chromium.gpu.fyi`][chromium.gpu.fyi] console. Look at the
            short names and categories and try to come up with a reasonable
            organization.
    1.  If you were adding a new builder, you would need to also add the new
        machine to [`src/tools/mb/mb_config.pyl`][mb_config.pyl].

1. After the Chromium-side CL lands it will take some time for all of
   the configuration changes to be picked up by the system. The bot
   will probably be in a red or purple state, claiming that it can't
   find its configuration. (It might also be in an "empty" state, not
   running any jobs at all.)

1. *After* the Chromium-side CL lands and the bot is on the console, create a CL
   in the [`tools/build`][tools/build] workspace which does the
   following. Here's an [example
   CL](https://chromium-review.googlesource.com/1041145).
    1.  Adds the new VMs to [`chromium_gpu_fyi.py`][chromium_gpu_fyi.py] in
        `scripts/slave/recipe_modules/chromium_tests/`. Make sure to set the
        `serialize_tests` property to `True`. This is specified for waterfall
        bots, but not trybots, and helps avoid overloading the physical
        hardware. Double-check the `BUILD_CONFIG` and `parent_buildername`
        properties for each. They must match the Release/Debug flavor of the
        builder, like `GPU FYI Win Builder` vs. `GPU FYI Win Builder (dbg)`.
    1.  Get this reviewed and landed. This step tells the Chromium recipe about
        the newly-deployed waterfall bot, so it knows which JSON file to load
        out of src/testing/buildbot and which entry to look at.
    1.  It used to be necessary to retrain recipe expectations
        (`scripts/slave/recipes.py --use-bootstrap test train`). This doesn't
        appear to be necessary any more, but it's something to watch out for if
        your CL fails presubmit for some reason.

1. Note that it is crucial that the bot be deployed before hooking it up in the
   tools/build workspace. In the new LUCI world, if the parent builder can't
   find its child testers to trigger, that's a hard error on the parent. This
   will cause the builders to fail. You can and should prepare the tools/build
   CL in advance, but make sure it doesn't land until the bot's on the console.

[bots.cfg]:            https://chrome-internal.googlesource.com/infradata/config/+/master/configs/chromium-swarm/bots.cfg
[infradata/config]:    https://chrome-internal.googlesource.com/infradata/config/
[cr-buildbucket.cfg]:  https://chromium.googlesource.com/chromium/src/+/master/infra/config/global/cr-buildbucket.cfg
[luci-milo.cfg]:  https://chromium.googlesource.com/chromium/src/+/master/infra/config/global/luci-milo.cfg
[luci-scheduler.cfg]:  https://chromium.googlesource.com/chromium/src/+/master/infra/config/global/luci-scheduler.cfg
[GPU FYI Win Builder]: https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/GPU%20FYI%20Win%20Builder

### How to start running tests on a new GPU type on an existing try bot

Let's say that you want to cause the `win_chromium_rel_ng` try bot to run tests
on CoolNewGPUType in addition to the types it currently runs (as of this
writing, NVIDIA and AMD). To do this:

1.  Make sure there is enough hardware capacity. Unfortunately, tools to report
    utilization of the Swarming pool are still being developed, but a
    back-of-the-envelope estimate is that you will need a minimum of 30
    machines in the Swarming pool to run the current set of GPU tests on the
    tryservers. We estimate that 90 machines will be needed in order to
    additionally run the WebGL 2.0 conformance tests. Plan for the larger
    capacity, as it's desired to run the larger test suite on as many
    configurations as possible.
2.  Deploy Release and Debug testers on the chromium.gpu waterfall, following
    the instructions for the chromium.gpu.fyi waterfall above. You will also
    need to temporarily add suppressions to
    [`tests/masters_recipes_test.py`][tests/masters_recipes_test.py] for these
    new testers since they aren't yet covered by try bots and are going on a
    non-FYI waterfall. Make sure these run green for a day or two before
    proceeding.
3.  Create a CL in the tools/build workspace, adding the new Release tester
    to `win_chromium_rel_ng`'s `bot_ids` list
    in `scripts/slave/recipe_modules/chromium_tests/trybots.py`. Rerun
    `scripts/slave/recipes.py --use-bootstrap test train`.
4.  Once the CL in (3) lands, the commit queue will **immediately** start
    running tests on the CoolNewGPUType configuration. Be vigilant and make
    sure that tryjobs are green. If they are red for any reason, revert the CL
    and figure out offline what went wrong.

[tests/masters_recipes_test.py]: https://chromium.googlesource.com/chromium/tools/build/+/master/tests/masters_recipes_test.py

### How to add a new manually-triggered trybot

There are a lot of one-off GPU types on the chromium.gpu.fyi waterfall and
sometimes a failure happens just on one type. It's helpful to just be able to
send a tryjob to a particular machine. Doing so requires a specific trybot to be
set up because most if not all of the existing trybots trigger tests on more
than one type of GPU.

Here are the steps to set up a new trybot which runs tests just on one
particular GPU type. Let's consider that we are adding a manually-triggered
trybot for the Win7 NVIDIA GPUs in Release mode. We will call the new bot
`gpu_manual_try_win7_nvidia_rel`.

1.  File a Chrome Infrastructure Labs ticket requesting ~3 virtual
    machines. These will do builds and trigger jobs on the physical hardware,
    and need to match the OS of the physical machines. See this [example
    ticket](http://crbug.com/839216).

1.  Once the VMs are ready, create a CL in the
    [`infradata/config`][infradata/config] (Google internal) workspace which
    does the following. Git configure your user.email to @google.com if
    necessary. Here's an [example
    CL](https://chrome-internal-review.googlesource.com/620773).
    1.  Adds a new "bot_group" block in the "manually-triggered GPU trybots"
        section of [`configs/chromium-swarm/bots.cfg`][bots.cfg]. Look in the
        optional GPU tryserver section for the closest configuration you can
        find to copy from -- for example, Windows, Android,
        etc. (win_optional_gpu_tests_rel, android_optional_gpu_tests_rel). The
        "dimensions" tag contains the name of the trybot,
        e.g. "builder:gpu_manual_try_win7_nvidia_rel".
    1.  Get this reviewed and landed. This step makes these machines the ones
        which perform the builds for this new trybot.

1.  Create a CL in the Chromium workspace which does the following. Here's an
    [example CL](https://chromium-review.googlesource.com/1044767).
    1.  Updates [`cr-buildbucket.cfg`][cr-buildbucket.cfg]:
        *   Add the new trybot to the `luci.chromium.try` bucket. This is a
            one-liner, with "name" being "gpu_manual_try_win7_nvidia_rel" and
            "mixins" being the OS-appropriate mixin, in this case
            "win-optional-gpu-try". (We're repurposing the existing ACLs for the
            "optional" GPU trybots for these manually-triggered ones.)
    1.  Updates [`luci-milo.cfg`][luci-milo.cfg]:
        *   Add "builders" blocks for the new trybot to the `luci.chromium.try` and
            `tryserver.chromium.win` consoles.
    1.  Adds the new trybot to
        [`src/tools/mb/mb_config.pyl`][mb_config.pyl]. Reuse the same mixin as
        for the optional GPU trybot; in this case,
        `gpu_fyi_tests_release_trybot_x86`.
    1.  Get this CL reviewed and landed.

1. Create a CL in the [`tools/build`][tools/build] workspace which does the
   following. Here's an [example
   CL](https://chromium-review.googlesource.com/1044761).

    1.  Adds the new trybot to a "Manually-triggered GPU trybots" section in
        `scripts/slave/recipe_modules/chromium_tests/trybots.py`. Create this
        section after the "Optional GPU bots" section for the appropriate
        tryserver (`tryserver.chromium.win`, `tryserver.chromium.mac`,
        `tryserver.chromium.linux`, `tryserver.chromium.android`). Have the bot
        mirror the appropriate waterfall bot; in this case, the buildername to
        mirror is `GPU FYI Win Builder` and the tester is `Win7 FYI Release
        (NVIDIA)`.
    1.  Adds an exception for your new trybot in `tests/masters_recipes_test.py`,
        under `FAKE_BUILDERS`, under the appropriate tryserver waterfall (in
        this case, `master.tryserver.chromium.win`). This is because this is a
        LUCI-only bot, and this test verifies the old buildbot configurations.
    1.  Get this reviewed and landed. This step tells the Chromium recipe about
        the newly-deployed trybot, so it knows which JSON file to load out of
        src/testing/buildbot and which entry to look at to understand which
        tests to run and on what physical hardware.
    1.  It used to be necessary to retrain recipe expectations
        (`scripts/slave/recipes.py --use-bootstrap test train`). This doesn't
        appear to be necessary any more, but it's something to watch out for if
        your CL fails presubmit for some reason.

1.  Now we need to add the new trybot to the Gerrit UI. This is most easily done
    using the Gerrit UI itself. (If on any CL you select "Choose Tryjobs", it
    says "Don't see the bots you want? Edit this repo's buildbucket.config to
    add them". That's the file we are going to edit.) Here's an [example
    CL](https://chromium-review.googlesource.com/1044866).
    1.  Go to the [`chromium/src`][chromium/src] repo in the Gerrit UI.
    1.  Click "Repo settings" in the upper-left corner.
    1.  Click "Commands".
    1.  Click the "Edit repo config" button.
    1.  This opens the project config by default. You don't want this, so close
        it using the "CLOSE" link at the upper right.
    1.  Now you're in a CL titled "Edit Repo Config". Click the "OPEN" link.
    1.  It will prompt you to open a file. Begin typing `buildbucket.config` and
        it will auto-complete. Click "Open".
    1.  Add the new trybot, in this case `gpu_manual_try_win7_nvidia_rel`, to
        the `luci.chromium.try` bucket. *BE CAREFUL* to include the leading tab;
        it is semantically important. (Note that this matches the "pool"
        dimension specified in bots.cfg in the infradata/config workspace.)
    1.  Click "Save", and then "Close" (once "Save" is grayed out).
    1.  You're now back at the CL. Click "PUBLISH EDIT" near the top right.
    1.  Now you're in normal CL mode again. You can now click the "Edit" button
        to edit the CL description; please do this.
    1.  Send this out to one of the Git admins; they're listed in the gitadmin
        column in [`go/chromecals`][go/chromecals]. The Git admin has to both +1
        AND land the CL.

At this point the new trybot should show up in the Gerrit UI and it should be
possible to send a CL to it.

[chromium/src]:  https://chromium-review.googlesource.com/q/project:chromium%252Fsrc+status:open
[go/chromecals]: http://go/chromecals


### How to add a new "optional" try bot

TODO(kbr): the naming of the "optional" try bots is confusing and
unfortunate. They should probably be renamed to something like "extratests" or
"extra_tests", so perhaps a new naming convention of "gpu_win_extratests_rel" or
"win_gpu_extratests_rel". Unfortunately making this change at this point
requires touching tons of files across many workspaces and is unlikely to happen
unless someone highly motivated wants to pick up the task.

The "optional" GPU try bots are a concession to the reality that there are some
long-running GPU test suites that simply can not run against every Chromium CL.
They run some additional tests that are usually run only on the
chromium.gpu.fyi waterfall. Some of these tests, like the WebGL 2.0 conformance
suite, are intended to be run on the normal try bots once hardware capacity is
available. Some are not intended to ever run on the normal try bots.

The optional try bots are a little different because they mirror waterfall bots
that don't actually exist. The waterfall bots' specifications exist only to
tell the optional try bots which tests to run.

Let's say that you intended to add a new such optional try bot on Windows. Call
it `win_new_optional_tests_rel` for example. Now, if you wanted to just add
this GPU type to the existing `win_optional_gpu_tests_rel` try bot, you'd
just follow the instructions above
([How to start running tests on a new GPU type on an existing try bot](#How-to-start-running-tests-on-a-new-GPU-type-on-an-existing-try-bot)). The steps below describe how to spin up
an entire new optional try bot.

1.  Make sure that you have some swarming capacity for the new GPU type. Since
    it's not running against all Chromium CLs you don't need the recommended 30
    minimum bots, though ~10 would be good.
1.  Create a CL in the Chromium workspace:
    1.  Add your new bot (for example, "Optional Win7 Release
        (CoolNewGPUType)") to the chromium.gpu.fyi waterfall in
        [generate_buildbot_json.py]. (Note, this is a bad example: the
        "optional" bots have special semantics in this script. You'd probably
        want to define some new category of bot if you didn't intend to add
        this to `win_optional_gpu_tests_rel`.) 
    1.  Re-run the script to regenerate the JSON files.
1.  Land the above CL.
1.  Create a CL in the tools/build workspace:
    1.  Modify `masters/master.tryserver.chromium.win`'s [master.cfg] and
        [slaves.cfg] to add the new tryserver. Follow the pattern for the
        existing `win_optional_gpu_tests_rel` tryserver. Namely, add the new
        entry to master.cfg, and add the new tryserver to the
        `optional_builders` list in `slaves.cfg`.
    1.  Modify [`chromium_gpu_fyi.py`][chromium_gpu_fyi.py] to add the new
        "Optional Win7 Release (CoolNewGPUType)" entry.
    1.  Modify [`trybots.py`][trybots.py] to add
        the new `win_new_optional_tests_rel` try bot, mirroring "Optional
        Win7 Release (CoolNewGPUType)".
1.  Land the above CL and request an off-hours restart of the
    tryserver.chromium.win waterfall.
1.  Now you can send CLs to the new bot with:
    `git cl try -m tryserver.chromium.win -b win_new_optional_tests_rel`

[master.cfg]: https://chromium.googlesource.com/chromium/tools/build/+/master/masters/master.tryserver.chromium.win/master.cfg
[slaves.cfg]: https://chromium.googlesource.com/chromium/tools/build/+/master/masters/master.tryserver.chromium.win/slaves.cfg

### How to test and deploy a driver update

Let's say that you want to roll out an update to the graphics drivers on one of
the configurations like the Win7 NVIDIA bots. The responsible way to do this is
to run the new driver on one of the waterfalls for a day or two to make sure
the tests are running reliably green before rolling out the driver update
everywhere. To do this:

1.  Make sure that all of the current Swarming jobs for this OS and GPU
    configuration are targeted at the "stable" version of the driver in
    `src/testing/gpu/generate_buildbot_json.py`.
1.  File a `Build Infrastructure` bug, component `Infra>Labs`, to have ~4 of the
    physical machines already in the Swarming pool upgraded to the new version
    of the driver.
1.  If an "experimental" version of this bot doesn't yet exist, follow the
    instructions above for [How to add a new tester bot to the chromium.gpu.fyi
    waterfall](#How-to-add-a-new-tester-bot-to-the-chromium_gpu_fyi-waterfall)
    to deploy one.
1.  Have this experimental bot target the new version of the driver in
    `src/testing/gpu/generate_buildbot_json.py`.
1.  Hopefully, the new machine will pass the pixel tests. If it doesn't, then
    unfortunately, it'll be necessary to follow the instructions on
    [updating the pixel tests] to temporarily suppress the failures on this
    particular configuration. Keep the time window for these test suppressions
    as narrow as possible.
1.  Watch the new machine for a day or two to make sure it's stable.
1.  When it is, update `src/testing/gpu/generate_buildbot_json.py` to use the
    "gpu trigger script" functionality to select *either* the stable *or* the
    new driver version on the stable version of the bot. See [this
    CL](https://chromium-review.googlesource.com/882344) for an example, though
    that CL was targeting a different OS version rather than driver version.
1.  After that lands, ask the Chrome Infrastructure Labs team to roll out the
    driver update across all of the similarly configured bots in the swarming
    pool.
1.  If necessary, update pixel test expectations and remove the suppressions
    added above.
1.  Remove the alternate swarming dimensions for the stable bot from
    `generate_buildbot_json.py`, locking it to the new driver version.

Note that we leave the experimental bot in place. We could reclaim it, but it
seems worthwhile to continuously test the "next" version of graphics drivers as
well as the current stable ones.

[updating the pixel tests]: https://www.chromium.org/developers/testing/gpu-testing/#TOC-Updating-and-Adding-New-Pixel-Tests-to-the-GPU-Bots

## Credentials for various servers

Working with the GPU bots requires credentials to various services: the isolate
server, the swarming server, and cloud storage.

### Isolate server credentials

To upload and download isolates you must first authenticate to the isolate
server. From a Chromium checkout, run:

*   `./src/tools/swarming_client/auth.py login
    --service=https://isolateserver.appspot.com`

This will open a web browser to complete the authentication flow. A @google.com
email address is required in order to properly authenticate.

To test your authentication, find a hash for a recent isolate. Consult the
instructions on [Running Binaries from the Bots Locally] to find a random hash
from a target like `gl_tests`. Then run the following:

[Running Binaries from the Bots Locally]: https://www.chromium.org/developers/testing/gpu-testing#TOC-Running-Binaries-from-the-Bots-Locally

If authentication succeeded, this will silently download a file called
`delete_me` into the current working directory. If it failed, the script will
report multiple authentication errors. In this case, use the following command
to log out and then try again:

*   `./src/tools/swarming_client/auth.py logout
    --service=https://isolateserver.appspot.com`

### Swarming server credentials

The swarming server uses the same `auth.py` script as the isolate server. You
will need to authenticate if you want to manually download the results of
previous swarming jobs, trigger your own jobs, or run `swarming.py reproduce`
to re-run a remote job on your local workstation. Follow the instructions
above, replacing the service with `https://chromium-swarm.appspot.com`.

### Cloud storage credentials

Authentication to Google Cloud Storage is needed for a couple of reasons:
uploading pixel test results to the cloud, and potentially uploading and
downloading builds as well, at least in Debug mode. Use the copy of gsutil in
`depot_tools/third_party/gsutil/gsutil`, and follow the [Google Cloud Storage
instructions] to authenticate. You must use your @google.com email address and
be a member of the Chrome GPU team in order to receive read-write access to the
appropriate cloud storage buckets. Roughly:

1.  Run `gsutil config`
2.  Copy/paste the URL into your browser
3.  Log in with your @google.com account
4.  Allow the app to access the information it requests
5.  Copy-paste the resulting key back into your Terminal
6.  Press "enter" when prompted for a project-id (i.e., leave it empty)

At this point you should be able to write to the cloud storage bucket.

Navigate to
<https://console.developers.google.com/storage/chromium-gpu-archive> to view
the contents of the cloud storage bucket.

[Google Cloud Storage instructions]: https://developers.google.com/storage/docs/gsutil
