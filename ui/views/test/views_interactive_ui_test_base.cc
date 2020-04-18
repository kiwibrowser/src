// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/views_interactive_ui_test_base.h"

#include "base/path_service.h"
#include "mojo/edk/embedder/embedder.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace views {

ViewsInteractiveUITestBase::ViewsInteractiveUITestBase() {}

ViewsInteractiveUITestBase::~ViewsInteractiveUITestBase() {}

// static
void ViewsInteractiveUITestBase::InteractiveSetUp() {
  // Mojo is initialized here similar to how each browser test case initializes
  // Mojo when starting. This only works because each interactive_ui_test runs
  // in a new process.
  mojo::edk::Init();

  gl::GLSurfaceTestSupport::InitializeOneOff();
  ui::RegisterPathProvider();
  base::FilePath ui_test_pak_path;
  ASSERT_TRUE(base::PathService::Get(ui::UI_TEST_PAK, &ui_test_pak_path));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(ui_test_pak_path);
}

void ViewsInteractiveUITestBase::SetUp() {
  InteractiveSetUp();
  ViewsTestBase::SetUp();
}

}  // namespace views
