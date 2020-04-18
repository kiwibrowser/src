# Adding a new feature flag in chrome://flags

This document describes how to add your new feature behind a flag.

## Step 1: Adding a new `base::Feature`
This step would be different between where you want to use the flag.
For example, if you want to use the flag in src/content, you should add a base::Feature to the following files:

* [content/public/common/content_features.cc](https://cs.chromium.org/chromium/src/content/public/common/content_features.cc)
* [content/public/common/content_features.h](https://cs.chromium.org/chromium/src/content/public/common/content_features.h)

If you want to use the flag in blink, you should also read
[Runtime Enable Features](https://www.chromium.org/blink/runtime-enabled-features).

You can refer to [this CL](https://chromium-review.googlesource.com/c/554510/)
to see

1. where to add the base::Feature
[[1](https://chromium-review.googlesource.com/c/554510/8/content/public/common/content_features.cc#253)]
[[2](https://chromium-review.googlesource.com/c/554510/8/content/public/common/content_features.h)]
2. how to use it
[[1](https://chromium-review.googlesource.com/c/554510/8/content/common/service_worker/service_worker_utils.cc#153)]
3. how to wire the base::Feature to WebRuntimeFeatures
[[1](https://chromium-review.googlesource.com/c/554510/8/content/child/runtime_features.cc)]
[[2](https://chromium-review.googlesource.com/c/554510/8/third_party/blink/public/platform/web_runtime_features.h)]
[[3](https://chromium-review.googlesource.com/c/554510/third_party/blink/Source/platform/exported/web_runtime_features.cc)]
[[4](https://chromium-review.googlesource.com/c/554510/8/third_party/blink/renderer/platform/runtime_enabled_features.json5)]
4. how to use it in blink
[[1](https://chromium-review.googlesource.com/c/554510/8/third_party/blnk/renderere/core/workers/worker_thread.cc)]

Also, this patch added a virtual test for running layout tests with the flag.
When you add a flag, you can consider to use that.

## Step 2: Adding the feature flag to the chrome://flags UI.
You have to modify these four files in total.

* [chrome/browser/about_flags.cc](https://cs.chromium.org/chromium/src/chrome/browser/about_flags.cc)
* [chrome/browser/flag_descriptions.cc](https://cs.chromium.org/chromium/src/chrome/browser/flag_descriptions.cc)
* [chrome/browser/flag_descriptions.h](https://cs.chromium.org/chromium/src/chrome/browser/flag_descriptions.h)
* [tools/metrics/histograms/enums.xml](https://cs.chromium.org/chromium/src/tools/metrics/histograms/enums.xml)

At first you need to add an entry to __about_flags.cc__,
__flag_descriptions.cc__ and __flag_descriptions.h__. After that, try running the following test.

```bash
# Build unit_tests
ninja -C out/Default unit_tests
# Run AboutFlagsHistogramTest.CheckHistograms
./out/Default/unit_tests --gtest_filter=AboutFlagsHistogramTest.CheckHistograms
```

That test will ask you to add several entries to enums.xml.
You can refer to [this CL](https://chromium-review.googlesource.com/c/593707) as an example.
