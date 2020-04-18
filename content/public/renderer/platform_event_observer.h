// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_PLATFORM_EVENT_OBSERVER_H_
#define CONTENT_PUBLIC_RENDERER_PLATFORM_EVENT_OBSERVER_H_

#include "base/logging.h"
#include "base/macros.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_thread_observer.h"

namespace blink {
class WebPlatformEventListener;
}

namespace content {

// This class is used as a base class for PlatformEventObserver<ListenerType> to
// allow storing PlatformEventObserver<> with different typename in the same
// place.
class PlatformEventObserverBase {
 public:
  virtual ~PlatformEventObserverBase() { }

  // Methods that need to be exposed in PlatformEventObserverBase. Their purpose
  // is described in PlatformEventObserver<>.

  virtual void Start(blink::WebPlatformEventListener* listener) = 0;
  virtual void Stop() = 0;

  // Helper method that allows an sub-class to write its own test helper.
  // The |data| type MUST be known from the caller.
  virtual void SendFakeDataForTesting(void* data) { }
};

// PlatformEventObserver<> defines the basic skeleton for an object requesting
// the browser process to start/stop listening to some platform/hardware events
// and observe the result.
// The results are received via IPC, assuming that the object was correctly
// registered as an observer via the constructor taking a RenderThread.
template <typename ListenerType>
class PlatformEventObserver : public PlatformEventObserverBase,
                              public RenderThreadObserver {
 public:
  // Creates a PlatformEventObserver that doesn't listen to responses from the
  // browser process. Can be used for testing purposes or for observers that
  // have other means to get their results.
  PlatformEventObserver()
      : is_observing_(false),
        listener_(0) {
  }

  // Creates a PlatformEventObserver that registers to the RenderThread in order
  // to intercept the received IPC messages (via OnControlMessageReceived). If
  // |thread| is null, it will not register.
  explicit PlatformEventObserver(RenderThread* thread)
      : is_observing_(false),
        listener_(0) {
    if (thread)
      thread->AddObserver(this);
  }

  // The observer must automatically stop observing when destroyed in case it
  // did not stop before. Implementations of PlatformEventObserver must do
  // so by calling StopIfObserving() from their destructors.
  ~PlatformEventObserver() override {
    // If this assert fails, the derived destructor failed to invoke
    // StopIfObserving().
    DCHECK(!is_observing());
  }

  // Called when a new IPC message is received. Must be used to listen to the
  // responses from the browser process if any expected.
  bool OnControlMessageReceived(const IPC::Message& msg) override {
    return false;
  }

  // Start observing. Will request the browser process to start listening to the
  // events. |listener| will receive any response from the browser process.
  // Note: should not be called if already observing.
  void Start(blink::WebPlatformEventListener* listener) override {
    DCHECK(!is_observing());
    listener_ = static_cast<ListenerType*>(listener);
    is_observing_ = true;

    SendStartMessage();
  }

  // Stop observing. Will let the browser know that it doesn't need to observe
  // anymore.
  void Stop() override {
    DCHECK(is_observing());
    listener_ = 0;
    is_observing_ = false;

    SendStopMessage();
  }

 protected:
  // This method is expected to send an IPC to the browser process to let it
  // know that it should start observing.
  // It is expected for subclasses to override it.
  virtual void SendStartMessage() = 0;

  // This method is expected to send an IPC to the browser process to let it
  // know that it should start observing.
  // It is expected for subclasses to override it.
  virtual void SendStopMessage() = 0;

  // Implementations of PlatformEventObserver must call StopIfObserving()
  // from their destructor to shutdown in an orderly manner.
  // (As Stop() calls a virtual method, it cannot be handled by
  // ~PlatformEventObserver.)
  void StopIfObserving() {
    if (is_observing())
      Stop();
  }

  bool is_observing() const {
    return is_observing_;
  }

  ListenerType* listener() {
    return listener_;
  }

 private:
  bool is_observing_;
  ListenerType* listener_;

  DISALLOW_COPY_AND_ASSIGN(PlatformEventObserver);
};

} // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_PLATFORM_EVENT_OBSERVER_H_
