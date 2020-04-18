// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>

#include <memory>

#include "base/bind.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define SHADER(Src) #Src

namespace gpu {

class GLFenceSyncTest : public testing::Test {
 protected:
  void SetUp() override {
    sync_point_manager_.reset(new SyncPointManager());

    GLManager::Options options;
    options.sync_point_manager = sync_point_manager_.get();
    gl1_.Initialize(options);
    gl2_.Initialize(options);
  }

  void TearDown() override {
    gl2_.Destroy();
    gl1_.Destroy();

    sync_point_manager_.reset();
  }

  std::unique_ptr<SyncPointManager> sync_point_manager_;
  GLManager gl1_;
  GLManager gl2_;
};

TEST_F(GLFenceSyncTest, SimpleReleaseWait) {
  gl1_.MakeCurrent();

  SyncToken sync_token;
  glFlush();
  glGenSyncTokenCHROMIUM(sync_token.GetData());
  ASSERT_TRUE(GL_NO_ERROR == glGetError());

  // Make sure it is actually released.
  EXPECT_TRUE(sync_point_manager_->IsSyncTokenReleased(sync_token));

  gl2_.MakeCurrent();
  glWaitSyncTokenCHROMIUM(sync_token.GetConstData());
  glFinish();
}

static void TestCallback(int* storage, int assign) {
  *storage = assign;
}

TEST_F(GLFenceSyncTest, SimpleReleaseSignal) {
  gl1_.MakeCurrent();

  // Pause the command buffer so the fence sync does not immediately trigger.
  gl1_.SetCommandsPaused(true);

  SyncToken sync_token;
  glGenUnverifiedSyncTokenCHROMIUM(sync_token.GetData());
  glFlush();
  ASSERT_TRUE(sync_token.HasData());

  gl2_.MakeCurrent();
  int callback_called = 0;
  gl2_.SignalSyncToken(sync_token,
                       base::Bind(TestCallback, &callback_called, 1));

  gl1_.MakeCurrent();
  EXPECT_EQ(0, callback_called);

  gl1_.SetCommandsPaused(false);
  glFinish();

  EXPECT_EQ(1, callback_called);
}

}  // namespace gpu
