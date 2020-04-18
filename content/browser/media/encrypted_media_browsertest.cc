// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "content/browser/media/media_browsertest.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "media/base/media.h"
#include "media/base/media_switches.h"
#include "media/base/test_data_util.h"
#include "media/media_buildflags.h"
#include "media/mojo/buildflags.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

#if BUILDFLAG(ENABLE_MOJO_CDM) && !BUILDFLAG(ENABLE_LIBRARY_CDMS)
// When mojo CDM is enabled, External Clear Key is supported in //content/shell/
// by using mojo CDM with AesDecryptor running in the remote (e.g. GPU or
// Browser) process. When pepper CDM is supported, External Clear Key is
// supported in chrome/, which is tested in browser_tests.
#define SUPPORTS_EXTERNAL_CLEAR_KEY_IN_CONTENT_SHELL
#endif

namespace content {

// Available key systems.
const char kClearKeyKeySystem[] = "org.w3.clearkey";

#if defined(SUPPORTS_EXTERNAL_CLEAR_KEY_IN_CONTENT_SHELL)
const char kExternalClearKeyKeySystem[] = "org.chromium.externalclearkey";
#endif

// Supported media types.
const char kWebMVorbisAudioOnly[] = "audio/webm; codecs=\"vorbis\"";
const char kWebMOpusAudioOnly[] = "audio/webm; codecs=\"opus\"";
const char kWebMVp8VideoOnly[] = "video/webm; codecs=\"vp8\"";
const char kWebMVp9VideoOnly[] = "video/webm; codecs=\"vp9\"";
const char kWebMOpusAudioVp9Video[] = "video/webm; codecs=\"opus, vp9\"";
const char kWebMVorbisAudioVp8Video[] = "video/webm; codecs=\"vorbis, vp8\"";
const char kMp4FlacAudioOnly[] = "audio/mp4; codecs=\"flac\"";
const char kMp4Vp9VideoOnly[] =
    "video/mp4; codecs=\"vp09.00.10.08.01.02.02.02.00\"";
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
const char kMp4Avc1VideoOnly[] = "video/mp4; codecs=\"avc1.64001E\"";
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

// EME-specific test results and errors.
const char kEmeKeyError[] = "KEYERROR";
const char kEmeNotSupportedError[] = "NOTSUPPORTEDERROR";

const char kDefaultEmePlayer[] = "eme_player.html";

// The type of video src used to load media.
enum class SrcType { SRC, MSE };

// Must be in sync with CONFIG_CHANGE_TYPE in eme_player_js/global.js
enum class ConfigChangeType {
  CLEAR_TO_CLEAR = 0,
  CLEAR_TO_ENCRYPTED = 1,
  ENCRYPTED_TO_CLEAR = 2,
  ENCRYPTED_TO_ENCRYPTED = 3,
};

// Tests encrypted media playback with a combination of parameters:
// - char*: Key system name.
// - SrcType: The type of video src used to load media, MSE or SRC.
// It is okay to run this test as a non-parameterized test, in this case,
// GetParam() should not be called.
class EncryptedMediaTest
    : public MediaBrowserTest,
      public testing::WithParamInterface<std::tuple<const char*, SrcType>> {
 public:
  // Can only be used in parameterized (*_P) tests.
  const std::string CurrentKeySystem() { return std::get<0>(GetParam()); }

  // Can only be used in parameterized (*_P) tests.
  SrcType CurrentSourceType() { return std::get<1>(GetParam()); }

  void TestSimplePlayback(const std::string& encrypted_media,
                          const std::string& media_type) {
    RunSimpleEncryptedMediaTest(encrypted_media, media_type, CurrentKeySystem(),
                                CurrentSourceType());
  }

  void TestFrameSizeChange() {
    RunEncryptedMediaTest("encrypted_frame_size_change.html",
                          "frame_size_change-av_enc-v.webm",
                          kWebMVorbisAudioVp8Video, CurrentKeySystem(),
                          CurrentSourceType(), media::kEnded);
  }

  void TestConfigChange(ConfigChangeType config_change_type) {
    // TODO(xhwang): Even when config change is not supported we still start
    // content shell only to return directly here. We probably should not run
    // these test cases at all.
    if (CurrentSourceType() != SrcType::MSE) {
      DVLOG(0) << "Config change only happens when using MSE.";
      return;
    }

    base::StringPairs query_params;
    query_params.emplace_back("keySystem", CurrentKeySystem());
    query_params.emplace_back(
        "configChangeType",
        base::IntToString(static_cast<int>(config_change_type)));
    RunMediaTestPage("mse_config_change.html", query_params, media::kEnded,
                     true);
  }

  void RunEncryptedMediaTest(const std::string& html_page,
                             const std::string& media_file,
                             const std::string& media_type,
                             const std::string& key_system,
                             SrcType src_type,
                             const std::string& expectation) {
    base::StringPairs query_params;
    query_params.emplace_back("mediaFile", media_file);
    query_params.emplace_back("mediaType", media_type);
    query_params.emplace_back("keySystem", key_system);
    if (src_type == SrcType::MSE)
      query_params.emplace_back("useMSE", "1");
    RunMediaTestPage(html_page, query_params, expectation, true);
  }

  void RunSimpleEncryptedMediaTest(const std::string& media_file,
                                   const std::string& media_type,
                                   const std::string& key_system,
                                   SrcType src_type) {
    RunEncryptedMediaTest(kDefaultEmePlayer, media_file, media_type, key_system,
                          src_type, media::kEnded);
  }

  void TestMp4EncryptionPlayback(const std::string& media_file,
                                 const std::string& media_type,
                                 const std::string& expected_title) {
    if (CurrentSourceType() != SrcType::MSE) {
      DVLOG(0) << "Skipping test; Can only play MP4 encrypted streams by MSE.";
      return;
    }

    RunEncryptedMediaTest(kDefaultEmePlayer, media_file, media_type,
                          CurrentKeySystem(), SrcType::MSE, expected_title);
  }

 protected:
  // We want to fail quickly when a test fails because an error is encountered.
  void AddTitlesToAwait(content::TitleWatcher* title_watcher) override {
    MediaBrowserTest::AddTitlesToAwait(title_watcher);
    title_watcher->AlsoWaitForTitle(base::ASCIIToUTF16(kEmeNotSupportedError));
    title_watcher->AlsoWaitForTitle(base::ASCIIToUTF16(kEmeKeyError));
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kAutoplayPolicy,
        switches::autoplay::kNoUserGestureRequiredPolicy);
#if defined(SUPPORTS_EXTERNAL_CLEAR_KEY_IN_CONTENT_SHELL)
    scoped_feature_list_.InitWithFeatures({media::kExternalClearKeyForTesting},
                                          {});
#endif
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

using ::testing::Combine;
using ::testing::Values;

INSTANTIATE_TEST_CASE_P(SRC_ClearKey,
                        EncryptedMediaTest,
                        Combine(Values(kClearKeyKeySystem),
                                Values(SrcType::SRC)));

INSTANTIATE_TEST_CASE_P(MSE_ClearKey,
                        EncryptedMediaTest,
                        Combine(Values(kClearKeyKeySystem),
                                Values(SrcType::MSE)));

#if defined(SUPPORTS_EXTERNAL_CLEAR_KEY_IN_CONTENT_SHELL)
INSTANTIATE_TEST_CASE_P(SRC_ExternalClearKey,
                        EncryptedMediaTest,
                        Combine(Values(kExternalClearKeyKeySystem),
                                Values(SrcType::SRC)));

INSTANTIATE_TEST_CASE_P(MSE_ExternalClearKey,
                        EncryptedMediaTest,
                        Combine(Values(kExternalClearKeyKeySystem),
                                Values(SrcType::MSE)));
#endif

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_AudioOnly_WebM) {
  TestSimplePlayback("bear-a_enc-a.webm", kWebMVorbisAudioOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_AudioClearVideo_WebM) {
  TestSimplePlayback("bear-320x240-av_enc-a.webm", kWebMVorbisAudioVp8Video);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoAudio_WebM) {
  TestSimplePlayback("bear-320x240-av_enc-av.webm", kWebMVorbisAudioVp8Video);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoOnly_WebM) {
  TestSimplePlayback("bear-320x240-v_enc-v.webm", kWebMVp8VideoOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoOnly_WebM_Fullsample) {
  TestSimplePlayback("bear-320x240-v-vp9_fullsample_enc-v.webm",
                     kWebMVp9VideoOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoOnly_WebM_Subsample) {
  TestSimplePlayback("bear-320x240-v-vp9_subsample_enc-v.webm",
                     kWebMVp9VideoOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoClearAudio_WebM) {
  TestSimplePlayback("bear-320x240-av_enc-v.webm", kWebMVorbisAudioVp8Video);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_AudioOnly_WebM_Opus) {
#if defined(OS_ANDROID)
  if (!media::PlatformHasOpusSupport())
    return;
#endif
  TestSimplePlayback("bear-320x240-opus-a_enc-a.webm", kWebMOpusAudioOnly);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoAudio_WebM_Opus) {
#if defined(OS_ANDROID)
  if (!media::PlatformHasOpusSupport())
    return;
#endif
  TestSimplePlayback("bear-320x240-opus-av_enc-av.webm",
                     kWebMOpusAudioVp9Video);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoClearAudio_WebM_Opus) {
#if defined(OS_ANDROID)
  if (!media::PlatformHasOpusSupport())
    return;
#endif
  TestSimplePlayback("bear-320x240-opus-av_enc-v.webm", kWebMOpusAudioVp9Video);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_AudioOnly_MP4_FLAC) {
  TestMp4EncryptionPlayback("bear-flac-cenc.mp4", kMp4FlacAudioOnly,
                            media::kEnded);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_VideoOnly_MP4_VP9) {
  // MP4 without MSE is not support yet, http://crbug.com/170793.
  if (CurrentSourceType() != SrcType::MSE) {
    DVLOG(0) << "Skipping test; Can only play MP4 encrypted streams by MSE.";
    return;
  }
  TestSimplePlayback("bear-320x240-v_frag-vp9-cenc.mp4", kMp4Vp9VideoOnly);
}

// Strictly speaking this is not an "encrypted" media test. Keep it here for
// completeness.
IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, ConfigChangeVideo_ClearToClear) {
  TestConfigChange(ConfigChangeType::CLEAR_TO_CLEAR);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, ConfigChangeVideo_ClearToEncrypted) {
  TestConfigChange(ConfigChangeType::CLEAR_TO_ENCRYPTED);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, ConfigChangeVideo_EncryptedToClear) {
  TestConfigChange(ConfigChangeType::ENCRYPTED_TO_CLEAR);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest,
                       ConfigChangeVideo_EncryptedToEncrypted) {
  TestConfigChange(ConfigChangeType::ENCRYPTED_TO_ENCRYPTED);
}

// https://crbug.com/788748 https://crbug.com/794080
#if (defined(OS_ANDROID) || defined(OS_LINUX)) && defined(ADDRESS_SANITIZER)
#define MAYBE_FrameSizeChangeVideo DISABLED_FrameSizeChangeVideo
#else
#define MAYBE_FrameSizeChangeVideo FrameSizeChangeVideo
#endif
IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, MAYBE_FrameSizeChangeVideo) {
  TestFrameSizeChange();
}

#if BUILDFLAG(USE_PROPRIETARY_CODECS)
IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_Encryption_CENC) {
  TestMp4EncryptionPlayback("bear-640x360-v_frag-cenc.mp4", kMp4Avc1VideoOnly,
                            media::kEnded);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_Encryption_CBC1) {
  TestMp4EncryptionPlayback("bear-640x360-v_frag-cbc1.mp4", kMp4Avc1VideoOnly,
                            media::kError);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_Encryption_CENS) {
  TestMp4EncryptionPlayback("bear-640x360-v_frag-cens.mp4", kMp4Avc1VideoOnly,
                            media::kError);
}

IN_PROC_BROWSER_TEST_P(EncryptedMediaTest, Playback_Encryption_CBCS) {
  std::string expected_result =
      BUILDFLAG(ENABLE_CBCS_ENCRYPTION_SCHEME) ? media::kEnded : media::kError;
  TestMp4EncryptionPlayback("bear-640x360-v_frag-cbcs.mp4", kMp4Avc1VideoOnly,
                            expected_result);
}
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

IN_PROC_BROWSER_TEST_F(EncryptedMediaTest, UnknownKeySystemThrowsException) {
  RunEncryptedMediaTest(kDefaultEmePlayer, "bear-a_enc-a.webm",
                        kWebMVorbisAudioOnly, "com.example.foo", SrcType::MSE,
                        kEmeNotSupportedError);
}

}  // namespace content
