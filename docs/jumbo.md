# Jumbo / Unity builds

To improve compilation times it is possible to use "unity builds",
called Jumbo builds, in Chromium. The idea is to merge many
translation units ("source files") and compile them together. Since a
large portion of Chromium's code is in shared header files that
dramatically reduces the total amount of work needed.

## Build instructions

If jumbo isn't already enabled, you enable it in `gn` by setting
`use_jumbo_build = true` then compile as normal.

## Implementation

Jumbo is currently implemented as a combined `gn` template and a
python script. Eventually it may become a native `gn` feature. By
(indirectly) using the template `internal_jumbo_target`, each target
will split into one action to "merge" the files and one action to
compile the merged files and any files left outside the merge.

Template file: `//build/config/jumbo.gni`
Merge script: `//build/config/merge_for_jumbo.py`

### Merge

The "merge" is currently done by creating wrapper files that `#include` the
source files.

## Jumbo Pros and Cons

### Pros

* Everything compiles significantly faster. When fully enabled
  everywhere this can save hours for a full build (binaries and tests)
  on a moderate computer.  Linking is faster because there is less
  redundant data (debug information, inline functions) to merge.
* Certain code bugs can be statically detected by the compiler when it
  sees more/all the relevant source code.

### Cons

* By merging many files, symbols that have internal linkage in
  different `cc` files can collide and cause compilation errors.
* The smallest possible compilation unit grows which can add
  10-20 seconds to some single file recompilations (though link
  times often shrink).

### Mixed blessing
* Slightly different compiler warnings will be active.

## Tuning

By default at most `50`, or `8` when using goma, files are merged at a
time. The more files that are are merged, the less total CPU time is
needed, but parallelism is reduced. This number can be changed by
setting `jumbo_file_merge_limit`.

## Naming

The term jumbo is used to avoid the confusion resulting from talking
about unity builds since unity is also the name of a graphical
environment, a 3D engine, a webaudio filter and part of the QUIC
congestion control code. Jumbo has been used as name for a unity build
system in another browser engine.

## Want to make your favourite piece of code jumbo?

1. Add `import("//build/config/jumbo.gni")` to `BUILD.gn`.
2. Change your target, for instance `static_library`, to
   `jumbo_static_library`. So far `source_set`, `component`,
   `static_library` and `split_static_library` are supported.
3. Recompile and test.

### Example
Change from:

    source_set("foothing") {
      sources = [
        "foothing.cc"
        "fooutil.cc"
        "fooutil.h"
      ]
    }
to:

    import("//build/config/jumbo.gni")  # ADDED LINE
    jumbo_source_set("foothing") {      # CHANGED LINE
      sources = [
        "foothing.cc"
        "fooutil.cc"
        "fooutil.h"
      ]
    }


If you see some compilation errors about colliding symbols, resolve
those by renaming symbols or removing duplicate code.  If it's
impractical to change the code, add a `jumbo_excluded_sources`
variable to your target in `BUILD.gn`:

`jumbo_excluded_sources = [ "problematic_file.cc" ]`

## More information and pictures
There are more information and pictures in a
[Google Document](https://docs.google.com/document/d/19jGsZxh7DX8jkAKbL1nYBa5rcByUL2EeidnYsoXfsYQ)

## Mailing List
Public discussions happen on the generic blink-dev and chromium-dev
mailing lists.

https://groups.google.com/a/chromium.org/group/chromium-dev/topics

## Bugs / feature requests
Related bugs use the label `jumbo` in the bug database.
See [the open bugs](http://code.google.com/p/chromium/issues/list?q=label:jumbo).
