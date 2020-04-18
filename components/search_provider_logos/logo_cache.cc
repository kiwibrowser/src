// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_provider_logos/logo_cache.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace search_provider_logos {

namespace {

// The cached logo metadata is persisted as JSON using these keys.
const char kSourceUrlKey[] = "url";
const char kExpirationTimeKey[] = "expiration_time";
const char kCanShowAfterExpirationKey[] = "can_show_after_expiration";
const char kFingerprintKey[] = "fingerprint";
const char kTypeKey[] = "type";
const char kOnClickURLKey[] = "on_click_url";
const char kFullPageURLKey[] = "full_page_url";
const char kAltTextKey[] = "alt_text";
const char kMimeTypeKey[] = "mime_type";
const char kNumBytesKey[] = "num_bytes";
const char kAnimatedUrlKey[] = "animated_url";
const char kLogUrlKey[] = "log_url";
const char kCtaLogUrlKey[] = "cta_log_url";

const char kSimpleType[] = "SIMPLE";
const char kAnimatedType[] = "ANIMATED";
const char kInteractiveType[] = "INTERACTIVE";

bool GetTimeValue(const base::DictionaryValue& dict,
                  const std::string& key,
                  base::Time* time) {
  std::string str;
  int64_t internal_time_value;
  if (dict.GetString(key, &str) &&
      base::StringToInt64(str, &internal_time_value)) {
    *time = base::Time::FromInternalValue(internal_time_value);
    return true;
  }
  return false;
}

void SetTimeValue(base::DictionaryValue& dict,
                  const std::string& key,
                  const base::Time& time) {
  int64_t internal_time_value = time.ToInternalValue();
  dict.SetString(key, base::Int64ToString(internal_time_value));
}

LogoType LogoTypeFromString(base::StringPiece type) {
  if (type == kSimpleType) {
    return LogoType::SIMPLE;
  }
  if (type == kAnimatedType) {
    return LogoType::ANIMATED;
  }
  if (type == kInteractiveType) {
    return LogoType::INTERACTIVE;
  }
  LOG(WARNING) << "invalid type " << type;
  return LogoType::SIMPLE;
}

std::string LogoTypeToString(LogoType type) {
  switch (type) {
    case LogoType::SIMPLE:
      return kSimpleType;
    case LogoType::ANIMATED:
      return kAnimatedType;
    case LogoType::INTERACTIVE:
      return kInteractiveType;
  }
  NOTREACHED();
  return "";
}

}  // namespace

LogoCache::LogoCache(const base::FilePath& cache_directory)
    : cache_directory_(cache_directory),
      metadata_is_valid_(false) {
  // The LogoCache can be constructed on any thread, as long as it's used
  // on a single sequence after construction.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

LogoCache::~LogoCache() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void LogoCache::UpdateCachedLogoMetadata(const LogoMetadata& metadata) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(metadata_);
  DCHECK_EQ(metadata_->fingerprint, metadata.fingerprint);

  UpdateMetadata(std::make_unique<LogoMetadata>(metadata));
  WriteMetadata();
}

const LogoMetadata* LogoCache::GetCachedLogoMetadata() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ReadMetadataIfNeeded();
  return metadata_.get();
}

void LogoCache::SetCachedLogo(const EncodedLogo* logo) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<LogoMetadata> metadata;
  if (logo) {
    metadata = std::make_unique<LogoMetadata>(logo->metadata);
    logo_num_bytes_ = static_cast<int>(logo->encoded_image->size());
  }
  UpdateMetadata(std::move(metadata));
  WriteLogo(logo ? logo->encoded_image : nullptr);
}

std::unique_ptr<EncodedLogo> LogoCache::GetCachedLogo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  ReadMetadataIfNeeded();
  if (!metadata_)
    return nullptr;

  scoped_refptr<base::RefCountedString> encoded_image =
      new base::RefCountedString();
  if (!base::ReadFileToString(GetLogoPath(), &encoded_image->data())) {
    UpdateMetadata(nullptr);
    return nullptr;
  }

  if (encoded_image->size() != static_cast<size_t>(logo_num_bytes_)) {
    // Delete corrupt metadata and logo.
    DeleteLogoAndMetadata();
    UpdateMetadata(nullptr);
    return nullptr;
  }

  std::unique_ptr<EncodedLogo> logo(new EncodedLogo());
  logo->encoded_image = encoded_image;
  logo->metadata = *metadata_;
  return logo;
}

