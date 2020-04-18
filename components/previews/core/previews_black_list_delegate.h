// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_DELEGATE_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "base/time/time.h"

namespace previews {

// An interface for PreviewsBlackList delegate. This interface is for responding
// to events occur in PreviewsBlackList (e.g. New blacklisted host and user is
// blacklisted).
class PreviewsBlacklistDelegate {
 public:
  PreviewsBlacklistDelegate() {}
  virtual ~PreviewsBlacklistDelegate() {}

  // Notifies |this| that |host| has been blacklisted at |time|. This method is
  // guaranteed to be called when a previously whitelisted host is now
  // blacklisted.
  virtual void OnNewBlacklistedHost(const std::string& host,
                                    base::Time time) = 0;

  // Notifies |this| that the user blacklisted has changed, and it is
  // guaranteed to be called when the user blacklisted status is changed.
  virtual void OnUserBlacklistedStatusChange(bool blacklisted) = 0;

  // Notifies |this| that the blacklist is cleared at |time|.
  virtual void OnBlacklistCleared(base::Time time) = 0;
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_BLACK_LIST_DELEGATE_H_
