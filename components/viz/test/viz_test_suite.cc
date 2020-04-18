// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/test/viz_test_suite.h"

#include "base/message_loop/message_loop.h"
#include "base/threading/thread_id_name_manager.h"
#include "components/viz/test/paths.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace viz {

VizTestSuite::VizTestSuite(int argc, char** argv)
    : base::TestSuite(argc, argv) {}

VizTestSuite::~VizTestSuite() = default;

void VizTestSuite::Initialize() {
  base::TestSuite::Initialize();
  gl::GLSurfaceTestSupport::InitializeOneOff();
  Paths::RegisterPathProvider();

  message_loop_ = std::make_unique<base::MessageLoop>();

  base::ThreadIdNameManager::GetInstance()->SetName("Main");

  base::DiscardableMemoryAllocator::SetInstance(&discardable_memory_allocator_);
}

void VizTestSuite::Shutdown() {
  message_loop_ = nullptr;

  base::TestSuite::Shutdown();
}

}  // namespace viz
