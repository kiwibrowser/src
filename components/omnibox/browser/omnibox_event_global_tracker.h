// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EVENT_GLOBAL_TRACKER_H_
#define COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EVENT_GLOBAL_TRACKER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/macros.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

struct OmniboxLog;

// Omnibox code tracks events on a per-user-context basis, but there are
// several clients who need to observe these events for all user contexts
// (e.g., all Profiles in the //chrome embedder).  This class serves as an
// intermediary to bridge the gap: omnibox code calls the
// OmniboxEventGlobalTracker singleton on an event of interest, and it then
// forwards the event to its registered observers.
class OmniboxEventGlobalTracker {
 public:
  typedef base::Callback<void(OmniboxLog*)> OnURLOpenedCallback;

  // Returns the instance of OmniboxEventGlobalTracker.
  static OmniboxEventGlobalTracker* GetInstance();

  // Registers |cb| to be invoked when user open an URL from the omnibox.
  std::unique_ptr<base::CallbackList<void(OmniboxLog*)>::Subscription>
  RegisterCallback(const OnURLOpenedCallback& cb);

  // Called to notify all registered callbacks that an URL was opened from
  // the omnibox.
  void OnURLOpened(OmniboxLog* log);

 private:
  friend struct base::DefaultSingletonTraits<OmniboxEventGlobalTracker>;

  OmniboxEventGlobalTracker();
  ~OmniboxEventGlobalTracker();

  base::CallbackList<void(OmniboxLog*)> on_url_opened_callback_list_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxEventGlobalTracker);
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EVENT_GLOBAL_TRACKER_H_
