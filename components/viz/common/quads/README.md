# quads/

This directory contains the transport data structures for submitting frames
from the viz client to the service, and returning data back to the client.

The root of the structure for submitting frames is CompositorFrame.
Viz-specific types that are included in the CompositorFrame are all part
of this directory, including RenderPass, DrawQuads and SharedQuadState.
