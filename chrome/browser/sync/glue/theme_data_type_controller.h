// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_THEME_DATA_TYPE_CONTROLLER_H_
#define CHROME_BROWSER_SYNC_GLUE_THEME_DATA_TYPE_CONTROLLER_H_

#include "base/macros.h"
#include "components/sync/driver/async_directory_type_controller.h"

class Profile;

namespace browser_sync {

class ThemeDataTypeController : public syncer::AsyncDirectoryTypeController {
 public:
  // |dump_stack| is called when an unrecoverable error occurs.
  ThemeDataTypeController(const base::Closure& dump_stack,
                          syncer::SyncClient* sync_client,
                          Profile* profile);
  ~ThemeDataTypeController() override;

 private:
  // AsyncDirectoryTypeController implementation.
  bool StartModels() override;

  Profile* const profile_;
  DISALLOW_COPY_AND_ASSIGN(ThemeDataTypeController);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_THEME_DATA_TYPE_CONTROLLER_H_
