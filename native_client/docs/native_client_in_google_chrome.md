Native Client is now built into the Dev channel of Google Chrome. This page
tells you how to enable and use the integrated version of Native Client.

**Note:** For now, the integrated version of Native Client has less
functionality than the plug-in. If you want to see everything that Native Client
can do, [download the tarball](downloads.md) and install the plug-in. For
details of what's different about the integrated version, see [Known issues]
(NativeClientInGoogleChrome#Known_issues.md).

# How to enable Native Client in Google Chrome

1. [Download Google Chrome](http://www.google.com/chrome), if you don't already
   have it.
2. Subscribe to the [Dev channel](http://dev.chromium.org/getting-involved/dev-channel).
3. Launch Google Chrome from the command line, adding **--enable-nacl**. On Mac
   and Linux, if you're using dev channel release 5.0.375.9 or 5.0.371.0,
   respectively (or an earlier version), also add **--no-sandbox**. On Windows,
   your command should look like this:

        chrome.exe --enable-nacl

**Warning:** We recommend running Google Chrome with the --no-sandbox or
--enable-nacl flag **only** for testing Native Client and **not for regular web
browsing**.

# How to find and run examples and tests

1.  In any browser, bring up the [Getting Started]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/getting_started.html)
    document. You'll follow some—but not all—of its instructions. Specifically:
    1.  [Get the software]
        (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/getting_started.html#software).
        (Download the tarball, extract the files, and make sure you have the
        right version of Python.)
    2.  [Start a local HTTP server]
        (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/getting_started.html#httpd),
        if one isn't already running on your computer.
2.  In Google Chrome, view the Browser Test Page, using **localhost** for the
    hostname. For example: [http://localhost:5103/scons-out/nacl-x86-32/staging/examples.html](http://localhost:5103/scons-out/nacl-x86-32/staging/examples.html)
3.  Click links to examples and tests to run them.

When Google Chrome is launched with the **--enable-nacl** flag, the integrated
version of Native Client is used to run the examples and tests. Otherwise, the
Native Client plug-in (if installed) is used to run them.

The following tests should work in the integrated version of Native Client:

* Tests that have no audio or graphics:
  The results of these tests should be the same as if you executed them in
  Google Chrome using the Native Client plug-in.

* Mandelbrot performance test:
  This works because all drawing is from JavaScript; Native Client is used only
  for calculations.

The following examples don't currently work:

* Examples with graphics (except for the Mandelbrot performance test):
  This includes everything in the Examples column except the SRPC hello
  world example.

For details about what each example and test contains, see the [Examples and
Tests](http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/examples.html)
page and the README files in the source code.

# Known issues

* Graphics and audio from native code don't work unless you're using the
  proposed NPAPI improvements ([announcement]
  (http://groups.google.com/group/native-client-announce/browse_thread/thread/3607403cbc165499),
  [spec](https://wiki.mozilla.org/Plugins:PlatformIndependentNPAPI)). These
  new 2D and 3D APIs provide access to video and audio.
* In some Dev-channel builds of Google Chrome, enabling Native Client requires
  disabling the Google Chrome sandbox.

# More information

For information about the design and implementation of Google Chrome, see the
[Chromium project website](http://dev.chromium.org).
