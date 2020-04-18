.. _devcycle-native-client-modules:

.. include:: /migration/deprecation.inc

#####################
Native Client Modules
#####################

This document describes the classes and functions that you need to implement in
a Native Client module in order for Chrome to load, initialize, and run it.  The
requirements are the same regardless of whether or not the module uses PNaCl,
but depend on whether the module is written in C or C++.

.. contents::
  :local:
  :backlinks: none
  :depth: 2

Introduction
============

Native Client modules do not have a ``main()`` function. When a module loads,
the Native Client runtime calls the code in the module to create an instance and
initialize the interfaces for the APIs the module uses. This initialization
sequence depends on whether the module is written in C or C++ and requires that
you implement specific functions in each case.


Writing modules in C
====================

The C API uses a prefix convention to show whether an interface is implemented
in the browser or in a module. Interfaces starting with ``PPB_`` (which can be
read as "Pepper *browser*") are implemented in the browser and they are called
from your module. Interfaces starting with ``PPP_`` ("Pepper *plugin*") are
implemented in the module; they are called from the browser and will execute on
the main thread of the module instance.

When you implement a Native Client module in C you must include these components:

* The functions ``PPP_InitializeModule`` and ``PPP_GetInterface``
* Code that implements the interface ``PPP_Instance`` and any other C interfaces
  that your module uses

For each PPP interface, you must implement all of its functions, create the
struct through which the browser calls the interface, and insure that the
function ``PPP_GetInterface`` returns the appropriate struct for the interface.

For each PPB interface, you must declare a pointer to the interface and
initialize the pointer with a call to ``get_browser`` inside
``PPP_InitializeModule``.

These steps are illustrated in the code excerpt below, which shows the
implementation and initialization of the required ``PPP_Instance``
interface. The code excerpt also shows the initialization of three additional
interfaces which are not required: ``PPB_Instance`` (through which the Native
Client module calls back to the browser) and ``PPB_InputEvent`` and
``PPP_InputEvent``.

.. naclcode::

  #include <stdlib.h>
  #include <string.h>
  #include "ppapi/c/pp_errors.h"
  #include "ppapi/c/ppp.h"
  // Include the interface headers.
  // PPB APIs describe calls from the module to the browser.
  // PPP APIs describe calls from the browser to the functions defined in your module.
  #include "ppapi/c/ppb_instance.h"
  #include "ppapi/c/ppp_instance.h"
  #include "ppapi/c/ppb_input_event.h"
  #include "ppapi/c/ppp_input_event.h"

  // Create pointers for each PPB interface that your module uses.
  static PPB_Instance* ppb_instance_interface = NULL;
  static PPB_InputEvent* ppb_input_event_interface = NULL;

  // Define all the functions for each PPP interface that your module uses.
  // Here is a stub for the first function in PPP_Instance.
  static PP_Bool Instance_DidCreate(PP_Instance instance,
                                    uint32_t argc,
                                    const char* argn[],
                                    const char* argv[]) {
          return PP_TRUE;
  }
  // ... more API functions ...

  // Define PPP_GetInterface.
  // This function should return a non-NULL value for every interface you are using.
  // The string for the name of the interface is defined in the interface's header file.  
  // The browser calls this function to get pointers to the interfaces that your module implements.
  PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
          // Create structs for each PPP interface.
          // Assign the interface functions to the data fields.
           if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) {
                  static PPP_Instance instance_interface = {
                          &Instance_DidCreate,
                          // The definitions of these functions are not shown
                          &Instance_DidDestroy,
                          &Instance_DidChangeView,
                          &Instance_DidChangeFocus,
                          &Instance_HandleDocumentLoad
                  };
                  return &instance_interface;
           }

           if (strcmp(interface_name, PPP_INPUT_EVENT_INTERFACE) == 0) {
                  static PPP_InputEvent input_interface = {
                          // The definition of this function is not shown.
                          &Instance_HandleInput,
                  };
                  return &input_interface;
           }
           // Return NULL for interfaces that you do not implement.
           return NULL;
  }

  // Define PPP_InitializeModule, the entry point of your module.
  // Retrieve the API for the browser-side (PPB) interfaces you will use.
  PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id, PPB_GetInterface get_browser) {
          ppb_instance_interface = (PPB_Instance*)(get_browser(PPB_INSTANCE_INTERFACE));
          ppb_input_event_interface = (PPB_InputEvent*)(get_browser(PPB_INPUT_EVENT_INTERFACE));
          return PP_OK;
  }


Writing modules in C++
======================

When you implement a Native Client module in C++ you must include these components:

* The factory function called ``CreateModule()``
* Code that defines your own Module class (derived from the ``pp::Module``
  class)
* Code that defines your own Instance class (derived from the ``pp:Instance``
  class)

In the "Hello tutorial" example (in the ``getting_started/part1`` directory of
the NaCl SDK), these three components are specified in the file
``hello_tutorial.cc``. Here is the factory function:

.. naclcode::

  namespace pp {
    Module* CreateModule() {
      return new HelloTutorialModule();
    }
  }

The ``CreateModule()`` factory function is the main binding point between a
module and the browser, and serves as the entry point into the module. The
browser calls ``CreateModule()`` when a module is first loaded; this function
returns a Module object derived from the ``pp::Module`` class. The browser keeps
a singleton of the Module object.

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

While the ``CreateModule()`` factory function, the ``Module`` class, and the
``Instance`` class are required for a Native Client application, the code
samples shown above don't actually do anything. Subsequent documents in the
Developer's Guide build on these code samples and add more interesting
functionality.
