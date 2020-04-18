# Cross-compiling Chrome/win

It's possible to build most parts of the codebase on a Linux or Mac host while
targeting Windows.  This document describes how to set that up, and current
restrictions.

What does *not* work:

* goma. Sorry. ([internal bug](http://b/64390790)) You can use the
  [jumbo build](jumbo.md) for faster build times.
* 64-bit renderer processes don't use V8 snapshots, slowing down their startup
  ([bug](https://crbug.com/803591))
* on Mac hosts, building a 32-bit chrome ([bug](https://crbug.com/794838))

All other targets build fine (including `chrome`, `browser_tests`, ...).

Uses of `.asm` files have been stubbed out.  As a result, some of Skia's
software rendering fast paths are not present in cross builds, Crashpad cannot
report crashes, and NaCl defaults to disabled and cannot be enabled in
cross builds ([.asm bug](https://crbug.com/762167)).

## .gclient setup

1. Tell gclient that you need Windows build dependencies by adding
   `target_os = ['win']` to the end of your `.gclient`.  (If you already
   have a `target_os` line in there, just add `'win'` to the list.) e.g.

       solutions = [
         {
           ...
         }
       ]
       target_os = ['android', 'win']

1. `gclient sync`, follow instructions on screen.

If you're at Google, this will automatically download the Windows SDK for you.
If this fails with an error: Please follow the instructions at
https://www.chromium.org/developers/how-tos/build-instructions-windows
then you may need to re-authenticate via:

    cd path/to/chrome/src
    # Follow instructions, enter 0 as project id.
    download_from_google_storage --config

If you are not at Google, you'll have to figure out how to get the SDK, and
you'll need to put a JSON file describing the SDK layout in a certain location.

## GN setup

Add `target_os = "win"` to your args.gn.  Then just build, e.g.

    ninja -C out/gnwin base_unittests.exe

## Copying and running chrome

A convenient way to copy chrome over to a Windows box is to build the
`mini_installer` target.  Then, copy just `mini_installer.exe` over
to the Windows box and run it to install the chrome you just built.

## Running tests on swarming

You can run the Windows binaries you built on swarming, like so:

    tools/run-swarmed.py -C out/gnwin -t base_unittests [ --gtest_filter=... ]

See the contents of run-swarmed.py for how to do this manually.

There's a bot doing 64-bit release cross builds at
https://ci.chromium.org/buildbot/chromium.clang/linux-win_cross-rel/
which also runs tests. You can look at it to get an idea of which tests pass in
the cross build.
