// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/display/output_protection_controller_mus.h"

#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace chromeos {

OutputProtectionControllerMus::OutputProtectionControllerMus() {
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface("ui", &output_protection_);
}

OutputProtectionControllerMus::~OutputProtectionControllerMus() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void OutputProtectionControllerMus::QueryStatus(
    int64_t display_id,
    const OutputProtectionDelegate::QueryStatusCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  output_protection_->QueryContentProtectionStatus(display_id, callback);
}

void OutputProtectionControllerMus::SetProtection(
    int64_t display_id,
    uint32_t desired_method_mask,
    const OutputProtectionDelegate::SetProtectionCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  output_protection_->SetContentProtection(display_id, desired_method_mask,
                                           callback);
}

}  // namespace chromeos
