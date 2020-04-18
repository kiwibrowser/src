.. _message-system:

.. include:: /migration/deprecation.inc

################
Messaging System
################

.. contents::
  :local:
  :backlinks: none
  :depth: 2

This section describes the messaging system used to communicate between the
JavaScript code and the Native Client module's C or C++ code in a
Native Client application. It introduces the concept of asynchronous
programming and the basic steps required to set up a Native Client module
that sends messages to and receive messages from JavaScript. This section
assumes you are familiar with the material presented in the
:doc:`Application Structure <application-structure>` section.

.. Note::
  :class: note

  The "Hello, World" example for getting started with NaCl is used here to
  illustrate basic programming techniques. You can find this code in
  the ``/getting_started/part2`` directory in the Native Client SDK download.

Reference information
=====================

For reference information related to the Pepper messaging API, see the
following documentation:

* `pp::Instance class </native-client/pepper_stable/cpp/classpp_1_1_instance>`_
  HandleMessage(), PostMessage())
* `pp::Module class </native-client/pepper_stable/cpp/classpp_1_1_module>`_
* `pp::Var class </native-client/pepper_stable/cpp/classpp_1_1_var>`_

Introduction to the messaging system
====================================

Native Client modules and JavaScript communicate by sending messages to each
other. The most basic form of a message is a string.  Messages support many
JavaScript types, including ints, arrays, array buffers, and dictionaries (see
`pp::Var </native-client/pepper_stable/cpp/classpp_1_1_var>`_,
`pp:VarArrayBuffer
</native-client/pepper_stable/cpp/classpp_1_1_var_array_buffer>`_, and the
general `messaging system documentation
</native-client/pepper_stable/c/struct_p_p_b___messaging__1__0>`_).  It's up to
you to decide on the type of message and define how to process the messages on
both the JavaScript and Native Client side. For the "Hello, World" example, we
will work with string-typed messages only.

When JavaScript posts a message to the Native Client module, the
Pepper ``HandleMessage()`` function is invoked on the module
side. Similarly, the Native Client module can post a message to
JavaScript, and this message triggers a JavaScript event listener for
``message`` events in the DOM. (See the W3C specification on
`Document Object Model Events
<http://www.w3.org/TR/DOM-Level-2-Events/events.html>`_ for more
information.) In the "Hello, World" example, the JavaScript functions for
posting and handling messages are named ``postMessage()`` and
``handleMessage()`` (but any names could be used). On the Native Client
C++ side, the Pepper Library functions for posting and handling
messages are:

* ``void pp::Instance::PostMessage(const Var &message)``
* ``virtual void pp::Instance::HandleMessage(const Var &message)``

If you want to receive messages from JavaScript, you need to implement the
``pp::Instance::HandleMessage()`` function in your Native Client module.

Design of the messaging system
------------------------------

The Native Client messaging system is analogous to the system used by
the browser to allow web workers to communicate (see the `W3 web
worker specification <http://www.w3.org/TR/workers>`_).  The Native
Client messaging system is designed to keep the web page responsive while the
Native Client module is performing potentially heavy processing in the
background. When JavaScript sends a message to the Native Client
module, the ``postMessage()`` call returns as soon as it sends its message
to the Native Client module. The JavaScript does not wait for a reply
from Native Client, thus avoiding bogging down the main JavaScript
thread. On the JavaScript side, you set up an event listener to
respond to the message sent by the Native Client module when it has
finished the requested processing and returns a message.

This asynchronous processing model keeps the main thread free while
avoiding the following problems:

* The JavaScript engine hangs while waiting for a synchronous call to return.
* The browser pops up a dialog when a JavaScript entry point takes longer
  than a few moments.
* The application hangs while waiting for an unresponsive Native Client module.

Communication tasks in the "Hello, World" example
=================================================

The following sections describe how the "Hello, World" example posts
and handles messages on both the JavaScript side and the Native Client
side of the application.

JavaScript code
---------------

The JavaScript code and HTML in the "Hello, World" example can be
found in the ``example.js``, ``common.js``, and ``index.html`` files.
The important steps are:

