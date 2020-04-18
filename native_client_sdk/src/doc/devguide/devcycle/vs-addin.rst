.. _devcycle-vs-addin:

.. include:: /migration/deprecation.inc

############################
Debugging with Visual Studio
############################


.. contents:: Table Of Contents
  :local:
  :backlinks: none
  :depth: 2

Whether you're porting an existing project or starting from scratch, the Native
Client Visual Studio add-in makes it easier to set up, build, run and debug
your Native Client app by integrating the Native Client SDK development tools
into the Visual Studio environment.

.. Note::
  :class: note

  The Native Client add-in requires Visual Studio 2012 or Visual Studio 2010
  with Service Pack 1. No other versions of Visual Studio are currently
  supported. Visual Studio Express is also not supported.

Introduction
============

The Native Client add-in for Visual Studio helps you develop your application
more efficiently in many ways:

* Organize and maintain your code as a Visual Studio project.
* Iteratively write and test your application more easily. Visual Studio
  handles the details of launching a web server to serve your module and run
  the module in Chrome with a debugger attached.
* Compile your module into a dynamically-linked library (DLL) using Visual
  Studio's C/C++ compiler and run it as a Pepper plugin. This allows you to
  develop code incrementally, coding and/or porting one feature at a time into
  the Pepper APIs while continuing to use native Windows APIs that would
  otherwise be unavailable in an actual Native Client module.
* Use Visual Studio's built-in debugger to debug your code while it’s running
  as a Pepper plugin.
* Compile your module into a .nexe or .pexe file using the Native Client SDK
  tools and run it as a bona fide Native Client module.
* Use the Native Client debugger, nacl-gdb, to test your code when it’s running
  as a Native Client object.

The add-in defines five new Visual Studio platforms: ``PPAPI``, ``NaCl32``,
``NaCl64``, ``NaClARM``, and ``PNaCl``. These platforms can be applied to the
debug configuration of solutions and projects. The platforms configure the
properties of your project so it can be built and run as either a Pepper plugin
or a Native Client module. The platforms also define the behavior associated
with the debug command so you can test your code while running in Visual
Studio.

Platforms
=========

It is helpful to consider the Visual Studio add-in platforms in two groups. One
contains the PPAPI platform only. The other group, which we'll call the Native
Client platforms, contains platforms that all have "NaCl" in their names:
``NaCl32``, ``NaCl64``, ``NaClARM``, and ``PNaCl``. The diagram below shows the
platforms, the ways they are normally used, and the build products they produce.

.. image:: /images/visualstudio4.png

Using platforms, your workflow is faster and more efficient. You can compile,
start, and debug your code with one click or key-press. When you press F5, the
“start debugging” command, Visual Studio automatically launches a web server to
serve your module (if necessary) along with an instance of Chrome that runs
your Native Client module, and also attaches an appropriate debugger.

You can switch between platforms as you work to compare the behavior of your
code.

When you run your project, Visual Studio launches the PPAPI and Native Client
platforms in different ways, as explained in the next sections.

The PPAPI platform
------------------

The PPAPI platform builds your module as a dynamic library and launches a
version of Chrome that’s configured to run the library as a plugin when it
encounters an ``<embed>`` element with ``type=application/x-nacl`` (ignoring
the information in the manifest file). When running in the PPAPI platform, you
can use Windows system calls that are unavailable in a regular Native Client
module built and running as a .nexe file. This offers the ability to port
existing code incrementally, rewriting functions using the PPAPI interfaces one
piece at a time. Since the module is built with Visual Studio’s native compiler
(MSBuild) you can use the Visual Studio debugger to control and inspect your
code.

The Native Client platforms
---------------------------

There are four Native Client platforms. All of them can be used to build Native
Client modules. When you run one of the Native Client platforms Visual Studio
builds the corresponding type of Native Client module (either a .nexe or
.pexe), starts a web server to serve it up, and launches a copy of Chrome that
fetches the module from the server and runs it. Visual Studio will also open a
terminal window, launch an instance of nacl-gdb, and attach it to your module's
process so you can use gdb commands to debug.

NaCl32 and NaCl64
^^^^^^^^^^^^^^^^^

