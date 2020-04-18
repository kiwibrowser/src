// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_MUS_AURA_INIT_H_
#define UI_VIEWS_MUS_AURA_INIT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "build/build_config.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/aura/env.h"
#include "ui/views/mus/mus_export.h"

namespace aura {
class Env;
}

namespace base {
class SingleThreadTaskRunner;
}

namespace font_service {
class FontLoader;
}

namespace service_manager {
class Connector;
class Identity;
}

namespace views {
class MusClient;
class ViewsDelegate;

// Sets up necessary state for aura when run with the viewmanager.
// |resource_file| is the path to the apk file containing the resources.
class VIEWS_MUS_EXPORT AuraInit {
 public:
  // TODO(sky): remove Mode. https://crbug.com/842365.
  enum class Mode {
    // Indicates AuraInit should target using aura with mus. This is deprecated.
    AURA_MUS,

    // Indicates AuraInit should target using aura with mus, for a Window
    // Manager client. This is deprecated.
    AURA_MUS_WINDOW_MANAGER,

    // Targets ws2. Mode will eventually be removed entirely and this will be
    // the default.
    AURA_MUS2,
  };

  ~AuraInit();

  // Returns an AuraInit if initialization can be completed successfully,
  // otherwise a nullptr is returned. If initialization fails then Aura is in an
  // unusable state, and calling services should shutdown.
  // |resource_file| is the file to load strings and 1x icons from.
  // |resource_file_200| can be an empty string, otherwise it is the file to
  // load 2x icons from.
  static std::unique_ptr<AuraInit> Create(
      service_manager::Connector* connector,
      const service_manager::Identity& identity,
      const std::string& resource_file,
      const std::string& resource_file_200 = std::string(),
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner = nullptr,
      Mode mode = Mode::AURA_MUS,
      bool register_path_provider = true);

  // Only valid if Mode::AURA_MUS was passed to constructor.
  MusClient* mus_client() { return mus_client_.get(); }

 private:
  AuraInit();

  // Returns true if AuraInit was able to successfully complete initialization.
  // If this returns false, then Aura is in an unusable state, and calling
  // services should shutdown.
  bool Init(service_manager::Connector* connector,
            const service_manager::Identity& identity,
            const std::string& resource_file,
            const std::string& resource_file_200,
            scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
            Mode mode,
            bool register_path_provider);

  bool InitializeResources(service_manager::Connector* connector,
                           const std::string& resource_file,
                           const std::string& resource_file_200,
                           bool register_path_provider);

#if defined(OS_LINUX)
  sk_sp<font_service::FontLoader> font_loader_;
#endif

  std::unique_ptr<aura::Env> env_;
  std::unique_ptr<MusClient> mus_client_;
  std::unique_ptr<ViewsDelegate> views_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AuraInit);
};

}  // namespace views

#endif  // UI_VIEWS_MUS_AURA_INIT_H_
