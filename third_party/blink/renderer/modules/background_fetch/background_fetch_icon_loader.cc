// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LiICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_icon_loader.h"

#include "skia/ext/image_operations.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/modules/background_fetch/background_fetch_bridge.h"
#include "third_party/blink/renderer/modules/background_fetch/icon_definition.h"
#include "third_party/blink/renderer/platform/graphics/color_behavior.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/image-decoders/image_frame.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/string_impl.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

namespace {

const unsigned long kIconFetchTimeoutInMs = 30000;
const int kMinimumIconSizeInPx = 16;
const double kAnySizeScore = 0.8;
const double kUnspecifiedSizeScore = 0.4;

}  // namespace

BackgroundFetchIconLoader::BackgroundFetchIconLoader() = default;
BackgroundFetchIconLoader::~BackgroundFetchIconLoader() {
  // We should've called Stop() before the destructor is invoked.
  DCHECK(stopped_ || icon_callback_.is_null());
}

void BackgroundFetchIconLoader::Start(BackgroundFetchBridge* bridge,
                                      ExecutionContext* execution_context,
                                      HeapVector<IconDefinition> icons,
                                      IconCallback icon_callback) {
  DCHECK(!stopped_);
  DCHECK_GE(icons.size(), 1u);
  DCHECK(bridge);

  icons_ = std::move(icons);
  bridge->GetIconDisplaySize(
      WTF::Bind(&BackgroundFetchIconLoader::DidGetIconDisplaySizeIfSoLoadIcon,
                WrapWeakPersistent(this), WrapWeakPersistent(execution_context),
                std::move(icon_callback)));
}

void BackgroundFetchIconLoader::DidGetIconDisplaySizeIfSoLoadIcon(
    ExecutionContext* execution_context,
    IconCallback icon_callback,
    const WebSize& icon_display_size_pixels) {
  if (icon_display_size_pixels.IsEmpty()) {
    std::move(icon_callback).Run(SkBitmap());
    return;
  }

  int best_icon_index =
      PickBestIconForDisplay(execution_context, icon_display_size_pixels);
  if (best_icon_index < 0) {
    // None of the icons provided was suitable.
    std::move(icon_callback).Run(SkBitmap());
    return;
  }
  KURL best_icon_url =
      execution_context->CompleteURL(icons_[best_icon_index].src());
  icon_callback_ = std::move(icon_callback);

  ThreadableLoaderOptions threadable_loader_options;
  threadable_loader_options.timeout_milliseconds = kIconFetchTimeoutInMs;

  ResourceLoaderOptions resource_loader_options;
  if (execution_context->IsWorkerGlobalScope())
    resource_loader_options.request_initiator_context = kWorkerContext;

  ResourceRequest resource_request(best_icon_url);
  resource_request.SetRequestContext(WebURLRequest::kRequestContextImage);
  resource_request.SetPriority(ResourceLoadPriority::kMedium);
  resource_request.SetRequestorOrigin(execution_context->GetSecurityOrigin());

  threadable_loader_ = ThreadableLoader::Create(*execution_context, this,
                                                threadable_loader_options,
                                                resource_loader_options);

  threadable_loader_->Start(resource_request);
}

int BackgroundFetchIconLoader::PickBestIconForDisplay(
    ExecutionContext* execution_context,
    const WebSize& icon_display_size_pixels) {
  int best_index = -1;
  double best_score = 0.0;
  for (size_t i = 0; i < icons_.size(); ++i) {
    // If the icon has no or invalid src, move on.
    if (!icons_[i].hasSrc())
      continue;
    KURL icon_url = execution_context->CompleteURL(icons_[i].src());
    if (!icon_url.IsValid() || icon_url.IsEmpty())
      continue;

    double score = GetIconScore(icons_[i], icon_display_size_pixels.width);
    if (!score)
      continue;
    // According to the spec, if two icons get the same score, we must use the
    // one that's declared last. (https://w3c.github.io/manifest/#icons-member).
    if (score >= best_score) {
      best_score = score;
      best_index = i;
    }
  }
  return best_index;
}

// The scoring works as follows:
// When the size is "any", the icon size score is kAnySizeScore.
// If unspecified, use the unspecified size score, kUnspecifiedSizeScore as
// icon score.

