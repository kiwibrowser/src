// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/browser/media/media_browsertest.h"
#include "media/base/media_switches.h"
#include "media/base/test_data_util.h"
#include "media/media_buildflags.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

// Common media types.
#if BUILDFLAG(USE_PROPRIETARY_CODECS) && !defined(OS_ANDROID)
const char kAAC_ADTS_AudioOnly[] = "audio/aac";
#endif
const char kWebMAudioOnly[] = "audio/webm; codecs=\"vorbis\"";
#if !defined(OS_ANDROID)
const char kWebMOpusAudioOnly[] = "audio/webm; codecs=\"opus\"";
#endif
const char kWebMVideoOnly[] = "video/webm; codecs=\"vp8\"";
const char kWebMAudioVideo[] = "video/webm; codecs=\"vorbis, vp8\"";
const char kMp4FlacAudioOnly[] = "audio/mp4; codecs=\"flac\"";

#if BUILDFLAG(USE_PROPRIETARY_CODECS) && \
    BUILDFLAG(ENABLE_MSE_MPEG2TS_STREAM_PARSER)
const char kMp2tAudioVideo[] = "video/mp2t; codecs=\"mp4a.40.2, avc1.42E01E\"";
#endif

namespace content {

class MediaSourceTest : public content::MediaBrowserTest {
 public:
  void TestSimplePlayback(const std::string& media_file,
                          const std::string& media_type,
                          const std::string& expectation) {
    base::StringPairs query_params;
    query_params.push_back(std::make_pair("mediaFile", media_file));
    query_params.push_back(std::make_pair("mediaType", media_type));
    RunMediaTestPage("media_source_player.html", query_params, expectation,
                     false);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kNoUserGestureRequiredPolicy);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_VideoAudio_WebM) {
  TestSimplePlayback("bear-320x240.webm", kWebMAudioVideo, media::kEnded);
}

IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_VideoOnly_WebM) {
  TestSimplePlayback("bear-320x240-video-only.webm", kWebMVideoOnly,
                     media::kEnded);
}

// TODO(servolk): Android is supposed to support AAC in ADTS container with
// 'audio/aac' mime type, but for some reason playback fails on trybots due to
// some issue in OMX AAC decoder (crbug.com/528361)
#if BUILDFLAG(USE_PROPRIETARY_CODECS) && !defined(OS_ANDROID)
IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_AudioOnly_AAC_ADTS) {
  TestSimplePlayback("sfx.adts", kAAC_ADTS_AudioOnly, media::kEnded);
}
#endif

// Opus is not supported in Android as of now.
#if !defined(OS_ANDROID)
IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_AudioOnly_Opus_WebM) {
  TestSimplePlayback("bear-opus.webm", kWebMOpusAudioOnly, media::kEnded);
}
#endif

IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_AudioOnly_WebM) {
  TestSimplePlayback("bear-320x240-audio-only.webm", kWebMAudioOnly,
                     media::kEnded);
}

IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_Type_Error) {
  TestSimplePlayback("bear-320x240-video-only.webm", kWebMAudioOnly,
                     media::kErrorEvent);
}

// Flaky test crbug.com/246308
// Test changed to skip checks resulting in flakiness. Proper fix still needed.
IN_PROC_BROWSER_TEST_F(MediaSourceTest, ConfigChangeVideo) {
  RunMediaTestPage("mse_config_change.html", base::StringPairs(), media::kEnded,
                   true);
}

#if BUILDFLAG(USE_PROPRIETARY_CODECS)

// TODO(chcunningham): Figure out why this is flaky on android. crbug/607841
#if !defined(OS_ANDROID)
IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_Video_MP4_Audio_WEBM) {
  base::StringPairs query_params;
  query_params.push_back(std::make_pair("videoFormat", "CLEAR_MP4"));
  query_params.push_back(std::make_pair("audioFormat", "CLEAR_WEBM"));
  RunMediaTestPage("mse_different_containers.html", query_params, media::kEnded,
                   true);
}
#endif  // !defined(OS_ANDROID)

IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_Video_WEBM_Audio_MP4) {
  base::StringPairs query_params;
  query_params.push_back(std::make_pair("videoFormat", "CLEAR_WEBM"));
  query_params.push_back(std::make_pair("audioFormat", "CLEAR_MP4"));
  RunMediaTestPage("mse_different_containers.html", query_params, media::kEnded,
                   true);
}

#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_AudioOnly_FLAC_MP4) {
  TestSimplePlayback("bear-flac_frag.mp4", kMp4FlacAudioOnly, media::kEnded);
}

#if BUILDFLAG(USE_PROPRIETARY_CODECS)
#if BUILDFLAG(ENABLE_MSE_MPEG2TS_STREAM_PARSER)
IN_PROC_BROWSER_TEST_F(MediaSourceTest, Playback_AudioVideo_Mp2t) {
  TestSimplePlayback("bear-1280x720.ts", kMp2tAudioVideo, media::kEnded);
}
#endif
#endif

}  // namespace content
