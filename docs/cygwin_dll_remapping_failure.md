# Handling repeated failures of rebaseall to allow cygwin remaps

Sometimes DLLs over which cygwin has no control get mapped into cygwin
processes at locations that cygwin has chosen for its libraries.
This has been seen primarily with anti-virus DLLs. When this occurs,
cygwin must be instructed during the rebase to avoid the area of
memory where that DLL is mapped.

## Background

Some background for this is available on
http://www.dont-panic.cc/capi/2007/10/29/git-svn-fails-with-fatal-error-unable-to-remap/

Because of unix fork semantics (presumably), cygwin libraries must be
mapped in the same location in both parent and child of a fork.  All
cygwin libraries have hints in them as to where they should be mapped
in a processes address space; if those hints are followed, each
library will be mapped in the same location in both address spaces.
However, Windows is perfectly happy mapping a DLL anywhere in the
address space; the hint is not considered controlling.  The remapping
error occurs when a cygwin process starts and one of its libraries
cannot be mapped to the location specified by its hint.

/usr/bin/rebaseall changes the DLL hints for all of the cygwin
libraries so that there are no inter-library conflicts; it does this
by choosing a contiguous but not overlapping library layout starting
at a base address and working down.  This process makes sure there are
no intra-cygwin conflicts, but cannot deal with conflicts with
external DLLs that are in cygwin process address spaces
(e.g. anti-virus DLLs).

To handle this case, you need to figure out what the problematic
non-cygwin library is, where it is in the address space, and do the
rebase all so that no cygwin hints map libraries to that location.

## Details

*   Download the ListDLLs executable from
    [sysinternals](http://technet.microsoft.com/en-us/sysinternals/bb896656.aspx)
*   Run it as administrator while some cygwin commands are running.
*   Scan the output for the cygwin process (identifiable by the command) and for
    DLLs in that process that do not look like cygwin DLLs (like an AV). Note
    the location of those libraries (there will usually only be the one).
*   Pick an address space location lower than its starting address.
*   Quit all cygwin processes.
*   Run a windows command shell as administrator
*   cd in \cygwin\bin
*   Run `ash /usr/bin/rebaseall -b <base address>` (This command can also take a
    `-v` flag if you want to see the DLL layout.)

That should fix the problem.

## Failed rebaseall

If you pick a base address that is too low, you may end up with a broken cygwin
install. You can reinstall it by running cygwin's setup.exe again, and on the
package selection page, clicking the "All" entry to Reinstall. You may have to
do this twice, as you may get errors on the first reinstall pass.
