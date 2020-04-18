// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/post_processor_factory.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/scoped_native_library.h"
#include "base/strings/stringprintf.h"
#include "chromecast/media/cma/backend/post_processors/post_processor_wrapper.h"
#include "chromecast/public/media/audio_post_processor2_shlib.h"
#include "chromecast/public/media/audio_post_processor_shlib.h"

namespace chromecast {
namespace media {

namespace {

const char kV1SoCreateFunction[] = "AudioPostProcessorShlib_Create";
const char kV2SoCreateFunctionFormat[] = "AudioPostProcessor2Shlib%sCreate";

}  // namespace

using CreatePostProcessor2Function =
    AudioPostProcessor2* (*)(const std::string&, int);

using CreatePostProcessorFunction = AudioPostProcessor* (*)(const std::string&,
                                                            int);

PostProcessorFactory::PostProcessorFactory() = default;
PostProcessorFactory::~PostProcessorFactory() = default;

std::unique_ptr<AudioPostProcessor2> PostProcessorFactory::CreatePostProcessor(
    const std::string& library_path,
    const std::string& post_processor_type,
    const std::string& config,
    int channels) {
  libraries_.push_back(std::make_unique<base::ScopedNativeLibrary>(
      base::FilePath(library_path)));
  CHECK(libraries_.back()->is_valid())
      << "Could not open post processing library " << library_path;

  if (!post_processor_type.empty()) {
    std::string create_function = base::StringPrintf(
        kV2SoCreateFunctionFormat, post_processor_type.c_str());
    auto v2_create = reinterpret_cast<CreatePostProcessor2Function>(
        libraries_.back()->GetFunctionPointer(create_function.c_str()));

    DCHECK(v2_create) << "Could not find " << create_function << "() in "
                      << library_path;

    return base::WrapUnique(v2_create(config, channels));
  }

  auto v1_create = reinterpret_cast<CreatePostProcessorFunction>(
      libraries_.back()->GetFunctionPointer(kV1SoCreateFunction));

  DCHECK(v1_create) << "Could not find " << kV1SoCreateFunction << "() in "
                    << library_path;

  LOG(WARNING) << "[Deprecated]: AudioPostProcessor will be deprecated soon."
               << " Please update " << library_path
               << " to AudioPostProcessor2.";

  return std::make_unique<AudioPostProcessorWrapper>(
      base::WrapUnique(v1_create(config, channels)), channels);
}

}  // namespace media
}  // namespace chromecast
