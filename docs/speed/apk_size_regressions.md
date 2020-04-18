# How to Deal with Android Size Alerts

 >
 > Not all alerts should not have a bug created for them. Please read on...
 >

[TOC]

## Step 1: Identify the Commit

### MonochromePublic.apk Alerts (Single Commit)

 * Zoom in on the graph to make sure the alert is not
   [off-by-one](https://github.com/catapult-project/catapult/issues/3444)
   * Replace `&num_points=XXXX` with `&rev=COMMIT_POSITION` in the URL.
   * It will be obvious from this whether or not the point is off. Use the
     "nudge" feature to correct it when this happens.

### MonochromePublic.apk Alerts (Multiple Commits or Rolls)

 * Bisects [will not help you](https://bugs.chromium.org/p/chromium/issues/detail?id=678338).
 * File a bug using template in Step 2.
 * If you can afford to run a fire-and-forget command locally, use a
   [local Android checkout](https://chromium.googlesource.com/chromium/src/+/master/docs/android_build_instructions.md)
   along with [`//tools/binary_size/diagnose_bloat.py`](https://chromium.googlesource.com/chromium/src/+/master/tools/binary_size/README.md)
   to build all commits locally and find the culprit.

**Example:**

     tools/binary_size/diagnose_bloat.py AFTER_GIT_REV --reference-rev BEFORE_GIT_REV --subrepo v8 --all

You can usually find the before and after revs in the roll commit message
([example](https://chromium.googlesource.com/chromium/src/+/10c40fd863f4ae106650bba93b845f25c9b733b1))

### Monochrome.apk Alerts

 * The regression most likely already occurred in the upstream
   MonochromePublic.apk target. Look at the
   [graph of Monochrome.apk and MonochromePublic.apk overlaid](https://chromeperf.appspot.com/report?sid=cfc29eed1238fd38fb5e6cf83bdba6c619be621b606e03e5dfc2e99db14c418b&num_points=1500)
   to find the culprit and de-dupe with upstream alert.
 * If no upstream regression was found, look through the downstream commits
   within the given date range to find the culprit.
    * Via `git log --format=fuller` (be sure to look at `CommitDate` and not
      `AuthorDate`)
 * If the culprit is not obvious, follow the steps from the "multiple commits"
   section above, filing a bug and running `diagnose_bloat.py`
   (with `--subrepo=clank`).

## Step 2: File Bug or Silence Alert

If the code clearly justifies the size increase, silence the alert.

Otherwise, file a bug (TODO: [Make this template automatic](https://github.com/catapult-project/catapult/issues/3150)):

 * Change the bug's title from `X%` to `XXkb`
 * Assign to commit author
 * Set description to (replacing **bold** parts):

> Caused by "**First line of commit message**"
>
> Commit: **abc123abc123abc123abc123abc123abc123abcd**
>
> Link to size graph:
> [https://chromeperf.appspot.com/report?sid=bb23072657e2d7ca892a1c3fa4643b1ee29b3a0a44d0732adda87168e89c0380&num_points=10&rev=**480214**](https://chromeperf.appspot.com/report?sid=bb23072657e2d7ca892a1c3fa4643b1ee29b3a0a44d0732adda87168e89c0380&num_points=10&rev=480214)
>
> Debugging size regressions is documented at:
> https://chromium.googlesource.com/chromium/src/+/master/docs/speed/apk_size_regressions.md#Debugging-Apk-Size-Increase
>
> Based on the graph: **20kb of native code, 8kb of pngs.**
>
> _**Option 1:**_
>
> It looks to me that the size increase is expected.<br>
> Feel free to close as "Won't Fix" unless you can see some way to reduce size.
>
> _**Option 2:**_
>
> It looks like this increase was probably unexpected or might be avoidable.<br>
> Please have a look and either:
>
> 1. Close as "Won't Fix" with a short justification, or
> 2. Land a revert / fix-up.
>
> _**Option 3:**_
>
> It's not clear to me whether or not this increase was expected.<br>
> Please have a look and either:
>
> 1. Close as "Won't Fix" with a short justification, or
> 2. Land a revert / fix-up.
>
> _**Optional addition for commits > 75kb:**_
>
> It typically takes about a week of engineering time to reduce binary size by
> 100kb so we'd really appreciate you taking some time exploring options to
> address this regression!

*If you went with **Option 2**, and the regression is > 50kb, add
ReleaseBlock-Stable **M-6-** (next branch cut).*

Once the initial bug is filed, add a follow-up comment with the output of:

``` sh
tools/binary_size/diagnose_bloat.py GIT_REV --cloud
```

# Debugging Apk Size Increase

## Step 1: Identify what Grew

Figure out which file within the `.apk` increased (native library, dex, pak
resources, etc) by looking at the breakdown in the size graphs linked to in the
bug (if it was not linked in the bug, see above).

**See [//docs/speed/binary_size/metrics.md](https://chromium.googlesource.com/chromium/src/+/master/docs/speed/binary_size/metrics.md)
for a description of the metrics we track.**

## Step 2: Analyze

### Growth is from Translations

 * There is likely nothing that can be done. Translations are expensive.
 * Close as `Won't Fix`.

### Growth is from Native Resources (pak files)

 * Ensure `compress="gzip"` is used for all `chrome:` pages.
 * Use [//tools/binary_size/diagnose_bloat.py](https://chromium.googlesource.com/chromium/src/+/master/tools/binary_size/README.md)
   to show a diff of pak entries.

### Growth is from Images

  * Would [a VectorDrawable](https://codereview.chromium.org/2857893003/) be better?
  * If it's lossy, consider [using webp](https://codereview.chromium.org/2615243002/).
  * Ensure you've optimized with
    [tools/resources/optimize-png-files.sh](https://cs.chromium.org/chromium/src/tools/resources/optimize-png-files.sh).
  * There is some [Googler-specific guidance](https://goto.google.com/clank/engineering/best-practices/adding-image-assets) as well.

### Growth is from Native Code

 * Use [//tools/binary_size/diagnose_bloat.py](https://chromium.googlesource.com/chromium/src/+/master/tools/binary_size/README.md)
to show a diff of ELF symbols.
   * Googlers should use the speedy `--cloud` option. E.g.:
   * `tools/binary_size/diagnose_bloat.py 0f30c9488bd2bdc1db2749cd4683994a764a44c9 --cloud`
 * Paste the diff into the bug.
 * If the diff looks reasonable, close as `Won't Fix`.
 * Otherwise, try to refactor a bit (e.g.
 [move code out of templates](https://bugs.chromium.org/p/chromium/issues/detail?id=716393)).
 * If symbols are larger than expected, use the `Disassemble()` feature of `supersize console` to see what is going on.

### Growth is from Java Code

 * Use [//tools/binary_size/diagnose_bloat.py](https://chromium.googlesource.com/chromium/src/+/master/tools/binary_size/README.md)
   to show a diff of Java symbols.
 * Ensure any new Java deps are as specific as possible.

### Growth is from "other lib size" or "Unknown files size"

 * File a bug against agrieve@ to fix
   [resource_sizes.py](https://cs.chromium.org/chromium/src/build/android/resource_sizes.py).
 * ...or fix it yourself. This script will output the list of unknown filenames.

### You Would Like Assistance

 * Feel free to email [binary-size@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/binary-size).

# For Binary Size Sheriffs

## Step 1: Check work queue daily

 * Bugs requiring sheriffs to take a look at are labeled `Performance-Sheriff` and `Performance-Size`.
 * After resolving the bug by finding an owner or debugging or commenting, remove the `Performance-Sheriff` label.

## Step 2: Check alerts regularly

 * **IMPORTANT**: Check the [perf bot page](https://ci.chromium.org/buildbot/chromium.perf/Android%20Builder%20Perf/)
 several times a day to make sure it isn't broken (and ping/file a bug if it is).
 * Check [alert page](https://chromeperf.appspot.com/alerts?sheriff=Binary%20Size%20Sheriff) regularly for new alerts.
 * Join [binary-size-alerts@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/binary-size-alerts). Eventually it will be all set up.
 * Deal with alerts as outlined above.
