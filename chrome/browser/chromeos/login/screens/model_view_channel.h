// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MODEL_VIEW_CHANNEL_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MODEL_VIEW_CHANNEL_H_

namespace base {
class DictionaryValue;
}

namespace chromeos {

// Communication channel between model and view. This is a temprorary approach,
// finally ScreenManager class should be responsible for that.
class ModelViewChannel {
 public:
  virtual ~ModelViewChannel() {}

  virtual void CommitContextChanges(const base::DictionaryValue& diff) = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_MODEL_VIEW_CHANNEL_H_
