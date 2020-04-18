// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/key_systems.h"

#include <stddef.h>

#include <memory>

#include "base/containers/hash_tables.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "media/base/key_system_names.h"
#include "media/base/key_system_properties.h"
#include "media/base/media.h"
#include "media/base/media_client.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

namespace media {

const char kClearKeyKeySystem[] = "org.w3.clearkey";

// These names are used by UMA. Do not change them!
const char kClearKeyKeySystemNameForUMA[] = "ClearKey";
const char kUnknownKeySystemNameForUMA[] = "Unknown";

struct MimeTypeToCodecs {
  const char* mime_type;
  SupportedCodecs codecs;
};

// Mapping between containers and their codecs.
// Only audio codecs can belong to a "audio/*" mime_type, and only video codecs
// can belong to a "video/*" mime_type.
static const MimeTypeToCodecs kMimeTypeToCodecsMap[] = {
    {"audio/webm", EME_CODEC_WEBM_AUDIO_ALL},
    {"video/webm", EME_CODEC_WEBM_VIDEO_ALL},
    {"audio/mp4", EME_CODEC_MP4_AUDIO_ALL},
    {"video/mp4", EME_CODEC_MP4_VIDEO_ALL},
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
#if BUILDFLAG(ENABLE_MSE_MPEG2TS_STREAM_PARSER)
    {"video/mp2t", EME_CODEC_MP2T_VIDEO_ALL},
#endif  // BUILDFLAG(ENABLE_MSE_MPEG2TS_STREAM_PARSER)
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
};      // namespace media

struct NameToCodec {
  const char* name;
  EmeCodec codec;
};

// Mapping between codec names and enum values.
static const NameToCodec kCodecMap[] = {
    {"opus", EME_CODEC_WEBM_OPUS},      // Opus.
    {"vorbis", EME_CODEC_WEBM_VORBIS},  // Vorbis.
    {"vp8", EME_CODEC_WEBM_VP8},        // VP8.
    {"vp8.0", EME_CODEC_WEBM_VP8},      // VP8.
    {"vp9", EME_CODEC_WEBM_VP9},        // VP9.
    {"vp9.0", EME_CODEC_WEBM_VP9},      // VP9.
    {"vp09", EME_CODEC_COMMON_VP9},     // New multi-part VP9 for WebM and MP4.
    {"flac", EME_CODEC_MP4_FLAC},       // FLAC.
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
    {"mp4a", EME_CODEC_MP4_AAC},  // AAC.
#if BUILDFLAG(ENABLE_AC3_EAC3_AUDIO_DEMUXING)
    {"ac-3", EME_CODEC_MP4_AC3},   // AC3.
    {"ec-3", EME_CODEC_MP4_EAC3},  // EAC3.
#endif
#if BUILDFLAG(ENABLE_MPEG_H_AUDIO_DEMUXING)
    {"mhm1", EME_CODEC_MP4_MPEG_H_AUDIO},  // MPEG-H Audio.
#endif
    {"avc1", EME_CODEC_MP4_AVC1},  // AVC1 for MP4 and MP2T
    {"avc3", EME_CODEC_MP4_AVC1},  // AVC3 for MP4 and MP2T
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
    {"hev1", EME_CODEC_MP4_HEVC},  // HEV1.
    {"hvc1", EME_CODEC_MP4_HEVC},  // HVC1.
#endif
#if BUILDFLAG(ENABLE_DOLBY_VISION_DEMUXING)
    {"dva1", EME_CODEC_MP4_DV_AVC},  // DolbyVision AVC
    {"dvav", EME_CODEC_MP4_DV_AVC},  // DolbyVision AVC
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
    {"dvh1", EME_CODEC_MP4_DV_HEVC},  // DolbyVision HEVC
    {"dvhe", EME_CODEC_MP4_DV_HEVC},  // DolbyVision HEVC
#endif
#endif
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
};

class ClearKeyProperties : public KeySystemProperties {
 public:
  std::string GetKeySystemName() const override { return kClearKeyKeySystem; }

