// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/v8_initializer.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/debug/alias.h"
#include "base/debug/crash_logging.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/sys_info.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "gin/gin_features.h"

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)
#if defined(OS_ANDROID)
#include "base/android/apk_assets.h"
#elif defined(OS_MACOSX)
#include "base/mac/foundation_util.h"
#endif
#endif  // V8_USE_EXTERNAL_STARTUP_DATA

namespace gin {

namespace {

// None of these globals are ever freed nor closed.
base::MemoryMappedFile* g_mapped_natives = nullptr;
base::MemoryMappedFile* g_mapped_snapshot = nullptr;

bool GenerateEntropy(unsigned char* buffer, size_t amount) {
  base::RandBytes(buffer, amount);
  return true;
}

void GetMappedFileData(base::MemoryMappedFile* mapped_file,
                       v8::StartupData* data) {
  if (mapped_file) {
    data->data = reinterpret_cast<const char*>(mapped_file->data());
    data->raw_size = static_cast<int>(mapped_file->length());
  } else {
    data->data = nullptr;
    data->raw_size = 0;
  }
}

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)

// File handles intentionally never closed. Not using File here because its
// Windows implementation guards against two instances owning the same
// PlatformFile (which we allow since we know it is never freed).
using OpenedFileMap =
    std::map<const char*,
             std::pair<base::PlatformFile, base::MemoryMappedFile::Region>>;
base::LazyInstance<OpenedFileMap>::Leaky g_opened_files =
    LAZY_INSTANCE_INITIALIZER;

const char kNativesFileName[] = "natives_blob.bin";

#if defined(OS_ANDROID)
const char kV8ContextSnapshotFileName64[] = "v8_context_snapshot_64.bin";
const char kV8ContextSnapshotFileName32[] = "v8_context_snapshot_32.bin";
const char kSnapshotFileName64[] = "snapshot_blob_64.bin";
const char kSnapshotFileName32[] = "snapshot_blob_32.bin";

#if defined(__LP64__)
#define kV8ContextSnapshotFileName kV8ContextSnapshotFileName64
#define kSnapshotFileName kSnapshotFileName64
#else
#define kV8ContextSnapshotFileName kV8ContextSnapshotFileName32
#define kSnapshotFileName kSnapshotFileName32
#endif

#else  // defined(OS_ANDROID)
const char kV8ContextSnapshotFileName[] = "v8_context_snapshot.bin";
const char kSnapshotFileName[] = "snapshot_blob.bin";
#endif  // defined(OS_ANDROID)

const char* GetSnapshotFileName(
    const V8Initializer::V8SnapshotFileType file_type) {
  switch (file_type) {
    case V8Initializer::V8SnapshotFileType::kDefault:
      return kSnapshotFileName;
    case V8Initializer::V8SnapshotFileType::kWithAdditionalContext:
      return kV8ContextSnapshotFileName;
  }
  NOTREACHED();
  return nullptr;
}

void GetV8FilePath(const char* file_name, base::FilePath* path_out) {
#if defined(OS_ANDROID)
  // This is the path within the .apk.
  *path_out =
      base::FilePath(FILE_PATH_LITERAL("assets")).AppendASCII(file_name);
#elif defined(OS_MACOSX)
  base::ScopedCFTypeRef<CFStringRef> natives_file_name(
      base::SysUTF8ToCFStringRef(file_name));
  *path_out = base::mac::PathForFrameworkBundleResource(natives_file_name);
#else
  base::FilePath data_path;
  bool r = base::PathService::Get(base::DIR_ASSETS, &data_path);
  DCHECK(r);
  *path_out = data_path.AppendASCII(file_name);
#endif
}

bool MapV8File(base::PlatformFile platform_file,
               base::MemoryMappedFile::Region region,
               base::MemoryMappedFile** mmapped_file_out) {
  DCHECK(*mmapped_file_out == NULL);
  std::unique_ptr<base::MemoryMappedFile> mmapped_file(
      new base::MemoryMappedFile());
  if (mmapped_file->Initialize(base::File(platform_file), region)) {
    *mmapped_file_out = mmapped_file.release();
    return true;
  }
  return false;
}

base::PlatformFile OpenV8File(const char* file_name,
                              base::MemoryMappedFile::Region* region_out) {
  // Re-try logic here is motivated by http://crbug.com/479537
  // for A/V on Windows (https://support.microsoft.com/en-us/kb/316609).

  // These match tools/metrics/histograms.xml
  enum OpenV8FileResult {
    OPENED = 0,
    OPENED_RETRY,
    FAILED_IN_USE,
    FAILED_OTHER,
    MAX_VALUE
  };
  base::FilePath path;
  GetV8FilePath(file_name, &path);

#if defined(OS_ANDROID)
  base::File file(base::android::OpenApkAsset(path.value(), region_out));
  OpenV8FileResult result = file.IsValid() ? OpenV8FileResult::OPENED
                                           : OpenV8FileResult::FAILED_OTHER;
#else
  // Re-try logic here is motivated by http://crbug.com/479537
  // for A/V on Windows (https://support.microsoft.com/en-us/kb/316609).
  const int kMaxOpenAttempts = 5;
  const int kOpenRetryDelayMillis = 250;

  OpenV8FileResult result = OpenV8FileResult::FAILED_IN_USE;
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ;
  base::File file;
  for (int attempt = 0; attempt < kMaxOpenAttempts; attempt++) {
    file.Initialize(path, flags);
    if (file.IsValid()) {
      *region_out = base::MemoryMappedFile::Region::kWholeFile;
      if (attempt == 0) {
        result = OpenV8FileResult::OPENED;
        break;
      } else {
        result = OpenV8FileResult::OPENED_RETRY;
        break;
      }
    } else if (file.error_details() != base::File::FILE_ERROR_IN_USE) {
      result = OpenV8FileResult::FAILED_OTHER;
#ifdef OS_WIN
      // TODO(oth): temporary diagnostics for http://crbug.com/479537
      std::string narrow(kNativesFileName);
      base::FilePath::StringType nativesBlob(narrow.begin(), narrow.end());
      if (path.BaseName().value() == nativesBlob) {
        base::File::Error file_error = file.error_details();
        base::debug::Alias(&file_error);
        LOG(FATAL) << "Failed to open V8 file '" << path.value()
                   << "' (reason: " << file.error_details() << ")";
      }
#endif  // OS_WIN
      break;
    } else if (kMaxOpenAttempts - 1 != attempt) {
      base::PlatformThread::Sleep(
          base::TimeDelta::FromMilliseconds(kOpenRetryDelayMillis));
    }
  }
#endif  // defined(OS_ANDROID)

  UMA_HISTOGRAM_ENUMERATION("V8.Initializer.OpenV8File.Result",
                            result,
                            OpenV8FileResult::MAX_VALUE);
  return file.TakePlatformFile();
}

OpenedFileMap::mapped_type& GetOpenedFile(const char* filename) {
  OpenedFileMap& opened_files(g_opened_files.Get());
  auto result = opened_files.emplace(filename, OpenedFileMap::mapped_type());
  OpenedFileMap::mapped_type& opened_file = result.first->second;
  bool is_new_file = result.second;

  // If we have no cache, try to open it and cache the result.
  if (is_new_file)
    opened_file.first = OpenV8File(filename, &opened_file.second);

  return opened_file;
}

enum LoadV8FileResult {
  V8_LOAD_SUCCESS = 0,
  V8_LOAD_FAILED_OPEN,
  V8_LOAD_FAILED_MAP,
  V8_LOAD_FAILED_VERIFY,  // Deprecated.
  V8_LOAD_MAX_VALUE
};

LoadV8FileResult MapOpenedFile(const OpenedFileMap::mapped_type& file_region,
                               base::MemoryMappedFile** mmapped_file_out) {
  if (file_region.first == base::kInvalidPlatformFile)
    return V8_LOAD_FAILED_OPEN;
  if (!MapV8File(file_region.first, file_region.second, mmapped_file_out))
    return V8_LOAD_FAILED_MAP;
  return V8_LOAD_SUCCESS;
}

#endif  // defined(V8_USE_EXTERNAL_STATUP_DATA)

}  // namespace

