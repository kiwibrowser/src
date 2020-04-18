// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/cdm_adapter.h"

#include <stddef.h>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "components/crash/core/common/crash_key.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/callback_registry.h"
#include "media/base/cdm_initialized_promise.h"
#include "media/base/cdm_key_information.h"
#include "media/base/channel_layout.h"
#include "media/base/decoder_buffer.h"
#include "media/base/decrypt_config.h"
#include "media/base/key_systems.h"
#include "media/base/limits.h"
#include "media/base/sample_format.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_types.h"
#include "media/base/video_util.h"
#include "media/cdm/cdm_auxiliary_helper.h"
#include "media/cdm/cdm_helpers.h"
#include "media/cdm/cdm_wrapper.h"
#include "ui/gfx/geometry/rect.h"
#include "url/origin.h"

namespace media {

namespace {

// Constants for UMA reporting of file size (in KB) via
// UMA_HISTOGRAM_CUSTOM_COUNTS. Note that the histogram is log-scaled (rather
// than linear).
constexpr int kSizeKBMin = 1;
constexpr int kSizeKBMax = 512 * 1024;  // 512MB
constexpr int kSizeKBBuckets = 100;

// Only support version 1 of Storage Id. However, the "latest" version can also
// be requested.
constexpr uint32_t kRequestLatestStorageIdVersion = 0;
constexpr uint32_t kCurrentStorageIdVersion = 1;
static_assert(kCurrentStorageIdVersion < 0x80000000,
              "Versions 0x80000000 and above are reserved.");

cdm::HdcpVersion ToCdmHdcpVersion(HdcpVersion hdcp_version) {
  switch (hdcp_version) {
    case media::HdcpVersion::kHdcpVersionNone:
      return cdm::kHdcpVersionNone;
    case media::HdcpVersion::kHdcpVersion1_0:
      return cdm::kHdcpVersion1_0;
    case media::HdcpVersion::kHdcpVersion1_1:
      return cdm::kHdcpVersion1_1;
    case media::HdcpVersion::kHdcpVersion1_2:
      return cdm::kHdcpVersion1_2;
    case media::HdcpVersion::kHdcpVersion1_3:
      return cdm::kHdcpVersion1_3;
    case media::HdcpVersion::kHdcpVersion1_4:
      return cdm::kHdcpVersion1_4;
    case media::HdcpVersion::kHdcpVersion2_0:
      return cdm::kHdcpVersion2_0;
    case media::HdcpVersion::kHdcpVersion2_1:
      return cdm::kHdcpVersion2_1;
    case media::HdcpVersion::kHdcpVersion2_2:
      return cdm::kHdcpVersion2_2;
  }

  NOTREACHED();
  return cdm::kHdcpVersion2_2;
}

cdm::SessionType ToCdmSessionType(CdmSessionType session_type) {
  switch (session_type) {
    case CdmSessionType::TEMPORARY_SESSION:
      return cdm::kTemporary;
    case CdmSessionType::PERSISTENT_LICENSE_SESSION:
      return cdm::kPersistentLicense;
    case CdmSessionType::PERSISTENT_RELEASE_MESSAGE_SESSION:
      return cdm::kPersistentKeyRelease;
  }

  NOTREACHED() << "Unexpected session type: " << static_cast<int>(session_type);
  return cdm::kTemporary;
}

cdm::InitDataType ToCdmInitDataType(EmeInitDataType init_data_type) {
  switch (init_data_type) {
    case EmeInitDataType::CENC:
      return cdm::kCenc;
    case EmeInitDataType::KEYIDS:
      return cdm::kKeyIds;
    case EmeInitDataType::WEBM:
      return cdm::kWebM;
    case EmeInitDataType::UNKNOWN:
      break;
  }

  NOTREACHED();
  return cdm::kKeyIds;
}

CdmPromise::Exception ToMediaExceptionType(cdm::Exception exception) {
  switch (exception) {
    case cdm::kExceptionTypeError:
      return CdmPromise::Exception::TYPE_ERROR;
    case cdm::kExceptionNotSupportedError:
      return CdmPromise::Exception::NOT_SUPPORTED_ERROR;
    case cdm::kExceptionInvalidStateError:
      return CdmPromise::Exception::INVALID_STATE_ERROR;
    case cdm::kExceptionQuotaExceededError:
      return CdmPromise::Exception::QUOTA_EXCEEDED_ERROR;
  }

  NOTREACHED() << "Unexpected cdm::Exception " << exception;
  return CdmPromise::Exception::INVALID_STATE_ERROR;
}

CdmMessageType ToMediaMessageType(cdm::MessageType message_type) {
  switch (message_type) {
    case cdm::kLicenseRequest:
      return CdmMessageType::LICENSE_REQUEST;
    case cdm::kLicenseRenewal:
      return CdmMessageType::LICENSE_RENEWAL;
    case cdm::kLicenseRelease:
      return CdmMessageType::LICENSE_RELEASE;
    case cdm::kIndividualizationRequest:
      return CdmMessageType::INDIVIDUALIZATION_REQUEST;
  }

  NOTREACHED() << "Unexpected cdm::MessageType " << message_type;
  return CdmMessageType::LICENSE_REQUEST;
}

CdmKeyInformation::KeyStatus ToCdmKeyInformationKeyStatus(
    cdm::KeyStatus status) {
  switch (status) {
    case cdm::kUsable:
      return CdmKeyInformation::USABLE;
    case cdm::kInternalError:
      return CdmKeyInformation::INTERNAL_ERROR;
    case cdm::kExpired:
      return CdmKeyInformation::EXPIRED;
    case cdm::kOutputRestricted:
      return CdmKeyInformation::OUTPUT_RESTRICTED;
    case cdm::kOutputDownscaled:
      return CdmKeyInformation::OUTPUT_DOWNSCALED;
    case cdm::kStatusPending:
      return CdmKeyInformation::KEY_STATUS_PENDING;
    case cdm::kReleased:
      return CdmKeyInformation::RELEASED;
  }

  NOTREACHED() << "Unexpected cdm::KeyStatus " << status;
  return CdmKeyInformation::INTERNAL_ERROR;
}

cdm::AudioCodec ToCdmAudioCodec(AudioCodec codec) {
  switch (codec) {
    case kCodecVorbis:
      return cdm::kCodecVorbis;
    case kCodecAAC:
      return cdm::kCodecAac;
    default:
      DVLOG(1) << "Unsupported AudioCodec " << codec;
      return cdm::kUnknownAudioCodec;
  }
}

cdm::VideoCodec ToCdmVideoCodec(VideoCodec codec) {
  switch (codec) {
    case kCodecVP8:
      return cdm::kCodecVp8;
    case kCodecH264:
      return cdm::kCodecH264;
    case kCodecVP9:
      return cdm::kCodecVp9;
    default:
      DVLOG(1) << "Unsupported VideoCodec " << codec;
      return cdm::kUnknownVideoCodec;
  }
}

cdm::VideoCodecProfile ToCdmVideoCodecProfile(VideoCodecProfile profile) {
  switch (profile) {
    case VP8PROFILE_ANY:
    // TODO(servolk): See crbug.com/592074. We'll need to update this code to
    // handle different VP9 profiles properly after adding VP9 profiles in
    // media/cdm/api/content_decryption_module.h in a separate CL.
    // For now return kProfileNotNeeded to avoid breaking unit tests.
    case VP9PROFILE_PROFILE0:
    case VP9PROFILE_PROFILE1:
    case VP9PROFILE_PROFILE2:
    case VP9PROFILE_PROFILE3:
      return cdm::kProfileNotNeeded;
    case H264PROFILE_BASELINE:
      return cdm::kH264ProfileBaseline;
    case H264PROFILE_MAIN:
      return cdm::kH264ProfileMain;
    case H264PROFILE_EXTENDED:
      return cdm::kH264ProfileExtended;
    case H264PROFILE_HIGH:
      return cdm::kH264ProfileHigh;
    case H264PROFILE_HIGH10PROFILE:
      return cdm::kH264ProfileHigh10;
    case H264PROFILE_HIGH422PROFILE:
      return cdm::kH264ProfileHigh422;
    case H264PROFILE_HIGH444PREDICTIVEPROFILE:
      return cdm::kH264ProfileHigh444Predictive;
    default:
      DVLOG(1) << "Unsupported VideoCodecProfile " << profile;
      return cdm::kUnknownVideoCodecProfile;
  }
}

cdm::VideoFormat ToCdmVideoFormat(VideoPixelFormat format) {
  switch (format) {
    case PIXEL_FORMAT_YV12:
      return cdm::kYv12;
    case PIXEL_FORMAT_I420:
      return cdm::kI420;
    default:
      DVLOG(1) << "Unsupported VideoPixelFormat " << format;
      return cdm::kUnknownVideoFormat;
  }
}

cdm::StreamType ToCdmStreamType(Decryptor::StreamType stream_type) {
  switch (stream_type) {
    case Decryptor::kAudio:
      return cdm::kStreamTypeAudio;
    case Decryptor::kVideo:
      return cdm::kStreamTypeVideo;
  }

  NOTREACHED() << "Unexpected Decryptor::StreamType " << stream_type;
  return cdm::kStreamTypeVideo;
}

Decryptor::Status ToMediaDecryptorStatus(cdm::Status status) {
  switch (status) {
    case cdm::kSuccess:
      return Decryptor::kSuccess;
    case cdm::kNoKey:
      return Decryptor::kNoKey;
    case cdm::kNeedMoreData:
      return Decryptor::kNeedMoreData;
    case cdm::kDecryptError:
      return Decryptor::kError;
    case cdm::kDecodeError:
      return Decryptor::kError;
    case cdm::kInitializationError:
    case cdm::kDeferredInitialization:
      break;
  }

  NOTREACHED() << "Unexpected cdm::Status " << status;
  return Decryptor::kError;
}

inline std::ostream& operator<<(std::ostream& out, cdm::Status status) {
  switch (status) {
    case cdm::kSuccess:
      return out << "kSuccess";
    case cdm::kNoKey:
      return out << "kNoKey";
    case cdm::kNeedMoreData:
      return out << "kNeedMoreData";
    case cdm::kDecryptError:
      return out << "kDecryptError";
    case cdm::kDecodeError:
      return out << "kDecodeError";
    case cdm::kInitializationError:
      return out << "kInitializationError";
    case cdm::kDeferredInitialization:
      return out << "kDeferredInitialization";
  }
  NOTREACHED();
  return out << "Invalid Status!";
}

SampleFormat ToMediaSampleFormat(cdm::AudioFormat format) {
  switch (format) {
    case cdm::kAudioFormatU8:
      return kSampleFormatU8;
    case cdm::kAudioFormatS16:
      return kSampleFormatS16;
    case cdm::kAudioFormatS32:
      return kSampleFormatS32;
    case cdm::kAudioFormatF32:
      return kSampleFormatF32;
    case cdm::kAudioFormatPlanarS16:
      return kSampleFormatPlanarS16;
    case cdm::kAudioFormatPlanarF32:
      return kSampleFormatPlanarF32;
    case cdm::kUnknownAudioFormat:
      return kUnknownSampleFormat;
  }

  NOTREACHED() << "Unexpected cdm::AudioFormat " << format;
  return kUnknownSampleFormat;
}

cdm::EncryptionScheme ToCdmEncryptionScheme(const EncryptionScheme& scheme) {
  switch (scheme.mode()) {
    case EncryptionScheme::CIPHER_MODE_UNENCRYPTED:
      return cdm::EncryptionScheme::kUnencrypted;
    case EncryptionScheme::CIPHER_MODE_AES_CTR:
      if (!scheme.pattern().IsInEffect())
        return cdm::EncryptionScheme::kCenc;
      break;
    case EncryptionScheme::CIPHER_MODE_AES_CBC:
      // Pattern should be required for 'cbcs' but is currently optional.
      return cdm::EncryptionScheme::kCbcs;
  }

  NOTREACHED();
  return cdm::EncryptionScheme::kUnencrypted;
}

cdm::EncryptionScheme ToCdmEncryptionScheme(const EncryptionMode& mode) {
  switch (mode) {
    case EncryptionMode::kUnencrypted:
      return cdm::EncryptionScheme::kUnencrypted;
    case EncryptionMode::kCenc:
      return cdm::EncryptionScheme::kCenc;
    case EncryptionMode::kCbcs:
      return cdm::EncryptionScheme::kCbcs;
  }

  NOTREACHED();
  return cdm::EncryptionScheme::kUnencrypted;
}

// Verify that OutputProtection types matches those in CDM interface.
// Cannot use conversion function because these are used in bit masks.
#define ASSERT_ENUM_EQ(media_enum, cdm_enum)                              \
  static_assert(                                                          \
      static_cast<int32_t>(media_enum) == static_cast<int32_t>(cdm_enum), \
      "Mismatched enum: " #media_enum " != " #cdm_enum)

ASSERT_ENUM_EQ(OutputProtection::LinkTypes::NONE, cdm::kLinkTypeNone);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::UNKNOWN, cdm::kLinkTypeUnknown);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::INTERNAL, cdm::kLinkTypeInternal);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::VGA, cdm::kLinkTypeVGA);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::HDMI, cdm::kLinkTypeHDMI);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::DVI, cdm::kLinkTypeDVI);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::DISPLAYPORT,
               cdm::kLinkTypeDisplayPort);
