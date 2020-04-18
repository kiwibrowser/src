// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREVIEWS_PREVIEWS_SERVICE_H_
#define CHROME_BROWSER_PREVIEWS_PREVIEWS_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}

namespace optimization_guide {
class OptimizationGuideService;
}

namespace previews {
class PreviewsIOData;
class PreviewsUIService;
}

// Keyed service that owns a previews::PreviewsUIService. PreviewsService lives
// on the UI thread.
class PreviewsService : public KeyedService {
 public:
  PreviewsService();
  ~PreviewsService() override;

  // Initializes the UI Service. |previews_io_data| is the main previews IO
  // object, and cannot be null. |optimization_guide_service| is the
  // Optimization Guide Service that is being listened to and is guaranteed to
  // outlive |this|. |io_task_runner| is the IO thread task runner.
  // |profile_path| is the path to user data on disc.
  void Initialize(
      previews::PreviewsIOData* previews_io_data,
      optimization_guide::OptimizationGuideService* optimization_guide_service,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      const base::FilePath& profile_path);

  previews::PreviewsUIService* previews_ui_service() {
    return previews_ui_service_.get();
  }

 private:
  // The previews UI thread service.
  std::unique_ptr<previews::PreviewsUIService> previews_ui_service_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsService);
};

#endif  // CHROME_BROWSER_PREVIEWS_PREVIEWS_SERVICE_H_
