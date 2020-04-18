.. _download:

.. include:: /migration/deprecation.inc

Download the Native Client SDK
==============================

This page provides an overview of the Native Client SDK, and instructions for
downloading and installing the SDK.

.. raw:: html
  
  <div id="home">
  <a class="button-nacl button-download" href="https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/nacl_sdk.zip">Download SDK Zip File</a>
  </div>

.. _sdk-overview:

Overview
--------

The Native Client SDK includes:

- **Support for multiple Pepper versions** to compile for specific minimum
  versions of Chrome.
- **Update utility** to download new bundles and updates to existing bundles.
- **Toolchains** to compile for Portable Native Client (PNaCl), traditional
  Native Client (NaCl), and for compiling architecture-specific Native Client
  applications with glibc.
- **Examples** Including C or C++ source files and header files illustrating
  how to use NaCl and Pepper, and Makefiles to build the example with each of
  the toolchains.
- **Tools** for validating Native Client modules and running modules from the
  command line.

Follow the steps below to download and install the Native Client SDK.

.. _prerequisites:

Prerequisites
-------------

.. _python27:

Python 2.7
^^^^^^^^^^

Make sure that the Python executable is in your ``PATH`` variable. Python 3.x is
not yet supported.
  
* On Mac and Linux, Python is likely preinstalled. Run the command ``python -V``
  in a terminal window, and make sure that the version you have is 2.7.x.
* On Windows, you may need to install Python. Go to `https://www.python.org/
  download/ <https://www.python.org/download/>`_ and select the latest 2.x
  version. In addition, be sure to add the Python directory (for example,
  ``C:\python27``) to the ``PATH`` `environment variable <https://en.wikipedia.
  org/wiki/Environment_variable>`_. Run ``python -V`` from a command line to
  verify that you properly configured the PATH variable.

.. _make:

Make
^^^^

* On the Mac, you need to install ``make`` on your system before you can build
  and run the examples in the SDK. One easy way to get ``make``, along with
  several other useful tools, is to install `Xcode Developer Tools 
  <https://developer.apple.com/technologies/tools/>`_. After installing Xcode,
  go to the XCode menu, open the Preferences dialog box then select Downloads
  and Components. Verify that Command Line Tools are installed.
* On Windows, the Native Client SDK includes a copy of GNU Make.

.. _platforms:

Platforms
---------

Native Client supports several operating systems, including Windows, Linux, OSX,
and ChromeOS. It supports several architectures including on x86-32, x86-64,
ARM, and MIPS.

.. _versioning:

Versions
--------

Chrome is released on a six week cycle, and developer versions of Chrome are
pushed to the public beta channel three weeks before each release. As with any
software, each release of Chrome may include changes to Native Client and the
Pepper interfaces that may require modification to existing applications.
However, modules compiled for one version of Pepper/Chrome should work with
subsequent versions of Pepper/Chrome. The SDK includes multiple versions of the
Pepper APIs to help developers make adjustments to API changes and take
advantage of new features: `stable </native-client/pepper_stable>`_, `beta
</native-client/pepper_beta>`_ and `dev </native-client/pepper_dev>`_.

.. _installing-the-sdk:

Installing the SDK
------------------

.. _downloading-and-unzipping:

Downloading and Unzipping
^^^^^^^^^^^^^^^^^^^^^^^^^

