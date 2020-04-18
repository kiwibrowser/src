// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "media/base/overlay_info.h"
#include "media/base/renderer_factory_selector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class RendererFactorySelectorTest : public testing::Test {
 public:
  typedef RendererFactorySelector::FactoryType FactoryType;

  class FakeFactory : public RendererFactory {
   public:
    FakeFactory(FactoryType type) : type_(type){};

    std::unique_ptr<Renderer> CreateRenderer(
        const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
        const scoped_refptr<base::TaskRunner>& worker_task_runner,
        AudioRendererSink* audio_renderer_sink,
        VideoRendererSink* video_renderer_sink,
        const RequestOverlayInfoCB& request_overlay_info_cb,
        const gfx::ColorSpace& target_color_space) override {
      return std::unique_ptr<Renderer>();
    }

    FactoryType factory_type() { return type_; }

   private:
    FactoryType type_;
  };

  RendererFactorySelectorTest() = default;
  ;

  void AddFactory(FactoryType type) {
    selector_.AddFactory(type, std::make_unique<FakeFactory>(type));
  };

  FactoryType GetCurrentlySelectedFactoryType() {
    return reinterpret_cast<FakeFactory*>(selector_.GetCurrentFactory())
        ->factory_type();
  }

  bool is_remoting_active() { return is_remoting_active_; }

 protected:
  RendererFactorySelector selector_;

  bool is_remoting_active_ = false;

  DISALLOW_COPY_AND_ASSIGN(RendererFactorySelectorTest);
};

TEST_F(RendererFactorySelectorTest, SetBaseFactory_SingleFactory) {
  AddFactory(FactoryType::DEFAULT);

  selector_.SetBaseFactoryType(FactoryType::DEFAULT);

  EXPECT_EQ(FactoryType::DEFAULT, GetCurrentlySelectedFactoryType());
}

TEST_F(RendererFactorySelectorTest, SetBaseFactory_MultipleFactory) {
  AddFactory(FactoryType::DEFAULT);
  AddFactory(FactoryType::MOJO);

  selector_.SetBaseFactoryType(FactoryType::DEFAULT);
  EXPECT_EQ(FactoryType::DEFAULT, GetCurrentlySelectedFactoryType());

  selector_.SetBaseFactoryType(FactoryType::MOJO);
  EXPECT_EQ(FactoryType::MOJO, GetCurrentlySelectedFactoryType());
}

#if defined(OS_ANDROID)
TEST_F(RendererFactorySelectorTest, SetUseMediaPlayer) {
  AddFactory(FactoryType::DEFAULT);
  AddFactory(FactoryType::MEDIA_PLAYER);
  selector_.SetBaseFactoryType(FactoryType::DEFAULT);

  selector_.SetUseMediaPlayer(false);
  EXPECT_EQ(FactoryType::DEFAULT, GetCurrentlySelectedFactoryType());

  selector_.SetUseMediaPlayer(true);
  EXPECT_EQ(FactoryType::MEDIA_PLAYER, GetCurrentlySelectedFactoryType());

  selector_.SetUseMediaPlayer(false);
  EXPECT_EQ(FactoryType::DEFAULT, GetCurrentlySelectedFactoryType());
}
#endif

TEST_F(RendererFactorySelectorTest, SetQueryIsRemotingActiveCB) {
  AddFactory(FactoryType::DEFAULT);
  AddFactory(FactoryType::COURIER);
  selector_.SetBaseFactoryType(FactoryType::DEFAULT);
  selector_.SetQueryIsRemotingActiveCB(
      base::Bind(&RendererFactorySelectorTest::is_remoting_active,
                 base::Unretained(this)));

  EXPECT_EQ(FactoryType::DEFAULT, GetCurrentlySelectedFactoryType());

  is_remoting_active_ = true;
  EXPECT_EQ(FactoryType::COURIER, GetCurrentlySelectedFactoryType());

  is_remoting_active_ = false;
  EXPECT_EQ(FactoryType::DEFAULT, GetCurrentlySelectedFactoryType());
}

}  // namespace media
