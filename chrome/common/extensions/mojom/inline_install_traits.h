// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, hence no include guard.

#include "chrome/common/extensions/api/webstore/webstore_api_constants.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "ipc/ipc_message_macros.h"

IPC_ENUM_TRAITS_MAX_VALUE(
    extensions::api::webstore::InstallStage,
    extensions::api::webstore::InstallStage::INSTALL_STAGE_INSTALLING)
IPC_ENUM_TRAITS_MAX_VALUE(extensions::webstore_install::Result,
                          extensions::webstore_install::RESULT_LAST)

