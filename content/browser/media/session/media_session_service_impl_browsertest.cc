// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_service_impl.h"

#include "base/command_line.h"
#include "base/run_loop.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/browser/media/session/media_session_player_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_content_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class MockMediaSessionObserver : public MediaSessionObserver {
 public:
  explicit MockMediaSessionObserver(
      MediaSession* session,
      const base::Closure& closure_on_actions_change)
      : MediaSessionObserver(session),
        closure_on_actions_change_(closure_on_actions_change) {}

  void MediaSessionActionsChanged(
      const std::set<blink::mojom::MediaSessionAction>& actions) override {
    // The actions might be empty when the service becomes routed for the first
    // time.
    if (actions.size() == 1)
      closure_on_actions_change_.Run();
  }

 private:
  base::Closure closure_on_actions_change_;
};

class MockWebContentsObserver : public WebContentsObserver {
 public:
  explicit MockWebContentsObserver(WebContents* contents,
                                   const base::Closure& closure_on_navigate)
      : WebContentsObserver(contents),
        closure_on_navigate_(closure_on_navigate) {}

  void DidFinishNavigation(NavigationHandle* navigation_handle) override {
    closure_on_navigate_.Run();
  }

 private:
  base::Closure closure_on_navigate_;
};

class MockMediaSessionPlayerObserver : public MediaSessionPlayerObserver {
 public:
  explicit MockMediaSessionPlayerObserver(RenderFrameHost* rfh)
      : render_frame_host_(rfh) {}

  ~MockMediaSessionPlayerObserver() override = default;

  void OnSuspend(int player_id) override {}
  void OnResume(int player_id) override {}
  void OnSeekForward(int player_id, base::TimeDelta seek_time) override {}
  void OnSeekBackward(int player_id, base::TimeDelta seek_time) override {}
  void OnSetVolumeMultiplier(int player_id, double volume_multiplier) override {
  }

  RenderFrameHost* render_frame_host() const override {
    return render_frame_host_;
  }

 private:
  RenderFrameHost* render_frame_host_;
};

void NavigateToURLAndWaitForFinish(Shell* window, const GURL& url) {
  base::RunLoop run_loop;
  MockWebContentsObserver observer(window->web_contents(),
                                   run_loop.QuitClosure());

  NavigateToURL(window, url);
  run_loop.Run();
}

char kSetUpMediaSessionScript[] =
    "navigator.mediaSession.playbackState = \"playing\";\n"
    "navigator.mediaSession.metadata = new MediaMetadata({ title: \"foo\" });\n"
    "navigator.mediaSession.setActionHandler(\"play\", _ => {});";

const int kPlayerId = 0;

}  // anonymous namespace

class MediaSessionServiceImplBrowserTest : public ContentBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ContentBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII("--enable-blink-features", "MediaSession");
  }

  void EnsurePlayer() {
    if (player_)
      return;

    player_.reset(new MockMediaSessionPlayerObserver(
        shell()->web_contents()->GetMainFrame()));

    MediaSessionImpl::Get(shell()->web_contents())
        ->AddPlayer(player_.get(), kPlayerId,
                    media::MediaContentType::Persistent);
  }

  MediaSessionImpl* GetSession() {
    return MediaSessionImpl::Get(shell()->web_contents());
  }

  MediaSessionServiceImpl* GetService() {
    RenderFrameHost* main_frame = shell()->web_contents()->GetMainFrame();
    if (GetSession()->services_.count(main_frame))
      return GetSession()->services_[main_frame];

    return nullptr;
  }

  bool ExecuteScriptToSetUpMediaSessionSync() {
    // Using the actions change as the signal of completion.
    base::RunLoop run_loop;
    MockMediaSessionObserver observer(GetSession(), run_loop.QuitClosure());
    bool result = ExecuteScript(shell(), kSetUpMediaSessionScript);
    run_loop.Run();
    return result;
  }

 private:
  std::unique_ptr<MockMediaSessionPlayerObserver> player_;
};

// Two windows from the same BrowserContext.
IN_PROC_BROWSER_TEST_F(MediaSessionServiceImplBrowserTest,
                       CrashMessageOnUnload) {
  NavigateToURL(shell(), GetTestUrl("media/session", "embedder.html"));
  // Navigate to a chrome:// URL to avoid render process re-use.
  NavigateToURL(shell(), GURL("chrome://flags"));
  // Should not crash.
}

// Tests for checking if the media session service members are correctly reset
// when navigating. Due to the mojo services have different message queues, it's
// hard to wait for the messages to arrive. Temporarily, the tests are using
// observers to wait for the message to be processed on the MediaSessionObserver
// side.

IN_PROC_BROWSER_TEST_F(MediaSessionServiceImplBrowserTest,
                       ResetServiceWhenNavigatingAway) {
  NavigateToURL(shell(), GetTestUrl(".", "title1.html"));
  EnsurePlayer();

  EXPECT_TRUE(ExecuteScriptToSetUpMediaSessionSync());

  EXPECT_EQ(blink::mojom::MediaSessionPlaybackState::PLAYING,
            GetService()->playback_state());
  EXPECT_TRUE(GetService()->metadata().has_value());
  EXPECT_EQ(1u, GetService()->actions().size());

  // Start a non-same-page navigation and check the playback state, metadata,
  // actions are reset.
  NavigateToURLAndWaitForFinish(shell(), GetTestUrl(".", "title2.html"));

  EXPECT_EQ(blink::mojom::MediaSessionPlaybackState::NONE,
            GetService()->playback_state());
  EXPECT_FALSE(GetService()->metadata().has_value());
  EXPECT_EQ(0u, GetService()->actions().size());
}

IN_PROC_BROWSER_TEST_F(MediaSessionServiceImplBrowserTest,
                       DontResetServiceForSameDocumentNavigation) {
  NavigateToURL(shell(), GetTestUrl(".", "title1.html"));
  EnsurePlayer();

  EXPECT_TRUE(ExecuteScriptToSetUpMediaSessionSync());

  // Start a fragment navigation and check the playback state, metadata,
  // actions are not reset.
  GURL fragment_change_url = GetTestUrl(".", "title1.html");
  fragment_change_url = GURL(fragment_change_url.spec() + "#some-anchor");
  NavigateToURLAndWaitForFinish(shell(), fragment_change_url);

  EXPECT_EQ(blink::mojom::MediaSessionPlaybackState::PLAYING,
            GetService()->playback_state());
  EXPECT_TRUE(GetService()->metadata().has_value());
  EXPECT_EQ(1u, GetService()->actions().size());
}

}  // namespace content