ASSERT_ENUM_EQ(OutputProtection::LinkTypes::NETWORK, cdm::kLinkTypeNetwork);
ASSERT_ENUM_EQ(OutputProtection::ProtectionType::NONE, cdm::kProtectionNone);
ASSERT_ENUM_EQ(OutputProtection::ProtectionType::HDCP, cdm::kProtectionHDCP);

// Fill |input_buffer| based on the values in |encrypted|. |subsamples|
// is used to hold some of the data. |input_buffer| will contain pointers
// to data contained in |encrypted| and |subsamples|, so the lifetime of
// |input_buffer| must be <= the lifetime of |encrypted| and |subsamples|.
void ToCdmInputBuffer(const DecoderBuffer& encrypted_buffer,
                      std::vector<cdm::SubsampleEntry>* subsamples,
                      cdm::InputBuffer_2* input_buffer) {
  // End of stream buffers are represented as empty resources.
  DCHECK(!input_buffer->data);
  if (encrypted_buffer.end_of_stream())
    return;

  input_buffer->data = encrypted_buffer.data();
  input_buffer->data_size = encrypted_buffer.data_size();
  input_buffer->timestamp = encrypted_buffer.timestamp().InMicroseconds();

  const DecryptConfig* decrypt_config = encrypted_buffer.decrypt_config();
  if (!decrypt_config) {
    DVLOG(2) << __func__ << ": Clear buffer.";
    return;
  }

  input_buffer->key_id =
      reinterpret_cast<const uint8_t*>(decrypt_config->key_id().data());
  input_buffer->key_id_size = decrypt_config->key_id().size();
  input_buffer->iv =
      reinterpret_cast<const uint8_t*>(decrypt_config->iv().data());
  input_buffer->iv_size = decrypt_config->iv().size();

  DCHECK(subsamples->empty());
  size_t num_subsamples = decrypt_config->subsamples().size();
  if (num_subsamples > 0) {
    subsamples->reserve(num_subsamples);
    for (const auto& sample : decrypt_config->subsamples()) {
      subsamples->push_back({sample.clear_bytes, sample.cypher_bytes});
    }
  }

  input_buffer->subsamples = subsamples->data();
  input_buffer->num_subsamples = num_subsamples;

  input_buffer->encryption_scheme =
      ToCdmEncryptionScheme(decrypt_config->encryption_mode());
  if (decrypt_config->HasPattern()) {
    input_buffer->pattern = {
        decrypt_config->encryption_pattern()->crypt_byte_block(),
        decrypt_config->encryption_pattern()->skip_byte_block()};
  }
}