#. Sets up an event listener to listen for ``message`` events from the
   Native Client module.
#. Implements an event handler that the event listener invokes to handle
   incoming ``message`` events.
#. Calls ``postMessage()`` to communicate with the NaCl module,
   after the page loads.

Step 1: From common.js
^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  function attachDefaultListeners() {
    // The NaCl module embed is created within the listenerDiv
    var listenerDiv = document.getElementById('listener');
    // ...

    // register the handleMessage function as the message event handler.
    listenerDiv.addEventListener('message', handleMessage, true);
    // ...
  }


Step 2: From example.js
^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  // This function is called by common.js when a message is received from the
  // NaCl module.
  function handleMessage(message) {
    // In the example, we simply log the data that's received in the message.
    var logEl = document.getElementById('log');
    logEl.textContent += message.data;
  }

  // In the index.html we have set up the appropriate divs:
  <body {attrs}>
    <!-- ... -->
    <div id="listener"></div>
    <div id="log"></div>
  </body>


Step 3: From example.js
^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  // From example.js, Step 3:
  function moduleDidLoad() {
    // After the NaCl module has loaded, common.naclModule is a reference to the
    // NaCl module's <embed> element.
    //
    // postMessage sends a message to it.
    common.naclModule.postMessage('hello');
  }


Native Client module
--------------------

The C++ code in the Native Client module of the "Hello, World" example:

#. Implements ``pp::Instance::HandleMessage()`` to handle messages sent
   by the JavaScript.
#. Processes incoming messages. This example simply checks that JavaScript
   has sent a "hello" message and not some other message.
#. Calls ``PostMessage()`` to send an acknowledgement back to the JavaScript
   code.  The acknowledgement is a string in the form of a ``Var`` that the
   JavaScript code can process.  In general, a ``pp::Var`` can be several
   JavaScript types, see the `messaging system documentation
   </native-client/pepper_stable/c/struct_p_p_b___messaging__1__0>`_.


.. naclcode::

  class HelloTutorialInstance : public pp::Instance {
   public:
    // ...

    // === Step 1: Implement the HandleMessage function. ===
    virtual void HandleMessage(const pp::Var& var_message) {

      // === Step 2: Process the incoming message. ===
      // Ignore the message if it is not a string.
      if (!var_message.is_string())
        return;

      // Get the string message and compare it to "hello".
      std::string message = var_message.AsString();
      if (message == kHelloString) {
        // === Step 3: Send the reply. ===
        // If it matches, send our response back to JavaScript.
        pp::Var var_reply(kReplyString);
        PostMessage(var_reply);
      }
    }
  };


Messaging in JavaScript code: More details.
===========================================

This section describes in more detail the messaging system code in the
JavaScript portion of the "Hello, World" example.

Setting up an event listener and handler
----------------------------------------

The following JavaScript code sets up an event listener for messages
posted by the Native Client module. It then defines a message handler
that simply logs the content of messages received from the module.