// static
void V8Initializer::Initialize(IsolateHolder::ScriptMode mode,
                               IsolateHolder::V8ExtrasMode v8_extras_mode) {
  static bool v8_is_initialized = false;
  if (v8_is_initialized)
    return;

  v8::V8::InitializePlatform(V8Platform::Get());

  if (base::FeatureList::IsEnabled(features::kV8OptimizeJavascript)) {
    static const char optimize[] = "--opt";
    v8::V8::SetFlagsFromString(optimize, sizeof(optimize) - 1);
  } else {
    static const char no_optimize[] = "--no-opt";
    v8::V8::SetFlagsFromString(no_optimize, sizeof(no_optimize) - 1);
  }

  if (IsolateHolder::kStrictMode == mode) {
    static const char use_strict[] = "--use_strict";
    v8::V8::SetFlagsFromString(use_strict, sizeof(use_strict) - 1);
  }
  if (IsolateHolder::kStableAndExperimentalV8Extras == v8_extras_mode) {
    static const char flag[] = "--experimental_extras";
    v8::V8::SetFlagsFromString(flag, sizeof(flag) - 1);
  }

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)
  v8::StartupData natives;
  GetMappedFileData(g_mapped_natives, &natives);
  v8::V8::SetNativesDataBlob(&natives);

  if (g_mapped_snapshot) {
    v8::StartupData snapshot;
    GetMappedFileData(g_mapped_snapshot, &snapshot);
    v8::V8::SetSnapshotDataBlob(&snapshot);
  }
#endif  // V8_USE_EXTERNAL_STARTUP_DATA

  v8::V8::SetEntropySource(&GenerateEntropy);
  v8::V8::Initialize();

  v8_is_initialized = true;
}