void* GetCdmHost(int host_interface_version, void* user_data) {
  if (!host_interface_version || !user_data)
    return nullptr;

  static_assert(
      CheckSupportedCdmHostVersions(cdm::Host_9::kVersion,
                                    cdm::Host_11::kVersion),
      "Mismatch between GetCdmHost() and IsSupportedCdmHostVersion()");

  DCHECK(IsSupportedCdmHostVersion(host_interface_version));

  CdmAdapter* cdm_adapter = static_cast<CdmAdapter*>(user_data);
  DVLOG(1) << "Create CDM Host with version " << host_interface_version;
  switch (host_interface_version) {
    case cdm::Host_9::kVersion:
      return static_cast<cdm::Host_9*>(cdm_adapter);
    case cdm::Host_10::kVersion:
      return static_cast<cdm::Host_10*>(cdm_adapter);
    case cdm::Host_11::kVersion:
      return static_cast<cdm::Host_11*>(cdm_adapter);
    default:
      NOTREACHED() << "Unexpected host interface version "
                   << host_interface_version;
      return nullptr;
  }
}

void ReportSystemCodeUMA(const std::string& key_system, uint32_t system_code) {
  base::UmaHistogramSparse(
      "Media.EME." + GetKeySystemNameForUMA(key_system) + ".SystemCode",
      system_code);
}

// These are reported to UMA server. Do not renumber or reuse values.
enum OutputProtectionStatus {
  kQueried = 0,
  kNoExternalLink = 1,
  kAllExternalLinksProtected = 2,
  // Note: Only add new values immediately before this line.
  kStatusCount
};

void ReportOutputProtectionUMA(OutputProtectionStatus status) {
  UMA_HISTOGRAM_ENUMERATION("Media.EME.OutputProtection", status,
                            OutputProtectionStatus::kStatusCount);
}

crash_reporter::CrashKeyString<256> g_origin_crash_key("cdm-origin");
using crash_reporter::ScopedCrashKeyString;

}  // namespace

// static
void CdmAdapter::Create(
    const std::string& key_system,
    const url::Origin& security_origin,
    const CdmConfig& cdm_config,
    CreateCdmFunc create_cdm_func,
    std::unique_ptr<CdmAuxiliaryHelper> helper,
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb,
    const CdmCreatedCB& cdm_created_cb) {
  DCHECK(!key_system.empty());
  DCHECK(!session_message_cb.is_null());
  DCHECK(!session_closed_cb.is_null());
  DCHECK(!session_keys_change_cb.is_null());
  DCHECK(!session_expiration_update_cb.is_null());

  scoped_refptr<CdmAdapter> cdm =
      new CdmAdapter(key_system, security_origin, cdm_config, create_cdm_func,
                     std::move(helper), session_message_cb, session_closed_cb,
                     session_keys_change_cb, session_expiration_update_cb);

  // |cdm| ownership passed to the promise.
  cdm->Initialize(std::make_unique<CdmInitializedPromise>(cdm_created_cb, cdm));
}

