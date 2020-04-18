.. include:: /migration/deprecation.inc

###################################
Native Client Manifest (nmf) Format
###################################

.. contents::
  :local:
  :backlinks: none
  :depth: 2

Overview
========

Every Native Client application has a `JSON-formatted <http://www.json.org/>`_
NaCl Manifest File (``nmf``). The ``nmf`` tells the browser where to
download and load your Native Client application files and libraries.
The file can also contain configuration options.


Field summary
=============

The following shows the supported top-level manifest fields. There is one
section that discusses each field in detail.  The only field that is required
is the ``program`` field.

.. naclcode::

  {
    // Required
    "program": { ... }

    // Only required for glibc
    "files": { ... }
  }

Field details
=============

program
-------

The ``program`` field specifies the main program that will be loaded
in the Native Client runtime environment. For a Portable Native Client
application, this is a URL for the statically linked bitcode ``pexe`` file.
For architecture-specific Native Client applications, this is a list
of URLs, one URL for each supported architecture (currently the choices
are "arm", "x86-32", and "x86-64"). For a dynamically linked executable,
``program`` is the dynamic loader used to load the dynamic executable
and its dynamic libraries.  See the :ref:`semantics <nmf_url_resolution>`
section for the rules on URL resolution.

Example of a ``program`` for Portable Native Client:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  {
    "program": {
      "portable": {
        // Required
        "pnacl-translate": {
          // url is required
          "url": "url_to_my_pexe",

          // optlevel is optional
          "optlevel": 2
        },
        // pnacl-debug is optional
        "pnacl-debug": {
          // url is required
          "url": "url_to_my_bitcode_bc",

          // optlevel is optional
          "optlevel": 0
        }
      }
    }
  }

.. _pnacl_nmf_optlevels:

Portable Native Client applications can also specify an ``optlevel`` field.
The ``optlevel`` field is an optimization level *hint*, which is a number
(zero and higher). Higher numbers indicate more optimization effort.
Setting a higher optimization level will improve the application's
performance, but it will also slow down the first load experience.
The default is ``optlevel`` is currently ``2``, and values higher
than 2 are no different than 2. If compute speed is not as important
as first load speed, an application could specify an ``optlevel``
of ``0``. Note that ``optlevel`` is only a *hint*. In the future, the
Portable Native Client translator and runtime may *automatically* choose
an ``optlevel`` to best balance load time and application performance.

A ``pnacl-debug`` section can specify an unfinalized pnacl llvm bitcode file
for debugging. The ``url`` provided in this section will be used when Native
Client debugging is enabled with either the ``--enable-nacl-debug`` Chrome
command line switch, or via ``about://flags``.


Example of a ``program`` for statically linked Native Client executables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  {
    "program": {
      // Required: at least one entry
      // Add one of these for each architecture supported by the application.
      "arm": { "url": "url_to_arm_nexe" },
      "x86-32": { "url": "url_to_x86_32_nexe" },
      "x86-64": { "url": "url_to_x86_64_nexe" }
    }
  }

Example of a ``program`` for dynamically linked Native Client executables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  {
    "program": {
      // Required: at least one entry
      // Add one of these for each architecture supported by the application.
      "x86-32": { "url": "lib32/runnable-ld.so" },
      "x86-64": { "url": "lib64/runnable-ld.so" }
    },
    // discussed in next section
    "files": {
      "main.nexe": {
        "x86-32": { "url": "url_to_x86_32_nexe" },
        "x86-64": { "url": "url_to_x86_64_nexe" }
      },
      // ...
    }
  }


files
-----

The ``files`` field specifies a dictionary of file resources to be used by a
Native Client application. This is not supported and not needed by Portable
Native Client applications (use the PPAPI `URL Loader interfaces
</native-client/pepper_stable/cpp/classpp_1_1_u_r_l_loader>`_ to load resources
instead). However, the ``files`` manifest field is important for dynamically
linked executables, which must load files before PPAPI is initialized. The
``files`` dictionary should include the main dynamic program and its dynamic
libraries.  There should be one file entry that corresponds to each a dynamic
library. Each file entry is a dictionary of supported architectures and the
URLs where the appropriate Native Client shared object (``.so``) for that
architecture may be found.

