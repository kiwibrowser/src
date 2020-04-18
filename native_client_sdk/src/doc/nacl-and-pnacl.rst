.. _nacl-and-pnacl:

.. include:: /migration/deprecation.inc

##############
NaCl and PNaCl
##############

This document describes the differences between **Native Client** and
**Portable Native Client**, and provides recommendations for when to use each.

.. contents::
  :local:
  :backlinks: none
  :depth: 2

.. _native-client-nacl:

Native Client (NaCl)
====================

Native Client enables the execution of native code securely inside web
applications through the use of advanced `Software Fault Isolation (SFI)
techniques <http://research.google.com/pubs/pub35649.html>`_. Native Client
allows you to harness a client machine's computational power to a fuller extent
than traditional web technologies. It does this by running compiled C and C++
code at near-native speeds, and exposing a CPU's full capabilities, including
SIMD vectors and multiple-core processing with shared memory.

While Native Client provides operating system independence, it requires you to
generate architecture-specific executables (**nexe**) for each hardware
platform. This is neither portable nor convenient, making it ill-suited for the
open web.

The traditional method of application distribution on the web is through self-
contained bundles of HTML, CSS, JavaScript, and other resources (images, etc.)
that can be hosted on a server and run inside a web browser. With this type of
distribution, a website created today should still work years later, on all
platforms. Architecture-specific executables are clearly not a good fit for
distribution on the web. Consequently, Native Client has been until recently
restricted to applications and browser extensions that are installed through the
Chrome Web Store.

.. _portable-native-client-pnacl:

Portable Native Client (PNaCl)
==============================

PNaCl solves the portability problem by splitting the compilation process
into two parts:

#. compiling the source code to a bitcode executable (pexe), and
#. translating the bitcode to a host-specific executable as soon as the module
   loads in the browser but before any code execution.

This portability aligns Native Client with existing open web technologies such
as JavaScript. You can distribute a pexe as part of an application (along with
HTML, CSS, and JavaScript), and the user's machine is simply able to run it.

With PNaCl, you'll generate a single pexe, rather than multiple platform-
specific nexes. Since the pexe uses an abstract, architecture- and OS-
independent format, it does not suffer from the portability problem described
above. Although, PNaCl can be more efficient on some operating systems than on
others. PNaCl boasts the same level of security as NaCl. Future versions of
hosting environments should have no problem executing the pexe, even on new
architectures. Moreover, if an existing architecture is enhanced, the pexe
doesn't need to be recompiled. In some cases the client-side translation will
automatically take advantage of new capabilities. A pexe can be part of any web
application. It does not have to be distributed through the Chrome Web Store. In
short, PNaCl combines the portability of existing web technologies with the
performance and security benefits of Native Client.

PNaCl is a new technology, and as such it still has a few limitations
as compared to NaCl. These limitations are described below.

.. _when-to-use-pnacl:

When to use PNaCl
=================

PNaCl is the preferred toolchain for Native Client, and the only way to deploy
Native Client modules without the Google Web Store. Unless your project is
subject to one of the narrow limitations described under ":ref:`When to use
NaCl<when-to-use-nacl>`", you should use PNaCl.

Chrome supports translation of pexe modules and their use in web applications
without requiring installation either of a browser plug-in or of the
applications themselves. Native Client and PNaCl are open-source technologies,
and our hope is that they will be added to other hosting platforms in the
future.

If controlled distribution through the Chrome Web Store is an important part of
your product plan, the benefits of PNaCl are less critical for you. But you can
still use the PNaCl toolchain and distribute your application through the Chrome
Web Store, and thereby take advantage of the conveniences of PNaCl, such as not
having to explicitly compile your application for all supported architectures.

.. _when-to-use-nacl:

When to use NaCl
================

Use NaCl if any of the following apply to your application:

* Your application requires architecture-specific instructions such as, for
  example, inline assembly. PNaCl tries to offer high-performance portable
  equivalents. One such example is PNaCl's :ref:`Portable SIMD Vectors 
  <portable_simd_vectors>`.
* Your application uses dynamic linking. PNaCl only supports static linking
  with a PNaCl port of the ``newlib`` C standard library. Dynamic linking and
  ``glibc`` are not yet supported in PNaCl. Work is under way to enable dynamic
  linking in future versions of PNaCl.
* Your application uses certain GNU extensions not supported by PNaCl's LLVM
  toolchain, like taking the address of a label for computed ``goto``, or nested
  functions.