CdmAdapter::CdmAdapter(
    const std::string& key_system,
    const url::Origin& security_origin,
    const CdmConfig& cdm_config,
    CreateCdmFunc create_cdm_func,
    std::unique_ptr<CdmAuxiliaryHelper> helper,
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb)
    : key_system_(key_system),
      origin_string_(security_origin.Serialize()),
      cdm_config_(cdm_config),
      create_cdm_func_(create_cdm_func),
      helper_(std::move(helper)),
      session_message_cb_(session_message_cb),
      session_closed_cb_(session_closed_cb),
      session_keys_change_cb_(session_keys_change_cb),
      session_expiration_update_cb_(session_expiration_update_cb),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      pool_(new AudioBufferMemoryPool()),
      weak_factory_(this) {
  DVLOG(1) << __func__;

  DCHECK(!key_system_.empty());
  DCHECK(create_cdm_func_);
  DCHECK(helper_);
  DCHECK(session_message_cb_);
  DCHECK(session_closed_cb_);
  DCHECK(session_keys_change_cb_);
  DCHECK(session_expiration_update_cb_);

  helper_->SetFileReadCB(
      base::Bind(&CdmAdapter::OnFileRead, weak_factory_.GetWeakPtr()));
}

CdmAdapter::~CdmAdapter() {
  DVLOG(1) << __func__;

  // Reject any outstanding promises and close all the existing sessions.
  cdm_promise_adapter_.Clear();

  if (audio_init_cb_)
    audio_init_cb_.Run(false);
  if (video_init_cb_)
    video_init_cb_.Run(false);
}

CdmWrapper* CdmAdapter::CreateCdmInstance(const std::string& key_system) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  CdmWrapper* cdm = CdmWrapper::Create(create_cdm_func_, key_system.data(),
                                       key_system.size(), GetCdmHost, this);
  DVLOG(1) << "CDM instance for " + key_system + (cdm ? "" : " could not be") +
                  " created.";

  if (cdm) {
    // The interface version is relatively small. So using normal histogram
    // instead of a sparse histogram is okay. The following DCHECK asserts this.
    DCHECK(cdm->GetInterfaceVersion() <= 30);
    UMA_HISTOGRAM_ENUMERATION("Media.EME.CdmInterfaceVersion",
                              cdm->GetInterfaceVersion(), 30);
  }

  return cdm;
}

void CdmAdapter::Initialize(std::unique_ptr<media::SimpleCdmPromise> promise) {
  DVLOG(1) << __func__;

  cdm_.reset(CreateCdmInstance(key_system_));
  if (!cdm_) {
    promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                    "Unable to create CDM.");
    return;
  }

  init_promise_id_ = cdm_promise_adapter_.SavePromise(std::move(promise));

  if (!cdm_->Initialize(cdm_config_.allow_distinctive_identifier,
                        cdm_config_.allow_persistent_state,
                        cdm_config_.use_hw_secure_codecs)) {
    // OnInitialized() will not be called by the CDM, which is the case for
    // CDM interfaces prior to CDM_10.
    OnInitialized(true);
    return;
  }

  // OnInitialized() will be called by the CDM.
}

int CdmAdapter::GetInterfaceVersion() {
  return cdm_->GetInterfaceVersion();
}

void CdmAdapter::SetServerCertificate(
    const std::vector<uint8_t>& certificate,
    std::unique_ptr<SimpleCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (certificate.size() < limits::kMinCertificateLength ||
      certificate.size() > limits::kMaxCertificateLength) {
    promise->reject(CdmPromise::Exception::TYPE_ERROR, 0,
                    "Incorrect certificate.");
    return;
  }

  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  cdm_->SetServerCertificate(promise_id, certificate.data(),
                             certificate.size());
}

void CdmAdapter::GetStatusForPolicy(
    HdcpVersion min_hdcp_version,
    std::unique_ptr<KeyStatusCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  if (!cdm_->GetStatusForPolicy(promise_id,
                                ToCdmHdcpVersion(min_hdcp_version))) {
    cdm_promise_adapter_.RejectPromise(
        promise_id, CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
        "GetStatusForPolicy not supported.");
  }
}

void CdmAdapter::CreateSessionAndGenerateRequest(
    CdmSessionType session_type,
    EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data,
    std::unique_ptr<NewSessionCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  cdm_->CreateSessionAndGenerateRequest(
      promise_id, ToCdmSessionType(session_type),
      ToCdmInitDataType(init_data_type), init_data.data(), init_data.size());
}

void CdmAdapter::LoadSession(CdmSessionType session_type,
                             const std::string& session_id,
                             std::unique_ptr<NewSessionCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  cdm_->LoadSession(promise_id, ToCdmSessionType(session_type),
                    session_id.data(), session_id.size());
}

void CdmAdapter::UpdateSession(const std::string& session_id,
                               const std::vector<uint8_t>& response,
                               std::unique_ptr<SimpleCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!session_id.empty());
  DCHECK(!response.empty());

  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  cdm_->UpdateSession(promise_id, session_id.data(), session_id.size(),
                      response.data(), response.size());
}

void CdmAdapter::CloseSession(const std::string& session_id,
                              std::unique_ptr<SimpleCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!session_id.empty());

  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  cdm_->CloseSession(promise_id, session_id.data(), session_id.size());
}

void CdmAdapter::RemoveSession(const std::string& session_id,
                               std::unique_ptr<SimpleCdmPromise> promise) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!session_id.empty());

  uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
  cdm_->RemoveSession(promise_id, session_id.data(), session_id.size());
}

CdmContext* CdmAdapter::GetCdmContext() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return this;
}

std::unique_ptr<CallbackRegistration> CdmAdapter::RegisterNewKeyCB(
    base::RepeatingClosure new_key_cb) {
  NOTIMPLEMENTED();
  return nullptr;
}

