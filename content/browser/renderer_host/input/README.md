# Renderer Host Input

This directory contains browser side input event handling code.

## TouchpadPinchEventQueue

In order for pages to override the behaviour of touchpad pinch zooming, we offer
synthetic wheel events that may be canceled. The `TouchpadPinchEventQueue`
accepts the gesture pinch events created from native events and sends the
appropriate wheel events to the renderer. Once the renderer has acknowledged a
wheel event, we offer the corresponding gesture pinch event back to the client
of the `TouchpadPinchEventQueue`. If the pinch has not been consumed by the page,
the client may go on to send the actual gesture pinch event to the renderer which
will perform the pinch zoom.

Note that for touchscreen gesture pinch events, there is no need for a similar
queue as touch events would have already been offered to the renderer before
being recognized as a pinch gesture.

See [issue 289887](https://crbug.com/289887) for the discussion on offering
wheel events for touchpad pinch.