  bool IsSupportedInitDataType(EmeInitDataType init_data_type) const override {
    return init_data_type == EmeInitDataType::CENC ||
           init_data_type == EmeInitDataType::WEBM ||
           init_data_type == EmeInitDataType::KEYIDS;
  }

  bool IsEncryptionSchemeSupported(
      EncryptionMode encryption_scheme) const override {
    switch (encryption_scheme) {
      case EncryptionMode::kCenc:
      case EncryptionMode::kCbcs:
        return true;
      case EncryptionMode::kUnencrypted:
        break;
    }
    NOTREACHED();
    return false;
  }

  SupportedCodecs GetSupportedCodecs() const override {
    // On Android, Vorbis, VP8, AAC and AVC1 are supported in MediaCodec:
    // http://developer.android.com/guide/appendix/media-formats.html
    // VP9 support is device dependent.
    return EME_CODEC_WEBM_ALL | EME_CODEC_MP4_ALL;
  }

  EmeConfigRule GetRobustnessConfigRule(
      EmeMediaType media_type,
      const std::string& requested_robustness) const override {
    return requested_robustness.empty() ? EmeConfigRule::SUPPORTED
                                        : EmeConfigRule::NOT_SUPPORTED;
  }
  EmeSessionTypeSupport GetPersistentLicenseSessionSupport() const override {
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }
  EmeSessionTypeSupport GetPersistentReleaseMessageSessionSupport()
      const override {
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }
  EmeFeatureSupport GetPersistentStateSupport() const override {
    return EmeFeatureSupport::NOT_SUPPORTED;
  }
  EmeFeatureSupport GetDistinctiveIdentifierSupport() const override {
    return EmeFeatureSupport::NOT_SUPPORTED;
  }
  bool UseAesDecryptor() const override { return true; }
};

// Returns whether the |key_system| is known to Chromium and is thus likely to
// be implemented in an interoperable way.
// True is always returned for a |key_system| that begins with "x-".
//
// As with other web platform features, advertising support for a key system
// implies that it adheres to a defined and interoperable specification.
//
// To ensure interoperability, implementations of a specific |key_system| string
// must conform to a specification for that identifier that defines
// key system-specific behaviors not fully defined by the EME specification.
// That specification should be provided by the owner of the domain that is the
// reverse of the |key_system| string.
// This involves more than calling a library, SDK, or platform API.
// KeySystemsImpl must be populated appropriately, and there will likely be glue
// code to adapt to the API of the library, SDK, or platform API.
//
// Chromium mainline contains this data and glue code for specific key systems,
// which should help ensure interoperability with other implementations using
// these key systems.
//
// If you need to add support for other key systems, ensure that you have
// obtained the specification for how to integrate it with EME, implemented the
// appropriate glue/adapter code, and added all the appropriate data to
// KeySystemsImpl. Only then should you change this function.
static bool IsPotentiallySupportedKeySystem(const std::string& key_system) {
  // Known and supported key systems.
  if (key_system == kWidevineKeySystem)
    return true;
  if (key_system == kClearKeyKeySystem)
    return true;

  // External Clear Key is known and supports suffixes for testing.
  if (IsExternalClearKey(key_system))
    return true;

  // Chromecast defines behaviors for Cast clients within its reverse domain.
  const char kChromecastRoot[] = "com.chromecast";
  if (IsChildKeySystemOf(key_system, kChromecastRoot))
    return true;

  // Implementations that do not have a specification or appropriate glue code
  // can use the "x-" prefix to avoid conflicting with and advertising support
  // for real key system names. Use is discouraged.
  const char kExcludedPrefix[] = "x-";
  return base::StartsWith(key_system, kExcludedPrefix,
                          base::CompareCase::SENSITIVE);
}

class KeySystemsImpl : public KeySystems {
 public:
  static KeySystemsImpl* GetInstance();