// static
void V8Initializer::GetV8ExternalSnapshotData(v8::StartupData* natives,
                                              v8::StartupData* snapshot) {
  GetMappedFileData(g_mapped_natives, natives);
  GetMappedFileData(g_mapped_snapshot, snapshot);
}

// static
void V8Initializer::GetV8ExternalSnapshotData(const char** natives_data_out,
                                              int* natives_size_out,
                                              const char** snapshot_data_out,
                                              int* snapshot_size_out) {
  v8::StartupData natives;
  v8::StartupData snapshot;
  GetV8ExternalSnapshotData(&natives, &snapshot);
  *natives_data_out = natives.data;
  *natives_size_out = natives.raw_size;
  *snapshot_data_out = snapshot.data;
  *snapshot_size_out = snapshot.raw_size;
}

#if defined(V8_USE_EXTERNAL_STARTUP_DATA)

// static
void V8Initializer::LoadV8Snapshot(V8SnapshotFileType snapshot_file_type) {
  if (g_mapped_snapshot) {
    // TODO(crbug.com/802962): Confirm not loading different type of snapshot
    // files in a process.
    return;
  }

  LoadV8FileResult result =
      MapOpenedFile(GetOpenedFile(GetSnapshotFileName(snapshot_file_type)),
                    &g_mapped_snapshot);
  // V8 can't start up without the source of the natives, but it can
  // start up (slower) without the snapshot.
  UMA_HISTOGRAM_ENUMERATION("V8.Initializer.LoadV8Snapshot.Result", result,
                            V8_LOAD_MAX_VALUE);
}

// static
void V8Initializer::LoadV8Natives() {
  if (g_mapped_natives)
    return;

  LoadV8FileResult result = MapOpenedFile(GetOpenedFile(kNativesFileName),
                                          &g_mapped_natives);
  if (result != V8_LOAD_SUCCESS) {
    LOG(FATAL) << "Couldn't mmap v8 natives data file, status code is "
               << static_cast<int>(result);
  }
}

// static
void V8Initializer::LoadV8SnapshotFromFD(
    base::PlatformFile snapshot_pf,
    int64_t snapshot_offset,
    int64_t snapshot_size,
    V8SnapshotFileType snapshot_file_type) {
  if (g_mapped_snapshot)
    return;

  if (snapshot_pf == base::kInvalidPlatformFile)
    return;

  base::MemoryMappedFile::Region snapshot_region =
      base::MemoryMappedFile::Region::kWholeFile;
  if (snapshot_size != 0 || snapshot_offset != 0) {
    snapshot_region.offset = snapshot_offset;
    snapshot_region.size = snapshot_size;
  }

  LoadV8FileResult result = V8_LOAD_SUCCESS;
  if (!MapV8File(snapshot_pf, snapshot_region, &g_mapped_snapshot))
    result = V8_LOAD_FAILED_MAP;
  if (result == V8_LOAD_SUCCESS) {
    g_opened_files.Get()[GetSnapshotFileName(snapshot_file_type)] =
        std::make_pair(snapshot_pf, snapshot_region);
  }
  UMA_HISTOGRAM_ENUMERATION("V8.Initializer.LoadV8Snapshot.Result", result,
                            V8_LOAD_MAX_VALUE);
}

// static
void V8Initializer::LoadV8NativesFromFD(base::PlatformFile natives_pf,
                                        int64_t natives_offset,
                                        int64_t natives_size) {
  if (g_mapped_natives)
    return;

  CHECK_NE(natives_pf, base::kInvalidPlatformFile);

  base::MemoryMappedFile::Region natives_region =
      base::MemoryMappedFile::Region::kWholeFile;
  if (natives_size != 0 || natives_offset != 0) {
    natives_region.offset = natives_offset;
    natives_region.size = natives_size;
  }

  if (!MapV8File(natives_pf, natives_region, &g_mapped_natives)) {
    LOG(FATAL) << "Couldn't mmap v8 natives data file";
  }
  g_opened_files.Get()[kNativesFileName] =
      std::make_pair(natives_pf, natives_region);
}

#if defined(OS_ANDROID)
// static
base::FilePath V8Initializer::GetNativesFilePath() {
  base::FilePath path;
  GetV8FilePath(kNativesFileName, &path);
  return path;
}

// static
base::FilePath V8Initializer::GetSnapshotFilePath(
    bool abi_32_bit,
    V8SnapshotFileType snapshot_file_type) {
  base::FilePath path;
  const char* filename = nullptr;
  switch (snapshot_file_type) {
    case V8Initializer::V8SnapshotFileType::kDefault:
      filename = abi_32_bit ? kSnapshotFileName32 : kSnapshotFileName64;
      break;
    case V8Initializer::V8SnapshotFileType::kWithAdditionalContext:
      filename = abi_32_bit ? kV8ContextSnapshotFileName32
                            : kV8ContextSnapshotFileName64;
      break;
  }
  CHECK(filename);

  GetV8FilePath(filename, &path);
  return path;
}
#endif  // defined(OS_ANDROID)
#endif  // defined(V8_USE_EXTERNAL_STARTUP_DATA)

}  // namespace gin
