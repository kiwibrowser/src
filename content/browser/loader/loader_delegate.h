// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_LOADER_DELEGATE_H_
#define CONTENT_BROWSER_LOADER_LOADER_DELEGATE_H_

#include <inttypes.h>

#include <memory>
#include <string>

#include "content/common/content_export.h"
#include "content/public/browser/resource_request_info.h"
#include "net/base/load_states.h"

namespace content {

// Delegate from loader to the rest of content. Should be interacted with on the
// IO thread unless otherwise noted.
//
// This is used for breaking dependencies between content at-large and
// content/browser/loader which will eventually be moved to a separate
// networking service. All methods in this interface should be asynchronous,
// since eventually this will be a Mojo interface. See https://crbug.com/622050
// and https://crbug.com/598073.
class CONTENT_EXPORT LoaderDelegate {
 public:
  virtual ~LoaderDelegate() {}

  // Notification that the load state for the given WebContents has changed.
  // NOTE: this method is called on the UI thread.
  virtual void LoadStateChanged(WebContents* web_contents,
                                const std::string& host,
                                const net::LoadStateWithParam& load_state,
                                uint64_t upload_position,
                                uint64_t upload_size) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_LOADER_DELEGATE_H_
