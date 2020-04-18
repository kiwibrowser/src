// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {
static const char kStartPlayerScript[] =
    "document.getElementById('long-video').play()";
static const char kPausePlayerScript[] =
    "document.getElementById('long-video').pause()";

enum class MediaSuspend {
  ENABLED,
  DISABLED,
};

enum class BackgroundResuming {
  ENABLED,
  DISABLED,
};

enum class SessionState {
  ACTIVE,
  SUSPENDED,
  INACTIVE,
};

struct VisibilityTestData {
  MediaSuspend media_suspend;
  BackgroundResuming background_resuming;
  SessionState session_state_before_hide;
  SessionState session_state_after_hide;
};
}

// Base class of MediaSession visibility tests. The class is intended
// to be used to run tests under different configurations. Tests
// should inheret from this class, set up their own command line per
// their configuration, and use macro INCLUDE_TEST_FROM_BASE_CLASS to
// include required tests. See
// media_session_visibility_browsertest_instances.cc for examples.
class MediaSessionImplVisibilityBrowserTest
    : public ContentBrowserTest,
      public ::testing::WithParamInterface<VisibilityTestData> {
 public:
  MediaSessionImplVisibilityBrowserTest() {
    VisibilityTestData params = GetVisibilityTestData();
    EnableDisableResumingBackgroundVideos(params.background_resuming ==
                                          BackgroundResuming::ENABLED);
  }

  ~MediaSessionImplVisibilityBrowserTest() override = default;

  void SetUpOnMainThread() override {
    ContentBrowserTest::SetUpOnMainThread();
    web_contents_ = shell()->web_contents();
    media_session_ = MediaSessionImpl::Get(web_contents_);

    media_session_state_loop_runners_[MediaSessionImpl::State::ACTIVE] =
        new MessageLoopRunner();
    media_session_state_loop_runners_[MediaSessionImpl::State::SUSPENDED] =
        new MessageLoopRunner();
    media_session_state_loop_runners_[MediaSessionImpl::State::INACTIVE] =
        new MessageLoopRunner();
    media_session_state_callback_subscription_ =
        media_session_->RegisterMediaSessionStateChangedCallbackForTest(
            base::Bind(&MediaSessionImplVisibilityBrowserTest::
                           OnMediaSessionStateChanged,
                       base::Unretained(this)));
  }

  void TearDownOnMainThread() override {
    // Unsubscribe the callback subscription before tearing down, so that the
    // CallbackList in MediaSession will be empty when it is destroyed.
    media_session_state_callback_subscription_.reset();
  }

  void EnableDisableResumingBackgroundVideos(bool enable) {
    if (enable)
      scoped_feature_list_.InitAndEnableFeature(media::kResumeBackgroundVideo);
    else
      scoped_feature_list_.InitAndDisableFeature(media::kResumeBackgroundVideo);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kNoUserGestureRequiredPolicy);
#if !defined(OS_ANDROID)
    command_line->AppendSwitch(switches::kEnableAudioFocus);
#endif  // !defined(OS_ANDROID)

    VisibilityTestData params = GetVisibilityTestData();

    if (params.media_suspend == MediaSuspend::ENABLED)
      command_line->AppendSwitch(switches::kEnableMediaSuspend);
    else
      command_line->AppendSwitch(switches::kDisableMediaSuspend);
  }

  const VisibilityTestData& GetVisibilityTestData() {
    return GetParam();
  }

  void StartPlayer() {
    LoadTestPage();

    LOG(INFO) << "Starting player";
    ClearMediaSessionStateLoopRunners();
    RunScript(kStartPlayerScript);
    LOG(INFO) << "Waiting for session to be active";
    WaitForMediaSessionState(MediaSessionImpl::State::ACTIVE);
  }

  // Maybe pause the player depending on whether the session state before hide
  // is SUSPENDED.
  void MaybePausePlayer() {
    ASSERT_TRUE(GetVisibilityTestData().session_state_before_hide !=
                SessionState::INACTIVE);
    if (GetVisibilityTestData().session_state_before_hide ==
        SessionState::ACTIVE)
      return;

    LOG(INFO) << "Pausing player";
    ClearMediaSessionStateLoopRunners();
    RunScript(kPausePlayerScript);
    LOG(INFO) << "Waiting for session to be suspended";
    WaitForMediaSessionState(MediaSessionImpl::State::SUSPENDED);
  }

  void HideTab() {
    LOG(INFO) << "Hiding the tab";
    ClearMediaSessionStateLoopRunners();
    web_contents_->WasHidden();
  }

  void CheckSessionStateAfterHide() {
    MediaSessionImpl::State state_before_hide =
        ToMediaSessionState(GetVisibilityTestData().session_state_before_hide);
    MediaSessionImpl::State state_after_hide =
        ToMediaSessionState(GetVisibilityTestData().session_state_after_hide);

    if (state_before_hide == state_after_hide) {
      LOG(INFO) << "Waiting for 1 second and check session state is unchanged";
      Wait(base::TimeDelta::FromSeconds(1));
      ASSERT_EQ(state_after_hide, media_session_->audio_focus_state_);
    } else {
      LOG(INFO) << "Waiting for Session to change";
      WaitForMediaSessionState(state_after_hide);
    }

    LOG(INFO) << "Test succeeded";
  }

 private:
  void LoadTestPage() {
    TestNavigationObserver navigation_observer(shell()->web_contents(), 1);
    shell()->LoadURL(GetTestUrl("media/session", "media-session.html"));
    navigation_observer.Wait();
  }

  void RunScript(const std::string& script) {
    ASSERT_TRUE(ExecuteScript(web_contents_->GetMainFrame(), script));
  }

  void ClearMediaSessionStateLoopRunners() {
    for (auto& state_loop_runner : media_session_state_loop_runners_)
      state_loop_runner.second = new MessageLoopRunner();
  }

  void OnMediaSessionStateChanged(MediaSessionImpl::State state) {
    ASSERT_TRUE(media_session_state_loop_runners_.count(state));
    media_session_state_loop_runners_[state]->Quit();
  }

  // TODO(zqzhang): This method is shared with
  // MediaRouterIntegrationTests. Move it into a general place.
  void Wait(base::TimeDelta timeout) {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(), timeout);
    run_loop.Run();
  }

  void WaitForMediaSessionState(MediaSessionImpl::State state) {
    ASSERT_TRUE(media_session_state_loop_runners_.count(state));
    media_session_state_loop_runners_[state]->Run();
  }

  MediaSessionImpl::State ToMediaSessionState(SessionState state) {
    switch (state) {
      case SessionState::ACTIVE:
        return MediaSessionImpl::State::ACTIVE;
        break;
      case SessionState::SUSPENDED:
        return MediaSessionImpl::State::SUSPENDED;
        break;
      case SessionState::INACTIVE:
        return MediaSessionImpl::State::INACTIVE;
        break;
      default:
        ADD_FAILURE() << "invalid SessionState to convert";
        return MediaSessionImpl::State::INACTIVE;
    }
  }

  base::test::ScopedFeatureList scoped_feature_list_;

  WebContents* web_contents_;
  MediaSessionImpl* media_session_;
  // MessageLoopRunners for waiting MediaSession state to change. Note that the
  // MessageLoopRunners can accept Quit() before calling Run(), thus the state
  // change can still be captured before waiting. For example, the MediaSession
  // might go active immediately after calling HTMLMediaElement.play(). A test
  // can listen to the state change before calling play(), and then wait for the
  // state change after play().
  std::map<MediaSessionImpl::State, scoped_refptr<MessageLoopRunner>>
      media_session_state_loop_runners_;
  std::unique_ptr<
      base::CallbackList<void(MediaSessionImpl::State)>::Subscription>
      media_session_state_callback_subscription_;

  DISALLOW_COPY_AND_ASSIGN(MediaSessionImplVisibilityBrowserTest);
};

