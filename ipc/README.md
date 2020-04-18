# Converting Legacy Chrome IPC To Mojo

Looking for [Mojo Documentation](/mojo)?

[TOC]

## Overview

The `//ipc` directory contains interfaces and implementation for Chrome's
legacy IPC system, including `IPC::Channel` and various macros for defining
messages and type serialization. For details on using this system please see the
original
[documentation](https://www.chromium.org/developers/design-documents/inter-process-communication).

Legacy IPC is **deprecated**, and Chrome developers are strongly discouraged
from introducing new messages using this system. [Mojo](/mojo) is the correct
IPC system to use moving forward. This document introduces developers to the
various tools available to help with conversion of legacy IPC messages to Mojo.
It assumes familiarity with [Mojom](/mojo/public/tools/bindings) syntax and
general use of Mojo [C++ bindings](/mojo/public/cpp/bindings).

In traditional Chrome IPC, we have One Big Pipe (the `IPC::Channel`) between
each connected process. Sending an IPC from one process to another means knowing
how to get a handle to the Channel interface (*e.g.,*
[RenderProcessHost::GetChannel](
https://cs.chromium.org/chromium/src/content/public/browser/render_process_host.h?rcl=722be98f7e4d7551710fb9cf30674750cdd0d857&l=196)
when sending from the browser to a renderer process), and then having either an
`IPC::MessageFilter` or some other appropriate `IPC::Listener` implementation
installed in the right place on the other side of the channel.

Because of this arrangement, any message sent on a channel is sent in FIFO order
with respect to all other messages on the channel. While this may be easier to
reason about in general, it carries with it the unfortunate consequence that
many unrelated messages in the system have an implicit, often unintended
ordering dependency.

It's primarily for this reason that conversion to Mojo IPC can be more
challenging than would otherwise be necessary, and that is why we have a number
of different tools available to facilitate such conversions.

## Deciding What to Do

There are few questions you should ask yourself before embarking upon any IPC
message conversion journey. Should this be part of a service? Does message
ordering matter with respect to other parts of the system? What is the meaning
of life?

### Moving Messages to Services

We have a small but growing number of services defined in
[`//services`](https://cs.chromium.org/chromium/src/services), each of which has
some set of public interfaces defined in their `public/interfaces` subdirectory.
In the limit, this is the preferred destination for any message conversions
pertaining to foundational system services (more info at
[https://www.chromium.org/servicification](https://www.chromium.org/servicification).)
For other code it may make sense to introduce services elsewhere (*e.g.*, in
`//chrome/services` or `//components/services`), or to simply
avoid using services altogether for now and instead define some one-off Mojom
interface alongside the old messages file.

If you need help deciding where a message should live, or if you feel it would
be appropriate to introduce a new service to implement some feature or large set
of messages, please post to
[`services-dev@chromium.org`](https://groups.google.com/a/chromium.org/forum/#!forum/services-dev)
with questions, concerns, and/or a brief proposal or design doc describing
the augmentation of an existing service or introduction of a new service.

See the [Using Services](#Using-Services) section below for details.

When converting messages that still require tight coupling to content or Chrome
code or which require unchanged ordering with respect to one or more remaining
legacy IPC messages, it is often not immediately feasible to move a message
definition or handler implementation into a service.

### Moving Messages to Not-Services

While this isn't strictly possible because everything is a service now, we model
all existing content processes as service instances and provide helpers to make
interface exposure and consumption between them relatively easy.

See [Using Content's Connectors](#Using-Content_s-Connectors) for details on
the recommended way to accomplish this.

See
[Using Content's Interface Registries](#Using-Content_s-Interface-Registries)
for details on the **deprecated** way to accomplish this.

Note that when converting messages to standalone Mojo interfaces, every
interface connection operates 100% independently of each other. This means that
ordering is only guaranteed over a single interface (ignoring
[associated interfaces](/mojo/public/tools/bindings#Associated-Interfaces).)
Consider this example:

``` cpp
mojom::FrobinatorPtr frob1;
RenderThread::Get()->GetConnector()->BindInterface(
    foo_service::mojom::kServiceName, &frob1);

mojom::FrobinatorPtr frob2;
RenderThread::Get()->GetConnector()->BindInterface(
    foo_service::mojom::kServiceName, &frob2);

// These are ordered on |frob1|.
frob1->Frobinate(1);
frob1->Frobinate(2);

// These are ordered on |frob2|.
frob2->Frobinate(1);
frob2->Frobinate(2);

// It is entirely possible, however, that the renderer receives:
//
// [frob1]Frobinate(1)
// [frob2]Frobinate(1)
// [frob1]Frobinate(2)
// [frob2]Frobinate(2)
//
// Because |frob1| and |frob2| guarantee no mutual ordering.
```

Also note that neither interface is ordered with respect to legacy
`IPC::Channel` messages. This can present significant problems when converting a
single message or group of messages which must retain ordering with respect to
others still on the Channel.

### When Ordering Matters

If ordering really matters with respect to other legacy messages in the system,
as is often the case for *e.g.* frame and navigation-related messages, you
almost certainly want to take advantage of
[Channel-associated interfaces](#Using-Channel-associated-Interfaces) to
eliminate any risk of introducing subtle behavioral changes.

Even if ordering only matters among a small set of messages which you intend to
move entirely to Mojom, you may wish to move them one-by-one in separate CLs.
In that case, it may make sense to use a Channel-associated interface during the
transitional period. Once all relevant messages are fully relocated into a
single Mojom interface, it's trivial to lift the interface away from Channel
association and into a proper independent service connection.

## Using Services

Suppose you have some IPC messages for safely decoding a PNG image:

``` cpp
IPC_MESSAGE_CONTROL2(UtilityMsg_DecodePNG,
                     int32_t request_id,
                     std::string /* png_data */);
IPC_MESSAGE_CONTROL2(UtilityHostMsg_PNGDecoded,
                     int32_t request_id,
                     int32_t width, int32_t height,
                     std::string /* rgba_data */);
```

This seems like a perfect fit for an addition to the sandboxed `data_decoder`
service. Your first order of business is to translate this into a suitable
public interface definition within that service:

``` cpp
// src/services/data_decoder/public/mojom/png_decoder.mojom
module data_decoder.mojom;

interface PngDecoder {
  Decode(array<uint8> png_data)
      => (int32 width, int32 height, array<uint32> rbga_data);
};
```

and you'll also want to define the implementation within
`//services/data_decoder`, pluging in some appropriate binder so the service
knows how to bind incoming interface requests to your implementation:

``` cpp
// src/services/data_decoder/png_decoder_impl.h
class PngDecoderImpl : public mojom::PngDecoder {
 public:
  static void BindRequest(mojom::PngDecoderRequest request) { /* ... */ }

  // mojom::PngDecoder:
  void Decode(const std::vector<uint8_t>& png_data,
              const DecodeCallback& callback) override { /* ... */ }
  // ...
};

// src/services/data_decoder/data_decoder_service.cc
// Not quite legitimate pseudocode...
DataDecoderService::DataDecoderService() {
  // ...
  registry_.AddInterface(base::Bind(&PngDecoderImpl::BindRequest));
}
```

and finally you need to update the usage of the old IPC by probably deleting
lots of ugly code which sets up a `UtilityProcessHost` and replacing it with
something like:

``` cpp
void OnDecodedPng(const std::vector<uint8_t>& rgba_data) { /* ... */ }

data_decoder::mojom::PngDecoderPtr png_decoder;
connector->BindInterface(data_decoder::mojom::kServiceName,
                         mojo::MakeRequest(&png_decoder));
png_decoder->Decode(untrusted_png_data, base::Bind(&OnDecodedPng));
```

Where to get a [`Connector`](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/connector.h)
is an interesting question, and the answer ultimately depends on where your code
is written. All service instances get a primordial `Connector` which can be
cloned arbitrarily many times and passed around to different threads.

If you're writing service code the answer is trivial since each `Service`
instance has direct access to a `Connector`. If you're writing code at or above
the content layer, the answer is slightly more interesting and is explained in
the [Using Content's Connectors](#Using-Content_s-Connectors) section below.

## Using Content's Connectors

As explained earlier in this document, all content processes are modeled as
service instances today. This means that all content processes have at least
one [`Connector`](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/connector.h)
instance which can be used to bind interfaces exposed by other services.

We define [`content::ServiceManagerConnection`](https://cs.chromium.org/chromium/src/content/public/common/service_manager_connection.h?rcl=dd92156efac57169b45aeb0094111b8d94302b12&l=38)
as a helper which fully encapsulates the service instance state within a given
Content process. The main thread of the browser process can access the global
instance by calling
[`content::ServiceManager::GetForProcess()`](https://cs.chromium.org/chromium/src/content/public/common/service_manager_connection.h?rcl=dd92156efac57169b45aeb0094111b8d94302b12&l=56),
and this object has a `GetConnector()` method which exposes the `Connector` for
that process.

The main thread of any Content child process can use
`content::ChildThread::GetServiceManagerConnection` or
`content::ChildThread::GetConnector` directly.

For example, any interfaces registered in
[`RenderProcessHostImpl::RegisterMojoInterfaces()`](https://cs.chromium.org/chromium/src/content/browser/renderer_host/render_process_host_impl.cc?rcl=dd92156efac57169b45aeb0094111b8d94302b12&l=1203)
can be acquired by a renderer as follows:

``` cpp
mojom::LoggerPtr logger;
content::RenderThread::Get()->GetConnector()->BindInterface(
    content::mojom::kBrowserServiceName, &logger);
logger->Log("Message to log here");
```

Usually `logger` will be saved in a field at construction time, so the
connection is only created once. There may be situations where you want to
create one connection per request, e.g. a new instance of the Mojo
implementation is created with some information about the request, and any
responses for this request go straight to that instance.

### On Other Threads

`Connector` instances can be created and asynchronously associated with each
other to maximize flexibility in when and how outgoing interface requests are
initiated.

For example if a background (*e.g.,* worker) thread in a renderer process wants
to make an outgoing service request, it can construct its own `Connector` --
which may be used immediately and retained on that thread -- and asynchronously
associate it with the main-thread `Connector` like so:

``` cpp
class Logger {
 public:
  explicit Logger(scoped_refptr<base::TaskRunner> main_thread_task_runner) {
    service_manager::mojom::ConnectorRequest request;

    // Of course we could also retain |connector| if we intend to use it again.
    auto connector = service_manager::Connector::Create(&request);
    // Replace service_name with the name of the service to bind on, e.g.
    // content::mojom::kBrowserServiceName.
    connector->BindInterface("service_name", &logger_);
    logger_->Log("Test Message.");

    // Doesn't matter when this happens, as long as it happens eventually.
    main_thread_task_runner->PostTask(
        FROM_HERE, base::BindOnce(&Logger::BindConnectorOnMainThread,
                                  std::move(request)));
  }

 private:
  static void BindConnectorOnMainThread(
      service_manager::mojom::ConnectorRequest request) {
    DCHECK(RenderThreadImpl::Get());
    RenderThreadImpl::Get()->GetConnector()->BindConnectorRequest(
        std::move(request));
  }

  mojom::LoggerPtr logger_;

  DISALLOW_COPY_AND_ASSIGN(Logger);
};
```

## Using Content's Interface Registries

**NOTE:** This section is here mainly for posterity and documentation of
existing usage. Please use `Connector` instead of using an `InterfaceProvider`
directly.

For convenience the Service Manager's
[client library](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/)
exposes two useful types: `InterfaceRegistry` and `InterfaceProvider`. These
objects generally exist as an intertwined pair with an `InterfaceRegistry` in
one process and a corresponding `InterfaceProvider` in another process.

The `InterfaceRegistry` is essentially just a mapping from interface name
to binder function:

``` cpp
void BindFrobinator(mojom::FrobinatorRequest request) {
  mojo::MakeStrongBinding(std::make_unique<FrobinatorImpl>, std::move(request));
}

// |registry| will hereby handle all incoming requests for "mojom::Frobinator"
// using the above function, which binds the request pipe handle to a new
// instance of |FrobinatorImpl|.
registry->AddInterface(base::Bind(&BindFrobinator));
```

while an `InterfaceProvider` exposes a means of requesting interfaces from a
remote `InterfaceRegistry`:

``` cpp
mojom::FrobinatorPtr frob;

// MakeRequest creates a new pipe, and GetInterface sends one end of it to
// the remote InterfaceRegistry along with the "mojom::Frobinator" name. The
// other end of the pipe is bound to |frob| which may immediately begin sending
// messages.
provider->GetInterface(mojo::MakeRequest(&frob));
frob->DoTheFrobinator();
```

For convenience, we stick an `InterfaceRegistry` and corresponding
`InterfaceProvider` in several places at the Content layer to facilitate
interface connection between browser and renderer processes:

| `InterfaceRegistry`                         | `InterfaceProvider`                        |
|---------------------------------------------|--------------------------------------------|
| `RenderProcessHost::GetInterfaceRegistry()` | `RenderThreadImpl::GetRemoteInterfaces()`  |
| `RenderThreadImpl::GetInterfaceRegistry()`  | `RenderProcessHost::GetRemoteInterfaces()` |
| `RenderFrameHost::GetInterfaceRegistry()`   | `RenderFrame::GetRemoteInterfaces()`       |
| `RenderFrame::GetInterfaceRegistry()`       | `RenderFrameHost::GetRemoteInterfaces()`   |

As noted above, use of these registries is generally discouraged.

### Deciding Which Interface Registry to Use

Once you have an implementation of a Mojo interface, the next thing to decide is
which registry and service to register it on.

For browser/renderer communication, you can register your Mojo interface
implementation in either the Browser or Renderer process (whichever side the
interface was implemented on). Usually, this involves calling `AddInterface()`
on the correct registry, passing a method that takes the Mojo Request object
(e.g. `sample::mojom::LoggerRequest`) and binding it (e.g.
`mojo::MakeStrongBinding()`, `bindings_.AddBinding()`, etc). Then the class that
needs this API can call `BindInterface()` on the connector for that process,
e.g.
`RenderThread::Get()->GetConnector()->BindInterface(mojom::kBrowserServiceName, std::move(&mojo_interface_))`.

**NOTE:** `content::ServiceManagerConnection::GetForProcess()` must be called in
the browser process on the main thread, and its connector can only be used on
the main thread; but you can clone connectors and move the clones around to
other threads. A `Connector` is only bound to the thread which first calls into
it.

Depending on what resources you need access to, the main classes are:

| Renderer Class  | Corresponding Browser Class |  Explanation                                                                                                       |
|-----------------|-----------------------------|--------------------------------------------------------------------------------------------------------------------|
| `RenderFrame`   | `RenderFrameHost`           |  A single frame. Use this for frame-to-frame messages.                                                             |
| `RenderView`    | `RenderViewHost`            | A view (conceptually a 'tab'). You cannot send Mojo messages to a `RenderView` directly, since frames in a tab can be in multiple processes (and the classes are deprecated). Migrate these to `RenderFrame` instead, or see section [Migrating IPC calls to `RenderView` or `RenderViewHost`](#UMigrating-IPC-calls-to-RenderView-or-RenderViewHost).  |
| `RenderProcess` | `RenderProcessHost`         | A process, containing multiple frames (probably from the same origin, but not always).                             |

**NOTE:** Previously, classes that ended with `Host` were implemented on the
browser side; the equivalent classes on the renderer side had the same name
without the `Host` suffix. We have since deviated from this convention since
Mojo interfaces are not intended to prescribe where their endpoints live, so
future classes should omit such suffixes and just describe the interface they
are providing.

Of course, any combination of the above is possible, e.g. `RenderProcessHost`
can register a Mojo interface that can be called by a `RenderFrame` (this would
be a way of the browser communicating with multiple frames at once).

Once you know which class you want the implementation to be registered in, find
the corresponding `Impl` class (e.g. `RenderProcessImpl`). There should be a
`RegisterMojoInterfaces()` method where you can add calls to `AddInterface`,
e.g. For a strong binding:

```cpp
  registry->AddInterface(base::Bind(&Logger::Create, GetID()));
```

Then in `Logger` we add a static `Create()` method that takes the
`LoggerRequest` object:

```cpp
// static
void Logger::Create(int render_process_id,
                        mojom::LoggerRequest request) {
  mojo::MakeStrongBinding(std::make_unique<Logger>(render_process_id),
                          std::move(request));
}
```

For a `BindingSet`, we can store a `std::unique_ptr<Logger>` on the
`RenderProcessHost` instead, e.g.:

```cpp
// render_process_host_impl.h:
std::unique_ptr<Logger> logger_;

// render_process_host_impl.cc:
logger_ = std::make_unique<Logger>(GetID());
registry->AddInterface(base::Bind(&Logger::BindRequest,
                       base::Unretained(logger_.get())));
```

Then in `Logger` we define the `BindRequest` method:

```h
class Logger : public sample::mojom::Logger {
 public:
  explicit Logger(int render_process_id);
  ~Logger() override;

  void BindRequest(mojom::LoggerRequest request);

  // sample::mojom::Logger:
  void Log(const std::string& message) override;
  void GetTail(GetTailCallback callback) override;

 private:
  mojo::BindingSet<sample::mojom::Logger> bindings_;

  DISALLOW_COPY_AND_ASSIGN(Logger);
};
```

```cpp
void Logger::BindRequest(mojom::LoggerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}
```

#### Setting up Capabilities

Once you've registered your interface, you need to add capabilities (resolved at
runtime) to the corresponding capabilities manifest json file.

The service manifest files (which contain the capability spec) are located in
[/content/public/app/mojo/](/content/public/app/mojo/). As a general rule, the
file you want to edit is the service which *provides* the interface (the side
which instantiates the implementation), and the part of the file you want to add
the name of the interface to is the service which *calls* the interface (i.e.
the side containing `LoggerPtr`).

You can usually just run your Mojo code and look at the error messages. The
errors look like:

```sh
[ERROR:service_manager.cc(158)] Connection InterfaceProviderSpec prevented
service: content_renderer from binding interface: content.mojom.Logger
exposed by: content_browser
```

This means something in the renderer process (called "content_renderer") was
trying to bind to `content.mojom.Logger` in the browser process (called
"content_browser"). To add a capability for this, we need to find the json file
with the capabilities for "content_browser", and add our new interface with name
`content.mojom.Logger` to the "renderer" section.

In this example, the capabilities for "content_browser" are implemented in
[content_browser_manifest.json](/content/public/app/mojo/content_browser_manifest.json).
It should look like:

```json
{
  "name": "content_browser",
  "display_name": "Content (browser process)",
  "interface_provider_specs": {
    "service_manager:connector": {
      "provides": {
        // ...
        "renderer": [
          //...
```

To add permission for `content.mojom.Logger`, add the string
`"content.mojom.Logger"` to the "renderer" list.

Similarly, if the error was:

```sh
[ERROR:service_manager.cc(158)] Connection InterfaceProviderSpec prevented
service: content_browser from binding interface: content.mojom.Logger exposed
by: content_renderer
```

We would want the
`interface_provider_specs.service_manager:connector.provides.browser` section in
[content_renderer_manifest.json](/content/public/app/mojo/content_renderer_manifest.json)
(which defines the capabilities for `content_renderer`).

TODO: Add more details on permission manifests here

## Using Channel-associated Interfaces

**NOTE**: Channel-associated interfaces are an interim solution to make the
transition to Mojo IPC easier in Chrome. You should not design new features
which rely on this system. The ballpark date of deletion for `IPC::Channel` is
projected to be somewhere around mid-2019, and obviously Channel-associated
interfaces can't live beyond that point.

Mojo has support for the concept of
[associated interfaces](/mojo/public/tools/bindings#Associated-Interfaces).
One interface is "associated" with another when it's a logically separate
interface but it shares an underlying message pipe, causing both interfaces to
maintain mutual FIFO message ordering. For example:

``` cpp
// db.mojom
module db.mojom;

interface Table {
  void AddRow(string data);
};

interface Database {
  QuerySize() => (uint64 size);
  AddTable(associated Table& table)
};

// db_client.cc
db::mojom::DatabasePtr db = /* ... get it from somewhere... */
db::mojom::TableAssociatedPtr new_table;
db->AddTable(mojo::MakeRequest(&new_table));
new_table->AddRow("my hovercraft is full of eels");
db->QuerySize(base::Bind([](uint64_t size) { /* ... */ }));
```

In the above code, the `AddTable` message will always arrive before the `AddRow`
message, which itself will always arrive before the `QuerySize` message. If the
`Table` interface were not associated with the `Database` pipe, it would be
possible for the `QuerySize` message to be received before `AddRow`,
potentially leading to unintended behavior.

The legacy `IPC::Channel` used everywhere today is in fact just another Mojo
interface, and developers have the ability to associate other arbitrary Mojo
interfaces with any given Channel. This means that you can define a set of Mojo
messages to convert old IPC messages, and implement them in a way which
perfectly preserves current message ordering.

There are many different facilities in place for taking advantage of
Channel-associated interfaces, and the right one for your use case depends on
how the legacy IPC message is used today. The subsections below cover various
interesting scenarios.

### Basic Usage

The most primitive way to use Channel-associated interfaces is by working
directly with `IPC::Channel` (IO thread) or more commonly `IPC::ChannelProxy`
(main thread). There are a handful of interesting interface methods here.

**On the IO thread** (*e.g.,* typically when working with process hosts that
aren't for render processes), the interesting methods are as follows:

[`IPC::Channel::GetAssociatedInterfaceSupport`](https://cs.chromium.org/chromium/src/ipc/ipc_channel.h?rcl=a896ff44a395a50ab18f5120f20b7eb5a9550247&l=235)
returns an object for working with interfaces associated with the `Channel`.
This is never null.

[`IPC::Channel::AssociatedInterfaceSupport::AddAssociatedInterface<T>`](
https://cs.chromium.org/chromium/src/ipc/ipc_channel.h?rcl=a896ff44a395a50ab18f5120f20b7eb5a9550247&l=115)
allows you to add a binding function to handle all incoming requests for a
specific type of associated interface. Callbacks added here are called on the IO
thread any time a corresponding interface request arrives on the `Channel.` If
no callback is registered for an incoming interface request, the request falls
through to the Channel's `Listener` via
[`IPC::Listener::OnAssociatedInterfaceRequest`](https://cs.chromium.org/chromium/src/ipc/ipc_listener.h?rcl=a896ff44a395a50ab18f5120f20b7eb5a9550247&l=40).

[`IPC::Channel::AssociatedInterfaceSupport::GetRemoteAssociatedInterface<T>`](
https://cs.chromium.org/chromium/src/ipc/ipc_channel.h?rcl=a896ff44a395a50ab18f5120f20b7eb5a9550247&l=124)
requests a Channel-associated interface from the remote endpoint of the channel.

**On the main thread**, typically when working with `RenderProcessHost`, basic
usage involves calls to
[`IPC::ChannelProxy::GetRemoteAssociatedInterface<T>`](
https://cs.chromium.org/chromium/src/ipc/ipc_channel_proxy.h?rcl=a896ff44a395a50ab18f5120f20b7eb5a9550247&l=196)
when making outgoing interface requests, or some implementation of
[`IPC::Listener::OnAssociatedInterfaceRequest`](https://cs.chromium.org/chromium/src/ipc/ipc_listener.h?rcl=a896ff44a395a50ab18f5120f20b7eb5a9550247&l=40)
when handling incoming ones.

TODO - Add docs for using AssociatedInterfaceRegistry where possible.

### BrowserMessageFilter

[BrowserMessageFilter](https://cs.chromium.org/chromium/src/content/public/browser/browser_message_filter.h?rcl=805f2ca5aa0902f56885ea3c8c0a42cb80d84522&l=37)
is a popular helper for listening to incoming legacy IPC messages on the browser
process IO thread and (typically) handling them there.

A common and totally reasonable tactic for converting a group of messages on an
existing `BrowserMessageFilter` is to define a similiarly named Mojom interface
in an inner `mojom` namespace (*e.g.,* a `content::FooMessageFilter` would have
a corresponding `content::mojom::FooMessageFilter` interface), and have the
`BrowserMessageFilter` implementation also implement
`BrowserAssociatedInterface<T>`.

Real code is probably the most useful explanation, so here are some example
conversion CLs which demonstrate practical `BrowserAssociatedInterface` usage.

[FrameHostMsg_SetCookie](https://codereview.chromium.org/2167513003) - This
CL introduces a `content::mojom::RenderFrameMessageFilter` interface
corresponding to the existing `content::RenderFrameMessageFilter` implementation
of `BrowserMessageFilter`. Of particular interest is the fact that it only
converts **one** of the messages on that filter. This is fine because ordering
among the messages -- Mojom or otherwise -- is unchanged.

[FrameHostMsg_GetCookie](https://codereview.chromium.org/2202723005) - A small
follow-up to the above CL, this converts another message on the same filter. It
is common to convert a large group of messages one-by-one in separate CLs. Also
note that this message, unlike the one above on the same interface, is
synchronous.

[ViewHostMsg_GenerateRoutingID](https://codereview.chromium.org/2351333002) -
Another small CL to introduce a new `BrowserAssociatedInterface`.

### Routed Per-Frame Messages To the Browser

Many legacy IPC messages are "routed" -- they carry a routing ID parameter which
is interpreted by the channel endpoint and used to pass a received message on to
some other more specific handler.

Messages received by the browser with a frame routing ID for example are routed
to the RenderFrameHost's owning [`WebContents`](https://cs.chromium.org/chromium/src/content/browser/web_contents/web_contents_impl.h?rcl=a12e52d81346dd23d8284763d44c4ee657f11cea&l=447)
with the corresponding `RenderFrameHostImpl` as additional context.

[This CL](https://codereview.chromium.org/2310583002) introduces usage of
[`WebContentsFrameBindingSet<T>`](https://cs.chromium.org/chromium/src/content/public/browser/web_contents_binding_set.h?rcl=d364139fee76154a1d9fa8875ad0cbb5ccb523c3&l=112), which helps establish
per-frame bindings for Channel-associated interfaces. Some hidden magic is done
to make it so that interface requests from a remote
[`RenderFrame AssociatedInterfaceProvider`](https://cs.chromium.org/chromium/src/content/public/renderer/render_frame.h?rcl=d364139fee76154a1d9fa8875ad0cbb5ccb523c3&l=170)
are routed to the appropriate `WebContentsFrameBindingSet`, typically installed
(as in this CL) by a `WebContentsObserver`.

When a message is received by an interface implementation using a
`WebContentsFrameBindingSet`, that object's `dispatch_context()` can be used
to retrieve the `RenderFrameHostImpl` targeted by the message. See the above CL
for additional clarity.

### Other Routed Messages To the Browser

Other routing IDs are used when targeting either specific `RenderViewHost` or
`RenderWidgetHost` instances. We don't currently have any facilities in place
to assist with these conversions. Because render views are essentially a
deprecated concept, messages targeting "view" routes should not be converted
as-is, but should instead be moved to target either widgets or frames
accordingly.

Facilities to assist in conversion of widget-routed messages may be added in the
future. Who knows, maybe you'll be the brave developer to add them (and to then
update this documentation, of course!) If you decide this is exactly what you
need but are nervous about the prospect of writing it yourself, please send a
friendly message to [chromium-mojo@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-mojo)
explaining the use case so we can help you get things done.

### Messages to a Renderer

[This CL](https://codereview.chromium.org/2381493003) converts `ViewMsg_New`
to a Mojo interface, by virtue of the fact that
`IPC::ChannelProxy::GetRemoteAssociatedInterface` from the browser process
results in an associated interface request arriving at
`ChildThreadImpl::OnAssociatedInterfaceRequest` in the corresponding child
process.

Similar message conversions are done by [this CL](https://codereview.chromium.org/2400313002).

Note that we do not currently have any helpers for converting routed messages
from browser to renderer. Please confer with
[chromium-mojo@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-mojo)
if such a use case is blocking your work.

## Miscellany

### Using Legacy IPC Traits

InsSome circumstances there may be a C++ enum, struct, or class that you want
to use in a Mojom via [type mapping](/mojo/public/cpp/bindings#Type-Mapping),
and that type may already have `IPC::ParamTraits` defined (possibly via
`IPC_STRUCT_TRAITS*` macros) for legacy IPC.

If this is the case and the Mojom which uses the type will definitely only be
called from and implemented in C++ code, *and* you have sufficient reason to
avoid moving or duplicating the type definition in Mojom, you can take advantage
of the existing `ParamTraits`.

#### The [Native] Attribute
In order to do this you must declare a placeholder type in Mojom somewhere, like
so:

```
module foo.mojom;

[Native]
enum WindowType;

[Native]
struct MyGiganticStructure;
```

The rest of your Mojom will use this typename when referring to the type, but
the wire format used is defined entirely by `IPC::ParamTraits<T>` for whatever
`T` to which you typemap the Mojom type. For example if you typemap
`foo::mojom::MyGiganticStructure` to `foo::MyGiganticStructure`, your typemap
must point to some header which defines
`IPC::ParamTraits<foo::MyGiganticStructure>`.

There are several examples of this traits implementation in common IPC traits
defined [here](https://code.google.com/p/chromium/codesearch#chromium/src/ipc/ipc_message_utils.h).

#### A Practical Example

Given the [`resource_messages.h`](https://cs.chromium.org/chromium/src/content/common/resource_messages.h?rcl=2e7a430d8d88222c04ab3ffb0a143fa85b3cec5b&l=215)
header with the following definition:

``` cpp
IPC_STRUCT_TRAITS_BEGIN(content::ResourceRequest)
  IPC_STRUCT_TRAITS_MEMBER(method)
  IPC_STRUCT_TRAITS_MEMBER(url)
  // ...
IPC_STRUCT_TRAITS_END()
```

and the [`resource_request.h`](https://cs.chromium.org/chromium/src/content/common/resource_request.h?rcl=dce9e476a525e4ff0304787935dc1a8c38392ac8&l=32)
header with the definition for `content::ResourceRequest`:

``` cpp
namespace content {

struct CONTENT_EXPORT ResourceRequest {
  // ...
};

}  // namespace content
```

we can declare a corresponding Mojom type:

```
module content.mojom;

[Native]
struct URLRequest;
```

and add a typemap like [url_request.typemap](https://cs.chromium.org/chromium/src/content/common/url_request.typemap)
to define the mapping:

```
mojom = "//content/public/common/url_loader.mojom"
public_headers = [ "//content/common/resource_request.h" ]
traits_headers = [ "//content/common/resource_messages.h" ]
...
type_mappings = [ "content.mojom.URLRequest=content::ResourceRequest" ]
```

Note specifically that `public_headers` includes the definition of the native
C++ type, and `traits_headers` includes the definition of the legacy IPC traits.

Finally, note that this same approach can be used to leverage existing
`IPC_ENUM_TRAITS` for `[Native]` Mojom enum aliases.

### Typemaps With Content and Blink Types

Using typemapping for messages that go between Blink and content browser code
can sometimes be tricky due to things like dependency cycles or confusion over
the correct place for some definition to live. There are some example
CLs provided here, but feel free to also contact
[chromium-mojo@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-mojo)
with specific details if you encounter trouble.

[This CL](https://codereview.chromium.org/2363533002) introduces a Mojom
definition and typemap for
`ui::WindowOpenDisposition` as a precursor to the IPC conversion below.

The [follow-up CL](https://codereview.chromium.org/2363573002) uses that
definition along with several other new typemaps (including native typemaps as
described above in [Using Legacy IPC Traits](#Using-Legacy-IPC-Traits)) to
convert the relatively large `ViewHostMsg_CreateWindow` message to Mojo.

### Utility Process Messages

Given that there are no interesting ordering dependencies among disparate IPC
messages to and from utility processes, and because the utility process is
already sort of a mixed bag of unrelated IPCs, the correct way to convert
utility process IPCs to Mojo is to move them into services.

We already have support for running services out-of-process (with or without a
sandbox), and many utility process operations already have a suitable service
home they could be moved to. For example, the `data_decoder` service in
[`//services/data_decoder`](https://cs.chromium.org/chromium/src/services/data_decoder/)
is a good place to stick utility process IPCs that do decoding of relatively
complex and untrusted data, of which (at the time of this writing) there are
quite a few.

When in doubt, contact
[`services-dev@chromium.org`](https://groups.google.com/a/chromium.org/forum/#!forum/services-dev)
with ideas, questions, suggestions, etc.

### Additional Documentation

[Chrome IPC to Mojo IPC Cheat Sheet](https://www.chromium.org/developers/design-documents/mojo/chrome-ipc-to-mojo-ipc-cheat-sheet)
:    A slightly dated but still valuable document covering some details
     regarding the conceptual mapping between legacy IPC and Mojo.

[Mojo Migration Guide](https://www.chromium.org/developers/design-documents/mojo/mojo-migration-guide)
:    Another slightly (more) data document covering the basics of IPC conversion
     to Mojo.

     TODO: The migration guide above should probably be deleted and the good
     parts merged into this document.