#. Download the `SDK update zip file
   <https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/nacl_sdk.zip>`_.

#. Unzip the file:

   * On Mac/Linux, run the command ``unzip nacl_sdk.zip`` in a terminal
     window.
   * On Windows, right-click on the .zip file and select "Extract All...". A
     dialog box opens; enter a location and click "Extract".

   A directory is created called ``nacl_sdk`` with the following files and
   directories:

   * ``naclsdk`` (and ``naclsdk.bat`` for Windows) --- the update utility,
     which is the command you run to download and update bundles.
   * ``sdk_cache`` --- a directory with a manifest file that lists the bundles
     you have already downloaded.
   * ``sdk_tools`` --- the code run by the ``naclsdk`` command.

.. _installing-the-stable-bundle:

Installing the stable bundle
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#. To see the SDK bundles that are available for download, go to the 
   ``nacl_sdk`` directory and run ``naclsdk`` with the ``list`` command. The SDK
   includes a separate bundle for each version of Chrome/Pepper.

   On Mac/Linux::

     $ cd nacl_sdk
     $ ./naclsdk list

   On Windows::

     > cd nacl_sdk
     > naclsdk list

   You should see output similar to this::

    Bundles:
     I: installed
     *: update available

      I  sdk_tools (stable)
         vs_addin (dev)
         pepper_31 (post_stable)
         pepper_32 (post_stable)
         pepper_33 (post_stable)
         pepper_34 (post_stable)
         pepper_35 (stable)
         pepper_36 (beta)
         pepper_37 (dev)
         pepper_canary (canary)


   The sample output above shows that several bundles are available for
   download, and that you have already installed the latest revision of the
   ``sdk_tools`` bundle, which was included in the zip file. You never need to
   update the ``sdk_tools`` bundle. It is updated automatically (if necessary)
   whenever you run ``naclsdk``.
   
   Bundles are labeled post-stable, stable, beta, dev, or canary. These labels
   usually correspond to the current versions of Chrome. We recommend that you
   develop against a "stable" bundle, because such bundles can be used by all
   current Chrome users. Native Client is designed to be backward-compatible.For
   example, applications developed with the ``pepper_37`` bundle can run in
   Chrome 37, Chrome 38, etc..

#. Run ``naclsdk`` with the ``update`` command to download recommended bundles,
   including the current "stable" bundle.

   On Mac/Linux::

     $ ./naclsdk update

   On Windows::

     > naclsdk update

   By default, ``naclsdk`` only downloads bundles that are recommended, 
   generally those that are "stable." For example, if the current "stable"
   bundle is ``pepper_35``, then the ``update`` downloads that bundle. To
   download the ``pepper_36`` bundle you must ask for it explicitly::

     $ ./naclsdk update pepper_36
  
   

.. _updating-bundles:

Updating bundles
----------------

#. Run ``naclsdk`` with the ``list`` command. This shows you the list of available
   bundles and verifies which bundles you have installed.

   On Mac/Linux::

     $ ./naclsdk list

   On Windows::

     > naclsdk list
     
   An asterisk (*) next to a bundle indicates that there is an update available
   it. For example::

    Bundles:
     I: installed
     *: update available

      I  sdk_tools (stable)
         vs_addin (dev)
         pepper_31 (post_stable)
         pepper_32 (post_stable)
         pepper_33 (post_stable)
         pepper_34 (post_stable)
      I* pepper_35 (stable)
         pepper_36 (beta)
         pepper_37 (dev)
         pepper_canary (canary)

   
   If you run ``naclsdk update`` now, it warns you with a message similar to
   this::

     WARNING: pepper_35 already exists, but has an update available. Run update
     with the --force option to overwrite the existing directory. Warning: This
     will overwrite any modifications you have made within this directory.

#. To download and install the new bundle, run:

   On Mac/Linux::

     $ ./naclsdk update --force

   On Windows::

     > naclsdk update --force

.. _help-with-the-naclsdk-utility:
     
Help with the ``naclsdk`` utility
---------------------------------

#. For more information about the ``naclsdk`` utility, run:

   On Mac/Linux::

     $ ./naclsdk help

   On Windows::

     > naclsdk help

.. _next-steps:

Next steps
----------

* Browse the `Release Notes <release-notes>`_ for important
  information about the SDK and new bundles.
* If you're just starting with Native Client, we recommend reading the 
  `Technical Overview <../overview>`_ and walking through the
  `Getting Started Tutorial <devguide/tutorial/tutorial-part1>`_.
* If you'd rather dive in, see
  `Building Native Client Modules <devguide/devcycle/building>`_.
