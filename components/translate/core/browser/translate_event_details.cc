// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_event_details.h"

namespace translate {

TranslateEventDetails::TranslateEventDetails(const std::string& in_filename,
                                             int in_line,
                                             const std::string& in_message)
    : filename(in_filename),
      line(in_line),
      message(in_message) {
  time = base::Time::Now();
}

}  // namespace translate
