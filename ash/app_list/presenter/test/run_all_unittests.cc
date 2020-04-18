// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "mojo/edk/embedder/embedder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace {

class AppListPresenterTestSuite : public base::TestSuite {
 public:
  AppListPresenterTestSuite(int argc, char** argv)
      : base::TestSuite(argc, argv) {}

 protected:
  void Initialize() override {
    gl::GLSurfaceTestSupport::InitializeOneOff();
    base::TestSuite::Initialize();
    ui::RegisterPathProvider();

    base::FilePath ui_test_pak_path;
    ASSERT_TRUE(base::PathService::Get(ui::UI_TEST_PAK, &ui_test_pak_path));
    ui::ResourceBundle::InitSharedInstanceWithPakPath(ui_test_pak_path);
  }

  void Shutdown() override {
    ui::ResourceBundle::CleanupSharedInstance();
    base::TestSuite::Shutdown();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AppListPresenterTestSuite);
};

}  // namespace

int main(int argc, char** argv) {
  AppListPresenterTestSuite test_suite(argc, argv);

  mojo::edk::Init();

  return base::LaunchUnitTests(argc, argv,
                               base::Bind(&AppListPresenterTestSuite::Run,
                                          base::Unretained(&test_suite)));
}
