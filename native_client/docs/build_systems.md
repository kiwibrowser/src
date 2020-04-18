**This page is deprecated.  Its contents have been [moved](https://www.chromium.org/nativeclient/how-tos/build-tcb).**

## What build system(s) is Native Client using?

The primary build system used by Native Client is [Scons]
(http://www.scons.org/). For historical reasons we are not using plain SCons but
an extension call Hammer.

The parts of the system shared with Chrome are also built using Chrome's build
system, gn.

We also have some Makefiles and some shell scripts for certain build tasks.

## Why is this such a complex mess?

The usual excuses:

*   Inherent complexity.
*   Historical reasons.
*   Entropy requires no maintenance.
*   ...

## Which files contain build system information?

For SCons it is: SConstruct, `**/build.scons`, `**/nacl.scons` There are also
relevant configuration files in `site_scons/site_tools/*`, and random Python
scripts located here and there.

For gn it is: `**/*.gn` and `**/*.gni`

## What is the difference between trusted and untrusted code?

"trusted code" encompasses components like:
* the browser plugin
* service runtime (`sel_ldr`)

It is compiled using regular compilers. Bugs in trusted code can compromise
system security, hence the name. As far as the build system is concerned
trusted code is described in `**/build.scons` files. The gn system only code
trusted code. "trusted code" lives in `src/trusted/**`

"untrusted code" encompasses components like:
* quake and other examples of Native Client executables
* libraries necessary to build quake

It is compiled using special sandboxing compilers. As far as the build system is
concerned trusted code is described in `**/nacl.scons` files. "untrusted code"
lives in `src/untrusted/**` and also in `tests/**`

Some code can be compiled either as trusted or shared code, e.g. libraries that
facilitate communication between trusted and untrusted code. Such code typically
lives in `src/shared/**` and has both build.scons and nacl.scons files.

## How do you use the  MODE= setting when invoking SCons?

The MODE= setting or its equivalent --mode is used to select whether you want to
compile trusted or untrusted code or both and how. Examples:

MODE=nacl * just build untrusted code * note that this doesn't build all of the
untrusted code. If you don't specify a trusted platform (e.g.
MODE=opt-linux,nacl) most of the tests will not be built.

MODE=opt-linux * just build (optimized) trusted code - you must be on a Linux
system

MODE=nacl,dbg-win * build both (trusted code will be unoptimized)- you must be
on a Windows system

NOTE: if you do not specify MODE, "`opt-<system-os>`" will be assumed.

NOTE: if you want to run integration tests, you need to build both trusted and
untrusted code simultaneously, because those tests involve running untrusted
code under the control of trusted code.

## What is the meaning of BUILD\_ARCH, TARGET\_ARCH, etc.  ?

Just like any cross compilation environment, there are some hairy configuration
issues which are controlled by BUILD\_ARCH, TARGET\_ARCH, etc. to force
conditional compilation and linkage.

It helps to revisit the [terminology]
(http://www.airs.com/ian/configure/configure_5.html) used by cross compilers to
better understand Native Client:

> BUILD\_SYSTEM: The system on which the tools will be build (initially) is
> called the build system.
>
> HOST\_SYSTEM: The system on which the tools will run is called the host
> system.
>
> TARGET\_SYSTEM: The system for which the tools generate code is called the
> target system.

For NaCl we only have **two** of these, sadly they have confusing names:

> BUILD\_PLATFORM: The system on which the trusted code runs.
>
> TARGET\_PLATFORM: The sandbox system that is being enforced by the trusted
> code.

The BUILD\_PLATFORM is closest in nature to the HOST\_SYSTEM, the
TARGET\_PLATFORM is closest to the TARGET\_SYSTEM. We do not have and an
equivalent to BUILD\_SYSTEM since we just assume and x86 (either 32 or 64
system).

## What kind of BUILD\_PLATFORM/TARGET\_PLATFORM  configurations are supported?

Conceptually we have

> BUILD\_PLATFORM = (BUILD\_ARCH, BUILD\_SUBARCH, BUILD\_OS)

and

> TARGET\_PLATFORM = (TARGET\_ARCH. TARGET\_SUBARCH)

NOTE:

There is no TARGET\_OS, since Native Client executables are OS independent.

The BUILD\_OS is usually tested for using SCons expressions like
"env.Bit('windows')" you cannot really control it as it is inherited from the
system you are building on, the BUILD\_SYSTEM in cross compiler speak.

Enumeration of all BUILD\_PLATFORMs:

(x86, 32, linux) (x86, 32, windows) (x86, 32, mac) (arm, 32, linux) // the 32 is
implicit as there is no 64bit arm (x86, 64, windows)

_**Special note for Windows users:** The Windows command-line build currently
relies on vcvarsXX.bat being called to set up the environment. The compiler's
target subarchitecture (32,64) is selected by the version of vcvars that you
called (vcvars32/vcvars64). If you call vcvars32 and then build with
platform=x86-64, you will get "target mismatch" errors._

Enumeration of all TARGET\_PLATFORMs: (x86, 32) (x86, 64) (arm, 32) // the 32 is
implicit as there is no 64bit arm

Usually BUILD\_ARCH == TARGET\_ARCH and BUILD\_SUBARCH == TARGET\_SUBARCH

There is ONLY ONE exception, you can build the ARM validator like so:

> BUILD\_ARCH = x86, BUILD\_SUBARCH=**, TARGET\_ARCH=arm TARGET\_SUBARCH=32**

In particular it is NOT possible to use different SUBARCHs for BUILD and TARGET.

## What is the relationship between TARGET\_PLATFORM and untrusted code?

The flavor of the untrusted code is derived from the TARGET\_PLATFORM

## Why are BUILD_and ARCH_ used inconsistently?

Usually BUILD\_ARCH == TARGET\_ARCH and BUILD\_SUBARCH == TARGET\_SUBARCH so
mistakes have no consequences.
