// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/tts/arc_tts_service.h"

#include <memory>

#include "base/threading/platform_thread.h"
#include "chrome/browser/speech/tts_controller_impl.h"
#include "chrome/test/base/testing_profile.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/test/fake_arc_session.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

namespace {

class TestableTtsController : public TtsControllerImpl {
 public:
  TestableTtsController() = default;
  ~TestableTtsController() override = default;

  void OnTtsEvent(int utterance_id,
                  TtsEventType event_type,
                  int char_index,
                  const std::string& error_message) override {
    last_utterance_id_ = utterance_id;
    last_event_type_ = event_type;
    last_char_index_ = char_index;
    last_error_message_ = error_message;
  }

  int last_utterance_id_;
  TtsEventType last_event_type_;
  int last_char_index_;
  std::string last_error_message_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestableTtsController);
};

class ArcTtsServiceTest : public testing::Test {
 public:
  ArcTtsServiceTest()
      : arc_service_manager_(std::make_unique<ArcServiceManager>()),
        testing_profile_(std::make_unique<TestingProfile>()),
        tts_controller_(std::make_unique<TestableTtsController>()),
        tts_service_(ArcTtsService::GetForBrowserContextForTesting(
            testing_profile_.get())) {
    tts_service_->set_tts_controller_for_testing(tts_controller_.get());
  }

  ~ArcTtsServiceTest() override { tts_service_->Shutdown(); }

 protected:
  ArcTtsService* tts_service() const { return tts_service_; }
  TestableTtsController* tts_controller() const {
    return tts_controller_.get();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<TestingProfile> testing_profile_;
  std::unique_ptr<TestableTtsController> tts_controller_;
  ArcTtsService* const tts_service_;

  DISALLOW_COPY_AND_ASSIGN(ArcTtsServiceTest);
};

// Tests that ArcTtsService can be constructed and destructed.
TEST_F(ArcTtsServiceTest, TestConstructDestruct) {}

// Tests that OnTtsEvent() properly calls into TtsController::OnTtsEvent().
TEST_F(ArcTtsServiceTest, TestOnTtsEvent) {
  tts_service()->OnTtsEvent(1, mojom::TtsEventType::START, 0, "");
  EXPECT_EQ(1, tts_controller()->last_utterance_id_);
  EXPECT_EQ(TTS_EVENT_START, tts_controller()->last_event_type_);
  EXPECT_EQ(0, tts_controller()->last_char_index_);
  EXPECT_EQ("", tts_controller()->last_error_message_);

  tts_service()->OnTtsEvent(1, mojom::TtsEventType::END, 10, "");
  EXPECT_EQ(1, tts_controller()->last_utterance_id_);
  EXPECT_EQ(TTS_EVENT_END, tts_controller()->last_event_type_);
  EXPECT_EQ(10, tts_controller()->last_char_index_);
  EXPECT_EQ("", tts_controller()->last_error_message_);

  tts_service()->OnTtsEvent(2, mojom::TtsEventType::INTERRUPTED, 0, "");
  EXPECT_EQ(2, tts_controller()->last_utterance_id_);
  EXPECT_EQ(TTS_EVENT_INTERRUPTED, tts_controller()->last_event_type_);
  EXPECT_EQ(0, tts_controller()->last_char_index_);
  EXPECT_EQ("", tts_controller()->last_error_message_);

  tts_service()->OnTtsEvent(3, mojom::TtsEventType::ERROR, 0, "");
  EXPECT_EQ(3, tts_controller()->last_utterance_id_);
  EXPECT_EQ(TTS_EVENT_ERROR, tts_controller()->last_event_type_);
  EXPECT_EQ(0, tts_controller()->last_char_index_);
  EXPECT_EQ("", tts_controller()->last_error_message_);
}

}  // namespace

}  // namespace arc
