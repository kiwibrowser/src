# Chrome Foundation Services

[TOC]

## Overview

This directory contains Chrome Foundation Services. If you think of Chrome as a
"portable OS," Chrome Foundation Services can be thought of as that OS'
foundational "system services" layer.

Roughly each subdirectory here corresponds to a service that:

  * is a client of `//services/service_manager` with its own unique Identity.
  * could logically run a standalone process for security/performance isolation
    benefits depending on the constraints of the host OS.

## API Standards

As illustrated above, the individual services in //services are intended for
graceful reusability across a broad variety of use cases. To enable this goal,
we have rigorous [standards](/services/api_standards.md) on services'
public APIs. Before doing significant work in //services (and especially before
becoming an owner of a service), please internalize these standards -- you are
responsible for upholding them.

## Service Directory Structure

Individual services are structured like so:

```
//services/foo/                   <-- Implementation code, may have subdirs.
              /public/
                     /cpp/        <-- C++ client libraries (optional)
                     /mojom/      <-- Mojom interfaces
```

## Dependencies

Code within `//services` may only depend on each other via each other's
`/public/` directories, *i.e.* implementation code may not be shared directly.

Service code should also take care to tightly limit the dependencies on static
libraries from outside of `//services`. Dependencies to large platform
layers like `//content`, `//chrome` or `//third_party/WebKit` must be avoided.

## Physical Packaging

Note that while it may be possible to build a discrete physical package (DSO)
for each service, products consuming these services may package them
differently, e.g. by combining them into a single package.

## Additional Documentation

[High-level Design Doc](https://docs.google.com/document/d/15I7sQyQo6zsqXVNAlVd520tdGaS8FCicZHrN0yRu-oU)

[Servicification Homepage](https://sites.google.com/a/chromium.org/dev/servicification)

[Servicification Strategies](/docs/servicification.md)

## Relationship To Other Top-Level Directories

Services can be thought of as integrators of library code from across the
Chromium repository, most commonly `//base` and `//mojo` (obviously) but for
each service also `//components`, `//ui`, *etc.* in accordance with the
functionality they provide.

Not everything in `//components` is automatically a service in its own right.
Think of `//components` as sort of like a `//lib`. Individual `//components` can
define, implement and use Mojom interfaces, but only `//services` have unique
identities with the Service Manager and so only `//services` make it possible
for Mojom interfaces to be acquired.

## Adding a new service

See the [Service Manager documentation](/services/service_manager) for more
details regarding how to define a service and expose or consume interfaces to
and from other services.

Please start a thread on [services-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/services-dev)
if you want to introduce a new service.

If you are servicifying existing Chromium code: Please first read the
[servicification strategies documentation](/docs/servicification.md), which
contains information that will hopefully make your task easier.
