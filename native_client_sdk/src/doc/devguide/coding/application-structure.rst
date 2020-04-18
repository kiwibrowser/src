.. _devcycle-application-structure:

.. include:: /migration/deprecation.inc

#####################
Application Structure
#####################

.. contents::
  :local:
  :backlinks: none
  :depth: 2

This section of the Developer's Guide describes the general structure of a
Native Client application. The section assumes you are familiar with the
material presented in the :doc:`Technical Overview <../../overview>`.


.. Note::
  :class: note

  The "Hello, World" example is used here to illustrate basic
  Native Client programming techniques. You can find this code in the
  */getting_started/part1* directory in the Native Client SDK download.

Application components
======================

A Native Client application typically contains the following components:

* an HTML file;
* JavaScript code, which can be included in the HTML file or contained in one or
  more separate .js files;
* CSS styles, which can be included in the HTML file or contained in one or more
  separate .css files;
* a Native Client manifest file (with a .nmf extension) that specifies how to
  load a Native Client module for different processors; and
* a Native Client module, written in C or C++, and compiled into a portable
  executable file (with a .pexe extension) or (if using the Chrome Web Store),
  architecture-specific executable files (with .nexe extensions).


Applications that are published in the `Chrome Web Store
<https://chrome.google.com/webstore/search?q=%22Native+Client%22+OR+NativeClient+OR+NaCl>`_
also include a Chrome
Web Store manifest file ``(manifest.json)`` and one or more icon files.

.. _html_file:

HTML file and the <embed> element
=================================

The ``<embed>`` element in an HTML file triggers the loading of a Native Client
module and specifies the rectangle on the web page that is managed by the
module. Here is the <embed> element from the "Hello, World" application:

.. naclcode::

  <embed id="hello_tutorial"
    width=0 height=0
    src="hello_tutorial.nmf"
    type="application/x-pnacl" />

In the ``<embed>`` element:

name
  is the DOM name attribute for the Native Client module
  ("nacl_module" is often used as a convention)
id
  specifies the DOM ID for the Native Client module
width, height
  specify the size in pixels of the rectangle on the web page that is
  managed by the Native Client module (if the module does not have a
  visible area, these values can be 0)
src
  refers to the Native Client manifest file that is used to determine
  which version of a module to load based on the architecture of the
  user's computer (see the following section for more information)
type
  specifies the MIME type of the embedded content; for Portable Native Client
  modules the type must be "application/x-pnacl". For architecture-specific
  Native Client modules the type must be "application/x-nacl"


.. _manifest_file:

Manifest Files
==============

Native Client applications have two types of manifest files: a Chrome Web Store
manifest file and a Native Client manifest file.

A **Chrome Web Store manifest file** is a file with information about a web
application that is published in the Chrome Web Store. This file, named
``manifest.json``, is required for applications that are published in the
Chrome Web Store. For more information about this file see :doc:`Distributing
Your Application <../distributing>`.  and the `Chrome Web Store manifest file
format </extensions/manifest>`_.

A **Native Client manifest file** is a file that specifies which Native Client
module (executable) to load. For PNaCl it specifies a single portable
executable; otherwise it specifies one for each of the supported end-user
computer architectures (for example x86-32, x86-64, or ARM). This file is
required for all Native Client applications. The extension for this file is
.nmf.

Manifest files for applications that use PNaCl are simple. Here is the manifest
for the hello world example:

.. naclcode::

  {
    "program": {
      "portable": {
        "pnacl-translate": {
          "url": "hello_tutorial.pexe"
        }
      }
    }
  }


For Chrome Web Store applications that do not use PNaCl, a typical manifest file
contains a `JSON <http://www.json.org/>`_ dictionary with a single top-level
key/value pair: the "program" key and a value consisting of a nested
dictionary. The nested dictionary contains keys corresponding to the names of
the supported computer architectures, and values referencing the file to load
for a given architecture—specifically, the URL of the .nexe file, given by the
``"url"`` key. URLs are specified relative to the location of the manifest file.
Here is an example:

.. naclcode::

  {
    "program": {
      "x86-32": {
        "url": "hello_tutorial_x86_32.nexe"
      },
      "x86-64": {
        "url": "hello_tutorial_x86_64.nexe"
      },
      "arm": {
        "url": "hello_tutorial_arm.nexe"
      }
    }
  }

