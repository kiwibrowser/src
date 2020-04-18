// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/media_router/media_router_views_ui.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/browser/media/router/test/mock_media_router.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/media_router/cast_dialog_controller.h"
#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using testing::_;
using testing::Invoke;
using testing::WithArg;

namespace media_router {

namespace {

constexpr char kRouteId[] = "route1";
constexpr char kSinkId[] = "sink1";
constexpr char kSinkName[] = "sink name";
constexpr char kSourceId[] = "source1";

}  // namespace

class MockControllerObserver : public CastDialogController::Observer {
 public:
  MOCK_METHOD1(OnModelUpdated, void(const CastDialogModel& model));
  MOCK_METHOD0(OnControllerInvalidated, void());
};

// Injects a MediaRouter instance into MediaRouterViewsUI.
class TestMediaRouterViewsUI : public MediaRouterViewsUI {
 public:
  explicit TestMediaRouterViewsUI(MediaRouter* router) : router_(router) {}
  ~TestMediaRouterViewsUI() override = default;

  MediaRouter* GetMediaRouter() const override { return router_; }

 private:
  MediaRouter* router_;
  DISALLOW_COPY_AND_ASSIGN(TestMediaRouterViewsUI);
};

class MediaRouterViewsUITest : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    // Store sink observers so that they can be notified in tests.
    ON_CALL(mock_router_, RegisterMediaSinksObserver(_))
        .WillByDefault(Invoke([this](MediaSinksObserver* observer) {
          media_sinks_observers_.push_back(observer);
          return true;
        }));

    SessionTabHelper::CreateForWebContents(web_contents());
    ui_ = std::make_unique<TestMediaRouterViewsUI>(&mock_router_);
    ui_->InitWithDefaultMediaSource(web_contents(), nullptr);
  }

  void TearDown() override {
    ui_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  std::vector<MediaSinksObserver*> media_sinks_observers_;
  MockMediaRouter mock_router_;
  std::unique_ptr<MediaRouterViewsUI> ui_;
};

TEST_F(MediaRouterViewsUITest, NotifyObserver) {
  MockControllerObserver observer;

  EXPECT_CALL(observer, OnModelUpdated(_))
      .WillOnce(WithArg<0>(Invoke([](const CastDialogModel& model) {
        EXPECT_TRUE(model.media_sinks.empty());
      })));
  ui_->AddObserver(&observer);

  MediaSink sink(kSinkId, kSinkName, SinkIconType::CAST_AUDIO);
  MediaSinkWithCastModes sink_with_cast_modes(sink);
  sink_with_cast_modes.cast_modes = {MediaCastMode::TAB_MIRROR};
  EXPECT_CALL(observer, OnModelUpdated(_))
      .WillOnce(WithArg<0>(Invoke([&sink](const CastDialogModel& model) {
        EXPECT_EQ(1u, model.media_sinks.size());
        const UIMediaSink& ui_sink = model.media_sinks[0];
        EXPECT_EQ(sink.id(), ui_sink.id);
        EXPECT_EQ(base::UTF8ToUTF16(sink.name()), ui_sink.friendly_name);
        EXPECT_EQ(UIMediaSinkState::AVAILABLE, ui_sink.state);
        EXPECT_EQ(static_cast<int>(UICastAction::CAST_TAB),
                  ui_sink.allowed_actions);
        EXPECT_EQ(sink.icon_type(), ui_sink.icon_type);
      })));
  ui_->OnResultsUpdated({sink_with_cast_modes});

  MediaRoute route(kRouteId, MediaSource(kSourceId), kSinkId, "", true, true);
  EXPECT_CALL(observer, OnModelUpdated(_))
      .WillOnce(
          WithArg<0>(Invoke([&sink, &route](const CastDialogModel& model) {
            EXPECT_EQ(1u, model.media_sinks.size());
            const UIMediaSink& ui_sink = model.media_sinks[0];
            EXPECT_EQ(sink.id(), ui_sink.id);
            EXPECT_EQ(UIMediaSinkState::CONNECTED, ui_sink.state);
            EXPECT_EQ(static_cast<int>(UICastAction::STOP),
                      ui_sink.allowed_actions);
            EXPECT_EQ(route.media_route_id(), ui_sink.route_id);
          })));
  ui_->OnRoutesUpdated({route}, {});

  EXPECT_CALL(observer, OnControllerInvalidated());
  ui_.reset();
}

TEST_F(MediaRouterViewsUITest, StartCasting) {
  MediaSource media_source =
      MediaSourceForTab(SessionTabHelper::IdForTab(web_contents()).id());
  EXPECT_CALL(mock_router_,
              CreateRouteInternal(media_source.id(), kSinkId, _, web_contents(),
                                  _, base::TimeDelta::FromSeconds(60), false));
  MediaSink sink(kSinkId, kSinkName, SinkIconType::GENERIC);
  for (MediaSinksObserver* observer : media_sinks_observers_)
    observer->OnSinksUpdated({sink}, std::vector<url::Origin>());
  ui_->StartCasting(kSinkId, MediaCastMode::TAB_MIRROR);
}

TEST_F(MediaRouterViewsUITest, StopCasting) {
  EXPECT_CALL(mock_router_, TerminateRoute(kRouteId));
  ui_->StopCasting(kRouteId);
}

}  // namespace media_router
