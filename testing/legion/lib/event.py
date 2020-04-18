# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Java-like event module.

To use this simply decorate the callback method with @event.Event, subscribe
to the callbacks using method += callback, and call the event method to
send the events.

class Foo(object):

  @event.Event
  def on_event_raised_1(self):
    # This is defined as a pass since this won't directly operate on the call.
    # This event takes no args when called.
    pass

  @event.Event
  def on_event_raised_2(self, arg):
    # This event takes 1 arg
    pass

  def do_something(self):
    # This method will raise an event on each handler
    self.on_event_raised_1()
    self.on_event_raised_2('foo')

To subscribe to events use the following code

def callback_1():
  print 'In callback 1'

def callback_2(arg):
  print 'In callback 2', arg

foo = Foo()
foo.on_event_raised_1 += callback_1
foo.on_event_raised_2 += callback_2
foo.do_something()

Each event can be associated with zero or more callback handlers, and each
callback handler can be associated with one or more events.

"""

import logging
import traceback


class Event(object):
  """"A Java-like event class."""

  def __init__(self, method):
    self._method = method
    self._callbacks = []

  def __iadd__(self, callback):
    """Allow method += callback syntax."""
    assert callback not in self._callbacks
    self._callbacks.append(callback)
    return self

  def __isub__(self, callback):
    """Allow method -= callback syntax."""
    self._callbacks.remove(callback)
    return self

  def __call__(self, *args, **kwargs):
    """Dispatches a method call to the appropriate callback handlers."""
    for callback in self._callbacks:
      try:
        callback(*args, **kwargs)
      except:  # pylint: disable=bare-except
        # Catch all exceptions here and log them. This way one exception won't
        # stop the remaining callbacks from being executed.
        logging.error(traceback.format_exc())
