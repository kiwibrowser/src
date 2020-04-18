// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_DATA_TYPE_CONTROLLER_H__
#define COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_DATA_TYPE_CONTROLLER_H__

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/search_engines/template_url_service.h"
#include "components/sync/driver/async_directory_type_controller.h"

namespace browser_sync {

// Controller for the SEARCH_ENGINES sync data type. This class tells sync
// how to load the model for this data type, and the superclasses manage
// controlling the rest of the state of the datatype with regards to sync.
class SearchEngineDataTypeController
    : public syncer::AsyncDirectoryTypeController {
 public:
  // |dump_stack| is called when an unrecoverable error occurs.
  SearchEngineDataTypeController(const base::Closure& dump_stack,
                                 syncer::SyncClient* sync_client,
                                 TemplateURLService* template_url_service);
  ~SearchEngineDataTypeController() override;

  TemplateURLService::Subscription* GetSubscriptionForTesting();

 private:
  // AsyncDirectoryTypeController:
  bool StartModels() override;
  void StopModels() override;

  void OnTemplateURLServiceLoaded();

  // A pointer to the template URL service that this data type will use.
  TemplateURLService* template_url_service_;

  // A subscription to the OnLoadedCallback so it can be cleared if necessary.
  std::unique_ptr<TemplateURLService::Subscription> template_url_subscription_;

  DISALLOW_COPY_AND_ASSIGN(SearchEngineDataTypeController);
};

}  // namespace browser_sync

#endif  // COMPONENTS_SEARCH_ENGINES_SEARCH_ENGINE_DATA_TYPE_CONTROLLER_H__
