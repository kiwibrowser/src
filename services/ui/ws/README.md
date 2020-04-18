This directory contains the Window Service implementation.

## Ids

Each client connected to the Window Service is assigned a unique id inside the
Window Service. This id is a monotonically increasing uint32_t. This is often
referred to as the client_id.

The Window Service uses a client_id of 1 for itself; 0 is not a valid client_id
in the Window Service.

As clients do not know their id, they always supply 0 as the client id in the
mojom related functions. Internally the Window Service maps 0 to the real client
id.

Windows have a couple of different (related) ids.

### ClientWindowId

ClientWindowId is a uint64_t pairing of a client_id and a window_id. The
window_id is a uint32_t assigned by the client, and should be unique within that
client's scope. When communicating with the Window Service, clients may use 0 as
the client_id to refer to their own windows. The Window Service maps 0 to the
real client_id. In Window Service code the id from the client is typically
referred to as the transport_window_id. Mojom functions that receive the
transport_window_id map it to a ClientWindowId. ClientWindowId is a real class
that provides type safety.

When a client is embedded in an existing window, the embedded client is given
visibility to a Window created by the embedder. In this case the Window Service
supplies the ClientWindowId to the embedded client and uses the ClientWindowId
at the time the Window was created (the ClientWindowId actually comes from the
FrameSinkId, see below for details on FrameSinkId). In other words, both the
embedder and embedded client use the same ClientWindowId for the Window. See
discussion on FrameSinkId for more details.

For a client to establish an embed root, it first calls
ScheduleEmbedForExistingClient(), so it can provide a window_id that is unique
within its own scope. That client then passes the returned token to what will
become its embedder to call EmbedUsingToken(). In this case, the embedder and
embedded client do not use the same ClientWindowId for the Window.

ClientWindowId is globally unique, but a Window may have multiple
ClientWindowIds associated with it.

TODO(sky): See http://crbug.com/817850 for making it so there is only one
ClientWindowId per Window.

### FrameSinkId

Each Window has a FrameSinkId that is needed for both hit-testing and
embedding. The FrameSinkId is initialized to the ClientWindowId of the client
creating the Window, but it changes during an embedding. In particular, when a
client calls Embed() the FrameSinkId of the Window changes such that the
client_id of the FrameSinkId matches the client_id of the client being
embedded and the sink_id is set to 0. The embedder is informed of this by way of
OnFrameSinkIdAllocated(). The embedded client is informed of the original
FrameSinkId (the client_id of the FrameSinkId matches the embedder's client_id).
In client code the embedded client ends up *always* using a client_id of 0 for
the FrameSinkId. This works because Viz knows the real client_id and handles
mapping 0 to the real client_id.

The FrameSinkId of top-level windows is set to the ClientWindowId from the
client requesting the top-level (top-levels are created and owned by the Window
Manager). The Window Manager is told the updated FrameSinkId when it is asked
to create the top-level (WmCreateTopLevelWindow()).

The FrameSinkId of an embed root's Window is set to the ClientWindowId of the
embed root's Window from the embedded client.

### LocalSurfaceId

The LocalSurfaceId (which contains unguessable) is necessary if the client wants
to submit a compositor-frame for the Window (it wants to show something on
screen), and not needed if the client only wants to submit a hit-test region.
The LocalSurfaceId may be assigned when the bounds and/or device-scale-factor
changes. The LocalSurfaceId can change at other times as well (perhaps to
synchronize an effect with the embedded client). The LocalSurfaceId is intended
to allow for smooth resizes and ensures at embed points the CompositorFrame from
both clients match. Client code supplies a LocalSurfaceId for windows that have
another client embedded in them as well as windows with a LayerTreeFrameSink.
The LocalSurfaceId comes from the owner of the window. The embedded client is
told of changes to the LocalSurfaceId by way of OnWindowBoundsChanged(). This is
still very much a work in progress.

FrameSinkId is derived from the embedded client, where as LocalSurfaceId
comes from the embedder.

### Event Processing

One of the key operations of the Window Service is event processing. This
includes maintaining state associated with the current input devices (such
as the location of the mouse cursor) as well dispatching to the appropriate
client. Event processing includes the following classes, see each for more
details:
. EventDispatcherImpl: events received from the platform are sent here first.
  If not already processing an event EventDispatcherImpl forwards the event to
  EventProcessor. If EventDispatcherImpl is processing an event it queues the
  event for later processing.
. EventProcessor: maintains state related to event processing, passing the
  appropriate events and targets to EventDispatcher for dispatch.
. AsyncEventDispatcher: dispatches an event to the client, notifying a callback
  when done. This interface is largely for testing with WindowTree providing
  the implementation.
. EventTargeter: used by EventProcessor to determine the ServerWindow to send an
  event to. Targetting is potentially asynchronous.

EventDispatcherImpl and EventProcessor both have delegates that can impact
targetting, as well as being notified during the lifecycle of processing.

EventInjector is not a core part of event processing. It allows remote clients
to inject events for testing, remoting and similar use cases. Events injected
via EventInjector end up going to EventProcessor.
