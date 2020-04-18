// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/renderer/media/key_systems_cast.h"

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "chromecast/chromecast_buildflags.h"
#include "chromecast/media/base/key_systems_common.h"
#include "components/cdm/renderer/android_key_systems.h"
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "media/base/eme_constants.h"
#include "media/base/key_system_properties.h"
#include "media/media_buildflags.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

using ::media::EmeConfigRule;
using ::media::EmeFeatureSupport;
using ::media::EmeInitDataType;
using ::media::EmeMediaType;
using ::media::EmeSessionTypeSupport;
using ::media::SupportedCodecs;

namespace chromecast {
namespace media {
namespace {

#if defined(PLAYREADY_CDM_AVAILABLE)
class PlayReadyKeySystemProperties : public ::media::KeySystemProperties {
 public:
  PlayReadyKeySystemProperties(SupportedCodecs supported_non_secure_codecs,
                               SupportedCodecs supported_secure_codecs,
                               bool persistent_license_support)
      : supported_non_secure_codecs_(supported_non_secure_codecs),
#if defined(OS_ANDROID)
        supported_secure_codecs_(supported_secure_codecs),
#endif  // defined(OS_ANDROID)
        persistent_license_support_(persistent_license_support) {
  }

  std::string GetKeySystemName() const override {
    return media::kChromecastPlayreadyKeySystem;
  }

  bool IsSupportedInitDataType(EmeInitDataType init_data_type) const override {
    return init_data_type == EmeInitDataType::CENC;
  }

  SupportedCodecs GetSupportedCodecs() const override {
    return supported_non_secure_codecs_;
  }

#if defined(OS_ANDROID)
  SupportedCodecs GetSupportedSecureCodecs() const override {
    return supported_secure_codecs_;
  }
#endif  // defined(OS_ANDROID)

  EmeConfigRule GetRobustnessConfigRule(
      EmeMediaType media_type,
      const std::string& requested_robustness) const override {
    if (requested_robustness.empty()) {
#if defined(OS_ANDROID)
      return EmeConfigRule::HW_SECURE_CODECS_REQUIRED;
#else
      return EmeConfigRule::SUPPORTED;
#endif  // defined(OS_ANDROID)
    }

    // Cast-specific PlayReady implementation does not currently recognize or
    // support non-empty robustness strings.
    return EmeConfigRule::NOT_SUPPORTED;
  }

  EmeSessionTypeSupport GetPersistentLicenseSessionSupport() const override {
    return persistent_license_support_ ? EmeSessionTypeSupport::SUPPORTED
                                       : EmeSessionTypeSupport::NOT_SUPPORTED;
  }

  EmeSessionTypeSupport GetPersistentReleaseMessageSessionSupport()
      const override {
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }

  EmeFeatureSupport GetPersistentStateSupport() const override {
    return EmeFeatureSupport::ALWAYS_ENABLED;
  }
  EmeFeatureSupport GetDistinctiveIdentifierSupport() const override {
    return EmeFeatureSupport::ALWAYS_ENABLED;
  }

  bool IsEncryptionSchemeSupported(
      ::media::EncryptionMode encryption_mode) const override {
    return encryption_mode == ::media::EncryptionMode::kCenc;
  }

