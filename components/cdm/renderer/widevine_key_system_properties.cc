// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cdm/renderer/widevine_key_system_properties.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

#if defined(WIDEVINE_CDM_AVAILABLE)

using media::EmeConfigRule;
using media::EmeFeatureSupport;
using media::EmeInitDataType;
using media::EmeMediaType;
using media::EmeSessionTypeSupport;
using media::SupportedCodecs;
using Robustness = cdm::WidevineKeySystemProperties::Robustness;

namespace cdm {
namespace {

Robustness ConvertRobustness(const std::string& robustness) {
  if (robustness.empty())
    return Robustness::EMPTY;
  if (robustness == "SW_SECURE_CRYPTO")
    return Robustness::SW_SECURE_CRYPTO;
  if (robustness == "SW_SECURE_DECODE")
    return Robustness::SW_SECURE_DECODE;
  if (robustness == "HW_SECURE_CRYPTO")
    return Robustness::HW_SECURE_CRYPTO;
  if (robustness == "HW_SECURE_DECODE")
    return Robustness::HW_SECURE_DECODE;
  if (robustness == "HW_SECURE_ALL")
    return Robustness::HW_SECURE_ALL;
  return Robustness::INVALID;
}

}  // namespace

WidevineKeySystemProperties::WidevineKeySystemProperties(
    base::flat_set<media::EncryptionMode> supported_encryption_schemes,
    media::SupportedCodecs supported_codecs,
#if defined(OS_ANDROID)
    media::SupportedCodecs supported_secure_codecs,
#endif  // defined(OS_ANDROID)
    Robustness max_audio_robustness,
    Robustness max_video_robustness,
    media::EmeSessionTypeSupport persistent_license_support,
    media::EmeSessionTypeSupport persistent_release_message_support,
    media::EmeFeatureSupport persistent_state_support,
    media::EmeFeatureSupport distinctive_identifier_support)
    : supported_encryption_schemes_(std::move(supported_encryption_schemes)),
      supported_codecs_(supported_codecs),
#if defined(OS_ANDROID)
      supported_secure_codecs_(supported_secure_codecs),
#endif  // defined(OS_ANDROID)
      max_audio_robustness_(max_audio_robustness),
      max_video_robustness_(max_video_robustness),
      persistent_license_support_(persistent_license_support),
      persistent_release_message_support_(persistent_release_message_support),
      persistent_state_support_(persistent_state_support),
      distinctive_identifier_support_(distinctive_identifier_support) {
}

WidevineKeySystemProperties::~WidevineKeySystemProperties() = default;

std::string WidevineKeySystemProperties::GetKeySystemName() const {
  return kWidevineKeySystem;
}

bool WidevineKeySystemProperties::IsSupportedInitDataType(
    EmeInitDataType init_data_type) const {
  // Here we assume that support for a container imples support for the
  // associated initialization data type. KeySystems handles validating
  // |init_data_type| x |container| pairings.
  if (init_data_type == EmeInitDataType::WEBM)
    return (supported_codecs_ & media::EME_CODEC_WEBM_ALL) != 0;
  if (init_data_type == EmeInitDataType::CENC)
    return (supported_codecs_ & media::EME_CODEC_MP4_ALL) != 0;

  return false;
}

bool WidevineKeySystemProperties::IsEncryptionSchemeSupported(
    media::EncryptionMode encryption_scheme) const {
  return supported_encryption_schemes_.count(encryption_scheme) != 0;
}

SupportedCodecs WidevineKeySystemProperties::GetSupportedCodecs() const {
  return supported_codecs_;
}

#if defined(OS_ANDROID)
SupportedCodecs WidevineKeySystemProperties::GetSupportedSecureCodecs() const {
  return supported_secure_codecs_;
}
#endif

EmeConfigRule WidevineKeySystemProperties::GetRobustnessConfigRule(
    EmeMediaType media_type,
    const std::string& requested_robustness) const {
  Robustness robustness = ConvertRobustness(requested_robustness);
  if (robustness == Robustness::INVALID)
    return EmeConfigRule::NOT_SUPPORTED;

  Robustness max_robustness = Robustness::INVALID;
  switch (media_type) {
    case EmeMediaType::AUDIO:
      max_robustness = max_audio_robustness_;
      break;
    case EmeMediaType::VIDEO:
      max_robustness = max_video_robustness_;
      break;
  }

  // We can compare robustness levels whenever they are not HW_SECURE_CRYPTO
  // and SW_SECURE_DECODE in some order. If they are exactly those two then the
  // robustness requirement is not supported.
  if ((max_robustness == Robustness::HW_SECURE_CRYPTO &&
       robustness == Robustness::SW_SECURE_DECODE) ||
      (max_robustness == Robustness::SW_SECURE_DECODE &&
       robustness == Robustness::HW_SECURE_CRYPTO) ||
      robustness > max_robustness) {
    return EmeConfigRule::NOT_SUPPORTED;
  }

#if defined(OS_CHROMEOS)
  // Hardware security requires remote attestation.
  if (robustness >= Robustness::HW_SECURE_CRYPTO)
    return EmeConfigRule::IDENTIFIER_REQUIRED;

  // For video, recommend remote attestation if HW_SECURE_ALL is available,
  // regardless of the value of |robustness|, because it enables hardware
  // accelerated decoding.
  // TODO(sandersd): Only do this when hardware accelerated decoding is
  // available for the requested codecs.
  if (media_type == EmeMediaType::VIDEO &&
      max_robustness == Robustness::HW_SECURE_ALL) {
    return EmeConfigRule::IDENTIFIER_RECOMMENDED;
  }
#elif defined(OS_ANDROID)
  // Require hardware secure codecs when SW_SECURE_DECODE or above is specified.
  if (robustness >= Robustness::SW_SECURE_DECODE) {
    return EmeConfigRule::HW_SECURE_CODECS_REQUIRED;
  }
#endif  // defined(OS_CHROMEOS)

  return EmeConfigRule::SUPPORTED;
}

EmeSessionTypeSupport
WidevineKeySystemProperties::GetPersistentLicenseSessionSupport() const {
  return persistent_license_support_;
}

EmeSessionTypeSupport
WidevineKeySystemProperties::GetPersistentReleaseMessageSessionSupport() const {
  return persistent_release_message_support_;
}

EmeFeatureSupport WidevineKeySystemProperties::GetPersistentStateSupport()
    const {
  return persistent_state_support_;
}

EmeFeatureSupport WidevineKeySystemProperties::GetDistinctiveIdentifierSupport()
    const {
  return distinctive_identifier_support_;
}

}  // namespace cdm

#endif  // WIDEVINE_CDM_AVAILABLE
