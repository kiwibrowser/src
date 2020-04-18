// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/debug_recording_session.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "media/audio/audio_debug_recording_manager.h"
#include "media/audio/audio_manager.h"
#include "services/audio/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

namespace {

#if defined(OS_WIN)
#define IntToStringType base::IntToString16
#else
#define IntToStringType base::IntToString
#endif

const base::FilePath::CharType* StreamTypeToStringType(
    media::AudioDebugRecordingStreamType stream_type) {
  switch (stream_type) {
    case media::AudioDebugRecordingStreamType::kInput:
      return FILE_PATH_LITERAL("input");
    case media::AudioDebugRecordingStreamType::kOutput:
      return FILE_PATH_LITERAL("output");
  }
  NOTREACHED();
  return FILE_PATH_LITERAL("output");
}

}  // namespace

DebugRecordingSession::DebugRecordingFileProvider::DebugRecordingFileProvider(
    mojom::DebugRecordingFileProviderRequest request,
    const base::FilePath& file_name_base)
    : binding_(this, std::move(request)), file_name_base_(file_name_base) {}

DebugRecordingSession::DebugRecordingFileProvider::
    ~DebugRecordingFileProvider() = default;

void DebugRecordingSession::DebugRecordingFileProvider::CreateWavFile(
    media::AudioDebugRecordingStreamType stream_type,
    uint32_t id,
    CreateWavFileCallback reply_callback) {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(
          [](const base::FilePath& file_name) {
            return base::File(file_name, base::File::FLAG_CREATE_ALWAYS |
                                             base::File::FLAG_WRITE);
          },
          file_name_base_.AddExtension(StreamTypeToStringType(stream_type))
              .AddExtension(IntToStringType(id))
              .AddExtension(FILE_PATH_LITERAL("wav"))),
      std::move(reply_callback));
}

DebugRecordingSession::DebugRecordingSession(
    const base::FilePath& file_name_base,
    std::unique_ptr<service_manager::Connector> connector) {
  DCHECK(connector);

  mojom::DebugRecordingFileProviderPtr file_provider;
  file_provider_ = std::make_unique<DebugRecordingFileProvider>(
      mojo::MakeRequest(&file_provider), file_name_base);

  connector->BindInterface(audio::mojom::kServiceName,
                           mojo::MakeRequest(&debug_recording_));
  if (debug_recording_.is_bound())
    debug_recording_->Enable(std::move(file_provider));
}

DebugRecordingSession::~DebugRecordingSession() {}

}  // namespace audio
