.. _migration:

WebAssembly Migration Guide
===========================

(P)NaCl Deprecation Announcements
---------------------------------

Given the momentum of cross-browser WebAssembly support, we plan to focus our
native code efforts on WebAssembly going forward and plan to remove support for
PNaCl in Q1 2018 (except for Chrome Apps). We believe that the vibrant
ecosystem around `WebAssembly <http://webassembly.org>`_
makes it a better fit for new and existing high-performance
web apps and that usage of PNaCl is sufficiently low to warrant deprecation.

We also recently announced the deprecation Q1 2018 of
`Chrome Apps
<https://blog.chromium.org/2016/08/from-chrome-apps-to-web.html>`_
outside of ChromeOS.


Toolchain Migration
-------------------

For the majority of (P)NaCl uses cases we recommend transitioning
from the NaCl SDK to `Emscripten
<http://webassembly.org/getting-started/developers-guide/>`_.
Migration is likely to be reasonably straightforward
if your application is portable to Linux, uses
`SDL <https://www.libsdl.org/>`_, or POSIX APIs.
While direct support for NaCl / Pepper APIs in not available,
we've attempted to list Web API equivalents.
For more challenging porting cases, please reach out on
native-client-discuss@googlegroups.com


API Migration
-------------

We've outlined here the status of Web Platform substitutes for each
of the APIs exposed to (P)NaCl.
Additionally, the table lists the library or option in Emscripten
that offers the closest substitute.

We expect to add shared memory threads support to WebAssembly in 2017,
as threads are crucial to matching (P)NaCl's most interesting use
cases. Migration items which assume forthcoming threads support
are marked below. If your application's flow control relies heavily on blocking
APIs, you may also find threads support is required for convenient porting.

While we've tried to be accurate in this table,
there are no doubt errors or omissions.
If you encounter one, please reach out to us on
native-client-discuss@googlegroups.com

.. contents::
  :local:
  :backlinks: none
  :depth: 2

PPAPI
-----
.. raw:: html
  :file: public.html

IRT
---
.. raw:: html
  :file: public.html

PPAPI (Apps)
------------
.. raw:: html
  :file: apps.html
