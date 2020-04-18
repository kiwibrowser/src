# gpu/

This directory contains viz APIs for access to the gpu services.

## ContextProvider

The primary interface to control access to the gpu and lifetime of client-side
gpu control structures (such as the GLES2Implementation that gives access to
the command buffer).
