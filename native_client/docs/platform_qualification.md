# Introduction

There is currently no design doc for our Platform Qualification routine, a check
which runs as the sandbox starts. This page is intended to evolve into such a
design doc as we rework the PQ system.

This document contains some forward-looking statements about the direction of
PQ, and should not be taken as a description of the current design. PQ is a work
in progress.

# Background

Native Client relies on hardware features on all platforms, even those where we
use Software Fault Isolation: * On both x86 platforms, we rely on the
instruction decoder's behavior, which must match our validator in terms of
instruction length, etc. * On x86-32, we rely on the hardware segment registers.
* On x86-64 and ARM, we rely on Data Execution Prevention being present and
active. (Intel calls this XD; AMD calls it NX; ARM calls it XN; because of this
babel we've chosen the vendor-neutral term DEP.)

We also require certain features to be present in the operating system.

We cannot blindly assume that our requirements are met, because of variations in
the hardware and software beneath us: * We support a range of x86
implementations (currently dating back to the 80486). Older machines may decode
newer instructions (e.g. SSE3) differently or incorrectly. * DEP is a recent
feature on both x86 and ARM. On x86, it can be disabled in the BIOS. On ARM, it
requires a relatively recent Linux kernel. * Linux is infinitely configurable,
and can be compiled without features we rely on (such as SysV SHM).

# Design

The Platform Qualification system is a chunk of code (in
`src/trusted/platform_qualify`) that checks our assumptions about the platform
beneath us. The important aspects of the design are 1. What we check, 1. How we
check it, and 1. When we check it.

## What We Check, and How We Check It

There are three classes of checks: architecture-specific and OS-specific. The
checks are broken out below.

### Architecture-specific checks: x86 (all versions)

#### CPUID Present And Working

We require the CPUID instruction (80486DX2 and later). We also require that it
works, and does not lie about the availability of certain late instruction set
additions (SSE and friends).

**Rationale:** early CPUs may decode later ISA additions in undefined ways,
including getting the length wrong -- causing us to lose instruction stream
synchronization and thus validation.

**May change:** under normal circumstances, only across a CPU replacement. In
virtualization, this may change without a restart.

**Method:** we check the presence of CPUID with a short assembly sequence. If
available, we execute it and parse the results. If the results indicate that SSE
is available, we attempt execution of some SSE sequences and check the results
before believing it.

### Architecture-specific checks: x86-32

#### OS restoring LDT

The OS restore LDT entries across context switches. On most operating systems
this is a non-issue, but some versions of Windows 64-bit don't do this even for
32-bit binaries.

**Rationale:** to set up our own base-bound segments for sandboxing, we modify
the LDT.

**May change:** across OS changes only, implying at least a process restart.

**Method:** hardcoded OS version tests.

### Architecture-specific checks: x86-64

#### DEP present and working

Data Execution Prevention (DEP) must be enabled and functional.

**Rationale:** The x86-64 sandbox allows the code-data boundary to fall at any
page. Thus jumps to non-validated data are trivial. We require DEP to prevent
this. Even on processors that support XD/NX, it can be disabled in the BIOS or
unsupported by the OS.

**May change:** across a reboot on real hardware. Under virtualization this can
change even while the process runs.

**Method:** we generate functions in heap and stack memory and execute them. We
verify that we take the expected exception.

### Architecture-specific checks: ARM

#### ARMv7-A or later

We require a minimum architecture of ARMv7-A.

**Rationale:** ARMv7 adds instructions that are important for PNaCl (64-bit
load/store exclusive), clearly defines the CPUID mechanism, and includes the
operations from ARMv6T2 that we require for correctness.

**May change:** across a reboot. Unlikely to change under virtualization.

**Method:** we use an assembly sequence, suggested by ARM Ltd, to verify
architecture revision.

#### DEP present and working

Data Execution Prevention (DEP) must be enabled and functional.

**Rationale:** The ARM sandbox allows the code-data boundary to fall at any
page. Thus jumps to non-validated data are trivial. We require DEP to prevent
this. Even on processors that support XN (all supported ARM processors), this
can be disabled by the operating system.

**May change:** across a reboot on real hardware. Under virtualization this can
change even while the process runs.

**Method:** we generate functions in heap and stack memory and execute them. We
verify that we take the expected exception.

### OS-specific checks: Linux

#### SysV SHM

SysV SHM must be supported and pass certain feature tests, including interacting
predictably with mmap (see
`src/trusted/platform_qualify/linux/sysv_shm_and_mmap.c`).

**Rationale:** _unclear_

**May change:** with kernel upgrades. (Note that with Ksplice this does not
imply a reboot necessarily, though Ksplice currently isn't capable of this sort
of change.)

**Method:** See source implementation.

### OS-specific checks: Mac OS X

_No checks currently implemented._

### OS-specific checks: Windows

#### XP SP2

`GetVersionEx` must be present, operational, and indicate Windows XP SP2 or
later.

**Rationale:** the APIs to check whether the system is 64-bit appeared in XP
SP2. This is in support of the LDT restoration check described above.

**May change:** across a reboot.

**Method:** reasonably simple analysis of results from `GetVersionEx`.

## When We Check

To the extent possible, all PQ checks should run at every `sel_ldr` startup.

We considered an alternative: putting the PQ checks in a separate binary, which
the browser/host system is expected to run before running NaCl. We (cbiffle and
bsy) feel like this rests responsibility for our correctness with a codebase we
don't control or supervise.

The main concerns with running PQ from `sel_ldr` are: 1. Whether it's possible.
Since `sel_ldr` runs inside a platform-specific outer sandbox, the types of
operations we can do in PQ are limited. Currently this does not appear to be a
problem. 1. Whether we can correctly restore state. PQ hooks signal handlers,
creates shared memory regions, etc. If any of these changes survive into
untrusted code execution, it could be dangerous. We must be confident that PQ
code can correctly tear down its changes. 1. Whether it's fast. We hope that
NaCl modules will be loaded frequently. The PQ checks must be low-latency
compared to the rest of `sel_ldr` startup, or (if possible) run while the nexe
is being downloaded.
