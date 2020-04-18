// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// PLEASE NOTE: this is a copy with modifications from chrome/browser/speech.
// It is temporary until a refactoring to move the chrome TTS implementation up
// into components and extensions/components can be completed.

#ifndef CHROMECAST_BROWSER_TTS_TTS_MESSAGE_FILTER_H_
#define CHROMECAST_BROWSER_TTS_TTS_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "chromecast/browser/tts/tts_controller.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class BrowserContext;
}

struct TtsUtteranceRequest;

class TtsMessageFilter : public content::BrowserMessageFilter,
                         public content::NotificationObserver,
                         public UtteranceEventDelegate,
                         public VoicesChangedDelegate {
 public:
  explicit TtsMessageFilter(content::BrowserContext* browser_context);

  // content::BrowserMessageFilter implementation.
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelClosing() override;
  void OnDestruct() const override;

  // UtteranceEventDelegate implementation.
  void OnTtsEvent(Utterance* utterance,
                  TtsEventType event_type,
                  int char_index,
                  const std::string& error_message) override;

  // VoicesChangedDelegate implementation.
  void OnVoicesChanged() override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<TtsMessageFilter>;

  ~TtsMessageFilter() override;

  void OnInitializeVoiceList();
  void OnSpeak(const TtsUtteranceRequest& utterance);
  void OnPause();
  void OnResume();
  void OnCancel();

  void OnChannelClosingInUIThread();

  void Cleanup();

  // Thread-safe check to make sure this class is still valid and not
  // about to be deleted.
  bool Valid();

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::BrowserContext* browser_context_;
  mutable base::Lock mutex_;
  mutable bool valid_;
  content::NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(TtsMessageFilter);
};

#endif  // CHROMECAST_BROWSER_TTS_TTS_MESSAGE_FILTER_H_
