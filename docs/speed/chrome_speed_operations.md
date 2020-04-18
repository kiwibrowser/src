# Chrome Speed Operations

Chrome Speed Operations exists to prevent Chrome from regressing on
responsiveness, smoothness, memory, power or other key performance metrics.
We seek to integrate a world-class benchmarking framework and performance
monitoring tools into Chrome's release process.

TL: sullivan@chromium.org<br>
TPM: benhenry@chromium.org
 
Speed Operations consists of 3 teams, working in tandem:

## Benchmarks
The benchmarks team provides:
  * A set of [releasing-oriented benchmarks](https://docs.google.com/document/d/1BM_6lBrPzpMNMtcyi2NFKGIzmzIQ1oH3OlNG27kDGNU/edit)
    that measure key user-visible performance metrics in important scenarios.
    We work closely with Chrome Speed Metrics team on the metrics and with
    Releasing team on the scenarios.
  * A set of [opinionated benchmarking frameworks](https://docs.google.com/document/d/1ni2MIeVnlH4bTj4yvEDMVNxgL73PqK_O9_NUm3NW3BA/edit)
    that make it easy for Chromium developers to add lower-level benchmarks for
    the areas of Chrome performance important to them.
  * Automation to run these benchmarks on our continuous build on several dozen
    real device types, on Windows, Mac, Linux, and Android, with hardware power
    monitoring.

TL: nednguyen@chromium.org<br>
Contact: benchmarking-dev@chromium.org

## Services
The [services](chrome_speed_services.md) team provides:
  * The [Chrome performance dashboard](https://chromeperf.appspot.com), which
    stores performance timeseries and related debugging data. The dashboard
    automatically detects regressions in these timeseries and has integration
    with Chromium's bug tracker for easy tracking.
  * Tools for [bisecting regressions](bisects.md) on our continuous build down
    to an exact culprit CL.
  * A [performance try job service](perf_trybots.md) which allows chromium
    developers to run benchmarks on unsubmitted CLs using the same hardware
    we use in the continuous build.

TL: simonhatch@chromium.org<br>
Contact: speed-services-dev@chromium.org

## Releasing
The releasing team provides:
  * Tracking of all performance regressions seen in user-visible performance
    both in the wild and in the lab.
  * A per-milestone report on Chrome's performance.
  * Recommendations about lab hardware and new test scenarios.
  * Management of performance sheriff rotations.

TPM: benhenry@chromium.org