  void UpdateIfNeeded();

  std::string GetKeySystemNameForUMA(const std::string& key_system) const;

  bool UseAesDecryptor(const std::string& key_system) const;

  // These two functions are for testing purpose only.
  void AddCodecMask(EmeMediaType media_type,
                    const std::string& codec,
                    uint32_t mask);
  void AddMimeTypeCodecMask(const std::string& mime_type, uint32_t mask);

  // Implementation of KeySystems interface.
  bool IsSupportedKeySystem(const std::string& key_system) const override;

  bool IsSupportedInitDataType(const std::string& key_system,
                               EmeInitDataType init_data_type) const override;

  bool IsEncryptionSchemeSupported(
      const std::string& key_system,
      EncryptionMode encryption_scheme) const override;

  EmeConfigRule GetContentTypeConfigRule(
      const std::string& key_system,
      EmeMediaType media_type,
      const std::string& container_mime_type,
      const std::vector<std::string>& codecs) const override;

  EmeConfigRule GetRobustnessConfigRule(
      const std::string& key_system,
      EmeMediaType media_type,
      const std::string& requested_robustness) const override;

  EmeSessionTypeSupport GetPersistentLicenseSessionSupport(
      const std::string& key_system) const override;

  EmeSessionTypeSupport GetPersistentReleaseMessageSessionSupport(
      const std::string& key_system) const override;

  EmeFeatureSupport GetPersistentStateSupport(
      const std::string& key_system) const override;

  EmeFeatureSupport GetDistinctiveIdentifierSupport(
      const std::string& key_system) const override;

 private:
  KeySystemsImpl();
  ~KeySystemsImpl() override;

  void InitializeUMAInfo();

  void UpdateSupportedKeySystems();

  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<KeySystemProperties>> key_systems);

  void RegisterMimeType(const std::string& mime_type, SupportedCodecs codecs);
  bool IsValidMimeTypeCodecsCombination(const std::string& mime_type,
                                        SupportedCodecs codecs) const;

  typedef base::hash_map<std::string, std::unique_ptr<KeySystemProperties>>
      KeySystemPropertiesMap;
  typedef base::hash_map<std::string, SupportedCodecs> MimeTypeToCodecsMap;
  typedef base::hash_map<std::string, EmeCodec> CodecMap;
  typedef base::hash_map<std::string, EmeInitDataType> InitDataTypesMap;

  // TODO(sandersd): Separate container enum from codec mask value.
  // http://crbug.com/417440
  // Potentially pass EmeMediaType and a container enum.
  SupportedCodecs GetCodecMaskForMimeType(
      const std::string& container_mime_type) const;
  EmeCodec GetCodecForString(const std::string& codec) const;

  // Map from key system string to KeySystemProperties instance.
  KeySystemPropertiesMap key_system_properties_map_;

  // This member should only be modified by RegisterMimeType().
  MimeTypeToCodecsMap mime_type_to_codecs_map_;
  CodecMap codec_map_;

  SupportedCodecs audio_codec_mask_;
  SupportedCodecs video_codec_mask_;

  // Makes sure all methods are called from the same thread.
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(KeySystemsImpl);
};

KeySystemsImpl* KeySystemsImpl::GetInstance() {
  static KeySystemsImpl* key_systems = new KeySystemsImpl();
  key_systems->UpdateIfNeeded();
  return key_systems;
}

// Because we use a thread-safe static, the key systems info must be populated
// when the instance is constructed.
KeySystemsImpl::KeySystemsImpl()
    : audio_codec_mask_(EME_CODEC_AUDIO_ALL),
      video_codec_mask_(EME_CODEC_VIDEO_ALL) {
  for (size_t i = 0; i < arraysize(kCodecMap); ++i) {
    const std::string& name = kCodecMap[i].name;
    DCHECK(!codec_map_.count(name));
    codec_map_[name] = kCodecMap[i].codec;
  }
  for (size_t i = 0; i < arraysize(kMimeTypeToCodecsMap); ++i) {
    RegisterMimeType(kMimeTypeToCodecsMap[i].mime_type,
                     kMimeTypeToCodecsMap[i].codecs);
  }

  // Always update supported key systems during construction.
  UpdateSupportedKeySystems();
}

