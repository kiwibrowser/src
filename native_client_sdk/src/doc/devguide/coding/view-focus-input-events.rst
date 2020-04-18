.. _view_focus_input_events:

.. include:: /migration/deprecation.inc

####################################
View Change, Focus, and Input Events
####################################

.. contents::
  :local:
  :backlinks: none
  :depth: 2

This section describes view change, focus, and input event handling for a
Native Client module. The section assumes you are familiar with the
material presented in the :doc:`Technical Overview <../../overview>`.

There are two examples used in this section to illustrate basic
programming techniques. The ``input_events`` example is used to
illustrate how your module can react to keyboard and mouse input
event.  The ``mouse_lock`` example is used to illustrate how your module
can react to view change events. You can find these examples in the
``/pepper_<version>/examples/api/input_event`` and 
``/pepper_<version>/examples/api/mouse_lock`` directories in the Native Client
SDK.  There is also the ppapi_simple library that can be used to to implement
most of the boiler plate.  The ``pi_generator`` example in
``/pepper_<version>/examples/demo/pi_generator`` uses ppapi_simple to manage
view change events and 2D graphics.


Overview
========

When a user interacts with the web page using a keyboard, mouse or some other
input device, the browser generates input events.  In a traditional web
application, these input events are passed to and handled in JavaScript,
typically through event listeners and event handlers. In a Native Client
application, user interaction with an instance of a module (e.g., clicking
inside the rectangle managed by a module) also generates input events, which
are passed to the module. The browser also passes view change and focus events
that affect a module's instance to the module. Native Client modules can
override certain functions in the `pp::Instance
</native-client/pepper_stable/cpp/classpp_1_1_instance>`_ class to handle input
and browser events. These functions are listed in the table below:

+-------------------------------------+----------------------------------------+
| Function                            | Use                                    |
+=====================================+========================================+
|``DidChangeView``                    |An implementation of this function might|
|  Called when the position, size, or |check the size of the module instance's |
|  clip rectangle of the module's     |rectangle has changed and reallocate the|
|  instance in the browser has        |graphcs context when a different size is|
|  changed. This event also occurs    |received.                               |
|  when the browser window is resized |                                        |
|  or the mouse wheel is scrolled.    |                                        |
+-------------------------------------+----------------------------------------+
|``DidChangeFocus``                   |An implementation of this function might|
|  Called when the module's instance  |start or stop an animation or a blinking|
|  in the browser has gone in or out  |cursor.                                 |
|  of focus (usually by clicking      |                                        |
|  inside or outside the module       |                                        |
|  instance). Having focus means that |                                        |
|  keyboard events will be sent to the|                                        |
|  module instance. An instance's     |                                        |
|  default condition is that it does  |                                        |
|  not have focus.                    |                                        |
+-------------------------------------+----------------------------------------+
|``HandleDocumentLoad``               |This API is only applicable when you are|
|  ``pp::Instance::Init()`` for a     |writing an extension to enhance the     |
|  full-frame module instance that was|abilities of the Chrome web browser. For|
|  instantiated based on the MIME     |example, a PDF viewer might implement   |
|  type of a DOMWindow navigation.    |this function to download and display a |
|  This situation only applies to     |PDF file.                               |
|  modules that are pre-registered to |                                        |
|  handle certain MIME types. If you  |                                        |
|  haven't specifically registered to |                                        |
|  handle a MIME type or aren't       |                                        |
|  positive this applies to you, your |                                        |
|  implementation of this function can|                                        |
|  just return false.                 |                                        |
+-------------------------------------+----------------------------------------+
|``HandleInputEvent``                 |An implementation of this function      |
|  Called when a user interacts with  |examines the input event type and       |
|  the module's instance in the       |branches accordingly.                   |
|  browser using an input device such |                                        |
|  as a mouse or keyboard. You must   |                                        |
|  register your module to accept     |                                        |
|  input events using                 |                                        |
|  ``RequestInputEvents()``           |                                        |
|  for mouse events and               |                                        |
|  ``RequestFilteringInputEvents()``  |                                        |
|  for keyboard events prior to       |                                        |
|  overriding this function.          |                                        |
+-------------------------------------+----------------------------------------+


