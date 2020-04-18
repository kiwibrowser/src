.. _nacl_io:

.. include:: /migration/deprecation.inc

###################
The nacl_io Library
###################

.. contents::
  :local:
  :backlinks: none
  :depth: 2

Introduction
============

``nacl_io`` is a utility library that provides implementations of standard
C APIs such as POSIX I/O (``stdio.h``) and BSD sockets (``sys/socket.h``).
Its primary function is to allow code that uses these standard APIs to be
compiled and used in a Native Client module. The library is included as part
of Native Client SDK and is implemented in on top of Pepper API.

Since Native Client modules cannot access the host machine's file system
directly, nacl_io provides several alternative filesystem types which can be
used by the application. For example, the Chrome browser supports the `HTML5
File System API <http://www.html5rocks.com/en/tutorials/file/filesystem/>`_
which provides access to a protected area of the local file system. This
filesystem can be accessed by an HTML page using JavaScript commands, and also
by a Native Client module using the Pepper :doc:`File IO API <file-io>`.

With nacl_io a Native Client application can mount an HTML5 filesystem and
access it via standard POSIX I/O function such as ``fopen``, ``fseek``,
``fread``, ``fwrite``, and ``fclose``, or their low level UNIX counterparts
``open``, ``lseek``, ``read``, ``write`` and ``close``. As well as the HTML5
file system, nacl_io provides several other file system types which are
described in the table below:

=========== ==================================================================
File System Description
=========== ==================================================================
memfs       An in-memory file system
html5fs     An HTML5 local file system, which can be persistent or temporary
http        Maps files on a remote webserver into the local filesystem.
dev         A file system containing special files (e.g.: ``/dev/null``)
=========== ==================================================================

Using nacl_io
=============

Using nacl_io is mostly just a matter of using the standard POSIX C library
functions. However, there are some steps required to initialize the library
and setup the filesystem mounts. In general the following steps will be needed
to use nacl_io in a NaCl application:

#. Link the application with the nacl_io library (``-lnacl_io``)
#. Initialize nacl_io at startup using the ``nacl_io_init_ppapi`` or
   ``nacl_io_init`` functions.
#. Mount any desired filesystems using the ``mount`` function. The arguments
   to ``mount`` for the different filesystem types are detailed in
   ``include/nacl_io/nacl_io.h``.
#. If you are going to mount an HTML5 file system, be sure to allocate space
   for it. You can either set the ``unlimitedStorage`` permission in the app's
   Web Store manifest file, or call the HTML5 QuotaManagement API. These
   options are explained in the :ref:`File IO documentation <quota_management>`.
#. Make sure that file and socket API calls are all made from the background
   thread. This is because the main Pepper thread does not support the blocking
   behavior needed by the POSIX I/O operations.

Logging in nacl_io
==================

Unlike most input/output for nacl_io, internal logging writes directly to the
``stderr`` stream of the NaCl process. It deliberately bypasses the standard
library functions implemented in nacl_io to avoid circular calls to itself.

The nacl_io demo
================

Building and running the demo
-----------------------------

The demo application launches a Native Client module that mounts three file
systems and displays a set of controls that let you work with them:

.. image:: /images/nacl_io1.png

Follow these steps to build and run the demo:

* Open a terminal in the demo directory::

    $ cd $NACL_SDK_ROOT/examples/demo/nacl_io_demo

* run the demo::

    $ make run

Once the demo is running, try these operations:

#. select the fopen command (when you select a command the fields in the line
   below will change according to the command)
#. type in the filename ``/persistent/test``
#. check the write checkbox and press the fopen button
#. select the fwrite command and select the file ``/persistent/test`` in the
   menu that appears below on the left
#. enter some data and press the fwrite button
#. select the fclose command, be sure the file ``/persistent/test`` is selected
   in the menu, and press the fclose button
#. select the fopen command
#. type in the filename ``/persistent/test``
#. check the fread checkbox and press the fopen button
#. select the fread command, be sure the file /persistent/test is selected in
   the menu, enter a byte count, and press the fread button

