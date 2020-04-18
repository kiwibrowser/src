# Android Debugging Instructions

Chrome on Android has java and c/c++ code. Each "side" have its own set of tools
for debugging. Here's some tips.

[TOC]

## Launching the app

You can launch the app by using one of the wrappers.

```shell
out/Default/bin/content_shell_apk launch [--args='--foo --bar'] 'data:text/html;utf-8,<html>Hello World!</html>'
out/Default/bin/chrome_public_apk launch [--args='--foo --bar'] 'data:text/html;utf-8,<html>Hello World!</html>'
```

## Log output

[Chromium logging from LOG(INFO)](https://chromium.googlesource.com/chromium/src/+/master/docs/android_logging.md)
etc., is directed to the Android logcat logging facility. You can filter the
messages, e.g. view chromium verbose logging, everything else at warning level
with:

```shell
adb logcat chromium:V cr.SomeComponent:V *:W
# or:
out/Default/bin/chrome_public_apk logcat
```

### Warnings for Blink developers

*   **Do not use fprintf or printf debugging!** This does not
    redirect to logcat.

*   Redirecting stdio to logcat, as documented
    [here](https://developer.android.com/studio/command-line/logcat.html#viewingStd),
    has a bad side-effect that it breaks `adb_install.py`. See
    [here for details](http://stackoverflow.com/questions/28539676/android-adb-fails-to-install-apk-to-nexus-5-on-windows-8-1).

## Take a screenshot

While your phone is plugged into USB, use the `screenshot.py` tool in
`build/android`. `envsetup.sh` should have put it in your path.

```shell
build/android/screenshot.py /tmp/screenshot.png
```

## Inspecting the view hierarchy

You can use either
[hierarchy viewer](https://developer.android.com/studio/profile/hierarchy-viewer-setup.html)
or [monitor](https://developer.android.com/studio/profile/monitor.html) to see
the Android view hierarchy and see the layout and drawing properties associated
with it.

While your phone is plugged into USB, you can inspect the Android view hierarchy
using the following command:

```shell
ANDROID_HVPROTO=ddm monitor
```

Setting `ANDROID_HVPROTO` allows you to inspect debuggable apps on non-rooted
devices.  When building a local version of Chromium, the build tools
automatically add `android:debuggable=true` to the `AndroidManifest.xml`, which
will allow you to inspect them on rooted devices.

Want to add some additional information to your Views? You can do that by
adding the
[@ViewDebug.ExportedProperty](https://developer.android.com/reference/android/view/ViewDebug.ExportedProperty.html)
annotation.

Example:

```java
@ViewDebug.ExportedProperty(category="chrome")
private int mSuperNiftyDrawingProperty;
```

## Debugging Java

For both apk and test targets, pass `--wait-for-java-debugger` to the wrapper
scripts.

Examples:

```shell
# Install, launch, and wait:
out/Default/bin/chrome_public_apk run --wait-for-java-debugger

# Launch, and have GPU process wait rather than Browser process:
out/Default/bin/chrome_public_apk launch --wait-for-java-debugger --debug-process-name privileged_process0

# Have Renderers wait:
out/Default/bin/chrome_public_apk launch --args="--renderer-wait-for-java-debugger"

# Have tests wait:
out/Default/bin/run_chrome_public_test_apk --wait-for-java-debugger
out/Default/bin/run_chrome_junit_tests --wait-for-java-debugger  # Specify custom port via --debug-socket=9999
```

### Android Studio
*   Open Android Studio ([instructions](android_studio.md))
*   Click "Run"->"Attach debugger to Android process" (see
[here](https://developer.android.com/studio/debug/index.html) for more).
    Click "Run"->"Attach to Local Process..." for Robolectric junit tests.

### Eclipse
*   In Eclipse, make a debug configuration of type "Remote Java Application".
    Choose a "Name" and set "Port" to `8700`.

*   Make sure Eclipse Preferences > Run/Debug > Launching > "Build (if required)
    before launching" is unchecked.

*   Run Android Device Monitor:

    ```shell
    third_party/android_tools/sdk/tools/monitor
    ```

*   Now select the process you want to debug in Device Monitor (the port column
    should now mention 8700 or xxxx/8700).

*   Run your debug configuration, and switch to the Debug perspective.

## Debugging C/C++

Use the wrapper script `gdb` command to enter into a gdb shell.

```shell
# Attaches to browser process.
out/Default/bin/content_shell_apk gdb
out/Default/bin/chrome_public_apk gdb

# Attaches to gpu process.
out/Default/bin/chrome_public_apk gdb --debug-process-name privileged_process0

# Attach to other processes ("chrome_public_apk ps" to show pids).
out/Default/bin/chrome_public_apk gdb --pid $PID
```

### Waiting for Debugger on Early Startup

Set the target command line flag with `--wait-for-debugger`.

Launch the debugger using one of the scripts from above.

Type `info threads` and look for a line like:

```
11 Thread 2564  clock_gettime () at bionic/libc/arch-arm/syscalls/clock_gettime.S:11
```

or perhaps:

```
1  Thread 10870      0x40127050 in nanosleep () from /tmp/user-adb-gdb-libs/system/lib/libc.so
```

We need to jump out of its sleep routine:

```
(gdb) thread 11
(gdb) up
(gdb) up
(gdb) return
Make base::debug::BreakDebugger() return now? (y or n) y
(gdb) continue
```

## Symbolizing Crash Stacks and Tombstones (C++)

If a crash has generated a tombstone in your device, use:

```shell
build/android/tombstones.py --output-directory out/Default
```

If you have a stack trace (from `adb logcat`) that needs to be symbolized, copy
it into a text file and symbolize with the following command (run from
`${CHROME_SRC}`):

```shell
third_party/android_platform/development/scripts/stack --output-directory out/Default [tombstone file | dump file]
```

`stack` can also take its input from `stdin`:

```shell
adb logcat -d | third_party/android_platform/development/scripts/stack --output-directory out/Default
```

Example:

```shell
third_party/android_platform/development/scripts/stack --output-directory out/Default ~/crashlogs/tombstone_07-build231.txt
```

## Deobfuscating Stack Traces (Java)

You will need the ProGuard mapping file that was generated when the application
that crashed was built. When building locally, these are found in:

```shell
out/Default/apks/ChromePublic.apk.mapping
out/Default/apks/ChromeModernPublic.apk.mapping
etc.
```

Build the `java_deobfuscate` tool:

```shell
ninja -C out/Default java_deobfuscate
```

Then run it via:

```shell
# For a file:
out/Default/bin/java_deobfuscate PROGUARD_MAPPING_FILE.mapping < FILE
# For logcat:
adb logcat | out/Default/bin/java_deobfuscate PROGUARD_MAPPING_FILE.mapping
```

## Get WebKit code to output to the adb log

In your build environment:

```shell
adb root
adb shell stop
adb shell setprop log.redirect-stdio true
adb shell start
```

In the source itself, use `fprintf(stderr, "message");` whenever you need to
output a message.

## Debug unit tests with GDB

To run unit tests use the following command:

```shell
out/Debug/bin/run_test_name -f <test_filter_if_any> --wait-for-debugger -t 6000
```

That command will cause the test process to wait until a debugger is attached.

To attach a debugger:

```shell
build/android/adb_gdb --output-directory=out/Default --package-name=org.chromium.native_test
```

After attaching gdb to the process you can use it normally. For example:

```
(gdb) break main
Breakpoint 1 at 0x9750793c: main. (2 locations)
(gdb) continue
```
