// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_SETTINGS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_SETTINGS_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/platform/fonts/generic_font_family_settings.h"

namespace blink {

class CORE_EXPORT WorkerSettings {
 public:
  explicit WorkerSettings(Settings*);
  static std::unique_ptr<WorkerSettings> Copy(WorkerSettings*);

  bool DisableReadingFromCanvas() const { return disable_reading_from_canvas_; }
  bool GetStrictMixedContentChecking() const {
    return strict_mixed_content_checking_;
  }
  bool GetAllowRunningOfInsecureContent() const {
    return allow_running_of_insecure_content_;
  }
  bool GetStrictlyBlockBlockableMixedContent() const {
    return strictly_block_blockable_mixed_content_;
  }

  const GenericFontFamilySettings& GetGenericFontFamilySettings() const {
    return generic_font_family_settings_;
  }

  void MakeGenericFontFamilySettingsAtomic() {
    generic_font_family_settings_.MakeAtomic();
  }

 private:
  void CopyFlagValuesFromSettings(Settings*);

  // The settings that are to be copied from main thread to worker thread
  // These setting's flag values must remain unchanged throughout the document
  // lifecycle.
  // We hard-code the flags as there're very few flags at this moment.
  bool disable_reading_from_canvas_ = false;
  bool strict_mixed_content_checking_ = false;
  bool allow_running_of_insecure_content_ = false;
  bool strictly_block_blockable_mixed_content_ = false;

  GenericFontFamilySettings generic_font_family_settings_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_SETTINGS_H_