Decryptor* CdmAdapter::GetDecryptor() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  // When using HW secure codecs, we cannot and should not use the CDM instance
  // to do decrypt and/or decode. Instead, we should use the CdmProxy.
  if (cdm_config_.use_hw_secure_codecs)
    return nullptr;

  return this;
}

int CdmAdapter::GetCdmId() const {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return helper_->GetCdmProxyCdmId();
}

void CdmAdapter::RegisterNewKeyCB(StreamType stream_type,
                                  const NewKeyCB& key_added_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  switch (stream_type) {
    case kAudio:
      new_audio_key_cb_ = key_added_cb;
      return;
    case kVideo:
      new_video_key_cb_ = key_added_cb;
      return;
  }

  NOTREACHED() << "Unexpected StreamType " << stream_type;
}

void CdmAdapter::Decrypt(StreamType stream_type,
                         scoped_refptr<DecoderBuffer> encrypted,
                         const DecryptCB& decrypt_cb) {
  DVLOG(3) << __func__ << ": " << encrypted->AsHumanReadableString();
  DCHECK(task_runner_->BelongsToCurrentThread());

  TRACE_EVENT0("media", "CdmAdapter::Decrypt");
  ScopedCrashKeyString scoped_crash_key(&g_origin_crash_key, origin_string_);

  cdm::InputBuffer_2 input_buffer = {};
  std::vector<cdm::SubsampleEntry> subsamples;
  std::unique_ptr<DecryptedBlockImpl> decrypted_block(new DecryptedBlockImpl());

  ToCdmInputBuffer(*encrypted, &subsamples, &input_buffer);
  cdm::Status status = cdm_->Decrypt(input_buffer, decrypted_block.get());

  if (status != cdm::kSuccess) {
    DVLOG(1) << __func__ << ": status = " << status;
    decrypt_cb.Run(ToMediaDecryptorStatus(status), nullptr);
    return;
  }

  scoped_refptr<DecoderBuffer> decrypted_buffer(
      DecoderBuffer::CopyFrom(decrypted_block->DecryptedBuffer()->Data(),
                              decrypted_block->DecryptedBuffer()->Size()));
  decrypted_buffer->set_timestamp(
      base::TimeDelta::FromMicroseconds(decrypted_block->Timestamp()));
  decrypt_cb.Run(Decryptor::kSuccess, std::move(decrypted_buffer));
}

void CdmAdapter::CancelDecrypt(StreamType stream_type) {
  // As the Decrypt methods are synchronous, nothing can be done here.
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void CdmAdapter::InitializeAudioDecoder(const AudioDecoderConfig& config,
                                        const DecoderInitCB& init_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(audio_init_cb_.is_null());

  cdm::AudioDecoderConfig_2 cdm_decoder_config = {};
  cdm_decoder_config.codec = ToCdmAudioCodec(config.codec());
  cdm_decoder_config.channel_count =
      ChannelLayoutToChannelCount(config.channel_layout());
  cdm_decoder_config.bits_per_channel = config.bits_per_channel();
  cdm_decoder_config.samples_per_second = config.samples_per_second();
  cdm_decoder_config.extra_data =
      const_cast<uint8_t*>(config.extra_data().data());
  cdm_decoder_config.extra_data_size = config.extra_data().size();
  cdm_decoder_config.encryption_scheme =
      ToCdmEncryptionScheme(config.encryption_scheme());

  cdm::Status status = cdm_->InitializeAudioDecoder(cdm_decoder_config);
  if (status != cdm::kSuccess && status != cdm::kDeferredInitialization) {
    DCHECK(status == cdm::kInitializationError);
    DVLOG(1) << __func__ << ": status = " << status;
    init_cb.Run(false);
    return;
  }

  audio_samples_per_second_ = config.samples_per_second();
  audio_channel_layout_ = config.channel_layout();

  if (status == cdm::kDeferredInitialization) {
    DVLOG(1) << "Deferred initialization in " << __func__;
    audio_init_cb_ = init_cb;
    return;
  }

  init_cb.Run(true);
}

void CdmAdapter::InitializeVideoDecoder(const VideoDecoderConfig& config,
                                        const DecoderInitCB& init_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(video_init_cb_.is_null());

  cdm::VideoDecoderConfig_2 cdm_decoder_config = {};
  cdm_decoder_config.codec = ToCdmVideoCodec(config.codec());
  cdm_decoder_config.profile = ToCdmVideoCodecProfile(config.profile());
  cdm_decoder_config.format = ToCdmVideoFormat(config.format());
  cdm_decoder_config.coded_size.width = config.coded_size().width();
  cdm_decoder_config.coded_size.height = config.coded_size().height();
  cdm_decoder_config.extra_data =
      const_cast<uint8_t*>(config.extra_data().data());
  cdm_decoder_config.extra_data_size = config.extra_data().size();
  cdm_decoder_config.encryption_scheme =
      ToCdmEncryptionScheme(config.encryption_scheme());

  cdm::Status status = cdm_->InitializeVideoDecoder(cdm_decoder_config);
  if (status != cdm::kSuccess && status != cdm::kDeferredInitialization) {
    DCHECK(status == cdm::kInitializationError);
    DVLOG(1) << __func__ << ": status = " << status;
    init_cb.Run(false);
    return;
  }

  pixel_aspect_ratio_ = config.GetPixelAspectRatio();

  if (status == cdm::kDeferredInitialization) {
    DVLOG(1) << "Deferred initialization in " << __func__;
    video_init_cb_ = init_cb;
    return;
  }

  init_cb.Run(true);
}

void CdmAdapter::DecryptAndDecodeAudio(scoped_refptr<DecoderBuffer> encrypted,
                                       const AudioDecodeCB& audio_decode_cb) {
  DVLOG(3) << __func__ << ": " << encrypted->AsHumanReadableString();
  DCHECK(task_runner_->BelongsToCurrentThread());

  TRACE_EVENT0("media", "CdmAdapter::DecryptAndDecodeAudio");
  ScopedCrashKeyString scoped_crash_key(&g_origin_crash_key, origin_string_);

  cdm::InputBuffer_2 input_buffer = {};
  std::vector<cdm::SubsampleEntry> subsamples;
  std::unique_ptr<AudioFramesImpl> audio_frames(new AudioFramesImpl());

  ToCdmInputBuffer(*encrypted, &subsamples, &input_buffer);
  cdm::Status status =
      cdm_->DecryptAndDecodeSamples(input_buffer, audio_frames.get());

  const Decryptor::AudioFrames empty_frames;
  if (status != cdm::kSuccess) {
    DVLOG(1) << __func__ << ": status = " << status;
    audio_decode_cb.Run(ToMediaDecryptorStatus(status), empty_frames);
    return;
  }

  Decryptor::AudioFrames audio_frame_list;
  DCHECK(audio_frames->FrameBuffer());
  if (!AudioFramesDataToAudioFrames(std::move(audio_frames),
                                    &audio_frame_list)) {
    DVLOG(1) << __func__ << " unable to convert Audio Frames";
    audio_decode_cb.Run(Decryptor::kError, empty_frames);
    return;
  }

  audio_decode_cb.Run(Decryptor::kSuccess, audio_frame_list);
}

void CdmAdapter::DecryptAndDecodeVideo(scoped_refptr<DecoderBuffer> encrypted,
                                       const VideoDecodeCB& video_decode_cb) {
  DVLOG(3) << __func__ << ": " << encrypted->AsHumanReadableString();
  DCHECK(task_runner_->BelongsToCurrentThread());

  TRACE_EVENT0("media", "CdmAdapter::DecryptAndDecodeVideo");
  ScopedCrashKeyString scoped_crash_key(&g_origin_crash_key, origin_string_);

  cdm::InputBuffer_2 input_buffer = {};
  std::vector<cdm::SubsampleEntry> subsamples;
  std::unique_ptr<VideoFrameImpl> video_frame = helper_->CreateCdmVideoFrame();

  ToCdmInputBuffer(*encrypted, &subsamples, &input_buffer);
  cdm::Status status =
      cdm_->DecryptAndDecodeFrame(input_buffer, video_frame.get());

  if (status != cdm::kSuccess) {
    DVLOG(1) << __func__ << ": status = " << status;
    video_decode_cb.Run(ToMediaDecryptorStatus(status), nullptr);
    return;
  }

  gfx::Rect visible_rect(video_frame->Size().width, video_frame->Size().height);
  scoped_refptr<VideoFrame> decoded_frame = video_frame->TransformToVideoFrame(
      GetNaturalSize(visible_rect, pixel_aspect_ratio_));
  if (!decoded_frame) {
    DLOG(ERROR) << __func__ << ": TransformToVideoFrame failed.";
    video_decode_cb.Run(Decryptor::kError, nullptr);
    return;
  }

  video_decode_cb.Run(Decryptor::kSuccess, decoded_frame);
}

void CdmAdapter::ResetDecoder(StreamType stream_type) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_->ResetDecoder(ToCdmStreamType(stream_type));
}