These interfaces are found in the `pp::Instance class
</native-client/pepper_stable/cpp/classpp_1_1_instance>`_.  The sections below
provide examples of how to handle these events.


Handling browser events
=======================

DidChangeView()
---------------

In the ``mouse_lock`` example, ``DidChangeView()`` checks the previous size
of instance's rectangle versus the new size.  It also compares
other state such as whether or not the app is running in full screen mode.
If none of the state has actually changed, no action is needed.
However, if the size of the view or other state has changed, it frees the
old graphics context and allocates a new one.

.. naclcode::

  void MouseLockInstance::DidChangeView(const pp::View& view) {
    // DidChangeView can get called for many reasons, so we only want to
    // rebuild the device context if we really need to.
    if ((size_ == view.GetRect().size()) &&
        (was_fullscreen_ == view.IsFullscreen()) && is_context_bound_) {
      return;
    }

    // ...

    // Reallocate the graphics context.
    size_ = view.GetRect().size();
    device_context_ = pp::Graphics2D(this, size_, false);
    waiting_for_flush_completion_ = false;

    is_context_bound_ = BindGraphics(device_context_);
    // ...

    // Remember if we are fullscreen or not
    was_fullscreen_ = view.IsFullscreen();
    // ...
  }


For more information about graphics contexts and how to manipulate images, see:

* `pp::ImageData class
  </native-client/pepper_stable/cpp/classpp_1_1_image_data>`_
* `pp::Graphics2D class
  </native-client/pepper_stable/cpp/classpp_1_1_graphics2_d>`_


DidChangeFocus()
----------------

``DidChangeFocus()`` is called when you click inside or outside of a
module's instance in the web page. When the instance goes out
of focus (click outside of the instance), you might do something
like stop an animation. When the instance regains focus, you can
restart the animation.

.. naclcode::

  void DidChangeFocus(bool focus) {
    // Do something like stopping animation or a blinking cursor in
    // the instance.
  }


Handling input events
=====================

Input events are events that occur when the user interacts with a
module instance using the mouse, keyboard, or other input device
(e.g., touch screen). This section describes how the ``input_events``
example handles input events.


Registering a module to accept input events
-------------------------------------------

Before your module can handle these events, you must register your
module to accept input events using ``RequestInputEvents()`` for mouse
events and ``RequestFilteringInputEvents()`` for keyboard events. For the
``input_events`` example, this is done in the constructor of the
``InputEventInstance`` class:

.. naclcode::

  class InputEventInstance : public pp::Instance {
   public:
    explicit InputEventInstance(PP_Instance instance)
        : pp::Instance(instance), event_thread_(NULL), callback_factory_(this) {
      RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL |
                         PP_INPUTEVENT_CLASS_TOUCH);
      RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
    }
    // ...
  };


``RequestInputEvents()`` and ``RequestFilteringInputEvents()`` accept a
combination of flags that identify the class of events that the instance is
requesting to receive. Input event classes are defined in the
`PP_InputEvent_Class
</native-client/pepper_stable/c/group___enums.html#gafe68e3c1031daa4a6496845ff47649cd>`_
enumeration in `ppb_input_event.h
</native-client/pepper_stable/c/ppb__input__event_8h>`_.


Determining and branching on event types
----------------------------------------

In a typical implementation, the ``HandleInputEvent()`` function determines the
type of each event using the ``GetType()`` function found in the ``InputEvent``
class. The ``HandleInputEvent()`` function then uses a switch statement to
branch on the type of input event. Input events are defined in the
`PP_InputEvent_Type
</native-client/pepper_stable/c/group___enums.html#gaca7296cfec99fcb6646b7144d1d6a0c5>`_
enumeration in `ppb_input_event.h
</native-client/pepper_stable/c/ppb__input__event_8h>`_.

