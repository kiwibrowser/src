// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PROVIDERS_CHROMIUM_VOICE_SEARCH_PROVIDER_H_
#define IOS_CHROME_BROWSER_PROVIDERS_CHROMIUM_VOICE_SEARCH_PROVIDER_H_

#include "ios/public/provider/chrome/browser/voice/voice_search_provider.h"

class ChromiumVoiceSearchProvider : public VoiceSearchProvider {
 public:
  ChromiumVoiceSearchProvider();
  ~ChromiumVoiceSearchProvider() override;

  // VoiceSearchProvider.
  bool IsVoiceSearchEnabled() const override;
  NSArray* GetAvailableLanguages() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromiumVoiceSearchProvider);
};

#endif  // IOS_CHROME_BROWSER_PROVIDERS_CHROMIUM_VOICE_SEARCH_PROVIDER_H_