KeySystemsImpl::~KeySystemsImpl() = default;

SupportedCodecs KeySystemsImpl::GetCodecMaskForMimeType(
    const std::string& container_mime_type) const {
  MimeTypeToCodecsMap::const_iterator iter =
      mime_type_to_codecs_map_.find(container_mime_type);
  if (iter == mime_type_to_codecs_map_.end())
    return EME_CODEC_NONE;

  DCHECK(IsValidMimeTypeCodecsCombination(container_mime_type, iter->second));
  return iter->second;
}

EmeCodec KeySystemsImpl::GetCodecForString(const std::string& codec) const {
  CodecMap::const_iterator iter = codec_map_.find(codec);
  if (iter != codec_map_.end())
    return iter->second;
  return EME_CODEC_NONE;
}

void KeySystemsImpl::UpdateIfNeeded() {
  if (GetMediaClient() && GetMediaClient()->IsKeySystemsUpdateNeeded())
    UpdateSupportedKeySystems();
}

void KeySystemsImpl::UpdateSupportedKeySystems() {
  DCHECK(thread_checker_.CalledOnValidThread());
  key_system_properties_map_.clear();

  std::vector<std::unique_ptr<KeySystemProperties>> key_systems_properties;

  // Add key systems supported by the MediaClient implementation.
  if (GetMediaClient()) {
    GetMediaClient()->AddSupportedKeySystems(&key_systems_properties);
  } else {
    DVLOG(1) << __func__ << " No media client to provide key systems";
  }

  // Clear Key is always supported.
  key_systems_properties.emplace_back(new ClearKeyProperties());

  AddSupportedKeySystems(std::move(key_systems_properties));
}

// Returns whether distinctive identifiers and persistent state can be reliably
// blocked for |properties| (and therefore be safely configurable).
static bool CanBlock(const KeySystemProperties& properties) {
  // When AesDecryptor is used, we are sure we can block.
  if (properties.UseAesDecryptor())
    return true;

  // For External Clear Key, it is either implemented as a library CDM (Clear
  // Key CDM), which is covered above, or by using AesDecryptor remotely, e.g.
  // via MojoCdm. In both cases, we can block. This is only used for testing.
  if (base::FeatureList::IsEnabled(media::kExternalClearKeyForTesting) &&
      IsExternalClearKey(properties.GetKeySystemName()))
    return true;

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  // When library CDMs are enabled, we are either using AesDecryptor, or using
  // the library CDM hosted in a sandboxed process. In both cases distinctive
  // identifiers and persistent state can be reliably blocked.
  return true;
#else
  // For other platforms assume the CDM can and will do anything. So we cannot
  // block.
  return false;
#endif
}