The platforms named NaCl32 and NaCl64 are targeted at x86 32-bit and 64-bit
systems respectively. You need both platforms to build a full set of .nexe
files when you are ready to distribute your application. Note, however, that
when you are testing in Visual Studio you must select the NaCl64 platform
(because Chrome for Windows runs Native Client in a 64-bit process). If you try
to run from the NaCl32 platform you will get an error message.

NaClARM
^^^^^^^

The NaClARM platform is targeted at ARM-based processors. You can build .nexe
files with the NaClARM platform in Visual Studio but you cannot run them from
there. You can use Visual Studio to create a Native Client module that includes
an ARM-based .nexe file and then run the module from a Chrome browser on an ARM
device, such as one of the newer Chromebook computers. See the instructions at
:doc:`Running Native Client Applications <running>` for more information on
testing your module in Chrome.

.. Note::
  :class: note

  Note: The NaClARM platform currently supports the newlib toolchain only.

PNaCl
^^^^^

The PNaCl (portable NaCl) platform is included in the Visual Studio Native
Client add-in versions 1.1 and higher. It supports the .pexe file format. A
.pexe file encodes your application as bitcode for a low level virtual machine
(LLVM). When you deliver a Native Client application as a PNaCl module, the
manifest file will contain a single .pexe file rather than multiple .nexe
files. The Chrome client transforms the LLVM bitcode into machine instructions
for the local system.

When you run the PNaCl platform from Visual Studio, Visual Studio uses the
Native Client SDK to transform the .pexe file into a NaCl64 .nexe file and runs
it as if you were working with a NaCl64 platform.

.. Note::
  :class: note

  Note: The PNaCl platform currently supports the newlib toolchain only.

Installing the add-in
=====================

In order to use the Native Client Visual Studio add-in, your development
environment should include:

* A 64-bit version of Windows Vista or Windows 7.
* Visual Studio 2012 or Visual Service 2010 with Service Pack 1.
* `Chrome <https://www.google.com/intl/en/chrome/browser/>`_ version 23 or
  greater. You can choose to develop using the latest `canary
  <https://www.google.com/intl/en/chrome/browser/canary.html>`_ build of
  Chrome, running the canary version side-by-side with (and separately from)
  your regular version of Chrome.
* :doc:`The Native Client SDK <../../sdk/download>` with the ``pepper_23``
  bundle or greater. The version of Chrome that you use must be equal or
  greater than the version of the SDK bundle.

Set environment variables
-------------------------

Before you run the installer you must define two Windows environment variables.
They point to the bundle in the Native Client SDK that you use to build your
module, and to the Chrome browser that you choose to use for debugging.

To set environment variables in Windows 7, go to the Start menu and search for
"environment." One of the links in the results is "Edit environment variables
for your account." (You can also reach this link from the ``Control Panel``
under ``User Accounts``.) Click on the link and use the buttons in the window
to create or change these user variables (the values shown below are only for
example):


+-------------------+----------------------------------------------------------+
| Variable Name     | Description                                              |
+===================+==========================================================+
| ``NACL_SDK_ROOT`` | The path to the pepper directory in the SDK.             |
|                   | For example: ``C:\nacl_sdk\pepper_23``                   |
+-------------------+----------------------------------------------------------+
| ``CHROME_PATH``   | The path to the .exe file for the version of Chrome you  |
|                   | are testing with.  For example:                          |
|                   | ``C:\Users\fred\AppData\Local\Google\Chrome              |
|                   | SxS\Application\chrome.exe``                             |
+-------------------+----------------------------------------------------------+



Download the add-in
-------------------

The Native Client Visual Studio add-in is a separate bundle in the SDK named
``vs_addin``. Open a command prompt window, go to the top-level SDK directory,
and run the update command, specifying the add-in bundle::

  naclsdk update vs_addin

This creates a folder named ``vs_addin``, containing the add-in itself, its
installer files, and a directory of examples.

.. Note::
  :class: note

  Note: The vs_addin bundle is only visible when you run ``naclsdk`` on a
  Windows system.

Run the installer
-----------------

