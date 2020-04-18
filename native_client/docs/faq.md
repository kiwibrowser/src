This page answers the most commonly asked questions about Native Client. If you
don't find the answer to your question here, please try the [discussion group]
(http://groups.google.com/group/native-client-discuss). Also sign up for the
[announcement group](http://groups.google.com/group/native-client-announce).

#### Contents

## General Native Client Questions

### What is Native Client?

Native Client is an open-source research technology from Google for running x86
native code in web applications, with the goal of maintaining the browser
neutrality, OS portability, and safety that people expect from web apps.

### Why is the download so large?

As this release is mainly intended for the security and research communities, we
focused on making them self-contained, and making it build robustly, at the cost
of download size. For the "nacl\_mac" download, here's what you get
(uncompressed sizes):

*   SDK 3rd party source for gcc, binutils, and other tools (67MB)
*   Prebuilt SDK (60MB)
*   Native Client prebuilt binaries (plug-in, container, examples) (22MB)
*   Image source for the map used to generate the spinning globe (5MB)
*   Source, documentation, everything else (9MB)

One download gives you everything you need to try the system, build your own
Native Client modules, and even build the SDK, plugins, and secure module
container.

### What's the process for becoming a contributor?

Before sending us any code, please sign a Contributor License Agreement (CLA).
The CLA protects you and us.

*   If you are an individual writing original source code and you're sure you
    own the intellectual property, sign an [individual CLA]
    (http://code.google.com/legal/individual-cla-v1.0.html). (If you work for a
    company, you might want to check with your employer about ownership of the
    intellectual property.)
*   If you work for a company that wants to allow you to contribute your work to
    Native Client, sign a [corporate CLA]
    (http://code.google.com/legal/corporate-cla-v1.0.html).

Once you've signed the CLA, you can contribute code by posting the diffs to the
[Native Client Discuss group]
(http://groups.google.com/group/native-client-discuss).

## Implementation Questions

### Why doesn't Native Client work on 64-bit Windows?

The short answer is that we intend to support 64-bit Windows, but it's going to
take some time because our 32-bit Windows solution depends on APIs that aren't
in some 64-bit versions. For the long answer, see the [discussion group post]
(http://groups.google.com/group/native-client-discuss/browse_thread/thread/3683b35e9ec74f02).

### Why does Native Client use a whitelist for the URLs of untrusted modules?

The whitelist is a precaution while we work on hardening the system and on
features that are required for defense in depth. Eventually, you should be able
to load Native Client modules from any http URL.

### How can I make Native Client load modules from non-localhost URLs?

Edit the whitelist in npapi\_plugin/origin.cc. For example, if you want to load
modules from a Google Code project named kw-test, add
`http://kw-test.googlecode.com` like this:

```
bool OriginIsInWhitelist(std::string origin) {
  static char const *allowed_origin[] = {
    "http://localhost",
    "http://localhost:80",
    "http://localhost:5103",
    "http://kw-test.googlecode.com",  // NEW: WHITELIST MY PROJECT'S WEBSITE
  };
  ...
}
```

After modifying the whitelist, rebuild as described in [Building Native Client]
(http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/building.html),
and then reinstall as described in [Getting Started]
(http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/getting_started.html).

## Build/Install Questions

### Why isn't the plug-in loading?

Assuming you've already [installed the plug-in]
(http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/getting_started.html#plugin),
verify that all files required by the plug-in are in place. For example, on
Windows three files are required: `sel_ldr.exe`, `SDL.dll`, and
`npGoogleNaClPlugin.dll`. See FilesInstalled for details.

### Why is make failing in cygwin (Windows)?

See the explanation and workaround in the discussion group: [notes for
makefiles, cygwin, and windows]
(http://groups.google.com/group/native-client-discuss/browse_thread/thread/a14268f6bd36cb83).

### I see a warning in my Windows build: "is Pywin32 present?" Should I worry?

No, not as long as your build finished successfully. This message is generated
by our build system and means that an optional component has not been found.
