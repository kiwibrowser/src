/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/image-decoders/gif/gif_image_decoder.h"

#include <limits>
#include <memory>
#include "third_party/blink/renderer/platform/image-decoders/gif/gif_image_reader.h"
#include "third_party/blink/renderer/platform/wtf/not_found.h"

namespace blink {

GIFImageDecoder::GIFImageDecoder(AlphaOption alpha_option,
                                 const ColorBehavior& color_behavior,
                                 size_t max_decoded_bytes)
    : ImageDecoder(alpha_option, color_behavior, max_decoded_bytes),
      repetition_count_(kAnimationLoopOnce) {}

GIFImageDecoder::~GIFImageDecoder() = default;

void GIFImageDecoder::OnSetData(SegmentReader* data) {
  if (reader_)
    reader_->SetData(data);
}

int GIFImageDecoder::RepetitionCount() const {
  // This value can arrive at any point in the image data stream.  Most GIFs
  // in the wild declare it near the beginning of the file, so it usually is
  // set by the time we've decoded the size, but (depending on the GIF and the
  // packets sent back by the webserver) not always.  If the reader hasn't
  // seen a loop count yet, it will return kCLoopCountNotSeen, in which case we
  // should default to looping once (the initial value for
  // |repetition_count_|).
  //
  // There are some additional wrinkles here. First, ImageSource::Clear()
  // may destroy the reader, making the result from the reader _less_
  // authoritative on future calls if the recreated reader hasn't seen the
  // loop count.  We don't need to special-case this because in this case the
  // new reader will once again return kCLoopCountNotSeen, and we won't
  // overwrite the cached correct value.
  //
  // Second, a GIF might never set a loop count at all, in which case we
  // should continue to treat it as a "loop once" animation.  We don't need
  // special code here either, because in this case we'll never change
  // |repetition_count_| from its default value.
  //
  // Third, we use the same GIFImageReader for counting frames and we might
  // see the loop count and then encounter a decoding error which happens
  // later in the stream. It is also possible that no frames are in the
  // stream. In these cases we should just loop once.
  if (IsAllDataReceived() && ParseCompleted() && reader_->ImagesCount() == 1)
    repetition_count_ = kAnimationNone;
  else if (Failed() || (reader_ && (!reader_->ImagesCount())))
    repetition_count_ = kAnimationLoopOnce;
  else if (reader_ && reader_->LoopCount() != kCLoopCountNotSeen)
    repetition_count_ = reader_->LoopCount();
  return repetition_count_;
}

bool GIFImageDecoder::FrameIsReceivedAtIndex(size_t index) const {
  return reader_ && (index < reader_->ImagesCount()) &&
         reader_->FrameContext(index)->IsComplete();
}

TimeDelta GIFImageDecoder::FrameDurationAtIndex(size_t index) const {
  return (reader_ && (index < reader_->ImagesCount()) &&
          reader_->FrameContext(index)->IsHeaderDefined())
             ? TimeDelta::FromMilliseconds(
                   reader_->FrameContext(index)->DelayTime())
             : TimeDelta();
}

bool GIFImageDecoder::SetFailed() {
  reader_.reset();
  return ImageDecoder::SetFailed();
}

bool GIFImageDecoder::HaveDecodedRow(size_t frame_index,
                                     GIFRow::const_iterator row_begin,
                                     size_t width,
                                     size_t row_number,
                                     unsigned repeat_count,
                                     bool write_transparent_pixels) {
  const GIFFrameContext* frame_context = reader_->FrameContext(frame_index);
  // The pixel data and coordinates supplied to us are relative to the frame's
  // origin within the entire image size, i.e.
  // (frameC_context->xOffset, frame_context->yOffset). There is no guarantee
  // that width == (size().width() - frame_context->xOffset), so
  // we must ensure we don't run off the end of either the source data or the
  // row's X-coordinates.
  const int x_begin = frame_context->XOffset();
  const int y_begin = frame_context->YOffset() + row_number;
  const int x_end = std::min(static_cast<int>(frame_context->XOffset() + width),
                             Size().Width());
  const int y_end = std::min(
      static_cast<int>(frame_context->YOffset() + row_number + repeat_count),
      Size().Height());
  if (!width || (x_begin < 0) || (y_begin < 0) || (x_end <= x_begin) ||
      (y_end <= y_begin))
    return true;

  const GIFColorMap::Table& color_table =
      frame_context->LocalColorMap().IsDefined()
          ? frame_context->LocalColorMap().GetTable()
          : reader_->GlobalColorMap().GetTable();

  if (color_table.IsEmpty())
    return true;

  GIFColorMap::Table::const_iterator color_table_iter = color_table.begin();

  // Initialize the frame if necessary.
  ImageFrame& buffer = frame_buffer_cache_[frame_index];
  if (!InitFrameBuffer(frame_index))
    return false;

  const size_t transparent_pixel = frame_context->TransparentPixel();
  GIFRow::const_iterator row_end = row_begin + (x_end - x_begin);
  ImageFrame::PixelData* current_address = buffer.GetAddr(x_begin, y_begin);

  // We may or may not need to write transparent pixels to the buffer.
  // If we're compositing against a previous image, it's wrong, and if
  // we're writing atop a cleared, fully transparent buffer, it's
  // unnecessary; but if we're decoding an interlaced gif and
  // displaying it "Haeberli"-style, we must write these for passes
  // beyond the first, or the initial passes will "show through" the
  // later ones.
  //
  // The loops below are almost identical. One writes a transparent pixel
  // and one doesn't based on the value of |write_transparent_pixels|.
  // The condition check is taken out of the loop to enhance performance.
  // This optimization reduces decoding time by about 15% for a 3MB image.
  if (write_transparent_pixels) {
    for (; row_begin != row_end; ++row_begin, ++current_address) {
      const size_t source_value = *row_begin;
      if ((source_value != transparent_pixel) &&
          (source_value < color_table.size())) {
        *current_address = color_table_iter[source_value];
      } else {
        *current_address = 0;
        current_buffer_saw_alpha_ = true;
      }
    }
  } else {
    for (; row_begin != row_end; ++row_begin, ++current_address) {
      const size_t source_value = *row_begin;
      if ((source_value != transparent_pixel) &&
          (source_value < color_table.size()))
        *current_address = color_table_iter[source_value];
      else
        current_buffer_saw_alpha_ = true;
    }
  }

  // Tell the frame to copy the row data if need be.
  if (repeat_count > 1)
    buffer.CopyRowNTimes(x_begin, x_end, y_begin, y_end);

  buffer.SetPixelsChanged(true);
  return true;
}

bool GIFImageDecoder::ParseCompleted() const {
  return reader_ && reader_->ParseCompleted();
}

bool GIFImageDecoder::FrameComplete(size_t frame_index) {
  // Initialize the frame if necessary.  Some GIFs insert do-nothing frames,
  // in which case we never reach HaveDecodedRow() before getting here.
  if (!InitFrameBuffer(frame_index))
    return SetFailed();

  if (!current_buffer_saw_alpha_)
    CorrectAlphaWhenFrameBufferSawNoAlpha(frame_index);

  frame_buffer_cache_[frame_index].SetStatus(ImageFrame::kFrameComplete);

  return true;
}

void GIFImageDecoder::ClearFrameBuffer(size_t frame_index) {
  if (reader_ && frame_buffer_cache_[frame_index].GetStatus() ==
                     ImageFrame::kFramePartial) {
    // Reset the state of the partial frame in the reader so that the frame
    // can be decoded again when requested.
    reader_->ClearDecodeState(frame_index);
  }
  ImageDecoder::ClearFrameBuffer(frame_index);
}

size_t GIFImageDecoder::DecodeFrameCount() {
  Parse(kGIFFrameCountQuery);
  // If decoding fails, |reader_| will have been destroyed.  Instead of
  // returning 0 in this case, return the existing number of frames.  This way
  // if we get halfway through the image before decoding fails, we won't
  // suddenly start reporting that the image has zero frames.
  return Failed() ? frame_buffer_cache_.size() : reader_->ImagesCount();
}

void GIFImageDecoder::InitializeNewFrame(size_t index) {
  ImageFrame* buffer = &frame_buffer_cache_[index];
  const GIFFrameContext* frame_context = reader_->FrameContext(index);
  buffer->SetOriginalFrameRect(
      Intersection(frame_context->FrameRect(), IntRect(IntPoint(), Size())));
  buffer->SetDuration(TimeDelta::FromMilliseconds(frame_context->DelayTime()));
  buffer->SetDisposalMethod(frame_context->GetDisposalMethod());
  buffer->SetRequiredPreviousFrameIndex(
      FindRequiredPreviousFrame(index, false));
}

void GIFImageDecoder::Decode(size_t index) {
  Parse(kGIFFrameCountQuery);

  if (Failed())
    return;

  UpdateAggressivePurging(index);

  Vector<size_t> frames_to_decode = FindFramesToDecode(index);
  for (auto i = frames_to_decode.rbegin(); i != frames_to_decode.rend(); ++i) {
    if (!reader_->Decode(*i)) {
      SetFailed();
      return;
    }

    // If this returns false, we need more data to continue decoding.
    if (!PostDecodeProcessing(*i))
      break;
  }

  // It is also a fatal error if all data is received and we have decoded all
  // frames available but the file is truncated.
  if (index >= frame_buffer_cache_.size() - 1 && IsAllDataReceived() &&
      reader_ && !reader_->ParseCompleted())
    SetFailed();
}

void GIFImageDecoder::Parse(GIFParseQuery query) {
  if (Failed())
    return;

  if (!reader_) {
    reader_ = std::make_unique<GIFImageReader>(this);
    reader_->SetData(data_);
  }

  if (!reader_->Parse(query))
    SetFailed();
}

void GIFImageDecoder::OnInitFrameBuffer(size_t frame_index) {
  current_buffer_saw_alpha_ = false;
}

bool GIFImageDecoder::CanReusePreviousFrameBuffer(size_t frame_index) const {
  DCHECK(frame_index < frame_buffer_cache_.size());
  return frame_buffer_cache_[frame_index].GetDisposalMethod() !=
         ImageFrame::kDisposeOverwritePrevious;
}

}  // namespace blink
