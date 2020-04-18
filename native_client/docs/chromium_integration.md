## Design documents

*   [Chromium integration for Win32, Linux and Mac]
    (http://docs.google.com/View?id=dhhbw89r_0c7w4rxdc)
*   [Chromium integration for Win64]
    (http://docs.google.com/View?id=dhhbw89r_2p9wgf5tg)

## Build system

Most of NaCl, including the trusted runtime and tests, is built using Scons.
However, Chromium ignores NaCl's Scons build system and uses gn to build NaCl
(in the same way as it ignores Webkit's own build system). This is why there are
`*.gn` and `*.gni` files in the NaCl tree, though the NaCl standalone build
doesn't use them. These gn files cover NaCl's trusted code but not tests.

## sel\_ldr

On some systems, the code that is normally in sel\_ldr is statically linked into
the Chromium executable. (The Gyp build also has a target to build a standalone
sel\_ldr, but this is mainly for testing purposes.)

Linking sel\_ldr into Chromium has some advantages: * It reduces version skew
issues. * It makes it easier to run sel\_ldr inside the sandbox. On Linux,
sel\_ldr can be launched by forking Chromium's Zygote process, which is already
set up to be sandboxed. This can also reduce per-instance startup costs a
little. * It makes it easier for sel\_ldr to link to Chromium-specific code
(e.g. code specific to the Chromium Linux sandbox).

However, there also some good reasons to build sel\_ldr as a separate
executable: * On Windows 64, sel\_ldr needs to be a 64-bit executable, while
Chromium itself is 32-bit, so they cannot be linked together. * On ARM, sel\_ldr
needs to get control early in the process startup in order to reserve the bottom
chunk of address space. * Even on x86-32, address space is relatively limited,
so it would be beneficial to avoid loading non-NaCl Chromium code into
sel\_ldr's address space.

## Platform support

**Platform**     | **Chromium process** | **NaCl process**      | **Outer sandbox supported?**
:--------------- | :------------------- | :-------------------- | :---------------------------
Linux, x86-32    | 32-bit               | 32-bit                | yes(1)
Linux, x86-64    | 64-bit               | 64-bit (currently)(2) | yes(1)
Linux, ARM       | 32-bit               | 32-bit                | in principle, yes(1)
Windows, x86-32  | 32-bit               | 32-bit                | yes
Windows, x86-64  | 32-bit(3)            | 64-bit                | yes
Mac OS X, x86-32 | 32-bit               | 32-bit                | yes(4)
Mac OS X, x86-64 | ?                    | 32-bit?               | yes(4)

1.  NaCl supports the SUID sandbox but not, currently, the seccomp sandbox. See
    LinuxOuterSandbox.
2.  Alternatively, we could use a 32-bit sel\_ldr process with 64-bit
    browser/renderer processes.
3.  For simplicity, Chromium uses the same 32-bit executable on both 32-bit and
    64-bit Windows. However, we cannot use the 32-bit sel\_ldr on Windows 64,
    because these versions of Windows drop support for setting up x86
    segmentation, which the x86-32 version of NaCl relies on.
4.  Mac OS X uses the standard Chrome renderer sandbox for the renderer process
    (the Native Client plugin) and the sel\_ldr process uses its own, very
    restrictive Chrome sandbox.

## Policy on dependencies

Native Client should not #include files from the "chrome" directory in the
Chromium source tree. However, #including files from other directories (such as
"base") might be OK. For some background, see [issue 469]
(http://code.google.com/p/nativeclient/issues/detail?id=469).

## See also

*   GypBuild