.. naclcode::

  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    Event* event_ptr = NULL;
    switch (event.GetType()) {
      case PP_INPUTEVENT_TYPE_UNDEFINED:
        break;
      case PP_INPUTEVENT_TYPE_MOUSEDOWN:
      case PP_INPUTEVENT_TYPE_MOUSEUP:
      case PP_INPUTEVENT_TYPE_MOUSEMOVE:
      case PP_INPUTEVENT_TYPE_MOUSEENTER:
      case PP_INPUTEVENT_TYPE_MOUSELEAVE:
      case PP_INPUTEVENT_TYPE_CONTEXTMENU: {
        pp::MouseInputEvent mouse_event(event);
        PP_InputEvent_MouseButton pp_button = mouse_event.GetButton();
        MouseEvent::MouseButton mouse_button = MouseEvent::kNone;
        switch (pp_button) {
          case PP_INPUTEVENT_MOUSEBUTTON_NONE:
            mouse_button = MouseEvent::kNone;
            break;
          case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
            mouse_button = MouseEvent::kLeft;
            break;
          case PP_INPUTEVENT_MOUSEBUTTON_MIDDLE:
            mouse_button = MouseEvent::kMiddle;
            break;
          case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
            mouse_button = MouseEvent::kRight;
            break;
        }
        event_ptr =
            new MouseEvent(ConvertEventModifier(mouse_event.GetModifiers()),
                           mouse_button,
                           mouse_event.GetPosition().x(),
                           mouse_event.GetPosition().y(),
                           mouse_event.GetClickCount(),
                           mouse_event.GetTimeStamp(),
                           event.GetType() == PP_INPUTEVENT_TYPE_CONTEXTMENU);
      } break;
      case PP_INPUTEVENT_TYPE_WHEEL: {
        pp::WheelInputEvent wheel_event(event);
        event_ptr =
            new WheelEvent(ConvertEventModifier(wheel_event.GetModifiers()),
                           wheel_event.GetDelta().x(),
                           wheel_event.GetDelta().y(),
                           wheel_event.GetTicks().x(),
                           wheel_event.GetTicks().y(),
                           wheel_event.GetScrollByPage(),
                           wheel_event.GetTimeStamp());
      } break;
      case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
      case PP_INPUTEVENT_TYPE_KEYDOWN:
      case PP_INPUTEVENT_TYPE_KEYUP:
      case PP_INPUTEVENT_TYPE_CHAR: {
        pp::KeyboardInputEvent key_event(event);
        event_ptr = new KeyEvent(ConvertEventModifier(key_event.GetModifiers()),
                                 key_event.GetKeyCode(),
                                 key_event.GetTimeStamp(),
                                 key_event.GetCharacterText().DebugString());
      } break;
      default: {
        // For any unhandled events, send a message to the browser
        // so that the user is aware of these and can investigate.
        std::stringstream oss;
        oss << "Default (unhandled) event, type=" << event.GetType();
        PostMessage(oss.str());
      } break;
    }
    event_queue_.Push(event_ptr);
    return true;
  }


Notice that the generic ``InputEvent`` received by ``HandleInputEvent()`` is
converted into a specific type after the event type is
determined.  The event types handled in the example code are
``MouseInputEvent``, ``WheelInputEvent``, and ``KeyboardInputEvent``.
There are also ``TouchInputEvents``.  For the latest list of event types,
see the `InputEvent documentation
</native-client/pepper_stable/c/classpp_1_1_input_event>`_.
For reference information related to the these event classes, see the
following documentation:

* `pp::MouseInputEvent class
  </native-client/pepper_stable/c/classpp_1_1_mouse_input_event>`_
* `pp::WheelInputEvent class
  </native-client/pepper_stable/c/classpp_1_1_wheel_input_event>`_
* `pp::KeyboardInputEvent class
  </native-client/pepper_stable/c/classpp_1_1_keyboard_input_event>`_


Threading and blocking
----------------------

``HandleInputEvent()`` in this example runs on the main module thread.
However, the bulk of the work happens on a separate worker thread (see
``ProcessEventOnWorkerThread``). ``HandleInputEvent()`` puts events in
the ``event_queue_`` and the worker thread takes events from the
``event_queue_``. This processing happens independently of the main
thread, so as not to slow down the browser.
