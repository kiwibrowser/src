// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/time.h"

#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"

namespace syncer {

int64_t TimeToProtoTime(const base::Time& t) {
  return (t - base::Time::UnixEpoch()).InMilliseconds();
}

base::Time ProtoTimeToTime(int64_t proto_t) {
  return base::Time::UnixEpoch() + base::TimeDelta::FromMilliseconds(proto_t);
}

std::string GetTimeDebugString(const base::Time& t) {
  return base::UTF16ToUTF8(base::TimeFormatFriendlyDateAndTime(t));
}

}  // namespace syncer