namespace {

VisibilityTestData kTestParams[] = {
    {MediaSuspend::ENABLED, BackgroundResuming::DISABLED,
     SessionState::SUSPENDED, SessionState::INACTIVE},
    {MediaSuspend::ENABLED, BackgroundResuming::DISABLED, SessionState::ACTIVE,
     SessionState::INACTIVE},
    {MediaSuspend::ENABLED, BackgroundResuming::ENABLED, SessionState::ACTIVE,
     SessionState::SUSPENDED},
    {MediaSuspend::ENABLED, BackgroundResuming::ENABLED,
     SessionState::SUSPENDED, SessionState::SUSPENDED},
    {MediaSuspend::DISABLED, BackgroundResuming::DISABLED,
     SessionState::SUSPENDED, SessionState::SUSPENDED},
    {MediaSuspend::DISABLED, BackgroundResuming::DISABLED, SessionState::ACTIVE,
     SessionState::ACTIVE},
    {MediaSuspend::DISABLED, BackgroundResuming::ENABLED, SessionState::ACTIVE,
     SessionState::ACTIVE},
    {MediaSuspend::DISABLED, BackgroundResuming::ENABLED,
     SessionState::SUSPENDED, SessionState::SUSPENDED},
};

}  // anonymous namespace

IN_PROC_BROWSER_TEST_P(MediaSessionImplVisibilityBrowserTest, TestEntryPoint) {
  StartPlayer();
  MaybePausePlayer();
  HideTab();
  CheckSessionStateAfterHide();
}

INSTANTIATE_TEST_CASE_P(MediaSessionImplVisibilityBrowserTestInstances,
                        MediaSessionImplVisibilityBrowserTest,
                        ::testing::ValuesIn(kTestParams));

}  // namespace content
