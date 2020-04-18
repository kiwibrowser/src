# Memory Benchmarks

This document describes benchmarks available to track Chrome's and
WebView's memory usage, where they live, what they measure, how to run them,
and on how to diagnose regressions.

[TOC]

## Glossary

*   **User story:** a set of actions to perform on a browser or device (e.g.
    open google homepage, type "foo", click search, scroll down, visit first
    result, etc.).
*   **Metric:** a data aggregation process that takes a Chrome trace as input
    (produced by a [Telemetry][] run) and produces a set of summary numbers as
    output (e.g. total GPU memory used).
*   **Benchmark:** a combination of (one or more) user stories and (one or
    more) metrics.

[Telemetry]: https://github.com/catapult-project/catapult/blob/master/telemetry/README.md

## System Health

*System health* is an effort to unify top-level benchmarks (as opposite to
micro-benchmarks and regression tests) that are suitable to capture
representative user stories.

### Benchmarks

System health memory benchmarks are:

*   [system_health.memory_mobile][system_health] -
    user stories running on Android devices.
*   [system_health.memory_desktop][system_health] -
    user stories running on desktop platforms.

These benchmarks are run continuously on the [chromium.perf][] waterfall,
collecting and reporting results on the
[Chrome Performance Dashboard][chromeperf].

