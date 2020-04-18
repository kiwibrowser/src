// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_X11_X11_EVENT_SOURCE_LIBEVENT_H_
#define UI_EVENTS_PLATFORM_X11_X11_EVENT_SOURCE_LIBEVENT_H_

#include "base/macros.h"
#include "base/message_loop/message_pump_libevent.h"
#include "ui/events/events_export.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/platform/x11/x11_event_source.h"

namespace ui {

// The XEventDispatcher interface is used in two different ways: the first is
// when classes want to receive XEvent directly and second is to say if classes,
// which also implement the PlatformEventDispatcher interface, are able to
// process next translated from XEvent to ui::Event events. Only used with Ozone
// X11 currently.
class EVENTS_EXPORT XEventDispatcher {
 public:
  // XEventDispatchers can be used to test if they are able to process next
  // translated event sent by a PlatformEventSource. If so, they must make a
  // promise internally to process next event sent by PlatformEventSource.
  virtual void CheckCanDispatchNextPlatformEvent(XEvent* xev) = 0;

  // Tells that an event has been dispatched and an event handling promise must
  // be removed.
  virtual void PlatformEventDispatchFinished() = 0;

  // Returns PlatformEventDispatcher if this XEventDispatcher is associated with
  // a PlatformEventDispatcher as well. Used to explicitly add a
  // PlatformEventDispatcher during a call from an XEventDispatcher to
  // AddXEventDispatcher.
  virtual PlatformEventDispatcher* GetPlatformEventDispatcher() = 0;

  // Sends XEvent to XEventDispatcher for handling. Returns true if the XEvent
  // was dispatched, otherwise false. After the first XEventDispatcher returns
  // true XEvent dispatching stops.
  virtual bool DispatchXEvent(XEvent* xevent) = 0;

 protected:
  virtual ~XEventDispatcher() {}
};

// A PlatformEventSource implementation for Ozone X11. Converts XEvents to
// ui::Events before dispatching.  For X11 specific events a separate list of
// XEventDispatchers is maintained. Uses Libevent to be notified for incoming
// XEvents.
class EVENTS_EXPORT X11EventSourceLibevent
    : public X11EventSourceDelegate,
      public PlatformEventSource,
      public base::MessagePumpLibevent::FdWatcher {
 public:
  explicit X11EventSourceLibevent(XDisplay* display);
  ~X11EventSourceLibevent() override;

  static X11EventSourceLibevent* GetInstance();

  // Adds a XEvent dispatcher to the XEvent dispatcher list.
  // Also calls XEventDispatcher::GetPlatformEventDispatcher
  // to explicitly add this |dispatcher| to a list of PlatformEventDispatchers
  // in case if XEventDispatcher has a PlatformEventDispatcher. Thus,
  // there is no need to separately add self to the list of
  // PlatformEventDispatchers. This is needed because XEventDispatchers are
  // tested if they can receive an XEvent based on a XID target. If so, the
  // translated XEvent into a PlatformEvent is sent to that
  // PlatformEventDispatcher.
  void AddXEventDispatcher(XEventDispatcher* dispatcher);

  // Removes an XEvent dispatcher from the XEvent dispatcher list.
  // Also explicitly removes an XEventDispatcher from a PlatformEventDispatcher
  // list if the XEventDispatcher has a PlatformEventDispatcher.
  void RemoveXEventDispatcher(XEventDispatcher* dispatcher);

  // X11EventSourceDelegate:
  void ProcessXEvent(XEvent* xevent) override;

 private:
  // Registers event watcher with Libevent.
  void AddEventWatcher();

  // Tells XEventDispatchers, which can also have PlatformEventDispatchers, that
  // a translated event is going to be sent next, then dispatches the event and
  // notifies XEventDispatchers the event has been sent out and, most probably,
  // consumed.
  void DispatchPlatformEvent(const PlatformEvent& event, XEvent* xevent);

  // Sends XEvent to registered XEventDispatchers.
  void DispatchXEventToXEventDispatchers(XEvent* xevent);

  // PlatformEventSource:
  void StopCurrentEventStream() override;
  void OnDispatcherListChanged() override;

  // base::MessagePumpLibevent::FdWatcher:
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  X11EventSource event_source_;

  // Keep track of all XEventDispatcher to send XEvents directly to.
  base::ObserverList<XEventDispatcher> dispatchers_xevent_;

  base::MessagePumpLibevent::FdWatchController watcher_controller_;
  bool initialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(X11EventSourceLibevent);
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_X11_X11_EVENT_SOURCE_LIBEVENT_H_
