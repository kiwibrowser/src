# Mojo Embedder Development Kit (EDK)
This document is a subset of the [Mojo documentation](/mojo/README.md).

[TOC]

## Overview

The Mojo EDK is a (binary-unstable) API which enables a process to use Mojo both
internally and for IPC to other Mojo-embedding processes.

Using any of the API surface in `//mojo/edk/embedder` requires a direct
dependency on the GN `//mojo/edk` target. Headers in `mojo/edk/system` are
reserved for internal use by the EDK only.

**NOTE:** Unless you are introducing a new binary entry point into the system
(*e.g.,* a new executable with a new `main()` definition), you probably don't
need to know anything about the EDK API. Most processes defined in the Chrome
repo today already fully initialize the EDK so that Mojo's other public APIs
"just work" out of the box.

## Basic Initialization

In order to use Mojo in a given process, it's necessary to call
`mojo::edk::Init` exactly once:

```
#include "mojo/edk/embedder/embedder.h"

int main(int argc, char** argv) {
  mojo::edk::Init();

  // Now you can create message pipes, write messages, etc

  return 0;
}
```

As it happens though, Mojo is less useful without some kind of IPC support as
well, and that's a second initialization step.

## IPC Initialization

You also need to provide the system with a background TaskRunner on which it can
watch for inbound I/O from any of the various other processes you will later
connect to it.

Here we'll just create a new background thread for IPC and let Mojo use that.
Note that in Chromium, we use the existing "IO thread" in the browser process
and content child processes.

```
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"

int main(int argc, char** argv) {
  mojo::edk::Init();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  // As long as this object is alive, all EDK API surface relevant to IPC
  // connections is usable and message pipes which span a process boundary will
  // continue to function.
  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  return 0;
}
```

This process is now fully prepared to use Mojo IPC!

Note that all existing process types in Chromium already perform this setup
very early during startup.

## Connecting Two Processes

Now suppose you're running a process which has initialized Mojo IPC, and you
want to launch another process which you know will also initialize Mojo IPC.
You want to be able to connect Mojo interfaces between these two processes.
Rejoice, because this section was written just for you.

NOTE: For legacy reasons, some API terminology may refer to concepts of "parent"
and "child" as a relationship between processes being connected by Mojo. This
relationship is today completely orthogonal to any notion of process hierarchy
in the OS, and so use of these APIs is not constrained by an adherence to any
such hierarchy.

Mojo requires you to bring your own OS pipe to the party, and it will do the
rest. It also provides a convenient mechanism for creating such pipes, known as
a `PlatformChannelPair`.

You provide one end of this pipe to the EDK in the local process via
`OutgoingBrokerClientInvitation` - which can also be used to create cross-
process message pipes (see the next section) - and you're responsible for
getting the other end into the remote process.

```
#include "base/process/process_handle.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"

// You write this. It launches a new process, passing the pipe handle
// encapsulated by |channel| by any means possible (e.g. on Windows or POSIX
// you may inhert the file descriptor/HANDLE at launch and pass a commandline
// argument to indicate its numeric value). Returns the handle of the new
// process.
base::ProcessHandle LaunchCoolChildProcess(
    mojo::edk::ScopedInternalPlatformHandle channel);

int main(int argc, char** argv) {
  mojo::edk::Init();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  // This is essentially always an OS pipe (domain socket pair, Windows named
  // pipe, etc.)
  mojo::edk::PlatformChannelPair channel;

  // This is a scoper which encapsulates the intent to connect to another
  // process. It exists because process connection is inherently asynchronous,
  // things may go wrong, and the lifetime of any associated resources is bound
  // by the lifetime of this object regardless of success or failure.
  mojo::edk::OutgoingBrokerClientInvitation invitation;

  base::ProcessHandle child_handle =
      LaunchCoolChildProcess(channel.PassClientHandle());

  // At this point it's safe for |invitation| to go out of scope and nothing
  // will break.
  invitation.Send(child_handle, channel.PassServerHandle());

  return 0;
}
```

