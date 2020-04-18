# Service Manager Embedder API

The Service Manager is a library for opaquely managing service process launch,
sandboxing, and lifetime; and for brokering Mojo interface connections among
services.

The Service Manager must be configured with a static set of declarative service
definitions. The embedder API provides a means of embedding core Service Manager
functionality into a standalone executable, delegating configuration and
assorted implementation details to the embedder.

As one example, the Content layer of Chromium embeds the Service Manager via
this API, and there it's used [WIP] to drive all process management and IPC
setup across the system.

This API is a work in progress, and more details will be documented here as the
API is stabilized.