The installer script is located inside the ``vs_addin`` folder in the SDK.
Right click on the file ``install.bat`` and run it as administrator.

The script always installs the NativeClient platforms, and asks you if you’d
like to install the PPAPI platform as well. You can skip the PPAPI step and run
the installer again later to add the PPAPI platform.

You can usually run the installer successfully with no arguments. The new
platforms are installed in ``C:\Program Files
(x86)\MSBuild\Microsoft.Cpp\v4.0\Platforms``.

In some cases system resources may not be in their default locations. You might
need to use these command line arguments when you run ``install.bat``:

* The MSBuild folder is assumed to be at ``C:\Program Files (x86)\MSBuild``.
  You can specify an alternate path with the flag ``--ms-build-path=<path>``.
  The installer assumes Visual Studio has created a user folder at
* The addin itself is normally installed in ``%USERPROFILE%\My Documents\Visual
  Studio 2012`` (or 2010 for Visual Studio 2010). You can specify alternate
  paths with the ``--vsuser-path=<path>`` flag.

From time to time an update to the Visual Studio add-in may become available.
Updates are performed just like an installation. Download the new add-in using
naclsdk update and run ``install.bat`` as administrator.

To uninstall the add-in, run ``install.bat`` as administrator and add the
``--uninstall`` flag. You'll need to run the Command Prompt program as
administrator in order to add the flag. Go the to the Windows start menu,
search for "Command Prompt," right click on the program and run it as
administrator.

You can verify that the add-in has been installed and determine its version by
selecting Add-in Manager in the Visual Studio Tools menu. If the add-in has
been installed it will appear in the list of available add-ins. Select it and
read its description.

Try the ``hello_world_gles`` sample project
===========================================

The add-in comes with an examples directory. Open the sample project
``examples\hello_world_gles\hello_world_gles.sln``. This project is an
application that displays a spinning cube.

Select the NaCl64 platform
--------------------------

Open the sample project in Visual Studio, select the ``Configuration Manager``,
and confirm that the active solution configuration is ``Debug`` and the active
project platform is ``NaCl64``. Note that the platform for the
``hello_world_gles`` project is also ``NaCl64``. (You can get to the
``Configuration Manager`` from the ``Build`` menu or the project’s
``Properties`` window.)

.. image:: /images/visualstudio1.png

Build and run the project
-------------------------

Use the debugging command (F5) to build and run the project. As the wheels
start to turn, you may be presented with one or more alerts. They are benign;
you can accept them and set options to ignore them when that’s possible. Some
of the messages you might see include:

* "This project is out of date, would you like to build it?"
* "Please specify the name of the executable file to be used for the debug
  session." This should be the value of the environment variable CHROME_PATH,
  which is usually supplied as the default value in the dialog.
* "Debugging information for chrome.exe cannot be found." This is to be
  expected, you are debugging your module's code, not Chrome.
* "Open file - security warning. The publisher could not be verified." If
  Visual Studio is complaining about x86_64-nacl-gdb.exe, that’s our debugger.
  Let it be.

Once you’ve passed these hurdles, the application starts to run and you’ll see
activity in three places:

#. A terminal window opens running ``nacl-gdb``.
#. Chrome launches running your module in a tab.
#. The Visual Studio output window displays debugging messages when you select
   the debug output item.
   Stop the debugging session by closing the Chrome window, or select the stop
   debugging command from the debug menu. The nacl-gdb window will close when
   you stop running the program.

Test the nacl-gdb debugger
--------------------------

Add a breakpoint at the SwapBuffers call in the function MainLoop, which is in
hello_world.cc.

.. image:: /images/visualstudio2.png

Start the debugger again (F5). This time the existing breakpoint is loaded into
nacl-gcb and the program will pause there. Type c to continue running. You can
use gdb commands to set more breakpoints and step through the application. For
details, see :ref:`Debugging with nacl-gdb <using_gdb>` (scroll down to the end
of the section to see some commonly used gdb commands).

Test the Visual Studio debugger
-------------------------------

If you’ve installed the ``PPAPI`` platform, go back to the ``Configuration
Manager`` and select the ``PPAPI`` platform. This time when Chrome launches the
``nacl-gdb`` window will not appear; the Visual Studio debugger is fully
engaged and on the job.