 private:
  const SupportedCodecs supported_non_secure_codecs_;
#if defined(OS_ANDROID)
  const SupportedCodecs supported_secure_codecs_;
#endif  // defined(OS_ANDROID)
  const bool persistent_license_support_;
};
#endif  // PLAYREADY_CDM_AVAILABLE

#if BUILDFLAG(IS_CAST_USING_CMA_BACKEND)
SupportedCodecs GetCastEmeSupportedCodecs() {
  SupportedCodecs codecs =
      ::media::EME_CODEC_MP4_AAC | ::media::EME_CODEC_MP4_AVC1 |
      ::media::EME_CODEC_COMMON_VP9 | ::media::EME_CODEC_WEBM_VP8 |
      ::media::EME_CODEC_WEBM_VP9;

#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
  codecs |= ::media::EME_CODEC_MP4_HEVC;
#endif  // BUILDFLAG(ENABLE_HEVC_DEMUXING)

#if BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)
  codecs |= ::media::EME_CODEC_MP4_DV_AVC;
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
  codecs |= ::media::EME_CODEC_MP4_DV_HEVC;
#endif  // BUILDFLAG(ENABLE_HEVC_DEMUXING)
#endif  // BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)

#if BUILDFLAG(ENABLE_AC3_EAC3_AUDIO_DEMUXING)
  codecs |= ::media::EME_CODEC_MP4_AC3 | ::media::EME_CODEC_MP4_EAC3;
#endif  // BUILDFLAG(ENABLE_AC3_EAC3_AUDIO_DEMUXING)

#if BUILDFLAG(ENABLE_MPEG_H_AUDIO_DEMUXING)
  codecs |= ::media::EME_CODEC_MP4_MPEG_H_AUDIO;
#endif  // BUILDFLAG(ENABLE_MPEG_H_AUDIO_DEMUXING)

  return codecs;
}

void AddCmaKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>*
        key_systems_properties,
    bool enable_persistent_license_support) {
  SupportedCodecs codecs = GetCastEmeSupportedCodecs();

  // |codecs| may not be used if Widevine and Playready aren't supported.
  ANALYZER_ALLOW_UNUSED(codecs);

#if defined(PLAYREADY_CDM_AVAILABLE)
  key_systems_properties->emplace_back(new PlayReadyKeySystemProperties(
      codecs, codecs, enable_persistent_license_support));
#endif  // defined(PLAYREADY_CDM_AVAILABLE)

#if defined(WIDEVINE_CDM_AVAILABLE)
  using Robustness = cdm::WidevineKeySystemProperties::Robustness;

  base::flat_set<::media::EncryptionMode> supported_encryption_schemes = {
      ::media::EncryptionMode::kCenc, ::media::EncryptionMode::kCbcs};

  key_systems_properties->emplace_back(new cdm::WidevineKeySystemProperties(
      supported_encryption_schemes,          // Encryption schemes.
      codecs,                                // Regular codecs.
      Robustness::HW_SECURE_ALL,             // Max audio robustness.
      Robustness::HW_SECURE_ALL,             // Max video robustness.
      EmeSessionTypeSupport::SUPPORTED,      // persistent-license.
      EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-release-message.
      // Note: On Chromecast, all CDMs may have persistent state.
      EmeFeatureSupport::ALWAYS_ENABLED,    // Persistent state.
      EmeFeatureSupport::ALWAYS_ENABLED));  // Distinctive identifier.
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
}
#elif defined(OS_ANDROID)
#if defined(PLAYREADY_CDM_AVAILABLE)
void AddCastPlayreadyKeySystemAndroid(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>*
        key_systems_properties) {
  DCHECK(key_systems_properties);
  SupportedKeySystemResponse response =
      cdm::QueryKeySystemSupport(kChromecastPlayreadyKeySystem);

  if (response.non_secure_codecs == ::media::EME_CODEC_NONE)
    return;

  key_systems_properties->emplace_back(new PlayReadyKeySystemProperties(
      response.non_secure_codecs, response.secure_codecs,
      false /* persistent_license_support */));
}
#endif  // defined(PLAYREADY_CDM_AVAILABLE)

void AddCastAndroidKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>*
        key_systems_properties) {
#if defined(PLAYREADY_CDM_AVAILABLE)
  AddCastPlayreadyKeySystemAndroid(key_systems_properties);
#endif  // defined(PLAYREADY_CDM_AVAILABLE)

#if defined(WIDEVINE_CDM_AVAILABLE)
  cdm::AddAndroidWidevine(key_systems_properties);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
}
#endif  // defined(OS_ANDROID)

}  // namespace

// TODO(yucliu): Split CMA/Android logics into their own files.
void AddChromecastKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>*
        key_systems_properties,
    bool enable_persistent_license_support,
    bool force_software_crypto) {
#if BUILDFLAG(IS_CAST_USING_CMA_BACKEND)
  AddCmaKeySystems(key_systems_properties, enable_persistent_license_support);
#elif defined(OS_ANDROID)
  AddCastAndroidKeySystems(key_systems_properties);
#endif  // defined(OS_ANDROID)
}

}  // namespace media
}  // namespace chromecast
