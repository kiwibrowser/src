// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_EMBEDDER_TEST_TIMER_HANDLING_DELEGATE_H_
#define TESTING_EMBEDDER_TEST_TIMER_HANDLING_DELEGATE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "testing/embedder_test.h"
#include "testing/test_support.h"

class EmbedderTestTimerHandlingDelegate : public EmbedderTest::Delegate {
 public:
  struct AlertRecord {
    std::wstring message;
    std::wstring title;
    int type;
    int icon;
  };

  struct Timer {
    int id;
    int interval;
    TimerCallback fn;
  };

  int Alert(FPDF_WIDESTRING message,
            FPDF_WIDESTRING title,
            int type,
            int icon) override {
    alerts_.push_back(
        {GetPlatformWString(message), GetPlatformWString(title), type, icon});
    return 0;
  }

  int SetTimer(int msecs, TimerCallback fn) override {
    int id = fail_next_timer_ ? 0 : ++next_timer_id_;
    expiry_to_timer_map_.insert(
        std::pair<int, Timer>(msecs + fake_elapsed_msecs_, {id, msecs, fn}));
    fail_next_timer_ = false;
    return id;
  }

  void KillTimer(int id) override {
    for (auto iter = expiry_to_timer_map_.begin();
         iter != expiry_to_timer_map_.end(); ++iter) {
      if (iter->second.id == id) {
        expiry_to_timer_map_.erase(iter);
        break;
      }
    }
  }

  void AdvanceTime(int increment_msecs) {
    fake_elapsed_msecs_ += increment_msecs;
    while (1) {
      auto iter = expiry_to_timer_map_.begin();
      if (iter == expiry_to_timer_map_.end()) {
        break;
      }
      if (iter->first > fake_elapsed_msecs_) {
        break;
      }
      Timer t = iter->second;
      expiry_to_timer_map_.erase(iter);
      expiry_to_timer_map_.insert(
          std::pair<int, Timer>(fake_elapsed_msecs_ + t.interval, t));
      t.fn(t.id);  // Fire timer.
    }
  }

  const std::vector<AlertRecord>& GetAlerts() const { return alerts_; }

  void SetFailNextTimer() { fail_next_timer_ = true; }

 protected:
  std::multimap<int, Timer> expiry_to_timer_map_;  // Keyed by timeout.
  bool fail_next_timer_ = false;
  int next_timer_id_ = 0;
  int fake_elapsed_msecs_ = 0;
  std::vector<AlertRecord> alerts_;
};

#endif  // TESTING_EMBEDDER_TEST_TIMER_HANDLING_DELEGATE_H_