Inspect the platform properties
-------------------------------

At this point, it may be helpful to take a look at the properties that are
associated with the PPAPI and Native Client platforms---see the settings in the
sample project as an example.

Developing for Native Client in Visual Studio
=============================================

After you’ve installed the add-in and tried the sample project, you’re ready to
start working with your own code. You can reuse the sample project and the
PPAPI and Native Client platforms it already has by replacing the source code
with your own. More likely, you will add the platforms to an existing project,
or to a new project that you create from scratch.

Adding platforms to a project
-----------------------------

Follow these steps to add the Native Client and PPAPI platforms to a project:

#. Open the Configuration Manager.
#. On the row corresponding to your project, click the Platform column dropdown
   menu and select ``<New...>``.
#. Select ``PPAPI``, ``NaCl32``, ``NaCl64``, or ``PNaCl`` from the New platform
   menu.
#. In most cases, you should select ``<Empty>`` in the “Copy settings from”
   menu.  **Never copy settings between ``PPAPI``, ``NaCl32``, ``NaCl64``,
   ``NaClARM``, or ``PNaCl`` platforms**. You can copy settings from a Win32
   platform, if one exists, but afterwards be sure that the project properties
   are properly set for the new platform, as mentioned in step 6 below.
#. If you like, check the “Create new solutions platform” box to create a
   solution platform in addition to a project platform. (This is optional, but
   it can be convenient since it lets you switch project platforms from the
   Visual Studio main window by selecting the solution platform that has the
   same name.)
#. Review the project properties for the new platform you just added. In most
   cases, the default properties for each platform should be correct, but it
   pays to check. Be especially careful about custom properties you may have
   set beforehand, or copied from a Win32 platform. Also confirm that the
   Configuration type is correct:

   * ``Dynamic Library`` for ``PPAPI``
   * ``Application (.pexe)`` for ``PNaCl``
   * ``Application (.nexe)`` for ``NaCl32``, ``NaCl64``, and ``NaClARM``

Selecting a toolchain
---------------------

When you build a Native Client module directly from the SDK you can use two
different toolchains, newlib or glibc. See :doc:`Dynamic Linking and Loading
with glibc <dynamic-loading>` for a description of the two toolchains and
instructions on how to build and deploy an application with the glibc
toolchain. The Native Client platforms offer you the same toolchain choice. You
can specify which toolchain to use in the project properties, under
``Configuration Properties > General > Native Client > Toolchain``.

.. Note::
  :class: note

  Currently, the NaClARM and PNaCl platforms only support the newlib toolchain.

There is no toolchain property for the PPAPI platform. The PPAPI platform uses
the toolchain and libraries that come with Visual Studio.

Adding libraries to a project
-----------------------------

If your Native Client application requires libraries that are not included in
the SDK you must add them to the project properties (under ``Configuration
Properties > Linker > Input > Additional Dependencies``), just like any other
Visual Studio project. This list of dependencies is a semi-colon delimited
list. On the PPAPI platform the library names include the .lib extension (e.g.,
``ppapi_cpp.lib;ppapi.lib``). On the Native Client platforms the extension is
excluded (e.g., ``ppapi_cpp;ppapi``).

Running a web server
--------------------

In order for the Visual Studio add-in to test your Native Client module, you
must serve the module from a web server. There are two options:

Running your own server
^^^^^^^^^^^^^^^^^^^^^^^

When you start a debug run Visual Studio launches Chrome and tries to connect
to the web server at the address found in the Chrome command arguments (see the
project’s Debugging > Command configuration property), which is usually
``localhost:$(NaClWebServerPort)``. If you are using your own server be sure to
specify its address in the command arguments property, and confirm that your
server is running before starting a debug session. Also be certain that the
server has all the files it needs to deliver a Native Client module (see
“Keeping track of all the pieces”, below).

Running the SDK server
^^^^^^^^^^^^^^^^^^^^^^

