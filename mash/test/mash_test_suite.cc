// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/test/mash_test_suite.h"

#include <memory>

#include "ash/public/cpp/config.h"
#include "ash/test/ash_test_helper.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/surfaces/frame_sink_id_allocator.h"
#include "components/viz/service/surfaces/surface_manager.h"
#include "ui/aura/env.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/reflector.h"
#include "ui/compositor/test/fake_context_factory.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace mash {
namespace test {

MashTestSuite::MashTestSuite(int argc, char** argv) : TestSuite(argc, argv) {}

MashTestSuite::~MashTestSuite() = default;

void MashTestSuite::Initialize() {
  base::TestSuite::Initialize();
  gl::GLSurfaceTestSupport::InitializeOneOff();

  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kOverrideUseSoftwareGLForTests);
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kEnableFeatures, features::kMash.name);
  feature_list_.InitAndEnableFeature(features::kMash);

  // Load ash mus strings and resources; not 'common' (Chrome) resources.
  base::FilePath resources;
  base::PathService::Get(base::DIR_MODULE, &resources);
  resources = resources.Append(FILE_PATH_LITERAL("ash_service_resources.pak"));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(resources);

  ash::AshTestHelper::config_ = ash::Config::MASH;

  base::DiscardableMemoryAllocator::SetInstance(&discardable_memory_allocator_);
  env_ = aura::Env::CreateInstance(aura::Env::Mode::MUS);

  context_factory_ = std::make_unique<ui::FakeContextFactory>();
  env_->set_context_factory(context_factory_.get());
  env_->set_context_factory_private(nullptr);
}

void MashTestSuite::Shutdown() {
  env_.reset();
  ui::ResourceBundle::CleanupSharedInstance();
  base::TestSuite::Shutdown();
}

}  // namespace test
}  // namespace mash
