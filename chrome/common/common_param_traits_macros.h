// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Singly or multiply-included shared traits file depending upon circumstances.
// This allows the use of IPC serialization macros in more than one IPC message
// file.
#ifndef CHROME_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#define CHROME_COMMON_COMMON_PARAM_TRAITS_MACROS_H_

#include "components/content_settings/core/common/content_settings_types.h"
#include "ipc/ipc_message_macros.h"

IPC_ENUM_TRAITS_MAX_VALUE(ContentSettingsType, CONTENT_SETTINGS_NUM_TYPES - 1)

#endif  // CHROME_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