void KeySystemsImpl::AddSupportedKeySystems(
    std::vector<std::unique_ptr<KeySystemProperties>> key_systems) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(key_system_properties_map_.empty());

  for (auto& properties : key_systems) {
    DCHECK(!properties->GetKeySystemName().empty());
    DCHECK(properties->GetPersistentLicenseSessionSupport() !=
           EmeSessionTypeSupport::INVALID);
    DCHECK(properties->GetPersistentReleaseMessageSessionSupport() !=
           EmeSessionTypeSupport::INVALID);
    DCHECK(properties->GetPersistentStateSupport() !=
           EmeFeatureSupport::INVALID);
    DCHECK(properties->GetDistinctiveIdentifierSupport() !=
           EmeFeatureSupport::INVALID);

    if (!IsPotentiallySupportedKeySystem(properties->GetKeySystemName())) {
      // If you encounter this path, see the comments for the function above.
      DLOG(ERROR) << "Unsupported name '" << properties->GetKeySystemName()
                  << "'. See code comments.";
      continue;
    }

    // Supporting persistent state is a prerequisite for supporting persistent
    // sessions.
    if (properties->GetPersistentStateSupport() ==
        EmeFeatureSupport::NOT_SUPPORTED) {
      DCHECK(properties->GetPersistentLicenseSessionSupport() ==
             EmeSessionTypeSupport::NOT_SUPPORTED);
      DCHECK(properties->GetPersistentReleaseMessageSessionSupport() ==
             EmeSessionTypeSupport::NOT_SUPPORTED);
    }

    // persistent-release-message sessions are not currently supported.
    // http://crbug.com/448888
    DCHECK(properties->GetPersistentReleaseMessageSessionSupport() ==
           EmeSessionTypeSupport::NOT_SUPPORTED);

    // If distinctive identifiers are not supported, then no other features can
    // require them.
    if (properties->GetDistinctiveIdentifierSupport() ==
        EmeFeatureSupport::NOT_SUPPORTED) {
      DCHECK(properties->GetPersistentLicenseSessionSupport() !=
             EmeSessionTypeSupport::SUPPORTED_WITH_IDENTIFIER);
      DCHECK(properties->GetPersistentReleaseMessageSessionSupport() !=
             EmeSessionTypeSupport::SUPPORTED_WITH_IDENTIFIER);
    }

    if (!CanBlock(*properties)) {
      DCHECK(properties->GetDistinctiveIdentifierSupport() ==
             EmeFeatureSupport::ALWAYS_ENABLED);
      DCHECK(properties->GetPersistentStateSupport() ==
             EmeFeatureSupport::ALWAYS_ENABLED);
    }

    DCHECK_EQ(key_system_properties_map_.count(properties->GetKeySystemName()),
              0u)
        << "Key system '" << properties->GetKeySystemName()
        << "' already registered";

#if defined(OS_ANDROID)
    // Ensure that the renderer can access the decoders necessary to use the
    // key system.
    if (!properties->UseAesDecryptor() && !HasPlatformDecoderSupport()) {
      DLOG(WARNING) << properties->GetKeySystemName() << " not registered";
      continue;
    }
#endif  // defined(OS_ANDROID)

    DVLOG(1) << __func__
             << ": Adding key system:" << properties->GetKeySystemName();
    key_system_properties_map_[properties->GetKeySystemName()] =
        std::move(properties);
  }
}

// Adds the MIME type with the codec mask after verifying the validity.
// Only this function should modify |mime_type_to_codecs_map_|.
void KeySystemsImpl::RegisterMimeType(const std::string& mime_type,
                                      SupportedCodecs codecs) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!mime_type_to_codecs_map_.count(mime_type));
  DCHECK(IsValidMimeTypeCodecsCombination(mime_type, codecs))
      << ": mime_type = " << mime_type << ", codecs = " << codecs;

  mime_type_to_codecs_map_[mime_type] = codecs;
}

// Returns whether |mime_type| follows a valid format and the specified codecs
// are of the correct type based on |*_codec_mask_|.
// Only audio/ or video/ MIME types with their respective codecs are allowed.
bool KeySystemsImpl::IsValidMimeTypeCodecsCombination(
    const std::string& mime_type,
    SupportedCodecs codecs) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (codecs == EME_CODEC_NONE)
    return true;

  if (base::StartsWith(mime_type, "audio/", base::CompareCase::SENSITIVE))
    return !(codecs & ~audio_codec_mask_);
  if (base::StartsWith(mime_type, "video/", base::CompareCase::SENSITIVE))
    return !(codecs & ~video_codec_mask_);

  return false;
}