For applications that use the :ref:`glibc<c_libraries>`
library, the manifest file must also contain a "files" key that specifies the
shared libraries that the applications use. This is discussed in detail in
:doc:`Dynamic Linking and Loading with glibc<../devcycle/dynamic-loading>`. To
see some example manifest files, build some of the example applications in the
SDK (run ``make`` in the example subdirectories) and look at the generated
manifest files.

In most cases, you can simply use the Python script provided with the SDK,
``create_nmf.py``, to create a manifest file for your application as part of the
compilation step (see the Makefile in any of the SDK examples for an
illustration of how to do so). The manifest file format is also
:doc:`documented<../../reference/nacl-manifest-format>`.

Modules and instances
=====================

A Native Client **module** is C or C++ code compiled into a PNaCl .pexe file or
a NaCl .nexe file.

An **instance** is a rectangle on a web page that is managed by a module. An
instance may have a dimension of width=0 and height=0, meaning that the instance
does not have any visible component on the web page. An instance is created by
including an ``<embed>`` element in a web page. The ``<embed>`` element
references a Native Client manifest file that loads the appropriate version of
the module (either portable, or specific to the end-user's architecture).  A
module may be included in a web page multiple times by using multiple
``<embed>`` elements that refer to the module; in this case the Native Client
runtime system loads the module once and creates multiple instances that are
managed by the module.


Native Client modules: A closer look
====================================

A Native Client module must include three components:

* a factory function called ``CreateModule()``
* a Module class (derived from the ``pp::Module`` class)
* an Instance class (derived from the ``pp:Instance`` class)

In the "Hello tutorial" example (in the ``getting_started/part1`` directory of
the NaCl SDK), these three components are specified in the file
``hello_tutorial.cc``. Here is the factory function:

.. naclcode::

  Module* CreateModule() {
    return new HelloTutorialModule();
  }

Native Client modules do not have a ``main()`` function. The ``CreateModule()``
factory function is the main binding point between a module and the browser, and
serves as the entry point into the module. The browser calls ``CreateModule()``
when a module is first loaded; this function returns a Module object derived
from the ``pp::Module`` class. The browser keeps a singleton of the Module
object.

Below is the Module class from the "Hello tutorial" example:

.. naclcode::

  class HelloTutorialModule : public pp::Module {
   public:
    HelloTutorialModule() : pp::Module() {}
    virtual ~HelloTutorialModule() {}

    virtual pp::Instance* CreateInstance(PP_Instance instance) {
      return new HelloTutorialInstance(instance);
    }
  };

The Module class must include a ``CreateInstance()`` method. The browser calls
the ``CreateInstance()`` method every time it encounters an ``<embed>`` element
on a web page that references the same module. The ``CreateInstance()`` function
creates and returns an Instance object derived from the ``pp::Instance`` class.

Below is the Instance class from the "Hello tutorial" example:

.. naclcode::

  class HelloTutorialInstance : public pp::Instance {
   public:
    explicit HelloTutorialInstance(PP_Instance instance) : pp::Instance(instance) {}
    virtual ~HelloTutorialInstance() {}

    virtual void HandleMessage(const pp::Var& var_message) {}
  };


As in the example above, the Instance class for your module will likely include
an implementation of the ``HandleMessage()`` function. The browser calls an
instance's ``HandleMessage()`` function every time the JavaScript code in an
application calls ``postMessage()`` to send a message to the instance. See the
:doc:`Native Client messaging system<message-system>` for more information about
how to send messages between JavaScript code and Native Client modules.

The NaCl code is only invoked to handle various browser-issued
events and callbacks. There is no need to shut down the NaCl instance by
calling the ``exit()`` function. NaCl modules will be shut down when the user
leaves the web page, or the NaCl module's ``<embed>`` is otherwise destroyed.
If the NaCl module does call the ``exit()`` function, the instance will
issue a ``crash`` event
:doc:`which can be handled in Javascript<progress-events>`.

While the ``CreateModule()`` factory function, the ``Module`` class, and the
``Instance`` class are required for a Native Client application, the code
samples shown above don't actually do anything. Subsequent sections in the
Developer's Guide build on these code samples and add more interesting
functionality.
