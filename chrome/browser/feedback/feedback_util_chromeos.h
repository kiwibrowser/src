// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_
#define CHROME_BROWSER_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_

#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"

class Profile;

namespace feedback_util {

// Sends a system log feedback from the given |profile| with the
// given |description|. |callback| will be invoked when the feedback is sent.
using SendSysLogFeedbackCallback = base::Callback<void(bool)>;
void SendSysLogFeedback(Profile* profile,
                        const std::string& description,
                        const SendSysLogFeedbackCallback& callback);

}  // namespace feedback_util

#endif  // CHROME_BROWSER_FEEDBACK_FEEDBACK_UTIL_CHROMEOS_H_