Other benchmarks maintained by the memory-infra team are discussed in the
[appendix](#Other-benchmarks).

[system_health]: https://chromium.googlesource.com/chromium/src/+/master/tools/perf/page_sets/system_health/
[chromium.perf]: https://build.chromium.org/p/chromium.perf/waterfall
[chromeperf]: https://chromeperf.appspot.com/report

### User stories

System health user stories are classified by the kind of interactions they
perform with the browser:

*   `browse` stories navigate to a URL and interact with the page; e.g.
    scroll, click on elements, navigate to subpages, navigate back.
*   `load` stories just navigate to a URL and wait for the page to
    load.
*   `background` stories navigate to a URL, possibly interact with the
    page, and then bring another app to the foreground (thus pushing the
    browser to the background).
*   `long_running` stories interact with a page for a longer period
    of time (~5 mins).
*   `blank` has a single story that just navigates to **about:blank**.

The full name of a story has the form `{interaction}:{category}:{site}` where:

*   `interaction` is one the labels given above;
*   `category` is used to group together sites with a similar purpose,
    e.g. `news`, `social`, `tools`;
*   `site` is a short name identifying the website in which the story mostly
    takes place, e.g. `cnn`, `facebook`, `gmail`.

For example `browse:news:cnn` and `background:social:facebook` are two system
health user stories.

Today, for most stories a garbage collection is forced at the end of the
story and a memory dump is then triggered. Metrics report the values
obtained from this single measurement.

## Continuous monitoring

![Chrome Performance Dashboard](https://storage.googleapis.com/chromium-docs.appspot.com/79d08f59cf497c761f7099ea427704c14e9afc03.png)

To view data from one of the benchmarks on the
[Chrome Performance Dashboard][chromeperf] you should select:

*   **Test suite:** The name of a *[benchmark](#Benchmarks)*.
*   **Bot:** The name of a *platform or device configuration*. Sign in to also
    see internal bots.
*   **Subtest (1):** The name of a *[metric](#Understanding-memory-metrics)*.
*   **Subtest (2):** The name of a *story group*; these have the form
    `{interaction}_{category}` for system health stories.
*   **Subtest (3):** The name of a *[user story](#User-stories)*
    (with `:` replaced by `_`).

If you are investigating a Perf dashboard alert and would like to see the
details, you can click on any point of the graph. It gives you the commit range,
buildbot output and a link to the trace file taken during the buildbot run.
(More information about reading trace files [here][memory-infra])

[memory-infra]: /docs/memory-infra/README.md

![Chrome Performance Dashboard Alert](https://storage.googleapis.com/chromium-docs.appspot.com/perfdashboard_alert.png)

## How to run the benchmarks

Benchmarks may be run on a local platform/device or remotely on a try job.

### How to run locally

After building, e.g. `ChromePublic.apk`, you can run a specific system health
story with the command:

```
$SRC/tools/perf/run_benchmark run system_health.memory_mobile \
    --browser android-chromium --story-filter load:search:google
```

This will run the story with a default of 3 repetitions and produce a
`results.html` file comparing results from this and any previous benchmark
runs. In addition, you'll also get individual [trace files][memory-infra]
for each story run by the benchmark. **Note:** by default only high level
metrics are shown, you may need to tick the "Show all" check box in order to
view some of the lower level memory metrics.

![Example results.html file](https://storage.googleapis.com/chromium-docs.appspot.com/ea60207d9bb4809178fe75923d6d1a2b241170ef.png)

Other useful options for this command are:

*   `--pageset-repeat [n]` - override the default number of repetitions
*   `--reset-results` - clear results from any previous benchmark runs in the
    `results.html` file.
*   `--results-label [label]` - give meaningful names to your benchmark runs,
    this way it is easier to compare them.

For WebView make sure to [replace the system WebView][webview_install]
on your device and use `--browser android-webview`.

[memory-infra]: /docs/memory-infra/README.md
[webview_install]: https://www.chromium.org/developers/how-tos/build-instructions-android-webview

### How to run a try job

Given a patch on a chromium checkout, try jobs provide a convenient way to
evaluate its memory implications on devices or platforms which
may not be immediately available to developers.

To start a try job [upload a CL][contributing] and run the command, e.g.:

```
$SRC/tools/perf/run_benchmark try android-nexus5 system_health.memory_mobile
```

This will run all of the system health stories for you, and conveniently
provide a `results.html` file comparing measurements with/without your patch.
Options like `--story-filter` and `--pageset-repeat` may also be passed to
this command.

To see the full list of available try bots run the command:

```
$SRC/tools/perf/run_benchmark try list
```

[contributing]: https://www.chromium.org/developers/contributing-code

## Understanding memory metrics

There is a large number of [memory-infra][] metrics, breaking down usage
attributed to different components and processes.

![memory-infra metrics](https://storage.googleapis.com/chromium-docs.appspot.com/a73239c6367ed0f844500e51ce1e04556cb99b4f.png)

Most memory metrics have the form
`memory:{browser}:{processes}:{source}:{component}:{kind}`
where:

*   **browser:** One of `chrome` or `webview`.
*   **processess:** One of `browser_process`, `renderer_processess`,
    `gpu_process`, or `all_processess`.
*   **source:** One of `reported_by_chrome` or `reported_by_os`
*   **component:** May be a Chrome component, e.g. `skia` or `sqlite`;
    details about a specific component, e.g. `v8:heap`; or a class of memory
    as seen by the OS, e.g. `system_memory:native_heap` or `gpu_memory`. If
    reported by chrome, the metrics are gathered by `MemoryDumpProvider`s,
    probes placed in the specific components' codebase. For example, in
    "memory:chrome:all_processes:reported_by_chrome:net:effective_size_avg,"
    the component is "net" which is Chrome's network stack and
    "reported_by_chrome" means that this metric is gathered via probes in
    the network stack.
*   **kind:** The kind of memory being reported. For metrics reported by
    Chrome this usually is `effective_size` (others are `locked_size`
    and `allocated_objects_size`); for metrics by the OS this usually is
    `proportional_resident_size` (others are `peak_resident_size` and
    `private_dirty_size`).

[memory-infra]: /docs/memory-infra/README.md

## Appendix

There are a few other benchmarks maintained by the memory-infra team.
These also use the same set of metrics as system health, but have differences
on the kind of stories that they run.

### memory.top_10_mobile

The [memory.top_10_mobile][memory_py] benchmark is in the process of being deprecated
in favor of system health benchmarks. This process, however, hasn't been
finalized and currently they are still the reference benchmark used for
decision making in the Android release process. Therefore, **it is important
to diagnose and fix regressions caught by this benchmark**.

The benchmark's work flow is:

- Cycle between:

  - load a page on Chrome, wait for it to load, [force garbage collection
    and measure memory][measure];
  - push Chrome to the background, force garbage collection and measure
    memory again.

- Repeat for each of 10 pages *without closing the browser*.

- Close the browser, re-open and repeat the full page set a total of 5 times.

- Story groups are either `foreground` or `background` depending on the state
  of the browser at the time of measurement.

The main difference to watch out between this and system health benchmarks is
that, since a single browser instance is kept open and shared by many
individual stories, they are not independent of each other. In particular, **do
not use the `--story-filter` argument when trying to reproduce regressions**
on these benchmarks, as doing so will affect the results.

[memory_py]: https://cs.chromium.org/chromium/src/tools/perf/benchmarks/memory.py
[measure]: https://github.com/catapult-project/catapult/blob/master/telemetry/telemetry/internal/actions/action_runner.py#L133

### Dual browser benchmarks

Dual browser benchmarks are intended to assess the memory implications of
shared resources between Chrome and WebView.

*   [memory.dual_browser_test][memory_extra_py] - cycle between doing Google
    searches on a WebView-based browser (a stand-in for the Google Search app)
    and loading pages on Chrome. Runs on Android devices only.

    Story groups are either `on_chrome` or `on_webview`, indicating the browser
    in foreground at the moment when the memory measurement was made.

*   [memory.long_running_dual_browser_test][memory_extra_py] - same as above,
    but the test is run for 60 iterations keeping both browsers alive for the
    whole duration of the test and without forcing garbage collection. Intended
    as a last-resort net to catch memory leaks not apparent on shorter tests.

[memory_extra_py]: https://cs.chromium.org/chromium/src/tools/perf/contrib/memory_extras/memory_extras.py
