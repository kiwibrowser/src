# Power Telemetry tests

[TOC]

## Overview

The Telemetry power benchmarks use BattOr, a small external power monitor, to collect power measurements while Chrome performs various tasks (a.k.a. user stories).

There are currently seven benchmarks that collect power data, grouped together by the type of task during which the power data is collected:

- **`system_health.common_desktop`**: A desktop-only benchmark in which each page focuses on a single, common way in which users use Chrome (e.g. browsing Facebook photos, shopping on Amazon, searching Google)
- **`system_health.common_mobile`**: A mobile-only benchmark that parallels `system_health.common_desktop`
- **`battor.trivial_pages`**: A Mac-only benchmark in which each page focuses on a single, extremely simple behavior (e.g. a blinking cursor, a CSS blur animation)
- **`battor.steady_state`**: A Mac-only benchmark in which each page focuses on a website that Chrome has exhibited pathological idle behavior in the past
- **`media.tough_video_cases_tbmv2`**: A desktop-only benchmark in which each page tests a particular media-related scenario (e.g. playing a 1080p, H264 video with sound)
- **`media.android.tough_video_cases_tbmv2`**: A mobile-only benchmark that parallels `media.tough_video_cases_tbmv2`
- **`power.idle_platform`**: A benchmark that sits idle without starting Chrome for various lengths of time. Used as a debugging benchmark to monitor machine background noise.

Note that these benchmarks are in the process of being consolidated and that there will likely be fewer, larger power benchmarks in the near future.

The legacy power benchmarks consist of:

- **`power.typical_10_mobile`**, which visits ten popular sites and uses Android-specific APIs to measure approximately how much power was consumed. This can't be deleted because it's still used by the Android System Health Council to assess whether Chrome Android is fit for release on hardware configurations for which BattOrs are not yet available.

## Running the tests remotely

If you're just trying to gauge whether your change has caused a power regression, you can do so by [running a benchmark remotely via a perf try job](https://chromium.googlesource.com/chromium/src/+/master/docs/speed/perf_trybots.md).

When you do this, be sure to use a configuration that's equipped with BattOrs:

- `android_nexus5X`
- `android-webview-arm64-aosp`
- `mac-retina`
- `mac-10-11`
- `winx64-high-dpi`

If you're unsure of which benchmark to choose, `system_health.common_[desktop/mobile]` is a safe, broad choice.

## Running the tests locally

For more in-depth analysis and shorter cycle times, it can be helpful to run the tests locally. Because the power benchmarks rely on having a BattOr, you'll need to get one before you can do so. If you're a Googler, you can ask around (many Chrome offices already have a BattOr in them) or request one at [go/battor-request-form](http://go/battor-request-form). If you're external to Google, you can contact the BattOr's manufacturer at <sales@mellowlabs.com>.

Once you have a BattOr, follow the instructions in the [BattOr laptop setup guide](https://docs.google.com/document/d/1UsHc990NRO2MEm5A3b9oRk9o7j7KZ1qftOrJyV1Tr2c/edit) to hook it up to your laptop. If you're using a phone with a BattOr, you'll need to run one USB to micro-USB cable from the host computer triggering the Telemetry tests to the BattOr and another from the host computer to the phone.

Once you've done this, you can start the Telemetry benchmark with:

```
./tools/perf/run_benchmark <benchmark_name> --browser=<browser>
```

where `benchmark_name` is one of the above benchmark names.

## Understanding power metrics

To understand our power metrics, it's important to understand the distinction between power and energy. *Energy* is what makes computers run and is measured in Joules, whereas *power* is the rate at which that energy is used and is measured in Joules per second.

Some of our power metrics measure energy, whereas others measure power. Specifically:

- We measure *energy* when the user cares about whether the task is completed (e.g. "energy required to load a page", "energy required to responsd to a mouse click").
- We measure *power* when the user cares about the power required to continue performing an action (e.g. "power while scrolling", "power while playing a video animation").

The full list of our metrics is as follows:

### Energy metrics
- **`load:energy_sum`**: Total energy used in between page navigations and first meaningful paint on all navigations in the story.
- **`scroll_response:energy_sum`**: Total energy used to respond to all scroll requests in the story.
- **`tap_response:energy_sum`**: Total energy used to respond to all taps in the story.
- **`keyboard_response:energy_sum`**: Total energy used to respond to all key entries in the story.

### Power metrics
- **`story:power_avg`**: Average power over the entire story.
- **`css_animation:power_avg`**: Average power over all CSS animations in the story.
- **`scroll_animation:power_avg`**: Average power over all scroll animations in the story.
- **`touch_animation:power_avg`**: Average power over all touch animations (e.g. finger drags) in the story.
- **`video_animation:power_avg`**: Average power over all videos played in the story.
- **`webgl_animation:power_avg`**: Average power over all WebGL animations in the story.
- **`idle:power_avg`**: Average power over all idle periods in the story.

### Other metrics
- **`cpu_time_percentage_avg`**: Average CPU load over the entire story.

## Adding new power test cases
We're not currently accepting new power stories until we've consolidated the existing ones.