If there is no web server running at the specified port, Visual Studio will try
to launch the simple Python web server that comes with the Native Client SDK.
It looks for a copy of the server in the SDK itself (at
``%NACL_SDK_ROOT%\tools\httpd.py``), and in the project directory
(``$(ProjectDir)/httpd.py``). If the server exists in one of those locations,
Visual Studio launches the server. The server output appears in Visual Studio’s
Output window, in the pane named “Native Client Web Server Output”. A server
launched in this way is terminated when the debugging session ends.

Keeping track of all the pieces
-------------------------------

No matter where the web server lives or how it’s launched you must make sure
that it has all the files that your application needs:

* All Native Client applications must have an :ref:`html host page
  <html_file>`. This file is typically called ``index.html``. The host page
  must have an embed tag with its type attribute set to
  ``application-type/x-nacl``. If you plan to use a Native Client platform the
  embed tag must also include a src attribute pointing to a Native Client
  manifest (.mnf) file.
* If you are using a Native Client platform you must include a valid
  :ref:`manifest file <manifest_file>`. The manifest file points to the .pexe
  or .nexe files that Visual Studio builds. These will be placed in the
  directory specified in the project’s ``General > Output Directory``
  configuration property, which is usually ``$(ProjectDir)$(ToolchainName)``.
  Visual Studio can use the Native Client SDK script create_nmf.py to
  automatically generate the manifest file for you.  To use this script set the
  project's ``Linker > General > Create NMF Automatically`` property to "yes."

If you are letting Visual Studio discover and run the SDK server, these files
should be placed in the project directory. If you are running your own server,
you must be sure that the host page ``index.html`` is placed in your server’s
root directory. Remember, if you’re using one of the Native Client platforms
the paths for the manifest file and .pexe or .nexe files must be reachable from
the server.

The structure of the manifest file can be more complicated if your application
uses Native Client's ability to dynamically link libraries. You may have to add
additional information about dynamically linked libraries to the manifest file
even if you create it automatically. The use and limitations of the create_nmf
tool are explained in :ref:`Generating a Native Client manifest file for a
dynamically linked application <dynamic_loading_manifest>`.

You can look at the example projects in the SDK to see how the index and
manifest files are organized. The example project ``hello_nacl`` has a
subdirectory also called ``hello_nacl``. That folder contains ``index.html``
and ``hello_nacl.nmf``. The nexe file is found in
``NaCl64\newlib\Debug\hello_nacl_64.nexe``. The ``hello_world_gles`` example
project contains a subdirectory called `hello_world_gles``. That directory
contains html files built with both toolchains (``index_glibc.html`` and
``index_newlib.html``). The .nexe and .nmf files are found in the newlib and
glibc subfolders. For additional information about the parts of a Native Client
application, see :doc:`Application Structure
<../coding/application-structure>`.

Using the debuggers
-------------------

PPAPI plugins are built natively by Visual Studio’s compiler (MSBuild), and
work with Visual Studio’s debugger in the usual way. You can set breakpoints in
the Visual Studio source code files before you begin debugging, and on-the-fly
while running the program.

NaCl32 and NaClARM executables (.nexe files) cannot be run or debugged from
Visual Studio.

NaCl64 executables (.nexe files) are compiled using one of the Native Client
toolchains in the SDK, which create an `ELF-formatted
<http://en.wikipedia.org/wiki/Executable_and_Linkable_Format>`_ executable. To
debug a running .nexe you must use nacl-gdb, which is a command line debugger
that is not directly integrated with Visual Studio. When you start a debugging
session running from a NaCl64 platform, Visual Studio automatically launches
nacl-gdb for you and attaches it to the nexe. Breakpoints that you set in
Visual Studio before you start debugging are transferred to nacl-gdb
automatically. During a NaCl debugging session you can only use nacl-gdb
commands.

The PNaCl platform generates a .pexe file. When you run the debugger add-in
translates the .pexe file to a .nexe file and runs the resulting binary with
nacl-gdb attached.

For additional information about nacl-gdb, see :ref:`Debugging with nacl-gdb
<using_gdb>` (scroll down to the end of the section to see some commonly used
gdb commands).

