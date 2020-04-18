// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/conflicts/third_party_metrics_recorder_win.h"

#include <vector>

#include "base/bind.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "chrome/browser/conflicts/module_info_win.h"

namespace {

// Returns true if the module is signed by Google.
bool IsGoogleModule(const ModuleInfoData& module_data) {
  static const wchar_t kGoogle[] = L"Google Inc";
  return module_data.inspection_result->certificate_info.subject == kGoogle;
}

// Returns true if the signer name begins with "Microsoft ". Signatures are
// typically "Microsoft Corporation" or "Microsoft Windows", but others may
// exist.
bool IsMicrosoftModule(const ModuleInfoData& module_data) {
  static const wchar_t kMicrosoft[] = L"Microsoft ";
  return base::StartsWith(
      module_data.inspection_result->certificate_info.subject, kMicrosoft,
      base::CompareCase::SENSITIVE);
}

}  // namespace

ThirdPartyMetricsRecorder::ThirdPartyMetricsRecorder() = default;

ThirdPartyMetricsRecorder::~ThirdPartyMetricsRecorder() = default;

void ThirdPartyMetricsRecorder::OnNewModuleFound(
    const ModuleInfoKey& module_key,
    const ModuleInfoData& module_data) {
  const CertificateInfo& certificate_info =
      module_data.inspection_result->certificate_info;
  module_count_++;
  if (certificate_info.type != CertificateType::NO_CERTIFICATE) {
    ++signed_module_count_;

    if (certificate_info.type == CertificateType::CERTIFICATE_IN_CATALOG)
      ++catalog_module_count_;

    if (IsMicrosoftModule(module_data)) {
      ++microsoft_module_count_;
    } else if (IsGoogleModule(module_data)) {
      // No need to count these explicitly.
    } else {
      // Count modules that are neither signed by Google nor Microsoft.
      // These are considered "third party" modules.
      if (module_data.module_types & ModuleInfoData::kTypeLoadedModule) {
        ++loaded_third_party_module_count_;
      } else {
        ++not_loaded_third_party_module_count_;
      }
    }
  }
}

void ThirdPartyMetricsRecorder::OnModuleDatabaseIdle() {
  if (metrics_emitted_)
    return;
  metrics_emitted_ = true;

  // Report back some metrics regarding third party modules and certificates.
  base::UmaHistogramCustomCounts("ThirdPartyModules.Modules.Loaded",
                                 loaded_third_party_module_count_, 1, 500, 50);
  base::UmaHistogramCustomCounts("ThirdPartyModules.Modules.NotLoaded",
                                 not_loaded_third_party_module_count_, 1, 500,
                                 50);
  base::UmaHistogramCustomCounts("ThirdPartyModules.Modules.Signed",
                                 signed_module_count_, 1, 500, 50);
  base::UmaHistogramCustomCounts("ThirdPartyModules.Modules.Signed.Microsoft",
                                 microsoft_module_count_, 1, 500, 50);
  base::UmaHistogramCustomCounts("ThirdPartyModules.Modules.Signed.Catalog",
                                 catalog_module_count_, 1, 500, 50);
  base::UmaHistogramCustomCounts("ThirdPartyModules.Modules.Total",
                                 module_count_, 1, 500, 50);
}
