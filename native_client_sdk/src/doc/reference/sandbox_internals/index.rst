.. _sandbox-internals-index:

.. include:: /migration/deprecation.inc

#################
Sandbox Internals
#################

The sandbox internals documentation describes implementation details for
Native Client sandboxing, which is also used by Portable Native
Client. These details can be useful to reimplement a sandbox, or to
write assembly code that follows sandboxing rules for Native Client
(Portable Native Client does not allow platform-specific assembly code).

As an implementation detail, the Native Client sandboxes described here
are currently used by Portable Native Client to execute code on the
corresponding machines in a safe manner. The portable bitcode contained
in a **pexe** is translated to a machine-specific **nexe** before
execution. This may change at a point in time: Portable Native Client
doesn't necessarily need these sandboxes to execute code on these
machines. Note that the Portable Native Client compiler itself is also
untrusted: it too runs in a Native Client sandbox described below.

Native Client has sandboxes for:

* :ref:`ARM 32-bit <arm-32-bit-sandbox>`.
* x86-32: the original design is described in `Native Client: A Sandbox
  for Portable, Untrusted x86 Native Code
  <http://research.google.com/pubs/archive/34913.pdf>`_, the current
  design has changed slightly since then.
* :ref:`x86-64 <x86-64-sandbox>`.
* MIPS32, described in the `overview of Native Client for MIPS
  <https://code.google.com/p/nativeclient/issues/attachmentText?id=2275&aid=22750018000&name=native-client-mips-0.4.txt>`_,
  and `bug 2275
  <https://code.google.com/p/nativeclient/issues/detail?id=2275>`_.