Note that you can’t use the Start Without Debugging command (Ctrl+F5) with a
project in the Debug configuration. If you do, Chrome will hang because the
Debug platform launches Chrome with the command argument
``--wait-for-debugger-children`` (in PPAPI) or ``--enable-nacl-debug`` (in a
Native Client platform). These flags cause Chrome to pause and wait for a
debugger to attach. If you use the Start Without Debugging command, no debugger
attaches and Chrome just waits patiently. To use Start Without Debugging,
switch to the Release configuration, or manually remove the offending argument
from the ``Command Arguments`` property.

Disable Chrome caching
----------------------

When you debug with a Native Client platform you might want to :ref:`disable
Chrome's cache <cache>` to be sure you are testing your latest and greatest
code.

A warning about PostMessage
---------------------------

Some Windows libraries define the symbol ``PostMessage`` as ``PostMessageW``.
This can cause havoc if you are working with the PPAPI platform and you use the
Pepper ``PostMessage()`` call in your module. Some Pepper API header files
contain a self-defensive fix that you might need yourself, while you are
testing on the PPAPI platform. Here it is:

.. naclcode::

  // If Windows defines PostMessage, undef it.
  #ifdef PostMessage
  #undef PostMessage
  #endif

Porting Windows applications to Native Client in Visual Studio
--------------------------------------------------------------

At Google I/O 2012 we demonstrated how to port a Windows desktop application to
Native Client in 60 minutes. The `video
<http://www.youtube.com/watch?v=1zvhs5FR0X8&feature=plcp>`_ is available to
watch on YouTube. The ``vs_addin/examples`` folder contains a pair of simple
examples that demonstrate porting process.  They are designed to be completed
in just 5 minutes. The two examples are called ``hello_nacl`` and
``hello_nacl_cpp``. They are essentially the same, but the former uses the C
PPAPI interface while the latter uses the C++ API.  The application is the
familiar "Hello, World."

Each example begins with the Windows desktop version running in the ``Win32``
platform. From there you move to the ``PPAPI`` platform, where you perform a
series of steps to set up the Native Client framework, use it to run the
desktop version, and then port the behavior from Windows calls to the PPAPI
interface.  You wind up with a program that uses no Windows functions, which
can run in either the ``PPAPI`` or the ``NaCl64`` platform.

The example projects use a single source file (``hello_nacl.c`` or
``hello_nacl_cpp.cpp``). Each step in the porting process is accomplished by
progressively defining the symbols STEP1 through STEP6 in the source. Inline
comments explain how each successive step changes the code. View the example
code to see how it's actually done. Here is a summary of the process:

Win32 Platform
^^^^^^^^^^^^^^

STEP1 Run the desktop application
  Begin by running the original Windows application in the Win32 platform.

PPAPI Platform
^^^^^^^^^^^^^^

STEP2 Launch Chrome with an empty Native Client module
  Switch to the PPAPI platform and include the code required to initialize a
  Native Module instance. The code is bare-boned, it does nothing but
  initialize the module. This step illustrates how Visual Studio handles all
  the details of launching a web-server and Chrome, and running the Native
  Client module as a Pepper plugin.

STEP3 Run the desktop application synchronously from the Native Client module
  The Native Client creates the window directly and then calls WndProc to run
  the desktop application. Since WndProc spins in its message loop, the call to
  initialize the module never returns. Close the Hello World window and the
  module initialization will finish.

STEP4 Running the desktop application and Native Client asynchronously
  In WndProc replace the message loop with a callback function. Now the app
  window and the Native Client module are running concurrently.

STEP5 Redirect output to the web page
  The module initialization code calls initInstanceInBrowserWindow rather than
  initInstanceInPCWindow. WndProc is no longer used. Instead, postMessage is
  called to place text (now "Hello, Native Client") in the web page rather than
  opening and writing to a window. Once you've reached this step you can start
  porting pieces of the application one feature at a time.

STEP6 Remove all the Windows code
  All the Windows code is def'd out, proving we are PPAPI-compliant. The
  functional code that is running is the same as STEP5.

NaCl64 Platform
^^^^^^^^^^^^^^^

Run the Native Client Module in the NaCl64 platform
  You are still running the STEP6 code, but as a Native Client module rather
  than a Pepper plugin.

.. TODO(sbc): port reference section?