Since ``program`` is used to refer to the dynamic linker that comes
with the NaCl port of glibc, the main program is specified in the
``files`` dictionary. The main program is specified under the
``"main.nexe"`` field of the ``files`` dictionary.


.. naclcode::

  {
    "program": {
      "x86-64": {"url": "lib64/runnable-ld.so"},
      "x86-32": {"url": "lib32/runnable-ld.so"}
    },
    "files": {
      "main.nexe" : {
        "x86-64": {"url": "pi_generator_x86_64.nexe"},
        "x86-32": {"url": "pi_generator_x86_32.nexe"}
      },
      "libpthread.so.5055067a" : {
        "x86-64": {"url": "lib64/libpthread.so.5055067a"},
        "x86-32": {"url": "lib32/libpthread.so.5055067a"}
      },
      "libppapi_cpp.so" : {
        "x86-64": {"url": "lib64/libppapi_cpp.so"},
        "x86-32": {"url": "lib32/libppapi_cpp.so"}
      },
      "libstdc++.so.6" : {
        "x86-64": {"url": "lib64/libstdc++.so.6"},
        "x86-32": {"url": "lib32/libstdc++.so.6"}
      },
      "libm.so.5055067a" : {  
        "x86-64": {"url": "lib64/libm.so.5055067a"},
        "x86-32": {"url": "lib32/libm.so.5055067a"}
      },
      "libgcc_s.so.1" : {
        "x86-64": {"url": "lib64/libgcc_s.so.1"},
        "x86-32": {"url": "lib32/libgcc_s.so.1"}
      },
      "libc.so.5055067a" : {  
        "x86-64": {"url": "lib64/libc.so.5055067a"},
        "x86-32": {"url": "lib32/libc.so.5055067a"}
      }
    }
  }


Dynamic libraries that the dynamic program depends upon and links in at program
startup must be listed in the ``files`` dictionary. Library files that are
loaded after startup using ``dlopen()`` should either be listed in the ``files``
dictionary, or should be made accessible by the ``nacl_io`` library.  The
``nacl_io`` library provides various file system *mounts* such as HTTP-based
file systems and memory-based file systems. The Native Client SDK includes
helpful tools for determining library dependencies and generating NaCl manifest
files for programs that that use dynamic linking. See
:ref:`dynamic_loading_manifest`.

Semantics
=========

Schema validation
-----------------

Manifests are validated before the program files are downloaded.
Schema validation checks the following properties:

* The schema must be valid JSON.
* The schema must conform to the grammar given above.
* If the program is not a PNaCl program, then the manifest
  must contain at least one applicable match for the current ISA
  in "program" and in every entry within "files".

If the manifest contains a field that is not in the official
set of supported fields, it is ignored. This allows the grammar to be
extended without breaking compatibility with older browsers.


Nexe matching
-------------

For Portable Native Client, there are no architecture variations, so
matching is simple.

For Native Client, the main nexe for the application is determined by
looking up the browser's current architecture in the ``"program"``
dictionary. Failure to provide an entry for the browser's architecture
will result in a load error.


File matching
-------------

All files (shared objects and other assets, typically) are looked up
by a UTF8 string that is the file name. To load a library with a certain
file name, the browser searches the ``"files"`` dictionary for an entry
corresponding to that file name. Failure to find that name in the
``"files"`` dictionary is a run-time error. The architecture matching
rule for all files is from most to least specific. That is, if there
is an exact match for the current architecture (e.g., "x86-32") it is
used in preference to more general "portable". This is useful for
non-architecture-specific asset files. Note that ``"files"`` is only
useful for files that must be loaded early in application startup
(before PPAPI interfaces are initialized to provide the standard
file loading mechanisms).


URL of the nmf file, from ``<embed>`` src, and data URI
-------------------------------------------------------

The URL for the manifest file should be specified by the ``src`` attribute
of the ``<embed>`` tag for a Native Client module instance. The URL for
a manifest file can refer to an actual file, or it can be a 
`data URI <http://en.wikipedia.org/wiki/Data_URI_scheme>`_
representing the contents of the file. Specifying the ``nmf`` contents
inline with a data URI can help reduce the amount of network traffic
required to load the Native Client application.

.. _nmf_url_resolution:

URL resolution
--------------

All URLs contained in a manifest are resolved relative to the URL of
the manifest. If the manifest was specified as a data URI, the URLs must
all be absolute.