void CdmAdapter::DeinitializeDecoder(StreamType stream_type) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_->DeinitializeDecoder(ToCdmStreamType(stream_type));

  // Reset the saved values from initializing the decoder.
  switch (stream_type) {
    case Decryptor::kAudio:
      audio_samples_per_second_ = 0;
      audio_channel_layout_ = CHANNEL_LAYOUT_NONE;
      break;
    case Decryptor::kVideo:
      pixel_aspect_ratio_ = 0.0;
      break;
  }
}

cdm::Buffer* CdmAdapter::Allocate(uint32_t capacity) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return helper_->CreateCdmBuffer(capacity);
}

void CdmAdapter::SetTimer(int64_t delay_ms, void* context) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  task_runner_->PostDelayedTask(FROM_HERE,
                                base::Bind(&CdmAdapter::TimerExpired,
                                           weak_factory_.GetWeakPtr(), context),
                                base::TimeDelta::FromMilliseconds(delay_ms));
}

void CdmAdapter::TimerExpired(void* context) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_->TimerExpired(context);
}

cdm::Time CdmAdapter::GetCurrentWallTime() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return base::Time::Now().ToDoubleT();
}

void CdmAdapter::OnResolveKeyStatusPromise(uint32_t promise_id,
                                           cdm::KeyStatus key_status) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_promise_adapter_.ResolvePromise(promise_id,
                                      ToCdmKeyInformationKeyStatus(key_status));
}

void CdmAdapter::OnResolvePromise(uint32_t promise_id) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_promise_adapter_.ResolvePromise(promise_id);
}

void CdmAdapter::OnResolveNewSessionPromise(uint32_t promise_id,
                                            const char* session_id,
                                            uint32_t session_id_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_promise_adapter_.ResolvePromise(promise_id,
                                      std::string(session_id, session_id_size));
}

void CdmAdapter::OnRejectPromise(uint32_t promise_id,
                                 cdm::Exception exception,
                                 uint32_t system_code,
                                 const char* error_message,
                                 uint32_t error_message_size) {
  // This is the central place for library CDM promise rejection. Cannot report
  // this in more generic classes like CdmPromise or CdmPromiseAdapter because
  // they may be used multiple times in one promise chain that involves IPC.
  ReportSystemCodeUMA(key_system_, system_code);

  // UMA to help track file related errors. See http://crbug.com/410630
  if (system_code == 0x27) {
    UMA_HISTOGRAM_CUSTOM_COUNTS("Media.EME.CdmFileIO.FileSizeKBOnError",
                                last_read_file_size_kb_, kSizeKBMin, kSizeKBMax,
                                kSizeKBBuckets);
  }

  DCHECK(task_runner_->BelongsToCurrentThread());
  cdm_promise_adapter_.RejectPromise(
      promise_id, ToMediaExceptionType(exception), system_code,
      std::string(error_message, error_message_size));
}

void CdmAdapter::OnSessionMessage(const char* session_id,
                                  uint32_t session_id_size,
                                  cdm::MessageType message_type,
                                  const char* message,
                                  uint32_t message_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  const uint8_t* message_ptr = reinterpret_cast<const uint8_t*>(message);
  session_message_cb_.Run(
      std::string(session_id, session_id_size),
      ToMediaMessageType(message_type),
      std::vector<uint8_t>(message_ptr, message_ptr + message_size));
}

