// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_BASE_SESSION_SERVICE_DELEGATE_H_
#define COMPONENTS_SESSIONS_CORE_BASE_SESSION_SERVICE_DELEGATE_H_

namespace sessions {

// The BaseSessionServiceDelegate decouples the BaseSessionService from
// chrome/content dependencies.
class BaseSessionServiceDelegate {
 public:
  BaseSessionServiceDelegate() {}

  // Returns true if save operations can be performed as a delayed task - which
  // is usually only used by unit tests.
  virtual bool ShouldUseDelayedSave() = 0;

  // Called when commands are about to be written to disc.
  virtual void OnWillSaveCommands() {}

 protected:
  virtual ~BaseSessionServiceDelegate() {}
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_CORE_BASE_SESSION_SERVICE_DELEGATE_H_
