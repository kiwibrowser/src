// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/notification_database_data.h"

namespace content {

NotificationDatabaseData::NotificationDatabaseData() = default;

NotificationDatabaseData::NotificationDatabaseData(
    const NotificationDatabaseData& other) = default;

NotificationDatabaseData& NotificationDatabaseData::operator=(
    const NotificationDatabaseData& other) = default;

NotificationDatabaseData::~NotificationDatabaseData() = default;

}  // namespace content
