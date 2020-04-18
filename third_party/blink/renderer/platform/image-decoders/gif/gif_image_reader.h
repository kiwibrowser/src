/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_IMAGE_DECODERS_GIF_GIF_IMAGE_READER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_IMAGE_DECODERS_GIF_GIF_IMAGE_READER_H_

// Define ourselves as the clientPtr.  Mozilla just hacked their C++ callback
// class into this old C decoder, so we will too.
#include <memory>
#include "third_party/blink/renderer/platform/image-decoders/gif/gif_image_decoder.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class FastSharedBufferReader;

const int kCLoopCountNotSeen = -2;

// List of possible parsing states.
enum GIFState {
  kGIFType,
  kGIFGlobalHeader,
  kGIFGlobalColormap,
  kGIFImageStart,
  kGIFImageHeader,
  kGIFImageColormap,
  kGIFImageBody,
  kGIFLZWStart,
  GIFLZW,
  kGIFSubBlock,
  kGIFExtension,
  kGIFControlExtension,
  kGIFConsumeBlock,
  kGIFSkipBlock,
  kGIFDone,
  kGIFCommentExtension,
  kGIFApplicationExtension,
  kGIFNetscapeExtensionBlock,
  kGIFConsumeNetscapeExtension,
  kGIFConsumeComment
};

struct GIFFrameContext;

// LZW decoder state machine.
class GIFLZWContext final {
  USING_FAST_MALLOC(GIFLZWContext);
  WTF_MAKE_NONCOPYABLE(GIFLZWContext);

 public:
  GIFLZWContext(blink::GIFImageDecoder* client,
                const GIFFrameContext* frame_context)
      : codesize(0),
        codemask(0),
        clear_code(0),
        avail(0),
        oldcode(0),
        firstchar(0),
        bits(0),
        datum(0),
        ipass(0),
        irow(0),
        rows_remaining(0),
        row_iter(nullptr),
        client_(client),
        frame_context_(frame_context) {}

  bool PrepareToDecode();
  bool OutputRow(GIFRow::const_iterator row_begin);
  bool DoLZW(const unsigned char* block, size_t bytes_in_block);
  bool HasRemainingRows() { return rows_remaining; }

 private:
  enum {
    kMaxDictionaryEntryBits = 12,
    // 2^kMaxDictionaryEntryBits
    kMaxDictionaryEntries = 4096,
  };

  // LZW decoding states and output states.
  int codesize;
  int codemask;
  int clear_code;  // Codeword used to trigger dictionary reset.
  int avail;       // Index of next available slot in dictionary.
  int oldcode;
  unsigned char firstchar;
  int bits;               // Number of unread bits in "datum".
  int datum;              // 32-bit input buffer.
  int ipass;              // Interlace pass; Ranges 1-4 if interlaced.
  size_t irow;            // Current output row, starting at zero.
  size_t rows_remaining;  // Rows remaining to be output.

  unsigned short prefix[kMaxDictionaryEntries];
  unsigned char suffix[kMaxDictionaryEntries];
  unsigned short suffix_length[kMaxDictionaryEntries];
  GIFRow row_buffer;  // Single scanline temporary buffer.
  GIFRow::iterator row_iter;

  // Initialized during construction and read-only.
  blink::GIFImageDecoder* client_;
  const GIFFrameContext* frame_context_;
};

// Data structure for one LZW block.
struct GIFLZWBlock {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  GIFLZWBlock(size_t position, size_t size)
      : block_position(position), block_size(size) {}

  size_t block_position;
  size_t block_size;
};

class GIFColorMap final {
  DISALLOW_NEW();

 public:
  typedef Vector<blink::ImageFrame::PixelData> Table;

  GIFColorMap() : is_defined_(false), position_(0), colors_(0) {}

  // Set position and number of colors for the RGB table in the data stream.
  void SetTablePositionAndSize(size_t position, size_t colors) {
    position_ = position;
    colors_ = colors;
  }
  void SetDefined() { is_defined_ = true; }
  bool IsDefined() const { return is_defined_; }

  // Build RGBA table using the data stream.
  void BuildTable(blink::FastSharedBufferReader*);
  const Table& GetTable() const { return table_; }

 private:
  bool is_defined_;
  size_t position_;
  size_t colors_;
  Table table_;
};

// LocalFrame output state machine.
struct GIFFrameContext {
  USING_FAST_MALLOC(GIFFrameContext);
  WTF_MAKE_NONCOPYABLE(GIFFrameContext);

 public:
  GIFFrameContext(int id)
      : frame_id_(id),
        x_offset_(0),
        y_offset_(0),
        width_(0),
        height_(0),
        transparent_pixel_(kNotFound),
        disposal_method_(blink::ImageFrame::kDisposeNotSpecified),
        data_size_(0),
        progressive_display_(false),
        interlaced_(false),
        delay_time_(0),
        current_lzw_block_(0),
        is_complete_(false),
        is_header_defined_(false),
        is_data_size_defined_(false) {}

  ~GIFFrameContext() = default;

  void AddLzwBlock(size_t position, size_t size) {
    lzw_blocks_.push_back(GIFLZWBlock(position, size));
  }

  bool Decode(blink::FastSharedBufferReader*,
              blink::GIFImageDecoder* client,
              bool* frame_decoded);