bool KeySystemsImpl::IsSupportedInitDataType(
    const std::string& key_system,
    EmeInitDataType init_data_type) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return false;
  }
  return key_system_iter->second->IsSupportedInitDataType(init_data_type);
}

bool KeySystemsImpl::IsEncryptionSchemeSupported(
    const std::string& key_system,
    EncryptionMode encryption_scheme) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return false;
  }
  return key_system_iter->second->IsEncryptionSchemeSupported(
      encryption_scheme);
}

std::string KeySystemsImpl::GetKeySystemNameForUMA(
    const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Here we maintain a short list of known key systems to facilitate UMA
  // reporting. Mentioned key systems are not necessarily supported by
  // the current platform.
  if (key_system == kWidevineKeySystem)
    return kWidevineKeySystemNameForUMA;

  if (key_system == kClearKeyKeySystem)
    return kClearKeyKeySystemNameForUMA;

  return kUnknownKeySystemNameForUMA;
}

bool KeySystemsImpl::UseAesDecryptor(const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    DLOG(ERROR) << key_system << " is not a known key system";
    return false;
  }
  return key_system_iter->second->UseAesDecryptor();
}

void KeySystemsImpl::AddCodecMask(EmeMediaType media_type,
                                  const std::string& codec,
                                  uint32_t mask) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!codec_map_.count(codec));
  codec_map_[codec] = static_cast<EmeCodec>(mask);
  if (media_type == EmeMediaType::AUDIO) {
    audio_codec_mask_ |= mask;
  } else {
    video_codec_mask_ |= mask;
  }
}

void KeySystemsImpl::AddMimeTypeCodecMask(const std::string& mime_type,
                                          uint32_t codecs_mask) {
  RegisterMimeType(mime_type, static_cast<EmeCodec>(codecs_mask));
}

bool KeySystemsImpl::IsSupportedKeySystem(const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!key_system_properties_map_.count(key_system))
    return false;

  return true;
}

EmeConfigRule KeySystemsImpl::GetContentTypeConfigRule(
    const std::string& key_system,
    EmeMediaType media_type,
    const std::string& container_mime_type,
    const std::vector<std::string>& codecs) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Make sure the container MIME type matches |media_type|.
  switch (media_type) {
    case EmeMediaType::AUDIO:
      if (!base::StartsWith(container_mime_type, "audio/",
                            base::CompareCase::SENSITIVE))
        return EmeConfigRule::NOT_SUPPORTED;
      break;
    case EmeMediaType::VIDEO:
      if (!base::StartsWith(container_mime_type, "video/",
                            base::CompareCase::SENSITIVE))
        return EmeConfigRule::NOT_SUPPORTED;
      break;
  }

  // Look up the key system's supported codecs.
  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return EmeConfigRule::NOT_SUPPORTED;
  }

  SupportedCodecs key_system_codec_mask =
      key_system_iter->second->GetSupportedCodecs();
#if defined(OS_ANDROID)
  SupportedCodecs key_system_secure_codec_mask =
      key_system_iter->second->GetSupportedSecureCodecs();
#endif  // defined(OS_ANDROID)

  // Check that the container is supported by the key system. (This check is
  // necessary because |codecs| may be empty.)
  SupportedCodecs mime_type_codec_mask =
      GetCodecMaskForMimeType(container_mime_type);
  if ((key_system_codec_mask & mime_type_codec_mask) == 0) {
    DVLOG(2) << " Container " << container_mime_type << " not supported by "
             << key_system;
    return EmeConfigRule::NOT_SUPPORTED;
  }

  // Check that the codecs are supported by the key system and container.
  EmeConfigRule support = EmeConfigRule::SUPPORTED;
  for (size_t i = 0; i < codecs.size(); i++) {
    SupportedCodecs codec = GetCodecForString(codecs[i]);
    if ((codec & key_system_codec_mask & mime_type_codec_mask) == 0) {
      DVLOG(2) << " Container/codec pair (" << container_mime_type << " / "
               << codecs[i] << ") not supported by " << key_system;
      return EmeConfigRule::NOT_SUPPORTED;
    }
#if defined(OS_ANDROID)
    // Check whether the codec supports a hardware-secure mode. The goal is to
    // prevent mixing of non-hardware-secure codecs with hardware-secure codecs,
    // since the mode is fixed at CDM creation.
    //
    // Because the check for regular codec support is early-exit, we don't have
    // to consider codecs that are only supported in hardware-secure mode. We
    // could do so, and make use of HW_SECURE_CODECS_REQUIRED, if it turns out
    // that hardware-secure-only codecs actually exist and are useful.
    if ((codec & key_system_secure_codec_mask) == 0)
      support = EmeConfigRule::HW_SECURE_CODECS_NOT_ALLOWED;
#endif  // defined(OS_ANDROID)
  }

  return support;
}

