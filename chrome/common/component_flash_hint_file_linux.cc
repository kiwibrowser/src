// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/component_flash_hint_file_linux.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#include <memory>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/files/memory_mapped_file.h"
#include "base/files/scoped_file.h"
#include "base/json/json_string_value_serializer.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "chrome/common/chrome_paths.h"
#include "crypto/secure_hash.h"
#include "crypto/secure_util.h"
#include "crypto/sha2.h"

namespace component_flash_hint_file {

namespace {

// The current version of the hints file.
const int kCurrentHintFileVersion = 0x10;
// The earliest version of the hints file.
const int kEarliestHintFileVersion = 0x10;
// The Version field in the JSON encoded file.
const char kVersionField[] = "Version";
// The HashAlgorithm field in the JSON encoded file.
const char kHashAlgoField[] = "HashAlgorithm";
// The Hash field in the JSON encoded file.
const char kHashField[] = "Hash";
// The PluginPath field in the JSON encoded file.
const char kPluginPath[] = "PluginPath";
// The PluginVersion field in the JSON encoded file.
const char kPluginVersion[] = "PluginVersion";
// For use with the scoped_ptr of an mmap-ed buffer
struct MmapDeleter {
  explicit MmapDeleter(size_t map_size) : map_size_(map_size) { }
  inline void operator()(uint8_t* ptr) const {
    if (ptr != MAP_FAILED)
      munmap(ptr, map_size_);
  }

 private:
  size_t map_size_;
};

// Hashes the plugin file and returns the result in the out params.
// |mapped_file| is the file to be hashed.
// |result| is the buffer, which must be of size crypto::kSHA256Length, which
// will contain the hash.
// |len| is the size of the buffer, which must be crypto::kSHA256Length.
void SHA256Hash(const base::MemoryMappedFile& mapped_file,
                void* result,
                size_t len) {
  CHECK_EQ(crypto::kSHA256Length, len);
  std::unique_ptr<crypto::SecureHash> secure_hash(
      crypto::SecureHash::Create(crypto::SecureHash::SHA256));
  secure_hash->Update(mapped_file.data(), mapped_file.length());
  secure_hash->Finish(result, len);
}

// This will serialize the file to disk as JSON. The format is:
// {
//  "Version": 0x10,
//  "HashAlgorithm": SecureHash::SHA256,
//  "Hash": <Base64 Encoded Hash>,
//  "PluginPath": /path/to/component/updated/flash.so,
//  "PluginVersion": "1.0.0.1"
//  }
bool WriteToDisk(const int version,
                 const crypto::SecureHash::Algorithm algorithm,
                 const std::string& hash,
                 const base::FilePath& plugin_path,
                 const std::string& flash_version) {
  base::FilePath hint_file_path;
  if (!base::PathService::Get(chrome::FILE_COMPONENT_FLASH_HINT,
                              &hint_file_path))
    return false;

  std::string encoded_hash;
  base::Base64Encode(hash, &encoded_hash);

  // Now construct a Value object to convert to JSON.
  base::DictionaryValue dict;
  dict.SetInteger(kVersionField, version);
  dict.SetInteger(kHashAlgoField, crypto::SecureHash::SHA256);
  dict.SetString(kHashField, encoded_hash);
  dict.SetString(kPluginPath, plugin_path.value());
  dict.SetString(kPluginVersion, flash_version);
  // Do the serialization of the DictionaryValue to JSON.
  std::string json_string;
  JSONStringValueSerializer serializer(&json_string);
  if (!serializer.Serialize(dict))
    return false;

  return base::ImportantFileWriter::WriteFileAtomically(hint_file_path,
                                                        json_string);
}

}  // namespace

bool TestExecutableMapping(const base::FilePath& path) {
  const base::ScopedFD fd(
      HANDLE_EINTR(open(path.value().c_str(), O_RDONLY | O_CLOEXEC)));
  if (!fd.is_valid())
    return false;
  const size_t map_size = sizeof(uint8_t);
  const MmapDeleter deleter(map_size);
  std::unique_ptr<uint8_t, MmapDeleter> buf_ptr(
      reinterpret_cast<uint8_t*>(mmap(nullptr, map_size, PROT_READ | PROT_EXEC,
                                      MAP_PRIVATE, fd.get(), 0)),
      deleter);
  return buf_ptr.get() != MAP_FAILED;
}

bool RecordFlashUpdate(const base::FilePath& unpacked_plugin,
                       const base::FilePath& moved_plugin,
                       const std::string& version) {
  base::MemoryMappedFile mapped_file;
  if (!mapped_file.Initialize(unpacked_plugin))
    return false;

  std::string hash(crypto::kSHA256Length, 0);
  SHA256Hash(mapped_file, base::data(hash), hash.size());

  return WriteToDisk(kCurrentHintFileVersion,
                     crypto::SecureHash::Algorithm::SHA256, hash, moved_plugin,
                     version);
}

bool DoesHintFileExist() {
  base::FilePath hint_file_path;
  if (!base::PathService::Get(chrome::FILE_COMPONENT_FLASH_HINT,
                              &hint_file_path))
    return false;
  return base::PathExists(hint_file_path);
}

bool VerifyAndReturnFlashLocation(base::FilePath* path,
                                  std::string* flash_version) {
  base::FilePath hint_file_path;
  if (!base::PathService::Get(chrome::FILE_COMPONENT_FLASH_HINT,
                              &hint_file_path))
    return false;

  std::string json_string;
  if (!base::ReadFileToString(hint_file_path, &json_string))
    return false;

  int error_code;
  std::string error_message;
  JSONStringValueDeserializer deserializer(json_string);
  const std::unique_ptr<base::Value> value =
      deserializer.Deserialize(&error_code, &error_message);

  if (!value) {
    LOG(ERROR)
        << "Could not deserialize the component updated Flash hint file. Error "
        << error_code << ": " << error_message;
    return false;
  }

  base::DictionaryValue* dict = nullptr;
  if (!value->GetAsDictionary(&dict))
    return false;

  int version;
  if (!dict->GetInteger(kVersionField, &version))
    return false;
  if (version < kEarliestHintFileVersion || version > kCurrentHintFileVersion)
    return false;

  int hash_algorithm;
  if (!dict->GetInteger(kHashAlgoField, &hash_algorithm))
    return false;
  if (hash_algorithm != crypto::SecureHash::SHA256)
    return false;

  std::string hash;
  if (!dict->GetString(kHashField, &hash))
    return false;

  std::string plugin_path_str;
  if (!dict->GetString(kPluginPath, &plugin_path_str))
    return false;

  std::string plugin_version_str;
  if (!dict->GetString(kPluginVersion, &plugin_version_str))
    return false;

  std::string decoded_hash;
  if (!base::Base64Decode(hash, &decoded_hash))
    return false;

  const base::FilePath plugin_path(plugin_path_str);
  base::MemoryMappedFile plugin_file;
  if (!plugin_file.Initialize(plugin_path))
    return false;

  std::vector<uint8_t> file_hash(crypto::kSHA256Length, 0);
  SHA256Hash(plugin_file, &file_hash[0], file_hash.size());
  if (!crypto::SecureMemEqual(base::data(file_hash), base::data(decoded_hash),
                              crypto::kSHA256Length)) {
    LOG(ERROR)
        << "The hash recorded in the component flash hint file does not "
           "match the actual hash of the flash plugin found on disk. The "
           "component flash plugin will not be loaded.";
    return false;
  }

  *path = plugin_path;
  flash_version->assign(plugin_version_str);
  return true;
}

}  // namespace component_flash_hint_file
