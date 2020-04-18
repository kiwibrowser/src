Distribution
============

Along with the interface definitions, the following drivers, state trackers,
and auxiliary modules are shipped in the standard Gallium distribution.

Drivers
-------

Intel i915
^^^^^^^^^^

Driver for Intel i915 and i945 chipsets.

Identity
^^^^^^^^

Wrapper driver. The identity driver is a simple skeleton that passes through
all of its :ref:`Context` and :ref:`Screen` methods to an underlying Context
and Screen, and as such, it is an excellent starting point for new drivers.

LLVM Softpipe
^^^^^^^^^^^^^

A version of :ref:`softpipe` that uses the Low-Level Virtual Machine to
dynamically generate optimized rasterizing pipelines.

nVidia nvfx
^^^^^^^^^^^

Driver for the nVidia nv30 and nv40 families of GPUs.

nVidia nv50
^^^^^^^^^^^

Driver for the nVidia nv50 family of GPUs.

VMware SVGA
^^^^^^^^^^^

Driver for VMware virtualized guest operating system graphics processing.

ATI r300
^^^^^^^^

Driver for the ATI/AMD r300, r400, and r500 families of GPUs.

.. _softpipe:

Softpipe
^^^^^^^^

Reference software rasterizer. Slow but accurate.

Trace
^^^^^

Wrapper driver. Trace dumps an XML record of the calls made to the
:ref:`Context` and :ref:`Screen` objects that it wraps.

Rbug
^^^^

Wrapper driver. :ref:`rbug` driver used with stand alone rbug-gui.

.. _galahad:

Galahad
^^^^^^^

Wrapper driver. Sanity checker for the internal gallium state. Normally
a driver should n't have to sanity check the input it gets from a state
tracker. Any wrong state received should be perceived as a state tracker bug.

State Trackers
--------------

.. _dri:

Direct Rendering Infrastructure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Tracker that implements the client-side DRI protocol, for providing direct
acceleration services to X11 servers with the DRI extension. Supports DRI1
and DRI2. Only GL is supported.

.. _egl:

EGL
^^^

Tracker for the Khronos EGL standard, used to set up GL and GLES contexts
without extra knowledge of the underlying windowing system.

GLX
^^^

MesaGL
^^^^^^

Tracker implementing a GL state machine. Not usable as a standalone tracker;
Mesa should be built with another state tracker, such as :ref:`DRI` or
:ref:`EGL`.

Python
^^^^^^

OpenVG
^^^^^^

WGL
^^^

Xorg/XFree86 DDX
^^^^^^^^^^^^^^^^

Tracker for XFree86 and Xorg X11 servers. Provides device-dependent
modesetting and acceleration as a DDX driver.

Auxiliary
---------

OS
^^

The OS module contains the abstractions for basic operating system services:

* memory allocation
* simple message logging
* obtaining run-time configuration option
* threading primitives

This is the bare minimum required to port Gallium to a new platform.

The OS module already provides the implementations of these abstractions for
the most common platforms.  When targeting an embedded platform no
implementation will be provided -- these must be provided separately.

CSO Cache
^^^^^^^^^

The CSO cache is used to accelerate preparation of state by saving
driver-specific state structures for later use.

.. _draw:

Draw
^^^^

Draw is a software :term:`TCL` pipeline for hardware that lacks vertex shaders
or other essential parts of pre-rasterization vertex preparation.

Gallivm
^^^^^^^

Indices
^^^^^^^

Indices provides tools for translating or generating element indices for
use with element-based rendering.

Pipe Buffer Managers
^^^^^^^^^^^^^^^^^^^^

Each of these managers provides various services to drivers that are not
fully utilizing a memory manager.

Remote Debugger
^^^^^^^^^^^^^^^

Runtime Assembly Emission
^^^^^^^^^^^^^^^^^^^^^^^^^

TGSI
^^^^

The TGSI auxiliary module provides basic utilities for manipulating TGSI
streams.

Translate
^^^^^^^^^

Util
^^^^