  int FrameId() const { return frame_id_; }
  void SetRect(unsigned x, unsigned y, unsigned width, unsigned height) {
    x_offset_ = x;
    y_offset_ = y;
    width_ = width;
    height_ = height;
  }
  blink::IntRect FrameRect() const {
    return blink::IntRect(x_offset_, y_offset_, width_, height_);
  }
  unsigned XOffset() const { return x_offset_; }
  unsigned YOffset() const { return y_offset_; }
  unsigned Width() const { return width_; }
  unsigned Height() const { return height_; }
  size_t TransparentPixel() const { return transparent_pixel_; }
  void SetTransparentPixel(size_t pixel) { transparent_pixel_ = pixel; }
  blink::ImageFrame::DisposalMethod GetDisposalMethod() const {
    return disposal_method_;
  }
  void SetDisposalMethod(blink::ImageFrame::DisposalMethod disposal_method) {
    disposal_method_ = disposal_method;
  }
  unsigned DelayTime() const { return delay_time_; }
  void SetDelayTime(unsigned delay) { delay_time_ = delay; }
  bool IsComplete() const { return is_complete_; }
  void SetComplete() { is_complete_ = true; }
  bool IsHeaderDefined() const { return is_header_defined_; }
  void SetHeaderDefined() { is_header_defined_ = true; }
  bool IsDataSizeDefined() const { return is_data_size_defined_; }
  int DataSize() const { return data_size_; }
  void SetDataSize(int size) {
    data_size_ = size;
    is_data_size_defined_ = true;
  }
  bool ProgressiveDisplay() const { return progressive_display_; }
  void SetProgressiveDisplay(bool progressive_display) {
    progressive_display_ = progressive_display;
  }
  bool Interlaced() const { return interlaced_; }
  void SetInterlaced(bool interlaced) { interlaced_ = interlaced; }

  void ClearDecodeState() { lzw_context_.reset(); }
  const GIFColorMap& LocalColorMap() const { return local_color_map_; }
  GIFColorMap& LocalColorMap() { return local_color_map_; }

 private:
  int frame_id_;
  unsigned x_offset_;
  unsigned y_offset_;  // With respect to "screen" origin.
  unsigned width_;
  unsigned height_;
  size_t transparent_pixel_;  // Index of transparent pixel. Value is kNotFound
                              // if there is no transparent pixel.
  blink::ImageFrame::DisposalMethod
      disposal_method_;  // Restore to background, leave in place, etc.
  int data_size_;

  bool progressive_display_;  // If true, do Haeberli interlace hack.
  bool interlaced_;           // True, if scanlines arrive interlaced order.

  unsigned delay_time_;  // Display time, in milliseconds, for this image in a
                         // multi-image GIF.

  std::unique_ptr<GIFLZWContext> lzw_context_;
  Vector<GIFLZWBlock> lzw_blocks_;  // LZW blocks for this frame.
  GIFColorMap local_color_map_;

  size_t current_lzw_block_;
  bool is_complete_;
  bool is_header_defined_;
  bool is_data_size_defined_;
};

class PLATFORM_EXPORT GIFImageReader final {
  USING_FAST_MALLOC(GIFImageReader);
  WTF_MAKE_NONCOPYABLE(GIFImageReader);

 public:
  GIFImageReader(blink::GIFImageDecoder* client = nullptr)
      : client_(client),
        state_(kGIFType),
        // Number of bytes for GIF type, either "GIF87a" or "GIF89a".
        bytes_to_consume_(6),
        bytes_read_(0),
        version_(0),
        screen_width_(0),
        screen_height_(0),
        sent_size_to_client_(false),
        loop_count_(kCLoopCountNotSeen),
        parse_completed_(false) {}

  ~GIFImageReader() = default;

  void SetData(scoped_refptr<blink::SegmentReader> data) {
    data_ = std::move(data);
  }
  bool Parse(blink::GIFImageDecoder::GIFParseQuery);
  bool Decode(size_t frame_index);

  size_t ImagesCount() const {
    if (frames_.IsEmpty())
      return 0;

    // This avoids counting an empty frame when the file is truncated right
    // after GIFControlExtension but before GIFImageHeader.
    // FIXME: This extra complexity is not necessary and we should just report
    // m_frames.size().
    return frames_.back()->IsHeaderDefined() ? frames_.size()
                                             : frames_.size() - 1;
  }
  int LoopCount() const { return loop_count_; }

  const GIFColorMap& GlobalColorMap() const { return global_color_map_; }

  const GIFFrameContext* FrameContext(size_t index) const {
    return index < frames_.size() ? frames_[index].get() : nullptr;
  }

  bool ParseCompleted() const { return parse_completed_; }

  void ClearDecodeState(size_t index) { frames_[index]->ClearDecodeState(); }

 private:
  bool ParseData(size_t data_position,
                 size_t len,
                 blink::GIFImageDecoder::GIFParseQuery);
  void SetRemainingBytes(size_t);

  void AddFrameIfNecessary();
  bool CurrentFrameIsFirstFrame() const {
    return frames_.IsEmpty() ||
           (frames_.size() == 1u && !frames_[0]->IsComplete());
  }

  blink::GIFImageDecoder* client_;

  // Parsing state machine.
  GIFState state_;           // Current decoder master state.
  size_t bytes_to_consume_;  // Number of bytes to consume for next stage of
                             // parsing.
  size_t bytes_read_;        // Number of bytes processed.

  // Global (multi-image) state.
  int version_;            // Either 89 for GIF89 or 87 for GIF87.
  unsigned screen_width_;  // Logical screen width & height.
  unsigned screen_height_;
  bool sent_size_to_client_;
  GIFColorMap global_color_map_;
  int loop_count_;  // Netscape specific extension block to control the number
                    // of animation loops a GIF renders.

  Vector<std::unique_ptr<GIFFrameContext>> frames_;

  scoped_refptr<blink::SegmentReader> data_;
  bool parse_completed_;
};

}  // namespace blink

#endif
