// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_UI_APP_LIST_INTERNAL_APP_INTERNAL_APP_MODEL_BUILDER_H_
#define CHROME_BROWSER_UI_APP_LIST_INTERNAL_APP_INTERNAL_APP_MODEL_BUILDER_H_

#include "base/macros.h"
#include "chrome/browser/ui/app_list/app_list_model_builder.h"

class AppListControllerDelegate;

// This class populates and maintains internal apps.
class InternalAppModelBuilder : public AppListModelBuilder {
 public:
  explicit InternalAppModelBuilder(AppListControllerDelegate* controller);

  ~InternalAppModelBuilder() override = default;

 private:
  // AppListModelBuilder:
  void BuildModel() override;

  DISALLOW_COPY_AND_ASSIGN(InternalAppModelBuilder);
};

#endif  // CHROME_BROWSER_UI_APP_LIST_INTERNAL_APP_INTERNAL_APP_MODEL_BUILDER_H_