The launched process code uses `IncomingBrokerClientInvitation` to get
connected, and might look something like:

```
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"

// You write this. It acquires the ScopedInternalPlatformHandle that was passed by
// whomever launched this process (i.e. LaunchCoolChildProcess above).
mojo::edk::ScopedInternalPlatformHandle GetChannelHandle();

int main(int argc, char** argv) {
  mojo::edk::Init();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  mojo::edk::IncomingBrokerClientInvitation::Accept(GetChannelHandle());

  return 0;
}
```

Now you have IPC initialized between two processes. For some practical examples
of how this is done, you can dig into the various multiprocess tests in the
`mojo_unittests` test suite.

## Bootstrapping Cross-Process Message Pipes

Having internal Mojo IPC support initialized is pretty useless if you don't have
any message pipes spanning the process boundary. Fortunately, this is made
trivial by the EDK: `OutgoingBrokerClientInvitation` has an
`AttachMessagePipe` method which synthesizes a new solitary message pipe
endpoint for your immediate use, and attaches the other end to the invitation
such that it can later be extracted by name by the invitee from the
`IncomingBrokerClientInvitation`.

We can modify our existing sample code as follows:

```
#include "base/command_line.h"
#include "base/process/process_handle.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "local/foo.mojom.h"  // You provide this

base::ProcessHandle LaunchCoolChildProcess(
    const base::CommandLine& command_line,
    mojo::edk::ScopedInternalPlatformHandle channel);

int main(int argc, char** argv) {
  mojo::edk::Init();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  mojo::edk::PlatformChannelPair channel;

  mojo::edk::OutgoingBrokerClientInvitation invitation;

  // Create a new message pipe with one end being retrievable in the new
  // process. Note that the name chosen for the attachment is arbitrary and
  // scoped to this invitation.
  mojo::ScopedMessagePipeHandle my_pipe =
      invitation.AttachMessagePipe("pretty_cool_pipe");

  base::ProcessHandle child_handle =
      LaunchCoolChildProcess(channel.PassClientHandle());
  invitation.Send(
      child_handle,
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  channel.PassServerHandle()));

  // We can start using our end of the pipe immediately. Here we assume the
  // other end will eventually be bound to a local::mojom::Foo implementation,
  // so we can start making calls on that interface.
  //
  // Note that this could even be done before the child process is launched and
  // it would still work as expected.
  local::mojom::FooPtr foo;
  foo.Bind(local::mojom::FooPtrInfo(std::move(my_pipe), 0));
  foo->DoSomeStuff(42);

  return 0;
}
```

and for the launched process:


```
#include "base/run_loop/run_loop.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "local/foo.mojom.h"  // You provide this

mojo::edk::ScopedInternalPlatformHandle GetChannelHandle();

class FooImpl : local::mojom::Foo {
 public:
  explicit FooImpl(local::mojom::FooRequest request)
      : binding_(this, std::move(request)) {}
  ~FooImpl() override {}

  void DoSomeStuff(int32_t n) override {
    // ...
  }

 private:
  mojo::Binding<local::mojom::Foo> binding_;

  DISALLOW_COPY_AND_ASSIGN(FooImpl);
};

int main(int argc, char** argv) {
  mojo::edk::Init();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO));

  mojo::edk::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  GetChannelHandle()));

  mojo::ScopedMessagePipeHandle my_pipe =
      invitation->ExtractMessagePipe("pretty_cool_pipe");

  FooImpl impl(local::mojom::FooRequest(std::move(my_pipe)));

  // Run forever!
  base::RunLoop().Run();

  return 0;
}
```

Note that the above samples assume an interface definition in
`//local/test.mojom` which would look something like:

```
module local.mojom;

interface Foo {
  DoSomeStuff(int32 n);
};
```

Once you've bootstrapped your process connection with a real mojom interface,
you can avoid any further mucking around with EDK APIs or raw message pipe
handles, as everything beyond this point - including the passing of other
interface pipes - can be handled eloquently using
[public bindings APIs](/mojo/README.md#High_Level-Bindings-APIs).

