# Windows Split DLLs

A build mode where chrome.dll is split into two separate DLLs. This was
undertaken as one possible workaround for toolchain limitations on Windows.

## How

Normally, you probably don't need to worry about doing this build. If for some
reason you need to build it locally:

1.  From a _Visual Studio Command Prompt_ running as **Administrator** run
    `python tools\win\split_link\install_split_link.py`.
1.  Set `GYP_DEFINES=chrome_split_dll=1`. In particular, don't have
    `component=shared_library`. Other things, like `buildtype` or `fastbuild`
    are fine.
1.  `gclient runhooks`
1.  `ninja -C out\Release chrome`

`chrome_split_dll` currently applies only to chrome.dll (and not test binaries).

## What

This is intended to be a temporary measure until either the toolchain is
improved or the code can be physically separated into two DLLs (based on a
browser/child split).

The link replacement forcibly splits chrome.dll into two halves based on a
description in `build\split_link_partition.py`. Code is primarily split along
browser/renderer lines. Roughly, Blink and its direct dependencies are in the
"chrome1.dll", and the rest of the browser code remains in "chrome.dll".

TODO: build\split_link_partition.py doesn't exist.

Splitting the code this way allows keeping maximum optimization on the Blink
portion of the code, which is important for performance.

There is a compile time define set when building in this mode
`CHROME_SPLIT_DLL`, however it should be used very sparingly-to-not-at-all.

## Details

This forcible split is implemented by putting .lib files in either one DLL or
the other, and causing unresolved externals that result during linking to be
forcibly exported from the other DLL. This works relatively cleanly for function
import/export, however it cannot work for data export.

There are relatively few instances where data exports are required across the
DLL boundary. The waterfall builder
https://build.chromium.org/p/chromium/waterfall?show=Win%20Split will detect when
new data exports are added, and these will need to be repaired. For constants,
the data can be duplicated to both DLLs, but for writeable data, a wrapping
set/get function will need to be added.

https://build.chromium.org/p/chromium/waterfall?show=Win%20Split does not exist.

Some more details can be found on the initial commit of the split_link script
https://src.chromium.org/viewvc/chrome?revision=200049&view=revision and the
associated bugs: https://crbug.com/237249 https://crbug.com/237267.