Setting up the 'message' handler on load
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  // From common.js

  // Listen for the DOM content to be loaded. This event is fired when
  // parsing of the page's document has finished.
  document.addEventListener('DOMContentLoaded', function() {
    var body = document.body;
    // ...
    var loadFunction = common.domContentLoaded;
    // ... set up parameters ...
    loadFunction(...);
  }

  // This function is exported as common.domContentLoaded.
  function domContentLoaded(...) {
    // ...
    if (common.naclModule == null) {
      // ...
      attachDefaultListeners();
      // initialize common.naclModule ...
    } else {
      // ...
    }
  }

  function attachDefaultListeners() {
    var listenerDiv = document.getElementById('listener');
    // ...
    listenerDiv.addEventListener('message', handleMessage, true);
    // ...
  }


Implementing the handler
^^^^^^^^^^^^^^^^^^^^^^^^

.. naclcode::

  // From example.js
  function handleMessage(message) {
    var logEl = document.getElementById('log');
    logEl.textContent += message.data;
  }


Note that the ``handleMessage()`` function is handed a message_event
containing ``data`` that you can display or manipulate in JavaScript. The
"Hello, World" application simply logs this data to the ``log`` div.


Messaging in the Native Client module: More details.
====================================================

This section describes in more detail the messaging system code in
the Native Client module portion of the "Hello, World" example.  

Implementing HandleMessage()
----------------------------

If you want the Native Client module to receive and handle messages
from JavaScript, you need to implement a ``HandleMessage()`` function
for your module's ``pp::Instance`` class. The
``HelloWorldInstance::HandleMessage()`` function examines the message
posted from JavaScript. First it examines that the type of the
``pp::Var`` is indeed a string (not a double, etc.). It then
interprets the data as a string with ``var_message.AsString()``, and
checks that the string matches ``kHelloString``. After examining the
message received from JavaScript, the code calls ``PostMessage()`` to
send a reply message back to the JavaScript side.

.. naclcode::

  namespace {

  // The expected string sent by the JavaScript.
  const char* const kHelloString = "hello";
  // The string sent back to the JavaScript code upon receipt of a message
  // containing "hello".
  const char* const kReplyString = "hello from NaCl";

  }  // namespace

  class HelloTutorialInstance : public pp::Instance {
   public:
    // ...
    virtual void HandleMessage(const pp::Var& var_message) {
      // Ignore the message if it is not a string.
      if (!var_message.is_string())
        return;

      // Get the string message and compare it to "hello".
      std::string message = var_message.AsString();
      if (message == kHelloString) {
        // If it matches, send our response back to JavaScript.
        pp::Var var_reply(kReplyString);
        PostMessage(var_reply);
      }
    }
  };


Implementing application-specific functions
-------------------------------------------

While the "Hello, World" example is very simple, your Native Client
module will likely include application-specific functions to perform
custom tasks in response to messages. For example the application
could be a compression and decompression service (two functions
exported).  The application could set up an application-specific
convention that messages coming from JavaScript are colon-separated
pairs of the form ``<command>:<data>``.  The Native Client module
message handler can then split the incoming string along the ``:``
character to determine which command to execute.  If the command is
"compress", then data to process is an uncompressed string.  If the
command is "uncompress", then data to process is an already-compressed
string. After processing the data asynchronously, the application then
returns the result to JavaScript.


Sending messages back to the JavaScript code
--------------------------------------------

The Native Client module sends messages back to the JavaScript code
using ``PostMessage()``. The Native Client module always returns
its values in the form of a ``pp::Var`` that can be processed by the
browser's JavaScript. In this example, the message is posted at the
end of the Native Client module's ``HandleMessage()`` function:

.. naclcode::

  PostMessage(var_reply);


Sending and receiving other ``pp::Var`` types
---------------------------------------------

Besides strings, ``pp::Var`` can represent other types of JavaScript
objects. For example, messages can be JavaScript objects. These
richer types can make it easier to implement an application's
messaging protocol.

To send a dictionary from the NaCl module to JavaScript simply create
a ``pp::VarDictionary`` and then call ``PostMessage`` with the
dictionary.

.. naclcode::

  pp::VarDictionary dictionary;
  dictionary.Set(pp::Var("command"), pp::Var(next_command));
  dictionary.Set(pp::Var("param_int"), pp::Var(123));
  pp::VarArray an_array;
  an_array.Set(0, pp::Var("string0"));
  an_array.Set(1, pp::Var("string1"))
  dictionary.Set(pp::Var("param_array"), an_array);
  PostMessage(dictionary);


Here is how to create a similar object in JavaScript and send it to
the NaCl module:

.. naclcode::

  var dictionary = {
    command: next_command,
    param_int: 123,
    param_array: ['string0', 'string1']
  }
  nacl_module.postMessage(dictionary);


To receive a dictionary-typed message in the NaCl module, test that
the message is truly a dictionary type, then convert the message
with the ``pp::VarDictionary`` class.

.. naclcode::

  virtual void HandleMessage(const pp::Var& var) {
    if (var.is_dictionary()) {
      pp::VarDictionary dictionary(var);
      // Use the dictionary
      pp::VarArray keys = dictionary.GetKeys();
      // ...
    } else {
      // ...
    }
  }
