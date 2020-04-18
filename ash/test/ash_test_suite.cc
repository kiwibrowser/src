// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/ash_test_suite.h"

#include "ash/public/cpp/config.h"
#include "ash/test/ash_test_environment.h"
#include "ash/test/ash_test_helper.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/i18n/rtl.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/aura_test_context_factory.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_paths.h"
#include "ui/base/ui_base_switches.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/gfx/gfx_paths.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace ash {

AshTestSuite::AshTestSuite(int argc, char** argv) : TestSuite(argc, argv) {}

AshTestSuite::~AshTestSuite() = default;

void AshTestSuite::Initialize() {
  base::TestSuite::Initialize();
  gl::GLSurfaceTestSupport::InitializeOneOff();

  gfx::RegisterPathProvider();
  ui::RegisterPathProvider();

  // Force unittests to run using en-US so if we test against string output,
  // it'll pass regardless of the system language.
  base::i18n::SetICUDefaultLocale("en_US");

  // Load ash test resources and en-US strings; not 'common' (Chrome) resources.
  base::FilePath path;
  base::PathService::Get(base::DIR_MODULE, &path);
  base::FilePath ash_test_strings =
      path.Append(FILE_PATH_LITERAL("ash_test_strings.pak"));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(ash_test_strings);

  if (ui::ResourceBundle::IsScaleFactorSupported(ui::SCALE_FACTOR_100P)) {
    base::FilePath ash_test_resources_100 =
        path.AppendASCII(AshTestEnvironment::Get100PercentResourceFileName());
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        ash_test_resources_100, ui::SCALE_FACTOR_100P);
  }
  if (ui::ResourceBundle::IsScaleFactorSupported(ui::SCALE_FACTOR_200P)) {
    base::FilePath ash_test_resources_200 =
        path.Append(FILE_PATH_LITERAL("ash_test_resources_200_percent.pak"));
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        ash_test_resources_200, ui::SCALE_FACTOR_200P);
  }

  const bool is_mash = base::FeatureList::IsEnabled(features::kMash);
  AshTestHelper::config_ = is_mash ? Config::MASH : Config::CLASSIC;

  base::DiscardableMemoryAllocator::SetInstance(&discardable_memory_allocator_);
  env_ = aura::Env::CreateInstance(is_mash ? aura::Env::Mode::MUS
                                           : aura::Env::Mode::LOCAL);

  if (is_mash) {
    context_factory_ = std::make_unique<aura::test::AuraTestContextFactory>();
    env_->set_context_factory(context_factory_.get());
    env_->set_context_factory_private(nullptr);
  }
}

void AshTestSuite::Shutdown() {
  env_.reset();
  ui::ResourceBundle::CleanupSharedInstance();
  base::TestSuite::Shutdown();
}

}  // namespace ash
