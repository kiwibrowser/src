# Retrieving Code Analysis Warnings

Several times a day the Chromium code base is built with Microsoft VC++'s
`/analyze` compile option. This does static code analysis which has found
numerous bugs (see https://crbug.com/427616). While it is possible to visit the
`/analyze` builder page and look at the raw results
(https://build.chromium.org/p/chromium.fyi/builders/Chromium%20Windows%20Analyze)
this works very poorly.

As of this writing there are 2,702 unique warnings. Some of these are in header
files and fire multiple times so there are a total of 11,202 warning lines. Most
of these have been examined and found to be false positives. Therefore, in order
to sanely examine the /analyze warnings it is necessary to summarize the
warnings, and find what is new.

There are scripts to do this.

## Details

The necessary scripts, which currently run on Windows only, are checked in to
`tools\win\new_analyze_warnings`. Typical usage is like this:

    > set ANALYZE_REPO=d:\src\analyze_chromium
    > retrieve_latest_warnings.bat

The batch file using the associated Python scripts to retrieve the latest
results from the web page, create a summary file, and if previous results were
found create a new warnings file. Typical results look like this:

    analyze0067_full.txt
    analyze0067_summary.txt
    analyze0067_new.txt

If `ANALYZE_REPO` is set then the batch file goes to `%ANALYZE_REPO%\src`, does
a git pull, then does a checkout of the revision that corresponds to the latest
warnings, and then does a gclient sync. The warnings can then be easily
correlated to the specific source that triggered them.

## Understanding the results

The `new.txt` file lists new warnings, and fixed warnings. Usually it can
accurately identify them but sometimes all it can say is that the number of
instances of a particularly warning has changed, which is usually not of
interest. If you look at new warnings every day or two then the number of new
warnings is usually low enough to be quite manageable.

The `summary.txt` file groups warnings by type, and then sorts the groups by
frequency. Low frequency warnings are more likely to be real bugs, so focus on
those. However, all of the low-frequency have been investigated so at this time
they are unlikely to be real bugs.

The majority of new warnings are variable shadowing warnings. Until `-Wshadow`
is enabled for gcc/clang builds these warnings will continue to appear, and
unless they are actually buggy or are particularly confusing it is usually not
worth fixing them. One exception would be if you are planning to enable
`-Wshadow` in which case using the list or relevant shadowing warnings would be
ideal.

Some of the warnings say that out-of-range memory accesses will occur, which is
pretty scary. For instance "warning C6201: Index '-1' is out of valid index
range '0' to '4'". In most cases these are false positives so use your own
judgment when deciding whether to fix them.

The `full.txt` file contains the raw output and should usually be ignored.

If you have any questions then post to the chromium dev mailing list.
