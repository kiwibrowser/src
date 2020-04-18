# ResizeObserver

Implements ResizeObserver [spec]( https://github.com/WICG/ResizeObserver), which has a nice [explainer](https://github.com/WICG/ResizeObserver/blob/master/explainer.md).

The purpose of ResizeObserver is to expose a ResizeObserver DOM API that
notifies observer when Element's size has changed.

# Architecture overview

The externally exposed APIs are:

[`ResizeObserver.idl`](ResizeObserver.idl) implements a general observer pattern, observe(), unobserve(), disconnect().

[`ResizeObserverEntry.idl`](ResizeObserverEntry.idl) represents an observation to be delivered, with target and contentRect.

Classes used internally are:

[`ResizeObservation`](ResizeObservation.h) internal representation of an Element that is being observed, and its last observed size.

[`ResizeObserverController`](ResizeObservationController.h) ties Document to its ResizeObservers.

# Notification lifecycle

ResizeObserver needs to deliver notifications before every animation frame.

There are 2 phases of notification lifecycle:

###### 1) Size change detection

`ResizeObservation` stores last observed element size.

There are 2 ways to detect size change. One way is "pull", where every watched
Element's size is compared to last observed size. This is inefficient, because
all observed Elements must be polled in every frame.

That is why we use "push", where Element sets a flag that it's size has changed.
The flag gets set by calling `Element::SetNeedsResizeObserverUpdate()`. This
notifies ResizeObservation, which also notifies ResizeObserver.
`SetNeedsResizeObserverUpdate` has to be carefully added to all places
that might trigger a resize observation.

###### 2) Notification delivery

Notification delivery is done by calling `LocalFrameView::NotifyResizeObservers()`
for every rAF. It calls `ResizeObserverController::DeliverObservations()`
which delivers observations for all ResizeObservers that have detected size changes.
`DeliverObservations()` will not run if size has not changed. To deliver initial
observation, ResizeObserver must call `LocalFrameView::ScheduleAnimation`.

# Object lifetime

ResizeObserver objects are garbage collected. Figuring out how to arrange references
to ensure proper lifetime is tricky.

ResizeObserver must be kept alive as long as at least one of these is true:
1) There is a Javascript reference to ResizeObserver.
2) There is an active observation. This happens when Javascript has no more
references to ResizeObserver, but observations should still be delivered.

You can use the reference chain below to trace object lifetime chain:
```
Javascript => ResizeObserver
Element.resize_observer_data_ => ResizeObserver, ResizeObservation
Document.resize_observer_controller_ => ResizeObserverController
ResizeObserver => ResizeObserverEntry
ResizeObserver => ResizeObserverCallback
ResizeObserverEntry => Element
```
