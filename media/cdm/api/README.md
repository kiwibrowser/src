# Library CDM Interface

This directory contains files that define the shared library interface between
an Encrypted Media Extensions (EME) Content Decryption Module (CDM) and Chromium
or other user agents. It is used to build both user agents and CDMs.

This is also referred to as the "CDM interface" in the context of library CDM
and in this doc.

TODO(xhwang): Add more sections describing the CDM interface.

## Experimental and Stable CDM interface

A new CDM interface that's still under development is subject to change. This
is called an "experimental CDM interface". To avoid compatibility issues, a user
agent should not support an experimental CDM interface by default (it's okay to
support it behind a flag). Similarly, a CDM vendor should not ship a CDM using
an experimental CDM interface to end users.

The experimental status of a CDM interface ends when the development is complete
and the CDM interface is marked as stable.

On newer CDM interfaces, a static boolean member kIsStable is present to
indicate whether the CDM interface is stable or experimental.
