## Introduction

Currently NaCl's Chromium integration does not work when combined with
Chromium's [Linux sandbox]
(http://code.google.com/p/chromium/wiki/LinuxSandboxing) (specifically, the
[SUID sandbox](http://code.google.com/p/chromium/wiki/LinuxSUIDSandbox)). Using
NaCl under Chromium requires the options `--internal-nacl --no-sandbox` (e.g.
see [this announcement]
(http://googlechromereleases.blogspot.com/2010/02/dev-channel-update_12.html)).

## Tasks

The following things need to be done to make this work:

*   Make the combination work without `--no-sandbox`. This means making the
    renderer process work.
    *   Currently the plugin gives an assertion failure when it fails to open
        `/dev/urandom`, killing the renderer process. The fix is to open
        `/dev/urandom` before switching on sandboxing. - DONE
    *   The SRPC plugin tries to open `/dev/shm/XXX` to use shared memory, which
        fails. The fix is to create shared memory segments using an RPC to an
        unsandboxed process. - DONE
*   Make sel\_ldr run under the sandbox:
    *   Ensure that standalone sel\_ldr can work under the sandbox. Test this by
        running NaCl's test cases in this sandbox. (Some tests will work but
        others require filesystem access and will not work.)
    *   Ensure that the sel\_ldr side of the NaCl plugin works under the
        sandbox. Is there a way of testing this in isolation from the web
        browser?
    *   Hook it up in Chromium so that sel\_ldr is launched in the sandbox. -
        DONE

## Seccomp sandbox

Running NaCl under the seccomp sandbox raises the following issues:

*   NaCl's internal IMC library uses Linux's Unix domain socket "abstract
    namespace". This involves creating sockets using socket(), bind() and
    connect(), which are blocked by the seccomp sandbox.
    *   Immediate problem: NaClCommonDescMakeBoundSock() in nrd\_xfer.c goes
        into an infinite loop because NaClBoundSocket() repeatedly fails.
    *   We should probably abandon [IMCSockets](imc_sockets.md)' concept of
        SocketAddress/BoundSockets having names. We can use Linux
        SOCK\_SEQPACKET sockets for these endpoints instead; these will be
        unforgeable, not just unguessable.
*   x86's RDTSC instruction. See [this thread]
    (http://groups.google.com/group/native-client-discuss/browse_frm/thread/9ec643194eef0461/0ff10fbdff26db2f).

## Breakpad

The Linux SUID sandbox marks sandboxed processes as undumpable, which stops them
from being ptrace()'d. This appears to prevent the Breakpad crash reporting
system from working, since Breakpad currently uses ptrace() to read processes'
state.
