// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CUSTOM_HANDLERS_REGISTER_PROTOCOL_HANDLER_PERMISSION_REQUEST_H_
#define CHROME_BROWSER_CUSTOM_HANDLERS_REGISTER_PROTOCOL_HANDLER_PERMISSION_REQUEST_H_

#include "base/macros.h"
#include "chrome/browser/permissions/permission_request.h"
#include "chrome/common/custom_handlers/protocol_handler.h"

class ProtocolHandlerRegistry;

// This class provides display data for a permission request, shown when a page
// wants to register a protocol handler and was triggered by a user action.
class RegisterProtocolHandlerPermissionRequest : public PermissionRequest {
 public:
  RegisterProtocolHandlerPermissionRequest(
      ProtocolHandlerRegistry* registry,
      const ProtocolHandler& handler,
      GURL url,
      bool user_gesture);
  ~RegisterProtocolHandlerPermissionRequest() override;

 private:
  // PermissionRequest:
  IconId GetIconId() const override;
  base::string16 GetMessageTextFragment() const override;
  GURL GetOrigin() const override;
  void PermissionGranted() override;
  void PermissionDenied() override;
  void Cancelled() override;
  void RequestFinished() override;
  PermissionRequestType GetPermissionRequestType() const override;

  ProtocolHandlerRegistry* registry_;
  ProtocolHandler handler_;
  GURL origin_;

  DISALLOW_COPY_AND_ASSIGN(RegisterProtocolHandlerPermissionRequest);
};

#endif  // CHROME_BROWSER_CUSTOM_HANDLERS_REGISTER_PROTOCOL_HANDLER_PERMISSION_REQUEST_H_
