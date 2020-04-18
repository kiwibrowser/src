# **Mac Sandbox V2 Design Doc**
*Status: Final, Authors: kerrnel@chromium.org,rsesek@chromium.org, Last Updated: 2017-07-10*

# **Objective**

To improve security on macOS by sandboxing the currently unsandboxed
warmup phase of Chromium child processes, and to remove legacy artifacts
in the sandbox profiles by rewriting them to use the most modern
profile features.

# **Background**

Chromium currently runs an unsandboxed warm up routine to acquire
system resources, before entering the sandbox. The design doc
provides a full implementation design and deployment strategy to
sandbox the warmup phase. This document also provides a high level
overview of the macOS provided sandbox.

In the current warm up phase, Chromium calls system frameworks which
acquires an unspecified number of resources before being sandboxed,
and those resources change with every new OS update from Apple.
This [2009 Chromium blog
post](https://blog.chromium.org/2009/06/google-chrome-sandboxing-and-mac-os-x.html&sa=D&ust=1492473048358000&usg=AFQjCNGEbmCLUqoH9-BeudDcNf5NmW-UcQ)
explains that the warmup phase exists because it was unknown how
to determine what resources those APIs used at the time. By explicitly
enumerating all resources in the sandbox profiles, it is possible
to accurately audit Chromium's attack surface for each new macOS
version.

Anyone wishing to know more about the macOS sandbox profile language
should read the [Apple Sandbox
Guide](http://reverse.put.as/wp-content/uploads/2011/09/Apple-Sandbox-Guide-v1.0.pdf)
from reverse.put.as, or see the Appendix of this doc.

# **Current versus New Implementation**

The new sandbox will continue to use the OS provided sandboxing
framework and the "deny resource access by default" policy that the
sandbox currently uses. The change introduced by the new implementation
is the removal of the unsandboxed warmup phase.

# **Compatibility and Security Risk**

Chromium's sandbox incurs two levels of risk which must be weighed:
compatibility risk and security risk. Compatibility risk is the
risk that Apple changes the resources that a system framework
accesses, causing Chromium's sandbox to block the access. Security
risk is the possibility that Chromium will be compromised because the
sandbox allowed access to dangerous resources.

A more permissive sandbox profile reduces compatibility risk but
increases security risk and vice versa. See
[crbug.com/619981](https://bugs.chromium.org/p/chromium/issues/detail?id=619981)
for an example of how the current sandbox implementation already
incurs compatibility risk in favor of security risk.

The current sandbox also incurs a large security risk at the warmup
time, since that phase is unsandboxed. Rather than incurring both
compatibility and security risk during execution, the new sandbox
shifts the risk profile to take on more compatibility risk, since
the warmup phase is eliminated, but it eliminates security risk.

# **Success Criteria**

The V2 sandbox will be deemed a success when it has been deployed
to stable users for all processes in Chromium, and Chromium does not
lose functionality due to the new sandbox rules.

# **Design Overview**

The Chromium Helper executable will now receive a sandbox profile
and list of sandbox parameters (for example, the location of the
user's home directory). The Helper executable will then apply the
sandbox profile, with the parameters, to the process and then
continue its execution into the ChromeMain function.

# **Detailed Design**

## Sandbox Profile Principles

The sandbox profiles will use the following principles when
implementing the profiles for each process type.
*   The profiles deny resource access to anything not explicitly allowed.
*   Processes may access all system libraries and frameworks.
*   Explicitly list which system resources (files, folders, mach services, sysctls, IPC, IOKit calls) Chromium may access. Analyze each resource access for safety first.

## Sandbox Design

The current sandbox code lives in content/common/sandbox_mac.mm.
This file will continue to exist until the current sandbox is
officially deprecated and removed for all process types. Chromium
will use a switch to skip the current sandboxing code for each
process type as it is ported to the new sandbox.

The new sandbox will rewrite the Chromium code and profiles from
scratch, in order to leverage the most modern OS APIs and constructs,
which are easier to use and maintain.

## Code Structure

Chromium's architecture is multi-process with a main browser process
and some number of helper processes which performs tasks such as
rendering web pages, running GPU processes, or various utility
functions. The main browser process launches the other processes
using the "Google Chromium Helper" executable. The browser passes
command line flags to the Helper executable indicating what type
of process it should execute as.

The browser will now create a pipe to the Helper executable, and
using that pipe, the Helper executable will read a protobuf message
from the browser process. The profobuf message will contain the
profile string and a map of the sandbox parameters, and using a
lightweight protobuf prevents the error prone parsing code for a
whole parameters list from being re-implemented. This must happen
in the main executable itself, as the process must be sandboxed
before the framework is loaded.

The Helper executable will immediately initialize the sandbox, using
the profile and parameters, before continuing execution to the
ChromiumMain function.

# **Design Alternatives**

One alternative design is the current design where process acquires
numerous system resources early in the execution phase and then
applies the sandbox. However, this means that we do not have a
consistent and auditable way to track what resources Chromium is using
over time. Chromium's attack surface can increase significantly with
a new OS release, and we would not know. The new explicit profiles
make the attack surface very auditable and easy to understand.

Another alternative would have been the [Bootstrap
Sandbox](https://docs.google.com/presentation/d/1Npdbl7UF06wgKRsRWakiI3-qIlPDNN_-AT8H5tQjbYc/edit#slide=id.p)
plan, which proposed intercepting all mach connections to limit the
attack surface. However, with the "new" launchd released in OS X
10.10 Yosemite, messages could no longer be intercepted and forward.
This design solves the same problem by sandboxing the warmup phase,
and allowing the existing system sandbox to enforce the security.

# **Appendix**

## Sandbox Profile Language

This document has referenced sandbox profiles, which are written
in a subset of Scheme. This is a macOS supplied format called Sandbox
Profile Language (SBPL). As there is no official OS-provided
documentation for the SBPL, this design doc provides a short
description below so that other engineers can understand the
constructs that Chromium is leveraging.

SBPL files accept parameters. A parameter is a way to pass variable
information into the sandbox profile that is not known at compile-time,
for example the location of the user's home directory. A short
sample profile which uses parameters is below.


```
(version 1)
(deny default)
(allow file-read* (subpath (param "USER_HOME_DIR")))
```

This profile will allow the application to read all files in the
user's home directory, and nothing else. The (subpath) directive
says that it applies to all paths underneath the specified path,
and not just the actual path. The following code will apply the
sandbox, pass the parameters, and access a file in the home directory.


```
#import <Foundation/Foundation.h>
#import <sandbox.h>
#import <sys/stat.h>
// sandbox_init* functions are not in the public header.
int sandbox_init_with_parameters(const char *profile,
                                 uint64_t flags,
                                 const char *const parameters[],
                                 char **errorbuf);

int main(int argc, const char * argv[]) {
  const char profile[] = "(version 1)" \
      "(deny default)" \
      "(allow file-read* (subpath (param \"USER_HOME_DIR\")))";

  const char* home_dir = [NSHomeDirectory() UTF8String];
  // Parameters are passed as an array containing key,value,NULL.
  const char* parameters[] = { "USER_HOME_DIR", home_dir, NULL };

  if (sandbox_init_with_parameters(profile, 0, parameters, NULL))
    exit(1);

  const char* vim_rc =
      [[NSHomeDirectory() stringByAppendingString:@"/.vimrc"]
          UTF8String];

  struct stat sb;
  if (stat(vim_rc, &sb) == 0)
    printf(".vimrc file exists\n");

  return 0;
}
```

The following information must be collected as parameters for the
sandbox profiles to function:

*   Whether or not the sandbox should log failures
*   The user's home directory
*   Information about the specific OS version (10.9, 10.10, etc.)
*   The path to the main `Google Chromium.app` bundle
*   The PID of the Chromium process
*   If applicable, the path to the permitted directory that the process can read and write to.

The sandbox directives most commonly used in Chromium's V2 profiles
are documented here for future reference.

```(deny default)``` - All resource access is denied by default. The
sandbox must list allow rules for all resources.

```(path "/path")``` - Match exactly this path.

```(subpath "/path")``` - Match this path and all subpaths of this
path. ("/path", "/path/foo", etc.)

```(allow process-exec*)``` - Allows the specified binary to be
executed as a process.

```(allow mach-lookup (global-name "com.apple.system.logger"))``` -
Allows the process to communicate with the specified system service.

```(allow sysctl-read)``` - Allow access to sysctl() to read property
values.
