This page lists the platforms on which we've tried Native Client, both for
building it and for running it. Also see the ReleaseNotes.

**Note:** Please add a comment if you have additional information.

# Linux

## What works

We've tested building with the following configurations: * Ubuntu 8.04 (Hardy
Heron) * Ubuntu 9.04 (Jaunty Jackalope) * Ubuntu 9.10 (Karmic Koala)

Building and running on 64-bit systems isn't as well tested as on 32-bit
systems. Some time after Karmic stabilizes, we will drop support for Jaunty.

The Native Client plug-in has been successfully used in the following browsers:
* Firefox 3 * Firefox 2

## What doesn't work

*   The ia32-lib-dev package is, as of this writing, missing from the amd64
    Karmic Koala release of Ubuntu, and we cannot (yet) build the on Karmic
    amd64.

# Mac

## What works

Building Native Client requires Mac OS X 10.5.

Running Native Client modules should work on both 10.5 and 10.4.

The Native Client plug-in has been successfully used in the following browsers:
* Firefox 3 * Firefox 2 (partial support; control, command, and alt keys not
enabled) * Camino (appears to work, but has had minimal testing)

## What doesn't work

The Native Client plug-in doesn't currently work in the following browsers: *
Safari

# Windows

## What works

Native Client builds and runs on 32-bit Vista and all versions of Windows XP.

The Native Client plug-in has been successfully used in the following browsers:

*   Firefox 3
*   Firefox 2
*   Chrome
*   Safari
*   Opera

## What doesn't work

Native Client does not work on 64-bit versions of Windows Vista. Native Client
will silently fail on these systems, without running untrusted code. 64-bit
Vista lacks the system call required to set up protected memory segments.

The Native Client plug-in doesn't currently work in the following browsers: *
Internet Explorer
