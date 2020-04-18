/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chris Saari <saari@netscape.com>
 *   Apple Computer
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

/*
The Graphics Interchange Format(c) is the copyright property of CompuServe
Incorporated. Only CompuServe Incorporated is authorized to define, redefine,
enhance, alter, modify or change in any way the definition of the format.

CompuServe Incorporated hereby grants a limited, non-exclusive, royalty-free
license for the use of the Graphics Interchange Format(sm) in computer
software; computer software utilizing GIF(sm) must acknowledge ownership of the
Graphics Interchange Format and its Service Mark by CompuServe Incorporated, in
User and Technical Documentation. Computer software utilizing GIF, which is
distributed or may be distributed without User or Technical Documentation must
display to the screen or printer a message acknowledging ownership of the
Graphics Interchange Format and the Service Mark by CompuServe Incorporated; in
this case, the acknowledgement may be displayed in an opening screen or leading
banner, or a closing screen or trailing banner. A message such as the following
may be used:

    "The Graphics Interchange Format(c) is the Copyright property of
    CompuServe Incorporated. GIF(sm) is a Service Mark property of
    CompuServe Incorporated."

For further information, please contact :

    CompuServe Incorporated
    Graphics Technology Department
    5000 Arlington Center Boulevard
    Columbus, Ohio  43220
    U. S. A.

CompuServe Incorporated maintains a mailing list with all those individuals and
organizations who wish to receive copies of this document when it is corrected
or revised. This service is offered free of charge; please provide us with your
mailing address.
*/

#include "third_party/blink/renderer/platform/image-decoders/gif/gif_image_reader.h"

#include <string.h>
#include <algorithm>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/image-decoders/fast_shared_buffer_reader.h"

