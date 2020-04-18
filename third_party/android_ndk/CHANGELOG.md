Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * The deprecated headers have been removed. [Unified Headers] are now simply
   The Headers.

   For migration tips, see [Unified Headers Migration Notes].

 * GCC is no longer supported. It will not be removed from the NDK just yet, but
   is no longer receiving backports. It cannot be removed until after libc++ has
   become stable enough to be the default, as some parts of gnustl are still
   incompatible with Clang. It will be removed when the other STLs are removed
   in r18.

 * `libc++` is out of beta and is now the preferred STL in the NDK. Starting in
   r17, `libc++` is the default STL for CMake and standalone toolchains. If you
   manually selected a different STL, we strongly encourage you to move to
   `libc++`. For more details, see [this blog post].

 * Support for ARMv5 (armeabi), MIPS, and MIPS64 are deprecated. They will no
   longer build by default with ndk-build, but are still buildable if they are
   explicitly named, and will be included by "all", "all32", and "all64".
   Support for each of these has been removed in r17.

   Both CMake and ndk-build will issue a warning if you target any of these
   ABIs.

[Unified Headers]: docs/UnifiedHeaders.md
[Unified Headers Migration Notes]: docs/UnifiedHeadersMigration.md
[this blog post]: https://android-developers.googleblog.com/2017/09/introducing-android-native-development.html

NDK
---

 * ndk-build and CMake now link libatomic by default. Manually adding `-latomic`
   to your ldflags should no longer be necessary.
 * Clang static analyzer support for ndk-build has been fixed to work with Clang
   as a compiler. See https://github.com/android-ndk/ndk/issues/362.
 * Clang now defaults to -Oz instead of -Os. This should reduce generated code
   size increases compared to GCC.
 * GCC no longer uses -Bsymbolic by default. This allows symbol preemption as
   specified by the C++ standard and as required by ASAN. For libraries with
   large numbers of public symbols, this may increase the size of your binaries.
 * Updated binutils to version 2.27. This includes the fix for miscompiles for
   aarch64: https://sourceware.org/bugzilla/show_bug.cgi?id=21491.
 * Improved compatibility between our CMake toolchain file and newer CMake
   versions. The NDK's CMake toolchain file now completely supercedes CMake's
   built-in NDK support.
 * ndk-stack now works for arm64 on Darwin.

libc++
------

 * libandroid\_support now contains only APIs needed for supporting libc++ on
   old devices. See https://github.com/android-ndk/ndk/issues/300.

APIs
----

 * Added native APIs for Android O MR1.
     * [Neural Networks API]
     * [JNI Shared Memory API]

[Neural Networks API]: https://developer.android.com/ndk/guides/neuralnetworks/index.html
[JNI Shared Memory API]: https://developer.android.com/ndk/reference/sharedmem__jni_8h.html

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * [Issue 360]: `thread_local` variables with non-trivial destructors will cause
   segfaults if the containing library is `dlclose`ed on devices running M or
   newer, or devices before M when using a static STL. The simple workaround is
   to not call `dlclose`.
 * [Issue 374]: gabi++ (and therefore stlport) binaries can segfault when built
   for armeabi.
 * [Issue 399]: MIPS64 must use the integrated assembler. Clang defaults to
   using binutils rather than the integrated assembler for this target.
   ndk-build and cmake handle this for you, but make sure to use
   `-fintegrated-as` for MIPS64 for custom build systems.

[Issue 360]: https://github.com/android-ndk/ndk/issues/360
[Issue 374]: https://github.com/android-ndk/ndk/issues/374
[Issue 399]: https://github.com/android-ndk/ndk/issues/399