// For other sizes, the icon score lies in [0,1] and is computed by multiplying
// the dominant size score and aspect ratio score.
//
// The dominant size score lies in [0, 1] and is computed using
// dominant size and and |ideal_size|:
//   - If dominant_size < kMinimumIconSizeInPx, the size score is 0.0.
//   - For all other sizes, the score is calculated as
//     1/(1 + abs(dominant_size-ideal_size))
//   - If dominant_size < ideal_size, there is an upscaling penalty, which is
//     dominant_size/ideal_size.
//
// The aspect ratio score lies in [0, 1] and is computed by dividing the short
// edge length by the long edge. (Bias towards square icons assumed).
//
// Note: If this is an ico file containing multiple sizes, return the best
// score.
double BackgroundFetchIconLoader::GetIconScore(IconDefinition icon,
                                               const int ideal_size) {
  // Extract sizes from the icon definition, expressed as "<width>x<height>
  // <width>x<height> ...."
  if (!icon.hasSizes() || icon.sizes().IsEmpty())
    return kUnspecifiedSizeScore;

  String sizes = icon.sizes();
  // if any size is set to "any" return kAnySizeScore;
  if (sizes.LowerASCII() == "any")
    return kAnySizeScore;

  Vector<String> sizes_str;
  sizes.Split(" ", false /* allow_empty_entries*/, sizes_str);

  // Pick the first size.
  // TODO(nator): Add support for multiple sizes (.ico files).
  Vector<String> width_and_height_str;
  sizes_str[0].Split("x", false /* allow_empty_entries */,
                     width_and_height_str);
  // If sizes isn't in this format, consider it as 'unspecified'.
  if (width_and_height_str.size() != 2)
    return kUnspecifiedSizeScore;
  double width = width_and_height_str[0].ToDouble();
  double height = width_and_height_str[1].ToDouble();

  // Compute dominant size score
  int dominant_size = std::max(width, height);
  int short_size = std::min(width, height);
  if (dominant_size < kMinimumIconSizeInPx)
    return 0.0;

  double dominant_size_score = 1.0 / (1.0 + abs(dominant_size - ideal_size));
  if (dominant_size < ideal_size)
    dominant_size_score = dominant_size_score * dominant_size / ideal_size;
  // Compute aspect ratio score. If dominant_size is zero, we'd have returned
  // by now.
  double aspect_ratio_score = short_size / dominant_size;

  // Compute icon score.
  return aspect_ratio_score * dominant_size_score;
}

void BackgroundFetchIconLoader::Stop() {
  if (stopped_)
    return;

  stopped_ = true;
  if (threadable_loader_) {
    threadable_loader_->Cancel();
    threadable_loader_ = nullptr;
  }
}

void BackgroundFetchIconLoader::DidReceiveData(const char* data,
                                               unsigned length) {
  if (!data_)
    data_ = SharedBuffer::Create();
  data_->Append(data, length);
}

void BackgroundFetchIconLoader::DidFinishLoading(
    unsigned long resource_identifier) {
  if (stopped_)
    return;
  if (data_) {
    // Decode data.
    std::unique_ptr<ImageDecoder> decoder = ImageDecoder::Create(
        data_, true /* data_complete*/, ImageDecoder::kAlphaPremultiplied,
        ColorBehavior::TransformToSRGB());
    if (decoder) {
      // the |ImageFrame*| is owned by the decoder.
      ImageFrame* image_frame = decoder->DecodeFrameBufferAtIndex(0);
      if (image_frame) {
        std::move(icon_callback_).Run(image_frame->Bitmap());
        return;
      }
    }
  }
  RunCallbackWithEmptyBitmap();
}

void BackgroundFetchIconLoader::DidFail(const ResourceError& error) {
  RunCallbackWithEmptyBitmap();
}

void BackgroundFetchIconLoader::DidFailRedirectCheck() {
  RunCallbackWithEmptyBitmap();
}

void BackgroundFetchIconLoader::RunCallbackWithEmptyBitmap() {
  // If this has been stopped it is not desirable to trigger further work,
  // there is a shutdown of some sort in progress.
  if (stopped_)
    return;

  std::move(icon_callback_).Run(SkBitmap());
}

}  // namespace blink
