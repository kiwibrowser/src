// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_PERMISSION_REQUEST_ID_H_
#define CHROME_BROWSER_PERMISSIONS_PERMISSION_REQUEST_ID_H_

#include <string>

#include "url/gurl.h"

namespace content {
class RenderFrameHost;
}  // namespace content

// Uniquely identifies a particular permission request.
// None of the different attributes (render_process_id, render_frame_id or
// request_id) is enough to compare two requests. In order to check if
// a request is the same as another one, consumers of this class should use
// the operator== or operator!=.
class PermissionRequestID {
 public:
  PermissionRequestID(content::RenderFrameHost* render_frame_host,
                      int request_id);
  PermissionRequestID(int render_process_id,
                      int render_frame_id,
                      int request_id);
  ~PermissionRequestID();

  PermissionRequestID(const PermissionRequestID&) = default;
  PermissionRequestID& operator=(const PermissionRequestID&) = default;

  int render_process_id() const { return render_process_id_; }
  int render_frame_id() const { return render_frame_id_; }
  int request_id() const { return request_id_; }

  bool operator==(const PermissionRequestID& other) const;
  bool operator!=(const PermissionRequestID& other) const;

  std::string ToString() const;

 private:
  int render_process_id_;
  int render_frame_id_;
  int request_id_;
};

#endif  // CHROME_BROWSER_PERMISSIONS_PERMISSION_REQUEST_ID_H_