namespace blink {

namespace {

static constexpr unsigned kMaxColors = 256u;
static constexpr int kBytesPerColormapEntry = 3;

}  // namespace

// GETN(n, s) requests at least 'n' bytes available from 'q', at start of state
// 's'.
//
// Note: the hold will never need to be bigger than 256 bytes, as each GIF block
// (except colormaps) can never be bigger than 256 bytes. Colormaps are directly
// copied in the resp. global_colormap or dynamically allocated local_colormap,
// so a fixed buffer in GIFImageReader is good enough. This buffer is only
// needed to copy left-over data from one GifWrite call to the next.
#define GETN(n, s)           \
  do {                       \
    bytes_to_consume_ = (n); \
    state_ = (s);            \
  } while (0)

// Get a 16-bit value stored in little-endian format.
#define GETINT16(p) ((p)[1] << 8 | (p)[0])

// Send the data to the display front-end.
bool GIFLZWContext::OutputRow(GIFRow::const_iterator row_begin) {
  int drow_start = irow;
  int drow_end = irow;

  // Haeberli-inspired hack for interlaced GIFs: Replicate lines while
  // displaying to diminish the "venetian-blind" effect as the image is
  // loaded. Adjust pixel vertical positions to avoid the appearance of the
  // image crawling up the screen as successive passes are drawn.
  if (frame_context_->ProgressiveDisplay() && frame_context_->Interlaced() &&
      ipass < 4) {
    unsigned row_dup = 0;
    unsigned row_shift = 0;

    switch (ipass) {
      case 1:
        row_dup = 7;
        row_shift = 3;
        break;
      case 2:
        row_dup = 3;
        row_shift = 1;
        break;
      case 3:
        row_dup = 1;
        row_shift = 0;
        break;
      default:
        break;
    }

    drow_start -= row_shift;
    drow_end = drow_start + row_dup;

    // Extend if bottom edge isn't covered because of the shift upward.
    if (((frame_context_->Height() - 1) - drow_end) <= row_shift)
      drow_end = frame_context_->Height() - 1;

    // Clamp first and last rows to upper and lower edge of image.
    if (drow_start < 0)
      drow_start = 0;

    if ((unsigned)drow_end >= frame_context_->Height())
      drow_end = frame_context_->Height() - 1;
  }

  // Protect against too much image data.
  if ((unsigned)drow_start >= frame_context_->Height())
    return true;

  // CALLBACK: Let the client know we have decoded a row.
  if (!client_->HaveDecodedRow(frame_context_->FrameId(), row_begin,
                               frame_context_->Width(), drow_start,
                               drow_end - drow_start + 1,
                               frame_context_->ProgressiveDisplay() &&
                                   frame_context_->Interlaced() && ipass > 1))
    return false;

  if (!frame_context_->Interlaced()) {
    irow++;
  } else {
    do {
      switch (ipass) {
        case 1:
          irow += 8;
          if (irow >= frame_context_->Height()) {
            ipass++;
            irow = 4;
          }
          break;

        case 2:
          irow += 8;
          if (irow >= frame_context_->Height()) {
            ipass++;
            irow = 2;
          }
          break;

        case 3:
          irow += 4;
          if (irow >= frame_context_->Height()) {
            ipass++;
            irow = 1;
          }
          break;

        case 4:
          irow += 2;
          if (irow >= frame_context_->Height()) {
            ipass++;
            irow = 0;
          }
          break;

        default:
          break;
      }
    } while (irow > (frame_context_->Height() - 1));
  }
  return true;
}

// Performs Lempel-Ziv-Welch decoding. Returns whether decoding was successful.
// If successful, the block will have been completely consumed and/or
// rowsRemaining will be 0.
bool GIFLZWContext::DoLZW(const unsigned char* block, size_t bytes_in_block) {
  const size_t width = frame_context_->Width();

  if (row_iter == row_buffer.end())
    return true;

  for (const unsigned char* ch = block; bytes_in_block-- > 0; ch++) {
    // Feed the next byte into the decoder's 32-bit input buffer.
    datum += ((int)*ch) << bits;
    bits += 8;

    // Check for underflow of decoder's 32-bit input buffer.
    while (bits >= codesize) {
      // Get the leading variable-length symbol from the data stream.
      int code = datum & codemask;
      datum >>= codesize;
      bits -= codesize;

      // Reset the dictionary to its original state, if requested.
      if (code == clear_code) {
        codesize = frame_context_->DataSize() + 1;
        codemask = (1 << codesize) - 1;
        avail = clear_code + 2;
        oldcode = -1;
        continue;
      }

      // Check for explicit end-of-stream code.
      if (code == (clear_code + 1)) {
        // end-of-stream should only appear after all image data.
        if (!rows_remaining)
          return true;
        return false;
      }

      const int temp_code = code;
      unsigned short code_length = 0;
      if (code < avail) {
        // This is a pre-existing code, so we already know what it
        // encodes.
        code_length = suffix_length[code];
        row_iter += code_length;
      } else if (code == avail && oldcode != -1) {
        // This is a new code just being added to the dictionary.
        // It must encode the contents of the previous code, plus
        // the first character of the previous code again.
        code_length = suffix_length[oldcode] + 1;
        row_iter += code_length;
        *--row_iter = firstchar;
        code = oldcode;
      } else {
        // This is an invalid code. The dictionary is just initialized
        // and the code is incomplete. We don't know how to handle
        // this case.
        return false;
      }

      while (code >= clear_code) {
        *--row_iter = suffix[code];
        code = prefix[code];
      }

      *--row_iter = firstchar = suffix[code];

      // Define a new codeword in the dictionary as long as we've read
      // more than one value from the stream.
      if (avail < kMaxDictionaryEntries && oldcode != -1) {
        prefix[avail] = oldcode;
        suffix[avail] = firstchar;
        suffix_length[avail] = suffix_length[oldcode] + 1;
        ++avail;

        // If we've used up all the codewords of a given length
        // increase the length of codewords by one bit, but don't
        // exceed the specified maximum codeword size.
        if (!(avail & codemask) && avail < kMaxDictionaryEntries) {
          ++codesize;
          codemask += avail;
        }
      }
      oldcode = temp_code;
      row_iter += code_length;

      // Output as many rows as possible.
      GIFRow::iterator row_begin = row_buffer.begin();
      for (; row_begin + width <= row_iter; row_begin += width) {
        if (!OutputRow(row_begin))
          return false;
        rows_remaining--;
        if (!rows_remaining)
          return true;
      }

      if (row_begin != row_buffer.begin()) {
        // Move the remaining bytes to the beginning of the buffer.
        const size_t bytes_to_copy = row_iter - row_begin;
        memcpy(row_buffer.begin(), row_begin, bytes_to_copy);
        row_iter = row_buffer.begin() + bytes_to_copy;
      }
    }
  }
  return true;
}

void GIFColorMap::BuildTable(FastSharedBufferReader* reader) {
  if (!is_defined_ || !table_.IsEmpty())
    return;

  CHECK_LE(position_ + colors_ * kBytesPerColormapEntry, reader->size());
  DCHECK_LE(colors_, kMaxColors);
  char buffer[kMaxColors * kBytesPerColormapEntry];
  const unsigned char* src_colormap =
      reinterpret_cast<const unsigned char*>(reader->GetConsecutiveData(
          position_, colors_ * kBytesPerColormapEntry, buffer));
  table_.resize(colors_);
  for (Table::iterator iter = table_.begin(); iter != table_.end(); ++iter) {
    *iter = SkPackARGB32NoCheck(255, src_colormap[0], src_colormap[1],
                                src_colormap[2]);
    src_colormap += kBytesPerColormapEntry;
  }
}

// Decodes this frame. |frameDecoded| will be set to true if the entire frame is
// decoded. Returns true if decoding progressed further than before without
// error, or there is insufficient new data to decode further. Otherwise, a
// decoding error occurred; returns false in this case.
bool GIFFrameContext::Decode(FastSharedBufferReader* reader,
                             GIFImageDecoder* client,
                             bool* frame_decoded) {
  local_color_map_.BuildTable(reader);

  *frame_decoded = false;
  if (!lzw_context_) {
    // Wait for more data to properly initialize GIFLZWContext.
    if (!IsDataSizeDefined() || !IsHeaderDefined())
      return true;

    lzw_context_ = std::make_unique<GIFLZWContext>(client, this);
    if (!lzw_context_->PrepareToDecode()) {
      lzw_context_.reset();
      return false;
    }

    current_lzw_block_ = 0;
  }

  // Some bad GIFs have extra blocks beyond the last row, which we don't want to
  // decode.
  while (current_lzw_block_ < lzw_blocks_.size() &&
         lzw_context_->HasRemainingRows()) {
    size_t block_position = lzw_blocks_[current_lzw_block_].block_position;
    size_t block_size = lzw_blocks_[current_lzw_block_].block_size;
    if (block_position + block_size > reader->size())
      return false;

    while (block_size) {
      const char* segment = nullptr;
      size_t segment_length = reader->GetSomeData(segment, block_position);
      size_t decode_size = std::min(segment_length, block_size);
      if (!lzw_context_->DoLZW(reinterpret_cast<const unsigned char*>(segment),
                               decode_size))
        return false;
      block_position += decode_size;
      block_size -= decode_size;
    }
    ++current_lzw_block_;
  }

  // If this frame is data complete then the previous loop must have completely
  // decoded all LZW blocks.
  // There will be no more decoding for this frame so it's time to cleanup.
  if (IsComplete()) {
    *frame_decoded = true;
    lzw_context_.reset();
  }
  return true;
}

// Decodes a frame using GIFFrameContext:decode(). Returns true if decoding has
// progressed, or false if an error has occurred.
bool GIFImageReader::Decode(size_t frame_index) {
  FastSharedBufferReader reader(data_);
  global_color_map_.BuildTable(&reader);

  bool frame_decoded = false;
  GIFFrameContext* current_frame = frames_[frame_index].get();

  return current_frame->Decode(&reader, client_, &frame_decoded) &&
         (!frame_decoded || client_->FrameComplete(frame_index));
}

bool GIFImageReader::Parse(GIFImageDecoder::GIFParseQuery query) {
  if (bytes_read_ >= data_->size()) {
    // This data has already been parsed. For example, in deferred
    // decoding, a DecodingImageGenerator with more data may have already
    // used this same ImageDecoder to decode. This can happen if two
    // SkImages created by a DeferredImageDecoder are drawn/prerolled
    // out of order (with respect to how much data they had at creation
    // time).
    return !client_->Failed();
  }

  return ParseData(bytes_read_, data_->size() - bytes_read_, query);
}

// Parse incoming GIF data stream into internal data structures.
// Return true if parsing has progressed or there is not enough data.
// Return false if a fatal error is encountered.
bool GIFImageReader::ParseData(size_t data_position,
                               size_t len,
                               GIFImageDecoder::GIFParseQuery query) {
  if (!len) {
    // No new data has come in since the last call, just ignore this call.
    return true;
  }

  if (len < bytes_to_consume_)
    return true;

  FastSharedBufferReader reader(data_);

  // A read buffer of 16 bytes is enough to accomodate all possible reads for
  // parsing.
  char read_buffer[16];

  // Read as many components from |m_data| as possible. At the beginning of each
  // iteration, |dataPosition| is advanced by m_bytesToConsume to point to the
  // next component. |len| is decremented accordingly.
  while (len >= bytes_to_consume_) {
    const size_t current_component_position = data_position;

    // Mark the current component as consumed. Note that currentComponent will
    // remain pointed at this component until the next loop iteration.
    data_position += bytes_to_consume_;
    len -= bytes_to_consume_;

    switch (state_) {
      case GIFLZW:
        DCHECK(!frames_.IsEmpty());
        // m_bytesToConsume is the current component size because it hasn't been
        // updated.
        frames_.back()->AddLzwBlock(current_component_position,
                                    bytes_to_consume_);
        GETN(1, kGIFSubBlock);
        break;

      case kGIFLZWStart: {
        DCHECK(!frames_.IsEmpty());
        frames_.back()->SetDataSize(static_cast<unsigned char>(
            reader.GetOneByte(current_component_position)));
        GETN(1, kGIFSubBlock);
        break;
      }

      case kGIFType: {
        const char* current_component = reader.GetConsecutiveData(
            current_component_position, 6, read_buffer);

        // All GIF files begin with "GIF87a" or "GIF89a".
        if (!memcmp(current_component, "GIF89a", 6))
          version_ = 89;
        else if (!memcmp(current_component, "GIF87a", 6))
          version_ = 87;
        else
          return false;
        GETN(7, kGIFGlobalHeader);
        break;
      }

      case kGIFGlobalHeader: {
        const unsigned char* current_component =
            reinterpret_cast<const unsigned char*>(reader.GetConsecutiveData(
                current_component_position, 5, read_buffer));

        // This is the height and width of the "screen" or frame into which
        // images are rendered. The individual images can be smaller than
        // the screen size and located with an origin anywhere within the
        // screen.
        // Note that we don't inform the client of the size yet, as it might
        // change after we read the first frame's image header.
        screen_width_ = GETINT16(current_component);
        screen_height_ = GETINT16(current_component + 2);

        const size_t global_color_map_colors = 2
                                               << (current_component[4] & 0x07);

        if ((current_component[4] & 0x80) &&
            global_color_map_colors > 0) { /* global map */
          global_color_map_.SetTablePositionAndSize(data_position,
                                                    global_color_map_colors);
          GETN(kBytesPerColormapEntry * global_color_map_colors,
               kGIFGlobalColormap);
          break;
        }

        GETN(1, kGIFImageStart);
        break;
      }

      case kGIFGlobalColormap: {
        global_color_map_.SetDefined();
        GETN(1, kGIFImageStart);
        break;
      }

      case kGIFImageStart: {
        const char current_component =
            reader.GetOneByte(current_component_position);

        if (current_component == '!') {  // extension.
          GETN(2, kGIFExtension);
          break;
        }

        if (current_component == ',') {  // image separator.
          GETN(9, kGIFImageHeader);
          break;
        }

        // If we get anything other than ',' (image separator), '!'
        // (extension), or ';' (trailer), there is extraneous data
        // between blocks. The GIF87a spec tells us to keep reading
        // until we find an image separator, but GIF89a says such
        // a file is corrupt. We follow Mozilla's implementation and
        // proceed as if the file were correctly terminated, so the
        // GIF will display.
        GETN(0, kGIFDone);
        break;
      }

      case kGIFExtension: {
        const unsigned char* current_component =
            reinterpret_cast<const unsigned char*>(reader.GetConsecutiveData(
                current_component_position, 2, read_buffer));

        size_t bytes_in_block = current_component[1];
        GIFState exception_state = kGIFSkipBlock;

        switch (*current_component) {
          case 0xf9:
            exception_state = kGIFControlExtension;
            // The GIF spec mandates that the GIFControlExtension header block
            // length is 4 bytes, and the parser for this block reads 4 bytes,
            // so we must enforce that the buffer contains at least this many
            // bytes. If the GIF specifies a different length, we allow that, so
            // long as it's larger; the additional data will simply be ignored.
            bytes_in_block = std::max(bytes_in_block, static_cast<size_t>(4));
            break;

          // The GIF spec also specifies the lengths of the following two
          // extensions' headers (as 12 and 11 bytes, respectively). Because we
          // ignore the plain text extension entirely and sanity-check the
          // actual length of the application extension header before reading
          // it, we allow GIFs to deviate from these values in either direction.
          // This is important for real-world compatibility, as GIFs in the wild
          // exist with application extension headers that are both shorter and
          // longer than 11 bytes.
          case 0x01:
            // ignoring plain text extension
            break;

          case 0xff:
            exception_state = kGIFApplicationExtension;
            break;

          case 0xfe:
            exception_state = kGIFConsumeComment;
            break;
        }

        if (bytes_in_block)
          GETN(bytes_in_block, exception_state);
        else
          GETN(1, kGIFImageStart);
        break;
      }

      case kGIFConsumeBlock: {
        const unsigned char current_component = static_cast<unsigned char>(
            reader.GetOneByte(current_component_position));
        if (!current_component)
          GETN(1, kGIFImageStart);
        else
          GETN(current_component, kGIFSkipBlock);
        break;
      }

      case kGIFSkipBlock: {
        GETN(1, kGIFConsumeBlock);
        break;
      }

      case kGIFControlExtension: {
        const unsigned char* current_component =
            reinterpret_cast<const unsigned char*>(reader.GetConsecutiveData(
                current_component_position, 4, read_buffer));

        AddFrameIfNecessary();
        GIFFrameContext* current_frame = frames_.back().get();
        if (*current_component & 0x1)
          current_frame->SetTransparentPixel(current_component[3]);

        // We ignore the "user input" bit.

        // NOTE: This relies on the values in the FrameDisposalMethod enum
        // matching those in the GIF spec!
        int disposal_method = ((*current_component) >> 2) & 0x7;
        if (disposal_method < 4) {
          current_frame->SetDisposalMethod(
              static_cast<ImageFrame::DisposalMethod>(disposal_method));
        } else if (disposal_method == 4) {
          // Some specs say that disposal method 3 is "overwrite previous",
          // others that setting the third bit of the field (i.e. method 4) is.
          // We map both to the same value.
          current_frame->SetDisposalMethod(
              ImageFrame::kDisposeOverwritePrevious);
        }
        current_frame->SetDelayTime(GETINT16(current_component + 1) * 10);
        GETN(1, kGIFConsumeBlock);
        break;
      }

      case kGIFCommentExtension: {
        const unsigned char current_component = static_cast<unsigned char>(
            reader.GetOneByte(current_component_position));
        if (current_component)
          GETN(current_component, kGIFConsumeComment);
        else
          GETN(1, kGIFImageStart);
        break;
      }

      case kGIFConsumeComment: {
        GETN(1, kGIFCommentExtension);
        break;
      }

      case kGIFApplicationExtension: {
        // Check for netscape application extension.
        if (bytes_to_consume_ == 11) {
          const unsigned char* current_component =
              reinterpret_cast<const unsigned char*>(reader.GetConsecutiveData(
                  current_component_position, 11, read_buffer));

          if (!memcmp(current_component, "NETSCAPE2.0", 11) ||
              !memcmp(current_component, "ANIMEXTS1.0", 11))
            GETN(1, kGIFNetscapeExtensionBlock);
        }

        if (state_ != kGIFNetscapeExtensionBlock)
          GETN(1, kGIFConsumeBlock);
        break;
      }

      // Netscape-specific GIF extension: animation looping.
      case kGIFNetscapeExtensionBlock: {
        const int current_component = static_cast<unsigned char>(
            reader.GetOneByte(current_component_position));
        // GIFConsumeNetscapeExtension always reads 3 bytes from the stream; we
        // should at least wait for this amount.
        if (current_component)
          GETN(std::max(3, current_component), kGIFConsumeNetscapeExtension);
        else
          GETN(1, kGIFImageStart);
        break;
      }

      // Parse netscape-specific application extensions
      case kGIFConsumeNetscapeExtension: {
        const unsigned char* current_component =
            reinterpret_cast<const unsigned char*>(reader.GetConsecutiveData(
                current_component_position, 3, read_buffer));

        int netscape_extension = current_component[0] & 7;

        // Loop entire animation specified # of times. Only read the loop count
        // during the first iteration.
        if (netscape_extension == 1) {
          loop_count_ = GETINT16(current_component + 1);

          // Zero loop count is infinite animation loop request.
          if (!loop_count_)
            loop_count_ = kAnimationLoopInfinite;

          GETN(1, kGIFNetscapeExtensionBlock);
        } else if (netscape_extension == 2) {
          // Wait for specified # of bytes to enter buffer.

          // Don't do this, this extension doesn't exist (isn't used at all)
          // and doesn't do anything, as our streaming/buffering takes care of
          // it all. See http://semmix.pl/color/exgraf/eeg24.htm .
          GETN(1, kGIFNetscapeExtensionBlock);
        } else {
          // 0,3-7 are yet to be defined netscape extension codes
          return false;
        }
        break;
      }

      case kGIFImageHeader: {
        unsigned height, width, x_offset, y_offset;
        const unsigned char* current_component =
            reinterpret_cast<const unsigned char*>(reader.GetConsecutiveData(
                current_component_position, 9, read_buffer));

        /* Get image offsets, with respect to the screen origin */
        x_offset = GETINT16(current_component);
        y_offset = GETINT16(current_component + 2);

        /* Get image width and height. */
        width = GETINT16(current_component + 4);
        height = GETINT16(current_component + 6);

        // Some GIF files have frames that don't fit in the specified
        // overall image size. For the first frame, we can simply enlarge
        // the image size to allow the frame to be visible.  We can't do
        // this on subsequent frames because the rest of the decoding
        // infrastructure assumes the image size won't change as we
        // continue decoding, so any subsequent frames that are even
        // larger will be cropped.
        // Luckily, handling just the first frame is sufficient to deal
        // with most cases, e.g. ones where the image size is erroneously
        // set to zero, since usually the first frame completely fills
        // the image.
        if (CurrentFrameIsFirstFrame()) {
          screen_height_ = std::max(screen_height_, y_offset + height);
          screen_width_ = std::max(screen_width_, x_offset + width);
        }

        // Inform the client of the final size.
        if (!sent_size_to_client_ && client_ &&
            !client_->SetSize(screen_width_, screen_height_))
          return false;
        sent_size_to_client_ = true;

        if (query == GIFImageDecoder::kGIFSizeQuery) {
          // The decoder needs to stop. Hand back the number of bytes we
          // consumed from the buffer minus 9 (the amount we consumed to read
          // the header).
          SetRemainingBytes(len + 9);
          GETN(9, kGIFImageHeader);
          return true;
        }

        AddFrameIfNecessary();
        GIFFrameContext* current_frame = frames_.back().get();

        current_frame->SetHeaderDefined();

        // Work around more broken GIF files that have zero image width or
        // height.
        if (!height || !width) {
          height = screen_height_;
          width = screen_width_;
          if (!height || !width)
            return false;
        }
        current_frame->SetRect(x_offset, y_offset, width, height);
        current_frame->SetInterlaced(current_component[8] & 0x40);

        // Overlaying interlaced, transparent GIFs over
        // existing image data using the Haeberli display hack
        // requires saving the underlying image in order to
        // avoid jaggies at the transparency edges. We are
        // unprepared to deal with that, so don't display such
        // images progressively. Which means only the first
        // frame can be progressively displayed.
        // FIXME: It is possible that a non-transparent frame
        // can be interlaced and progressively displayed.
        current_frame->SetProgressiveDisplay(CurrentFrameIsFirstFrame());

        const bool is_local_colormap_defined = current_component[8] & 0x80;
        if (is_local_colormap_defined) {
          // The three low-order bits of currentComponent[8] specify the bits
          // per pixel.
          const size_t num_colors = 2 << (current_component[8] & 0x7);
          current_frame->LocalColorMap().SetTablePositionAndSize(data_position,
                                                                 num_colors);
          GETN(kBytesPerColormapEntry * num_colors, kGIFImageColormap);
          break;
        }

        GETN(1, kGIFLZWStart);
        break;
      }

      case kGIFImageColormap: {
        DCHECK(!frames_.IsEmpty());
        frames_.back()->LocalColorMap().SetDefined();
        GETN(1, kGIFLZWStart);
        break;
      }

      case kGIFSubBlock: {
        const size_t bytes_in_block = static_cast<unsigned char>(
            reader.GetOneByte(current_component_position));
        if (bytes_in_block) {
          GETN(bytes_in_block, GIFLZW);
        } else {
          // Finished parsing one frame; Process next frame.
          DCHECK(!frames_.IsEmpty());
          // Note that some broken GIF files do not have enough LZW blocks to
          // fully decode all rows; we treat this case as "frame complete".
          frames_.back()->SetComplete();
          GETN(1, kGIFImageStart);
        }
        break;
      }

      case kGIFDone: {
        parse_completed_ = true;
        return true;
      }

      default:
        // We shouldn't ever get here.
        return false;
        break;
    }
  }

  SetRemainingBytes(len);
  return true;
}

void GIFImageReader::SetRemainingBytes(size_t remaining_bytes) {
  DCHECK_LE(remaining_bytes, data_->size());
  bytes_read_ = data_->size() - remaining_bytes;
}

void GIFImageReader::AddFrameIfNecessary() {
  if (frames_.IsEmpty() || frames_.back()->IsComplete())
    frames_.push_back(base::WrapUnique(new GIFFrameContext(frames_.size())));
}

// FIXME: Move this method to close to doLZW().
bool GIFLZWContext::PrepareToDecode() {
  DCHECK(frame_context_->IsDataSizeDefined());
  DCHECK(frame_context_->IsHeaderDefined());

  // Since we use a codesize of 1 more than the datasize, we need to ensure
  // that our datasize is strictly less than the kMaxDictionaryEntryBits.
  if (frame_context_->DataSize() >= kMaxDictionaryEntryBits)
    return false;
  clear_code = 1 << frame_context_->DataSize();
  avail = clear_code + 2;
  oldcode = -1;
  codesize = frame_context_->DataSize() + 1;
  codemask = (1 << codesize) - 1;
  datum = bits = 0;
  ipass = frame_context_->Interlaced() ? 1 : 0;
  irow = 0;

  // We want to know the longest sequence encodable by a dictionary with
  // kMaxDictionaryEntries entries. If we ignore the need to encode the base
  // values themselves at the beginning of the dictionary, as well as the need
  // for a clear code or a termination code, we could use every entry to
  // encode a series of multiple values. If the input value stream looked
  // like "AAAAA..." (a long string of just one value), the first dictionary
  // entry would encode AA, the next AAA, the next AAAA, and so forth. Thus
  // the longest sequence would be kMaxDictionaryEntries + 1 values.
  //
  // However, we have to account for reserved entries. The first |datasize|
  // bits are reserved for the base values, and the next two entries are
  // reserved for the clear code and termination code. In theory a GIF can
  // set the datasize to 0, meaning we have just two reserved entries, making
  // the longest sequence (kMaxDictionaryEntries + 1) - 2 values long. Since
  // each value is a byte, this is also the number of bytes in the longest
  // encodable sequence.
  const size_t kMaxBytes = kMaxDictionaryEntries - 1;

  // Now allocate the output buffer. We decode directly into this buffer
  // until we have at least one row worth of data, then call outputRow().
  // This means worst case we may have (row width - 1) bytes in the buffer
  // and then decode a sequence |maxBytes| long to append.
  row_buffer.resize(frame_context_->Width() - 1 + kMaxBytes);
  row_iter = row_buffer.begin();
  rows_remaining = frame_context_->Height();

  // Clearing the whole suffix table lets us be more tolerant of bad data.
  for (int i = 0; i < clear_code; ++i) {
    suffix[i] = i;
    suffix_length[i] = 1;
  }
  return true;
}

}  // namespace blink