A look at the code
------------------

The demo is written C and comprises three files.

nacl_io_demo.c
^^^^^^^^^^^^^^

This is the demo's main file. The code here creates and initializes the Native
Client module instance. The Pepper function ``Instance_DidCreate`` initializes
nacl_io and mounts an HTML5 filesystem at ``/persistent``.

.. naclcode::

  static PP_Bool Instance_DidCreate(PP_Instance instance,
                                    uint32_t argc,
                                    const char* argn[],
                                    const char* argv[]) {
    g_instance = instance;
    nacl_io_init_ppapi(instance, get_browser_interface);
    mount(
        "",  /* source */
        "/persistent",  /* target */
        "html5fs",  /* filesystemtype */
        0,  /* mountflags */
        "type=PERSISTENT,expected_size=1048576");  /* data specific to the html5fs type */

    pthread_create(&g_handle_message_thread, NULL, &HandleMessageThread, NULL);
    InitializeMessageQueue();

    return PP_TRUE;
  }

Space is allocated to the ``/persistent`` file system after the module is
initialized. This is accomplished by the ``domContentLoaded`` function in
the file ``example.js``. This script is included in the module's html page (see
``examples/demo/index.html``):

.. naclcode::

  function domContentLoaded(name, tc, config, width, height) {
    navigator.webkitPersistentStorage.requestQuota(window.PERSISTENT, 1024 * 1024,
        function(bytes) {
          common.updateStatus(
              'Allocated ' + bytes + ' bytes of persistent storage.');
          common.createNaClModule(name, tc, config, width, height);
          common.attachDefaultListeners();
        },
        function(e) { alert('Failed to allocate space') });
  }

The ``Instance_DidCreate`` function also creates a worker thread that receives
messages sent from the html page and performs the specified file system
operations. The logic for the worker thread is encoded in the other two files,
described below.

queue.c
^^^^^^^

This file implements a circular queue that is used to receive messages from the
browser UI to the Native Client module. The file system commands in the
enqueued messages are executed on the worker thread. This keeps blocking calls
(like fread) off the main Native Client thread, which is a good thing. The
queue is initialized in nacl_io_demo.c ``Instance_DidCreate``.

handlers.c
^^^^^^^^^^

This file implements the stdio calls associated with the commands sent from the
browser. There is a separate ``Handle*`` function for each command: fopen,
fclose, fseek, fread, fwrite. The handlers are called from the
``HandleMessage`` function in nacl_io_demo.c, which runs in the worker
thread managing the message queue. The code for the ``fwrite`` handler appears
below. Notice that it does not contain any PPAPI calls and looks like
"ordinary" C code.


.. naclcode::

  int HandleFwrite(int num_params, char** params, char** output) {
    FILE* file;
    const char* file_index_string;
    const char* data;
    size_t data_len;
    size_t bytes_written;

    if (num_params != 2) {
      *output = PrintfToNewString("Error: fwrite takes 2 parameters.");
      return 1;
    }

    file_index_string = params[0];
    file = GetFileFromIndexString(file_index_string, NULL);
    data = params[1];
    data_len = strlen(data);

    if (!file) {
      *output = PrintfToNewString("Error: Unknown file handle %s.",
                                  file_index_string);
      return 2;
    }

    bytes_written = fwrite(data, 1, data_len, file);

    *output = PrintfToNewString("fwrite\1%s\1%d", file_index_string,
                                bytes_written);
    return 0;
  }

Reference Information
=====================

The example discussed here is included in the SDK in the directory
``examples/demo/nacl_io_demo``.

The nacl_io library is included in the SDK toolchain and is not a part of the
Pepper API. For reference information related to the nacl_io interface see
its header file in the SDK directory, located at
``include/nacl_io/nacl_io.h``.

For more about the HTML5 file system read the `specification
<http://dev.w3.org/2009/dap/file-system/pub/FileSystem/>`_.
