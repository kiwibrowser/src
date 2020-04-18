// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_MODEL_VIEW_CHANNEL_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_MODEL_VIEW_CHANNEL_H_

#include "base/values.h"
#include "chrome/browser/chromeos/login/screens/model_view_channel.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

class MockModelViewChannel : public ModelViewChannel {
 public:
  MockModelViewChannel();
  ~MockModelViewChannel() override;
  MOCK_METHOD1(CommitContextChanges, void(const base::DictionaryValue& diff));
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MOCK_MODEL_VIEW_CHANNEL_H_