// static
std::unique_ptr<LogoMetadata> LogoCache::LogoMetadataFromString(
    const std::string& str,
    int* logo_num_bytes) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(str);
  base::DictionaryValue* dict;
  if (!value || !value->GetAsDictionary(&dict))
    return nullptr;

  std::unique_ptr<LogoMetadata> metadata(new LogoMetadata());
  std::string source_url;
  std::string type;
  std::string on_click_url;
  std::string full_page_url;
  std::string animated_url;
  std::string log_url;
  std::string cta_log_url;
  if (!dict->GetString(kSourceUrlKey, &source_url) ||
      !dict->GetString(kFingerprintKey, &metadata->fingerprint) ||
      !dict->GetString(kTypeKey, &type) ||
      !dict->GetString(kOnClickURLKey, &on_click_url) ||
      !dict->GetString(kFullPageURLKey, &full_page_url) ||
      !dict->GetString(kAltTextKey, &metadata->alt_text) ||
      !dict->GetString(kAnimatedUrlKey, &animated_url) ||
      !dict->GetString(kLogUrlKey, &log_url) ||
      !dict->GetString(kCtaLogUrlKey, &cta_log_url) ||
      !dict->GetString(kMimeTypeKey, &metadata->mime_type) ||
      !dict->GetBoolean(kCanShowAfterExpirationKey,
                        &metadata->can_show_after_expiration) ||
      !dict->GetInteger(kNumBytesKey, logo_num_bytes) ||
      !GetTimeValue(*dict, kExpirationTimeKey, &metadata->expiration_time)) {
    return nullptr;
  }
  metadata->type = LogoTypeFromString(type);
  metadata->source_url = GURL(source_url);
  metadata->on_click_url = GURL(on_click_url);
  metadata->full_page_url = GURL(full_page_url);
  metadata->animated_url = GURL(animated_url);
  metadata->log_url = GURL(log_url);
  metadata->cta_log_url = GURL(cta_log_url);

  return metadata;
}

// static
void LogoCache::LogoMetadataToString(const LogoMetadata& metadata,
                                     int num_bytes,
                                     std::string* str) {
  base::DictionaryValue dict;
  dict.SetString(kSourceUrlKey, metadata.source_url.spec());
  dict.SetString(kFingerprintKey, metadata.fingerprint);
  dict.SetString(kTypeKey, LogoTypeToString(metadata.type));
  dict.SetString(kOnClickURLKey, metadata.on_click_url.spec());
  dict.SetString(kFullPageURLKey, metadata.full_page_url.spec());
  dict.SetString(kAltTextKey, metadata.alt_text);
  dict.SetString(kAnimatedUrlKey, metadata.animated_url.spec());
  dict.SetString(kLogUrlKey, metadata.log_url.spec());
  dict.SetString(kCtaLogUrlKey, metadata.cta_log_url.spec());
  dict.SetString(kMimeTypeKey, metadata.mime_type);
  dict.SetBoolean(kCanShowAfterExpirationKey,
                  metadata.can_show_after_expiration);
  dict.SetInteger(kNumBytesKey, num_bytes);
  SetTimeValue(dict, kExpirationTimeKey, metadata.expiration_time);
  base::JSONWriter::Write(dict, str);
}

base::FilePath LogoCache::GetLogoPath() {
  return cache_directory_.Append(FILE_PATH_LITERAL("logo"));
}

base::FilePath LogoCache::GetMetadataPath() {
  return cache_directory_.Append(FILE_PATH_LITERAL("metadata"));
}

void LogoCache::UpdateMetadata(std::unique_ptr<LogoMetadata> metadata) {
  metadata_ = std::move(metadata);
  metadata_is_valid_ = true;
}

void LogoCache::ReadMetadataIfNeeded() {
  if (metadata_is_valid_)
    return;

  std::unique_ptr<LogoMetadata> metadata;
  base::FilePath metadata_path = GetMetadataPath();
  std::string str;
  if (base::ReadFileToString(metadata_path, &str)) {
    metadata = LogoMetadataFromString(str, &logo_num_bytes_);
    if (!metadata) {
      // Delete corrupt metadata and logo.
      DeleteLogoAndMetadata();
    }
  }

  UpdateMetadata(std::move(metadata));
}

void LogoCache::WriteMetadata() {
  if (!EnsureCacheDirectoryExists())
    return;

  std::string str;
  LogoMetadataToString(*metadata_, logo_num_bytes_, &str);
  base::WriteFile(GetMetadataPath(), str.data(), static_cast<int>(str.size()));
}

void LogoCache::WriteLogo(scoped_refptr<base::RefCountedMemory> encoded_image) {
  if (!EnsureCacheDirectoryExists())
    return;

  if (!metadata_ || !encoded_image) {
    DeleteLogoAndMetadata();
    return;
  }

  // To minimize the chances of ending up in an undetectably broken state:
  // First, delete the metadata file, then update the logo file, then update the
  // metadata file.
  base::FilePath logo_path = GetLogoPath();
  base::FilePath metadata_path = GetMetadataPath();

  if (!base::DeleteFile(metadata_path, false))
    return;

  if (base::WriteFile(
          logo_path,
          encoded_image->front_as<char>(),
          static_cast<int>(encoded_image->size())) == -1) {
    base::DeleteFile(logo_path, false);
    return;
  }

  WriteMetadata();
}

void LogoCache::DeleteLogoAndMetadata() {
  base::DeleteFile(GetLogoPath(), false);
  base::DeleteFile(GetMetadataPath(), false);
}

bool LogoCache::EnsureCacheDirectoryExists() {
  if (base::DirectoryExists(cache_directory_))
    return true;
  return base::CreateDirectory(cache_directory_);
}

}  // namespace search_provider_logos
