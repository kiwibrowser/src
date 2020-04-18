**Please see the new [Release Notes]
(http://code.google.com/chrome/nativeclient/docs/releasenotes.html) page for
current information.**

## Obsolete Information:

This page summarizes the changes in each downloadable "tarball" of the Native
Client project. To get the latest tarball for each platform, see [Downloads]
(downloads.md).

Between 0.2 and 0.1, the Native Client source hierarchy changed substantially,
and the main development repository changed from an internal one to the
[nativeclient project's SVN repository]
(http://code.google.com/p/nativeclient/source/browse#svn/trunk).

#### Contents

## 0.2 release notes

This section describes the changes in each 0.2 release. You can find some more
information in the `RELEASE_NOTES` file of the `src/native_client/` directory
(here's the [latest version]
(http://code.google.com/p/nativeclient/source/browse/trunk/src/native_client/RELEASE_NOTES)).

### January 22, 2009

No changes except for a new expiration date: March 17, 2010

### November 2, 2009

No changes except for a new expiration date: January 14, 2010

### September 28, 2009

Includes all [changes up to r782]
(http://code.google.com/p/nativeclient/source/list?start=782). Most
significantly:

*   New expiration date: November 4, 2009
*   Added a new sample (tests/drawing) that illustrates the use of Anti-Grain
    Geometry
*   Continued progress on ARM and x86-64 ports
*   Various build and system changes to support integration into Google Chrome
*   New interface (nacl\_thread\_nice) for supporting real-time threads

### August 17, 2009

Includes all [changes up to r538]
(http://code.google.com/p/nativeclient/source/list?start=538), plus r543 (on
Google Code) and r544 (on Google Code). Most significantly:

*   New expiration date: September 29
*   More directory structure changes, such as:
    *   src/native\_client -> build/native\_client
    *   source tree root is now build (was nacl)
*   We recently removed support for the NPAPI bridge because of concerns about
    NPAPI's portability and security. We are proposing some revisions to NPAPI,
    which you can read about at
    http://wiki.mozilla.org/Plugins:PlatformIndependentNPAPI.

### July 9, 2009

Includes all [changes up to r343]
(http://code.google.com/p/nativeclient/source/list?start=343). Most
significantly:

*   New expiration date: August 18
*   Slimmed down tarballs (they no longer have third-party SDK source)
*   Install and header file cleanup
*   Decoder updates anticipating 64-bit support (r326 (on Google Code))
*   Support for new versions of GCC and binutils
*   Filter the user's environment prior to making it available to Native Client
    modules (r332 (on Google Code))
*   New convention checker, IncludeChecker, enforces that all includes using
    double quotes have paths starting with limited number of allowed prefixes;
    also flags use of '..' in path and includes that are NOT in the top part of
    the file (r294 (on Google Code))

### June 19, 2009

Includes all [changes up to r280]
(http://code.google.com/p/nativeclient/source/list?start=280). Most
significantly:

*   The getting started and build instructions have changed.
*   The source hierarchy has changed a great deal (and might still change).
*   Most of the directories under `scons-out` now have `-x86-32` added to their
    names.
*   The expiration date has _not_ changed — it's still July 8.

Some advice: * If you have bookmarks for the examples, update them to add
`-x86-32`. * If you've been downloading Native Client frequently, you might end
up with both old and new binaries under `scons-out` — for example,
`scons-out/nacl` and `scons-out/nacl-x86-32`. To avoid problems, delete the
non`-x86-32` versions of all the directories that contain binaries (that is, all
the `scons-out` directories except `doc`).

## 0.1 release notes

Release notes for 0.1 distributions are in the `RELEASE_NOTES` file in the
`nacl/googleclient/native_client/` directory.

**Note:** The date in the release notes might not match the date in this page.
The date in the notes reflects when the notes were edited, which might be a few
days before the release; this page lists the day the release went out.

You can find copies of the release notes in the SVN repository:

*   [Release notes: June 1, 2009]
    (http://nativeclient.googlecode.com/svn-history/r201/data/docs_tarball/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: April 23, 2009]
    (http://nativeclient.googlecode.com/svn-history/r168/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: March 30, 2009]
    (http://nativeclient.googlecode.com/svn-history/r155/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: March 12, 2009]
    (http://nativeclient.googlecode.com/svn-history/r134/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: February 11, 2009]
    (http://nativeclient.googlecode.com/svn-history/r119/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: January 16, 2009]
    (http://nativeclient.googlecode.com/svn-history/r67/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: December 22, 2008]
    (http://nativeclient.googlecode.com/svn-history/r46/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
*   [Release notes: December 11, 2008]
    (http://nativeclient.googlecode.com/svn-history/r15/trunk/nacl/googleclient/native_client/RELEASE_NOTES)