void CdmAdapter::OnSessionKeysChange(const char* session_id,
                                     uint32_t session_id_size,
                                     bool has_additional_usable_key,
                                     const cdm::KeyInformation* keys_info,
                                     uint32_t keys_info_count) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  CdmKeysInfo keys;
  keys.reserve(keys_info_count);
  for (uint32_t i = 0; i < keys_info_count; ++i) {
    const auto& info = keys_info[i];
    keys.push_back(std::make_unique<CdmKeyInformation>(
        info.key_id, info.key_id_size,
        ToCdmKeyInformationKeyStatus(info.status), info.system_code));
  }

  // TODO(jrummell): Handling resume playback should be done in the media
  // player, not in the Decryptors. http://crbug.com/413413.
  if (has_additional_usable_key) {
    if (!new_audio_key_cb_.is_null())
      new_audio_key_cb_.Run();
    if (!new_video_key_cb_.is_null())
      new_video_key_cb_.Run();
  }

  session_keys_change_cb_.Run(std::string(session_id, session_id_size),
                              has_additional_usable_key, std::move(keys));
}

void CdmAdapter::OnExpirationChange(const char* session_id,
                                    uint32_t session_id_size,
                                    cdm::Time new_expiry_time) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  session_expiration_update_cb_.Run(std::string(session_id, session_id_size),
                                    base::Time::FromDoubleT(new_expiry_time));
}

void CdmAdapter::OnSessionClosed(const char* session_id,
                                 uint32_t session_id_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  session_closed_cb_.Run(std::string(session_id, session_id_size));
}

void CdmAdapter::SendPlatformChallenge(const char* service_id,
                                       uint32_t service_id_size,
                                       const char* challenge,
                                       uint32_t challenge_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!cdm_config_.allow_distinctive_identifier) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindRepeating(&CdmAdapter::OnChallengePlatformDone,
                            weak_factory_.GetWeakPtr(), false, "", "", ""));
    return;
  }

  helper_->ChallengePlatform(std::string(service_id, service_id_size),
                             std::string(challenge, challenge_size),
                             base::Bind(&CdmAdapter::OnChallengePlatformDone,
                                        weak_factory_.GetWeakPtr()));
}

void CdmAdapter::OnChallengePlatformDone(
    bool success,
    const std::string& signed_data,
    const std::string& signed_data_signature,
    const std::string& platform_key_certificate) {
  cdm::PlatformChallengeResponse platform_challenge_response = {};
  if (success) {
    platform_challenge_response.signed_data =
        reinterpret_cast<const uint8_t*>(signed_data.data());
    platform_challenge_response.signed_data_length = signed_data.length();
    platform_challenge_response.signed_data_signature =
        reinterpret_cast<const uint8_t*>(signed_data_signature.data());
    platform_challenge_response.signed_data_signature_length =
        signed_data_signature.length();
    platform_challenge_response.platform_key_certificate =
        reinterpret_cast<const uint8_t*>(platform_key_certificate.data());
    platform_challenge_response.platform_key_certificate_length =
        platform_key_certificate.length();
  }

  cdm_->OnPlatformChallengeResponse(platform_challenge_response);
}

void CdmAdapter::EnableOutputProtection(uint32_t desired_protection_mask) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  helper_->EnableProtection(
      desired_protection_mask,
      base::BindOnce(&CdmAdapter::OnEnableOutputProtectionDone,
                     weak_factory_.GetWeakPtr()));
}

void CdmAdapter::OnEnableOutputProtectionDone(bool success) {
  // CDM needs to call QueryOutputProtectionStatus() to see if it took effect
  // or not.
  DVLOG(1) << __func__ << ": success = " << success;
}

void CdmAdapter::QueryOutputProtectionStatus() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  ReportOutputProtectionQuery();
  helper_->QueryStatus(
      base::Bind(&CdmAdapter::OnQueryOutputProtectionStatusDone,
                 weak_factory_.GetWeakPtr()));
}

void CdmAdapter::OnQueryOutputProtectionStatusDone(bool success,
                                                   uint32_t link_mask,
                                                   uint32_t protection_mask) {
  // The bit mask definition must be consistent between media::OutputProtection
  // and cdm::ContentDecryptionModule* interfaces. This is statically asserted
  // by ASSERT_ENUM_EQs above.

  // Return a query status of failure on error.
  cdm::QueryResult query_result;
  if (success) {
    query_result = cdm::kQuerySucceeded;
    ReportOutputProtectionQueryResult(link_mask, protection_mask);
  } else {
    DVLOG(1) << __func__ << ": query output protection status failed";
    query_result = cdm::kQueryFailed;
  }

  cdm_->OnQueryOutputProtectionStatus(query_result, link_mask, protection_mask);
}

void CdmAdapter::ReportOutputProtectionQuery() {
  if (uma_for_output_protection_query_reported_)
    return;

  ReportOutputProtectionUMA(OutputProtectionStatus::kQueried);
  uma_for_output_protection_query_reported_ = true;
}

void CdmAdapter::ReportOutputProtectionQueryResult(uint32_t link_mask,
                                                   uint32_t protection_mask) {
  DCHECK(uma_for_output_protection_query_reported_);

  if (uma_for_output_protection_positive_result_reported_)
    return;

  // Report UMAs for output protection query result.

  uint32_t external_links = (link_mask & ~cdm::kLinkTypeInternal);

  if (!external_links) {
    ReportOutputProtectionUMA(OutputProtectionStatus::kNoExternalLink);
    uma_for_output_protection_positive_result_reported_ = true;
    return;
  }

  const uint32_t kProtectableLinks =
      cdm::kLinkTypeHDMI | cdm::kLinkTypeDVI | cdm::kLinkTypeDisplayPort;
  bool is_unprotectable_link_connected =
      (external_links & ~kProtectableLinks) != 0;
  bool is_hdcp_enabled_on_all_protectable_links =
      (protection_mask & cdm::kProtectionHDCP) != 0;

  if (!is_unprotectable_link_connected &&
      is_hdcp_enabled_on_all_protectable_links) {
    ReportOutputProtectionUMA(
        OutputProtectionStatus::kAllExternalLinksProtected);
    uma_for_output_protection_positive_result_reported_ = true;
    return;
  }

  // Do not report a negative result because it could be a false negative.
  // Instead, we will calculate number of negatives using the total number of
  // queries and positive results.
}