EmeConfigRule KeySystemsImpl::GetRobustnessConfigRule(
    const std::string& key_system,
    EmeMediaType media_type,
    const std::string& requested_robustness) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return EmeConfigRule::NOT_SUPPORTED;
  }
  return key_system_iter->second->GetRobustnessConfigRule(media_type,
                                                          requested_robustness);
}

EmeSessionTypeSupport KeySystemsImpl::GetPersistentLicenseSessionSupport(
    const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return EmeSessionTypeSupport::INVALID;
  }
  return key_system_iter->second->GetPersistentLicenseSessionSupport();
}

EmeSessionTypeSupport KeySystemsImpl::GetPersistentReleaseMessageSessionSupport(
    const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return EmeSessionTypeSupport::INVALID;
  }
  return key_system_iter->second->GetPersistentReleaseMessageSessionSupport();
}

EmeFeatureSupport KeySystemsImpl::GetPersistentStateSupport(
    const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return EmeFeatureSupport::INVALID;
  }
  return key_system_iter->second->GetPersistentStateSupport();
}

EmeFeatureSupport KeySystemsImpl::GetDistinctiveIdentifierSupport(
    const std::string& key_system) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  KeySystemPropertiesMap::const_iterator key_system_iter =
      key_system_properties_map_.find(key_system);
  if (key_system_iter == key_system_properties_map_.end()) {
    NOTREACHED();
    return EmeFeatureSupport::INVALID;
  }
  return key_system_iter->second->GetDistinctiveIdentifierSupport();
}

KeySystems* KeySystems::GetInstance() {
  return KeySystemsImpl::GetInstance();
}

//------------------------------------------------------------------------------

bool IsSupportedKeySystemWithInitDataType(const std::string& key_system,
                                          EmeInitDataType init_data_type) {
  return KeySystemsImpl::GetInstance()->IsSupportedInitDataType(key_system,
                                                                init_data_type);
}

std::string GetKeySystemNameForUMA(const std::string& key_system) {
  return KeySystemsImpl::GetInstance()->GetKeySystemNameForUMA(key_system);
}

bool CanUseAesDecryptor(const std::string& key_system) {
  return KeySystemsImpl::GetInstance()->UseAesDecryptor(key_system);
}

// These two functions are for testing purpose only. The declaration in the
// header file is guarded by "#if defined(UNIT_TEST)" so that they can be used
// by tests but not non-test code. However, this .cc file is compiled as part of
// "media" where "UNIT_TEST" is not defined. So we need to specify
// "MEDIA_EXPORT" here again so that they are visible to tests.

MEDIA_EXPORT void AddCodecMask(EmeMediaType media_type,
                               const std::string& codec,
                               uint32_t mask) {
  KeySystemsImpl::GetInstance()->AddCodecMask(media_type, codec, mask);
}

MEDIA_EXPORT void AddMimeTypeCodecMask(const std::string& mime_type,
                                       uint32_t mask) {
  KeySystemsImpl::GetInstance()->AddMimeTypeCodecMask(mime_type, mask);
}

}  // namespace media
