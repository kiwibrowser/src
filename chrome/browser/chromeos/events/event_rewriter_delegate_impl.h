// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EVENTS_EVENT_REWRITER_DELEGATE_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_EVENTS_EVENT_REWRITER_DELEGATE_IMPL_H_

#include "base/macros.h"
#include "ui/chromeos/events/event_rewriter_chromeos.h"

class PrefService;

namespace chromeos {

class EventRewriterDelegateImpl : public ui::EventRewriterChromeOS::Delegate {
 public:
  EventRewriterDelegateImpl();
  ~EventRewriterDelegateImpl() override;

  void set_pref_service_for_testing(const PrefService* pref_service) {
    pref_service_for_testing_ = pref_service;
  }

  // ui::EventRewriterChromeOS::Delegate:
  bool RewriteModifierKeys() override;
  bool GetKeyboardRemappedPrefValue(const std::string& pref_name,
                                    int* result) const override;
  bool TopRowKeysAreFunctionKeys() const override;
  bool IsExtensionCommandRegistered(ui::KeyboardCode key_code,
                                    int flags) const override;

 private:
  const PrefService* GetPrefService() const;

  const PrefService* pref_service_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(EventRewriterDelegateImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_EVENTS_EVENT_REWRITER_DELEGATE_IMPL_H_