void CdmAdapter::OnDeferredInitializationDone(cdm::StreamType stream_type,
                                              cdm::Status decoder_status) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DVLOG_IF(1, decoder_status != cdm::kSuccess)
      << __func__ << ": status = " << decoder_status;

  switch (stream_type) {
    case cdm::kStreamTypeAudio:
      base::ResetAndReturn(&audio_init_cb_)
          .Run(decoder_status == cdm::kSuccess);
      return;
    case cdm::kStreamTypeVideo:
      base::ResetAndReturn(&video_init_cb_)
          .Run(decoder_status == cdm::kSuccess);
      return;
  }

  NOTREACHED() << "Unexpected cdm::StreamType " << stream_type;
}

cdm::FileIO* CdmAdapter::CreateFileIO(cdm::FileIOClient* client) {
  DVLOG(3) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!cdm_config_.allow_persistent_state) {
    DVLOG(1) << __func__ << ": Persistent state not allowed.";
    return nullptr;
  }

  return helper_->CreateCdmFileIO(client);
}

void CdmAdapter::RequestStorageId(uint32_t version) {
  if (!cdm_config_.allow_persistent_state ||
      !(version == kCurrentStorageIdVersion ||
        version == kRequestLatestStorageIdVersion)) {
    DVLOG(1) << __func__ << ": Persistent state not allowed ("
             << cdm_config_.allow_persistent_state
             << ") or invalid storage ID version (" << version << ").";
    task_runner_->PostTask(
        FROM_HERE, base::BindRepeating(&CdmAdapter::OnStorageIdObtained,
                                       weak_factory_.GetWeakPtr(), version,
                                       std::vector<uint8_t>()));
    return;
  }

  helper_->GetStorageId(version, base::Bind(&CdmAdapter::OnStorageIdObtained,
                                            weak_factory_.GetWeakPtr()));
}

void CdmAdapter::OnInitialized(bool success) {
  DVLOG(3) << __func__ << ": success = " << success;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_NE(init_promise_id_, CdmPromiseAdapter::kInvalidPromiseId);

  if (!success) {
    cdm_promise_adapter_.RejectPromise(
        init_promise_id_, CdmPromise::Exception::INVALID_STATE_ERROR, 0,
        "Unable to create CDM.");
  } else {
    cdm_promise_adapter_.ResolvePromise(init_promise_id_);
  }

  init_promise_id_ = CdmPromiseAdapter::kInvalidPromiseId;
}

cdm::CdmProxy* CdmAdapter::RequestCdmProxy(cdm::CdmProxyClient* client) {
  DVLOG(3) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());

  // CdmProxy should only be created once, at CDM initialization time.
  if (cdm_proxy_created_ ||
      init_promise_id_ == CdmPromiseAdapter::kInvalidPromiseId) {
    DVLOG(1) << __func__
             << ": CdmProxy can only be created once, and must be created "
                "during CDM initialization.";
    return nullptr;
  }

  cdm_proxy_created_ = true;
  return helper_->CreateCdmProxy(client);
}

void CdmAdapter::OnStorageIdObtained(uint32_t version,
                                     const std::vector<uint8_t>& storage_id) {
  cdm_->OnStorageId(version, storage_id.data(), storage_id.size());
}

bool CdmAdapter::AudioFramesDataToAudioFrames(
    std::unique_ptr<AudioFramesImpl> audio_frames,
    Decryptor::AudioFrames* result_frames) {
  const uint8_t* data = audio_frames->FrameBuffer()->Data();
  const size_t data_size = audio_frames->FrameBuffer()->Size();
  size_t bytes_left = data_size;
  const SampleFormat sample_format =
      ToMediaSampleFormat(audio_frames->Format());
  const int audio_channel_count =
      ChannelLayoutToChannelCount(audio_channel_layout_);
  const int audio_bytes_per_frame =
      SampleFormatToBytesPerChannel(sample_format) * audio_channel_count;
  if (audio_bytes_per_frame <= 0)
    return false;

  // Allocate space for the channel pointers given to AudioBuffer.
  std::vector<const uint8_t*> channel_ptrs(audio_channel_count, nullptr);
  do {
    // AudioFrames can contain multiple audio output buffers, which are
    // serialized into this format:
    // |<------------------- serialized audio buffer ------------------->|
    // | int64_t timestamp | int64_t length | length bytes of audio data |
    int64_t timestamp = 0;
    int64_t frame_size = -1;
    const size_t kHeaderSize = sizeof(timestamp) + sizeof(frame_size);
    if (bytes_left < kHeaderSize)
      return false;

    memcpy(&timestamp, data, sizeof(timestamp));
    memcpy(&frame_size, data + sizeof(timestamp), sizeof(frame_size));
    data += kHeaderSize;
    bytes_left -= kHeaderSize;

    // We should *not* have empty frames in the list.
    if (frame_size <= 0 ||
        bytes_left < base::checked_cast<size_t>(frame_size)) {
      return false;
    }

    // Setup channel pointers.  AudioBuffer::CopyFrom() will only use the first
    // one in the case of interleaved data.
    const int size_per_channel = frame_size / audio_channel_count;
    for (int i = 0; i < audio_channel_count; ++i)
      channel_ptrs[i] = data + i * size_per_channel;

    const int frame_count = frame_size / audio_bytes_per_frame;
    scoped_refptr<media::AudioBuffer> frame = media::AudioBuffer::CopyFrom(
        sample_format, audio_channel_layout_, audio_channel_count,
        audio_samples_per_second_, frame_count, &channel_ptrs[0],
        base::TimeDelta::FromMicroseconds(timestamp), pool_);
    result_frames->push_back(frame);

    data += frame_size;
    bytes_left -= frame_size;
  } while (bytes_left > 0);

  return true;
}

void CdmAdapter::OnFileRead(int file_size_bytes) {
  DCHECK_GE(file_size_bytes, 0);
  last_read_file_size_kb_ = file_size_bytes / 1024;

  if (file_size_uma_reported_)
    return;

  UMA_HISTOGRAM_CUSTOM_COUNTS("Media.EME.CdmFileIO.FileSizeKBOnFirstRead",
                              last_read_file_size_kb_, kSizeKBMin, kSizeKBMax,
                              kSizeKBBuckets);
  file_size_uma_reported_ = true;
}

}  // namespace media
