# Chrome Cleanup Tool

This directory contains the source code for the Chrome Cleanup Tool, a
standalone application distributed to Chrome users to find and remove Unwanted
Software (UwS).

## Integration with Chrome

The application is distributed in two versions:

1.  A Chrome component named the Software Reporter that finds UwS but does not
    have the ability to delete anything.
2.  A separate download named Chrome Cleanup Tool that both finds and removes UwS.

The Software Reporter runs in the background and, if it finds UwS that can be
removed, reports this to Chrome. Chrome then downloads the full Cleanup Tool
and shows a prompt on the settings page asking the user for permission to
remove the UwS.

This directory contains the source for both.

Code in Chromium that deals with the Software Reporter Tool and Chrome Cleanup
Tool includes:

*   [Software Reporter component updater](/chrome/browser/component_updater)
    (files `sw_reporter_installer_win*`)
*   [Chrome Cleanup Tool fetcher and launcher](/chrome/browser/safe_browsing/chrome_cleaner)
*   [Settings page resources](/chrome/browser/resources/settings/chrome_cleanup_page)
*   [Settings page user interaction handlers](/chrome/browser/ui/webui/settings)
    (files `chrome_cleanup_handler.*`)
*   [UI for modal dialogs](/chrome/browser/ui/views) (files `chrome_cleaner_*`)
*   [Shared constants and Mojo interfaces](/components/chrome_cleaner/public) -
    These are used for communication between Chrome and the Software Reporter /
    Chrome Cleanup Tool, so both this directory and the other Chromium
    directories above have dependencies on them.

## Status

Incomplete: the source code for the Software Reporter and Chrome Cleanup Tool
are currently being moved from a Google internal repository into this
directory.

## Contact

csharp@chromium.org
joenotcharles@chromium.org
