// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/command_line.h"
#include "chrome/browser/media/webrtc/webrtc_browsertest_base.h"
#include "content/public/common/content_switches.h"
#include "media/base/media_switches.h"

static const char kMainWebrtcTestHtmlPage[] = "/webrtc/webrtc_jsep01_test.html";

class WebRtcRtpBrowserTest : public WebRtcTestBase {
 public:
  WebRtcRtpBrowserTest() : left_tab_(nullptr), right_tab_(nullptr) {}

  void SetUpInProcessBrowserTestFixture() override {
    DetectErrorsInJavaScript();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kUseFakeDeviceForMediaStream);
    command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                    "RTCRtpSenderReplaceTrack");
    // Required by |CollectGarbage|.
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, "--expose-gc");
  }

 protected:
  void StartServer() { ASSERT_TRUE(embedded_test_server()->Start()); }

  void OpenTab(content::WebContents** tab) {
    // TODO(hbos): Just open the tab, don't "AndGetUserMediaInNewTab".
    *tab = OpenTestPageAndGetUserMediaInNewTab(kMainWebrtcTestHtmlPage);
  }

  void StartServerAndOpenTabs() {
    StartServer();
    OpenTab(&left_tab_);
    OpenTab(&right_tab_);
  }

  const TrackEvent* FindTrackEvent(const std::vector<TrackEvent>& track_events,
                                   const std::string& track_id) {
    auto event_it = std::find_if(track_events.begin(), track_events.end(),
                                 [&track_id](const TrackEvent& event) {
                                   return event.track_id == track_id;
                                 });
    return event_it != track_events.end() ? &(*event_it) : nullptr;
  }

  content::WebContents* left_tab_;
  content::WebContents* right_tab_;
};

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest, GetSenders) {
  StartServerAndOpenTabs();

  SetupPeerconnectionWithoutLocalStream(left_tab_);
  CreateAndAddStreams(left_tab_, 3);

  SetupPeerconnectionWithoutLocalStream(right_tab_);
  CreateAndAddStreams(right_tab_, 1);

  NegotiateCall(left_tab_, right_tab_);

  VerifyRtpSenders(left_tab_, 6);
  VerifyRtpSenders(right_tab_, 2);
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest, GetReceivers) {
  StartServerAndOpenTabs();

  SetupPeerconnectionWithoutLocalStream(left_tab_);
  CreateAndAddStreams(left_tab_, 3);

  SetupPeerconnectionWithoutLocalStream(right_tab_);
  CreateAndAddStreams(right_tab_, 1);

  NegotiateCall(left_tab_, right_tab_);

  VerifyRtpReceivers(left_tab_, 2);
  VerifyRtpReceivers(right_tab_, 6);
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest, AddAndRemoveTracksWithoutStream) {
  StartServerAndOpenTabs();

  SetupPeerconnectionWithoutLocalStream(left_tab_);
  SetupPeerconnectionWithoutLocalStream(right_tab_);

  // TODO(hbos): Here and in other "AddAndRemoveTracks" tests: when ontrack and
  // ended events are supported, verify that these are fired on the remote side
  // when tracks are added and removed. https://crbug.com/webrtc/7933

  // Add two tracks.
  EXPECT_EQ(0u, GetNegotiationNeededCount(left_tab_));
  std::vector<std::string> ids =
      CreateAndAddAudioAndVideoTrack(left_tab_, StreamArgumentType::NO_STREAM);
  // TODO(hbos): Should only fire once (if the "negotiationneeded" bit changes
  // from false to true), not once per track added. https://crbug.com/740501
  EXPECT_EQ(2u, GetNegotiationNeededCount(left_tab_));
  std::string audio_stream_id = ids[0];
  std::string audio_track_id = ids[1];
  std::string video_stream_id = ids[2];
  std::string video_track_id = ids[3];
  EXPECT_EQ("null", audio_stream_id);
  EXPECT_NE("null", audio_track_id);
  EXPECT_EQ("null", video_stream_id);
  EXPECT_NE("null", video_track_id);
  CollectGarbage(left_tab_);
  EXPECT_FALSE(HasLocalStreamWithTrack(left_tab_, kUndefined, audio_track_id));
  EXPECT_FALSE(HasLocalStreamWithTrack(left_tab_, kUndefined, video_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 2);
  // Negotiate call, sets remote description.
  NegotiateCall(left_tab_, right_tab_);
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 2);
  // TODO(hbos): When |addTrack| without streams results in SDP that does not
  // signal a remote stream to be added we should expect |stream_ids| to be
  // empty and |HasRemoteStreamWithTrack| to be false.
  // https://crbug.com/webrtc/7933
  std::vector<TrackEvent> track_events = GetTrackEvents(right_tab_);
  EXPECT_EQ(2u, track_events.size());
  const TrackEvent* audio_track_event =
      FindTrackEvent(track_events, audio_track_id);
  ASSERT_TRUE(audio_track_event);
  EXPECT_EQ(1u, audio_track_event->stream_ids.size());
  std::string remote_audio_stream_id = audio_track_event->stream_ids[0];
  EXPECT_TRUE(HasRemoteStreamWithTrack(right_tab_, remote_audio_stream_id,
                                       audio_track_id));
  const TrackEvent* video_track_event =
      FindTrackEvent(track_events, video_track_id);
  ASSERT_TRUE(video_track_event);
  EXPECT_EQ(1u, video_track_event->stream_ids.size());
  std::string remote_video_stream_id = video_track_event->stream_ids[0];
  EXPECT_TRUE(HasRemoteStreamWithTrack(right_tab_, remote_video_stream_id,
                                       video_track_id));

  // Remove first track.
  RemoveTrack(left_tab_, audio_track_id);
  CollectGarbage(left_tab_);
  EXPECT_EQ(3u, GetNegotiationNeededCount(left_tab_));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 1);
  // Re-negotiate call, sets remote description again.
  NegotiateCall(left_tab_, right_tab_);
  CollectGarbage(right_tab_);
  // No additional track events should have fired.
  EXPECT_EQ(2u, GetTrackEvents(right_tab_).size());
  EXPECT_FALSE(HasRemoteStreamWithTrack(right_tab_, remote_audio_stream_id,
                                        audio_track_id));
  EXPECT_TRUE(HasRemoteStreamWithTrack(right_tab_, remote_video_stream_id,
                                       video_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 1);

  // Remove second track.
  RemoveTrack(left_tab_, video_track_id);
  CollectGarbage(left_tab_);
  EXPECT_EQ(4u, GetNegotiationNeededCount(left_tab_));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 0);
  // Re-negotiate call, sets remote description again.
  NegotiateCall(left_tab_, right_tab_);
  CollectGarbage(right_tab_);
  // No additional track events should have fired.
  EXPECT_EQ(2u, GetTrackEvents(right_tab_).size());
  EXPECT_FALSE(HasRemoteStreamWithTrack(right_tab_, remote_audio_stream_id,
                                        audio_track_id));
  EXPECT_FALSE(HasRemoteStreamWithTrack(right_tab_, remote_video_stream_id,
                                        video_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 0);
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest,
                       AddAndRemoveTracksWithSharedStream) {
  StartServerAndOpenTabs();

  SetupPeerconnectionWithoutLocalStream(left_tab_);
  SetupPeerconnectionWithoutLocalStream(right_tab_);

  // Add two tracks.
  EXPECT_EQ(0u, GetNegotiationNeededCount(left_tab_));
  std::vector<std::string> ids = CreateAndAddAudioAndVideoTrack(
      left_tab_, StreamArgumentType::SHARED_STREAM);
  // TODO(hbos): Should only fire once (if the "negotiationneeded" bit changes
  // from false to true), not once per track added. https://crbug.com/740501
  EXPECT_EQ(2u, GetNegotiationNeededCount(left_tab_));
  std::string audio_stream_id = ids[0];
  std::string audio_track_id = ids[1];
  std::string video_stream_id = ids[2];
  std::string video_track_id = ids[3];
  EXPECT_NE("null", audio_stream_id);
  EXPECT_NE("null", audio_track_id);
  EXPECT_NE("null", video_stream_id);
  EXPECT_NE("null", video_track_id);
  EXPECT_EQ(audio_stream_id, video_stream_id);
  CollectGarbage(left_tab_);
  // TODO(hbos): When |getLocalStreams| is updated to return the streams of all
  // senders, not just |addStream|-streams, then this will be EXPECT_TRUE.
  // https://crbug.com/738918
  EXPECT_FALSE(
      HasLocalStreamWithTrack(left_tab_, audio_stream_id, audio_track_id));
  EXPECT_FALSE(
      HasLocalStreamWithTrack(left_tab_, video_stream_id, video_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 2);
  // Negotiate call, sets remote description.
  NegotiateCall(left_tab_, right_tab_);
  EXPECT_TRUE(
      HasRemoteStreamWithTrack(right_tab_, audio_stream_id, audio_track_id));
  EXPECT_TRUE(
      HasRemoteStreamWithTrack(right_tab_, video_stream_id, video_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 2);
  std::vector<TrackEvent> track_events = GetTrackEvents(right_tab_);
  EXPECT_EQ(2u, track_events.size());
  const TrackEvent* audio_track_event =
      FindTrackEvent(track_events, audio_track_id);
  ASSERT_TRUE(audio_track_event);
  ASSERT_EQ(1u, audio_track_event->stream_ids.size());
  EXPECT_EQ(audio_stream_id, audio_track_event->stream_ids[0]);
  const TrackEvent* video_track_event =
      FindTrackEvent(track_events, video_track_id);
  ASSERT_TRUE(video_track_event);
  ASSERT_EQ(1u, video_track_event->stream_ids.size());
  EXPECT_EQ(video_stream_id, video_track_event->stream_ids[0]);

  // Remove first track.
  RemoveTrack(left_tab_, audio_track_id);
  CollectGarbage(left_tab_);
  EXPECT_EQ(3u, GetNegotiationNeededCount(left_tab_));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 1);
  // Re-negotiate call, sets remote description again.
  NegotiateCall(left_tab_, right_tab_);
  CollectGarbage(right_tab_);
  // No additional track events should have fired.
  EXPECT_EQ(2u, GetTrackEvents(right_tab_).size());
  EXPECT_FALSE(
      HasRemoteStreamWithTrack(right_tab_, audio_stream_id, audio_track_id));
  EXPECT_TRUE(
      HasRemoteStreamWithTrack(right_tab_, video_stream_id, video_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 1);

  // Remove second track.
  RemoveTrack(left_tab_, video_track_id);
  CollectGarbage(left_tab_);
  EXPECT_EQ(4u, GetNegotiationNeededCount(left_tab_));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 0);
  // Re-negotiate call, sets remote description again.
  NegotiateCall(left_tab_, right_tab_);
  CollectGarbage(right_tab_);
  // No additional track events should have fired.
  EXPECT_EQ(2u, GetTrackEvents(right_tab_).size());
  EXPECT_FALSE(
      HasRemoteStreamWithTrack(right_tab_, audio_stream_id, audio_track_id));
  EXPECT_FALSE(
      HasRemoteStreamWithTrack(right_tab_, video_stream_id, video_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 0);
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest,
                       AddAndRemoveTracksWithIndividualStreams) {
  StartServerAndOpenTabs();

  SetupPeerconnectionWithoutLocalStream(left_tab_);
  SetupPeerconnectionWithoutLocalStream(right_tab_);

  // Add two tracks.
  EXPECT_EQ(0u, GetNegotiationNeededCount(left_tab_));
  std::vector<std::string> ids = CreateAndAddAudioAndVideoTrack(
      left_tab_, StreamArgumentType::INDIVIDUAL_STREAMS);
  // TODO(hbos): Should only fire once (if the "negotiationneeded" bit changes
  // from false to true), not once per track added. https://crbug.com/740501
  EXPECT_EQ(2u, GetNegotiationNeededCount(left_tab_));
  std::string audio_stream_id = ids[0];
  std::string audio_track_id = ids[1];
  std::string video_stream_id = ids[2];
  std::string video_track_id = ids[3];
  EXPECT_NE("null", audio_stream_id);
  EXPECT_NE("null", audio_track_id);
  EXPECT_NE("null", video_stream_id);
  EXPECT_NE("null", video_track_id);
  EXPECT_NE(audio_stream_id, video_stream_id);
  CollectGarbage(left_tab_);
  // TODO(hbos): When |getLocalStreams| is updated to return the streams of all
  // senders, not just |addStream|-streams, then this will be EXPECT_TRUE.
  // https://crbug.com/738918
  EXPECT_FALSE(
      HasLocalStreamWithTrack(left_tab_, audio_stream_id, audio_track_id));
  EXPECT_FALSE(
      HasLocalStreamWithTrack(left_tab_, video_stream_id, video_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 2);
  // Negotiate call, sets remote description.
  NegotiateCall(left_tab_, right_tab_);
  EXPECT_TRUE(
      HasRemoteStreamWithTrack(right_tab_, audio_stream_id, audio_track_id));
  EXPECT_TRUE(
      HasRemoteStreamWithTrack(right_tab_, video_stream_id, video_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 2);
  std::vector<TrackEvent> track_events = GetTrackEvents(right_tab_);
  EXPECT_EQ(2u, track_events.size());
  const TrackEvent* audio_track_event =
      FindTrackEvent(track_events, audio_track_id);
  ASSERT_TRUE(audio_track_event);
  ASSERT_EQ(1u, audio_track_event->stream_ids.size());
  EXPECT_EQ(audio_stream_id, audio_track_event->stream_ids[0]);
  const TrackEvent* video_track_event =
      FindTrackEvent(track_events, video_track_id);
  ASSERT_TRUE(video_track_event);
  ASSERT_EQ(1u, video_track_event->stream_ids.size());
  EXPECT_EQ(video_stream_id, video_track_event->stream_ids[0]);

  // Remove first track.
  RemoveTrack(left_tab_, audio_track_id);
  CollectGarbage(left_tab_);
  EXPECT_EQ(3u, GetNegotiationNeededCount(left_tab_));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_TRUE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 1);
  // Re-negotiate call, sets remote description again.
  NegotiateCall(left_tab_, right_tab_);
  CollectGarbage(right_tab_);
  // No additional track events should have fired.
  EXPECT_EQ(2u, GetTrackEvents(right_tab_).size());
  EXPECT_FALSE(
      HasRemoteStreamWithTrack(right_tab_, audio_stream_id, audio_track_id));
  EXPECT_TRUE(
      HasRemoteStreamWithTrack(right_tab_, video_stream_id, video_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_TRUE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 1);

  // Remove second track.
  RemoveTrack(left_tab_, video_track_id);
  CollectGarbage(left_tab_);
  EXPECT_EQ(4u, GetNegotiationNeededCount(left_tab_));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, audio_track_id));
  EXPECT_FALSE(HasSenderWithTrack(left_tab_, video_track_id));
  VerifyRtpSenders(left_tab_, 0);
  // Re-negotiate call, sets remote description again.
  NegotiateCall(left_tab_, right_tab_);
  CollectGarbage(right_tab_);
  // No additional track events should have fired.
  EXPECT_EQ(2u, GetTrackEvents(right_tab_).size());
  EXPECT_FALSE(
      HasRemoteStreamWithTrack(right_tab_, audio_stream_id, audio_track_id));
  EXPECT_FALSE(
      HasRemoteStreamWithTrack(right_tab_, video_stream_id, video_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, audio_track_id));
  EXPECT_FALSE(HasReceiverWithTrack(right_tab_, video_track_id));
  VerifyRtpReceivers(right_tab_, 0);
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest, GetReceiversSetRemoteDescription) {
  StartServerAndOpenTabs();
  EXPECT_EQ("ok", ExecuteJavascript("createReceiverWithSetRemoteDescription()",
                                    left_tab_));
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest, SwitchRemoteStreamAndBackAgain) {
  StartServerAndOpenTabs();
  EXPECT_EQ("ok",
            ExecuteJavascript("switchRemoteStreamAndBackAgain()", left_tab_));
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest,
                       SwitchRemoteStreamWithoutWaitingForPromisesToResolve) {
  StartServerAndOpenTabs();
  EXPECT_EQ("ok", ExecuteJavascript(
                      "switchRemoteStreamWithoutWaitingForPromisesToResolve()",
                      left_tab_));
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest, TrackSwitchingStream) {
  StartServerAndOpenTabs();
  EXPECT_EQ("ok", ExecuteJavascript("trackSwitchingStream()", left_tab_));
}

IN_PROC_BROWSER_TEST_F(WebRtcRtpBrowserTest,
                       RTCRtpSenderReplaceTrackSendsNewVideoTrack) {
  StartServer();
  OpenTab(&left_tab_);
  EXPECT_EQ("test-passed",
            ExecuteJavascript(
                "testRTCRtpSenderReplaceTrackSendsNewVideoTrack()", left_tab_));
}
