// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/browser/media/session/mock_media_session_player_observer.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_content_type.h"
#include "media/base/media_switches.h"

namespace content {

class AudioFocusDelegateDefaultBrowserTest : public ContentBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnableAudioFocus);
  }

  void Run(WebContents* start_contents, WebContents* interrupt_contents) {
    std::unique_ptr<MockMediaSessionPlayerObserver>
        player_observer(new MockMediaSessionPlayerObserver);

    MediaSessionImpl* media_session = MediaSessionImpl::Get(start_contents);
    ASSERT_TRUE(media_session);

    MediaSessionImpl* other_media_session =
        MediaSessionImpl::Get(interrupt_contents);
    ASSERT_TRUE(other_media_session);

    player_observer->StartNewPlayer();
    media_session->AddPlayer(player_observer.get(), 0,
                             media::MediaContentType::Persistent);
    EXPECT_TRUE(media_session->IsActive());
    EXPECT_FALSE(other_media_session->IsActive());

    player_observer->StartNewPlayer();
    other_media_session->AddPlayer(player_observer.get(), 1,
                                   media::MediaContentType::Persistent);
    EXPECT_FALSE(media_session->IsActive());
    EXPECT_TRUE(other_media_session->IsActive());

    media_session->Stop(MediaSessionImpl::SuspendType::UI);
    other_media_session->Stop(MediaSessionImpl::SuspendType::UI);
  }
};

// Two windows from the same BrowserContext.
IN_PROC_BROWSER_TEST_F(AudioFocusDelegateDefaultBrowserTest,
                       ActiveWebContentsPauseOthers) {
  Run(shell()->web_contents(), CreateBrowser()->web_contents());
}

// Regular BrowserContext is interrupted by OffTheRecord one.
IN_PROC_BROWSER_TEST_F(AudioFocusDelegateDefaultBrowserTest,
                       RegularBrowserInterruptsOffTheRecord) {
  Run(shell()->web_contents(), CreateOffTheRecordBrowser()->web_contents());
}

// OffTheRecord BrowserContext is interrupted by regular one.
IN_PROC_BROWSER_TEST_F(AudioFocusDelegateDefaultBrowserTest,
                       OffTheRecordInterruptsRegular) {
  Run(CreateOffTheRecordBrowser()->web_contents(), shell()->web_contents());
}

}  // namespace content
