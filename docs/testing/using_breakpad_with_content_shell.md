# Using breakpad with content shell

When running layout tests, it is possible to use
[breakpad](../../third_party/breakpad/) to capture stack traces on crashes while
running without a debugger attached and with the sandbox enabled.

## Setup

On all platforms, build the target `blink_tests`.

*** note
**Mac:** Add `enable_dsyms = 1` to your
[gn build arguments](../../tools/gn/docs/quick_start.md) before building. This
slows down linking several minutes, so don't just always set it by default.
***

*** note
**Linux:** Add `use_debug_fission = true` to your
[gn build arguments](../../tools/gn/docs/quick_start.md) before building.
***

Then, create a directory where the crash dumps will be stored:

* Linux/Mac:
  ```bash
  mkdir /tmp/crashes
  ```
* Android:
  ```bash
  adb shell mkdir /data/local/tmp/crashes
  ```
* Windows:
  ```bash
  mkdir %TEMP%\crashes
  ```

## Running content shell with breakpad

Breakpad can be enabled by passing `--enable-crash-reporter` and
`--crash-dumps-dir` to content shell:

* Linux:
  ```bash
  out/Debug/content_shell --enable-crash-reporter \
      --crash-dumps-dir=/tmp/crashes chrome://crash
  ```
* Mac:
  ```bash
  out/Debug/Content\ Shell.app/Contents/MacOS/Content\ Shell \
      --enable-crash-reporter --crash-dumps-dir=/tmp/crashes chrome://crash
  ```
* Windows:
  ```bash
  out\Default\content_shell.exe --enable-crash-reporter ^
      --crash-dumps-dir=%TEMP%\crashes chrome://crash
  ```
* Android:
  ```bash
  build/android/adb_install_apk.py out/Default/apks/ContentShell.apk
  build/android/adb_content_shell_command_line --enable-crash-reporter \
      --crash-dumps-dir=/data/local/tmp/crashes chrome://crash
  build/android/adb_run_content_shell
  ```

## Retrieving the crash dump

On Linux and Android, we first have to retrieve the crash dump. On Mac and
Windows, this step can be skipped.

* Linux:
  ```bash
  components/crash/content/tools/dmp2minidump.py /tmp/crashes/*.dmp /tmp/minidump
  ```
* Android:
  ```bash
  adb pull $(adb shell ls /data/local/tmp/crashes/*) /tmp/chromium-renderer-minidump.dmp
  components/breakpad/tools/dmp2minidump /tmp/chromium-renderer-minidump.dmp /tmp/minidump
  ```

## Symbolizing the crash dump

On all platforms except for Windows, we need to convert the debug symbols to a
format that breakpad can understand.

* Linux:
  ```bash
  components/crash/content/tools/generate_breakpad_symbols.py \
      --build-dir=out/Default --binary=out/Default/content_shell \
      --symbols-dir=out/Default/content_shell.breakpad.syms --clear --jobs=16
  ```
* Mac:
  ```bash
  components/crash/content/tools/generate_breakpad_symbols.py \
      --build-dir=out/Default \
      --binary=out/Default/Content\ Shell.app/Contents/MacOS/Content\ Shell \
      --symbols-dir=out/Default/content_shell.breakpad.syms --clear --jobs=16
  ```
* Android:
  ```bash
  components/crash/content/tools/generate_breakpad_symbols.py \
      --build-dir=out/Default \
      --binary=out/Default/lib/libcontent_shell_content_view.so \
      --symbols-dir=out/Default/content_shell.breakpad.syms --clear
  ```

Now we can generate a stack trace from the crash dump. Assuming the crash dump
is in minidump.dmp:

* Linux/Android/Mac:
  ```bash
  out/Default/minidump_stackwalk minidump.dmp out/Debug/content_shell.breakpad.syms
  ```
* Windows:
  ```bash
  "c:\Program Files (x86)\Windows Kits\8.0\Debuggers\x64\cdb.exe" ^
      -y out\Default -c ".ecxr;k30;q" -z minidump.dmp
  ```
