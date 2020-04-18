// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_PUBLIC_CPP_FEATURES_H_
#define UI_MESSAGE_CENTER_PUBLIC_CPP_FEATURES_H_

#include "base/feature_list.h"
#include "ui/message_center/public/cpp/message_center_public_export.h"

namespace message_center {

// This feature controls whether the new (material design) style notifications
// should be used.
MESSAGE_CENTER_PUBLIC_EXPORT extern const base::Feature kNewStyleNotifications;

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_PUBLIC_CPP_FEATURES_H_
