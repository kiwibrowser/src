# chromium.perf Waterfall

## Overview

The [chromium.perf waterfall](http://build.chromium.org/p/chromium.perf/waterfall)
continuously builds and runs our performance tests on real Android, Windows,
Mac, and Linux hardware. Results are reported to the
[Performance Dashboard](https://chromeperf.appspot.com/) for analysis. The
[Perfbot Health Sheriffing Rotation](bot_health_sheriffing/main.md) ensures that the benchmarks stay green. The [Perf Sheriff Rotation](perf_regression_sheriffing.md) ensures that any regressions detected by those benchmarks are addressed quickly. Together, these rotations maintain
[Chrome's Core Principles](https://www.chromium.org/developers/core-principles)
of speed:

> "If you make a change that regresses measured performance, you will be
> required to fix it or revert".

## Contact

  * You can reach the Chromium performance sheriffs at perf-sheriffs@chromium.org.
  * Bugs on waterfall issues should have Component:
    [Speed>Benchmarks>Waterfall](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ASpeed%3EBenchmarks%3EWaterfall+&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids).

## Links

  * [Perf Sheriff Rotation](perf_regression_sheriffing.md)
  * [Perfbot Health Sheriffing Rotation](bot_health_sheriffing/main.md)
  * TODO: Page on how to repro failures locally
  * TODO: Page on how to connect to bot in lab
  * TODO: Page on how to hack on buildbot/recipe code
  * TODO: Page on bringing up new hardware
