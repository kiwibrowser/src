// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_EXTENSION_DATA_TYPE_CONTROLLER_H_
#define CHROME_BROWSER_SYNC_GLUE_EXTENSION_DATA_TYPE_CONTROLLER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "components/sync/driver/async_directory_type_controller.h"
#include "components/sync/driver/generic_change_processor.h"

class Profile;

namespace browser_sync {

// TODO(zea): Rename this and ExtensionSettingsDTC to ExtensionOrApp*, since
// both actually handle the APP datatypes as well.
class ExtensionDataTypeController
    : public syncer::AsyncDirectoryTypeController {
 public:
  // |dump_stack| is called when an unrecoverable error occurs.
  ExtensionDataTypeController(
      syncer::ModelType type,  // Either EXTENSIONS or APPS.
      const base::Closure& dump_stack,
      syncer::SyncClient* sync_client,
      Profile* profile);
  ~ExtensionDataTypeController() override;

 private:
  // AsyncDirectoryTypeController implementation.
  bool StartModels() override;

  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionDataTypeController);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_EXTENSION_DATA_TYPE_CONTROLLER_H_
