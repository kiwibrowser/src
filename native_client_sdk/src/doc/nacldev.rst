.. _nacldev:

.. include:: /migration/deprecation.inc

####################
NaCl Dev Environment
####################

Feature Status
==============

Here is a summary of feature status. We hope to overcome these limitations
in the near future:

  * NaCl Development Environment

    * General

      * Supported:

        * Bash (built-in)
        * Python (built-in)
        * Git (built-in)
        * Make (built-in)
        * Vim (built-in)
        * Nano (built-in)
        * Curl (built-in) + geturl for web
        * GCC w/ GLibC (x86-32 and x86-64 only)
        * Lua (install with: `package -i lua && . setup-environment`)
        * Ruby (install with: `package -i ruby && . setup-environment`)
        * Nethack! (install with: `package -i nethack && . setup-environment`)

      * Unsupported:

        * Targeting Newlib
        * Targeting PNaCl (getting close...)
        * Forking in bash
        * Pipes / Redirection
        * Symbolic and hard links

    * Missing / broken on ARM:

      * GCC (unsupported)

  * Debugger
 
    * Open issue causing it to suck CPU cycles (even when not in use).
