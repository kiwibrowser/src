// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REQUEST_MANAGER_H_
#define CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REQUEST_MANAGER_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"

namespace content {

class BackgroundFetchRegistrationId;
class BackgroundFetchRequestInfo;

// Interface for manager requests that are part of a Background Fetch.
// Implementations maintain a queue of requests for each given
// |BackgroundFetchRegistrationId| that may be backed by a database.
class BackgroundFetchRequestManager {
 public:
  virtual ~BackgroundFetchRequestManager() {}

  // Marks that the |request|, part of the Background Fetch identified by
  // |registration_id|, has completed.
  virtual void MarkRequestAsComplete(
      const BackgroundFetchRegistrationId& registration_id,
      scoped_refptr<BackgroundFetchRequestInfo> request) = 0;

  // Called when the job identified by |registration_id| has been aborted along
  // with the GUIDs of any associated downloads that were still active.
  virtual void OnJobAborted(
      const BackgroundFetchRegistrationId& registration_id,
      std::vector<std::string> aborted_guids) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_REQUEST_MANAGER_H_
