// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_VIDEO_PICTURE_H_
#define MEDIA_VIDEO_PICTURE_H_

#include <stdint.h>

#include <vector>

#include "gpu/command_buffer/common/mailbox.h"
#include "media/base/media_export.h"
#include "media/base/video_types.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace media {

// A picture buffer that is composed of one or more GLES2 textures.
// This is the media-namespace equivalent of PP_PictureBuffer_Dev.
class MEDIA_EXPORT PictureBuffer {
 public:
  using TextureIds = std::vector<uint32_t>;

  PictureBuffer(int32_t id, const gfx::Size& size);
  PictureBuffer(int32_t id,
                const gfx::Size& size,
                const TextureIds& client_texture_ids);
  PictureBuffer(int32_t id,
                const gfx::Size& size,
                const TextureIds& client_texture_ids,
                const TextureIds& service_texture_ids,
                uint32_t texture_target,
                VideoPixelFormat pixel_format);
  PictureBuffer(int32_t id,
                const gfx::Size& size,
                const TextureIds& client_texture_ids,
                const std::vector<gpu::Mailbox>& texture_mailboxes,
                uint32_t texture_target,
                VideoPixelFormat pixel_format);
  PictureBuffer(const PictureBuffer& other);
  ~PictureBuffer();

  // Returns the client-specified id of the buffer.
  int32_t id() const { return id_; }

  // Returns the size of the buffer.
  gfx::Size size() const { return size_; }

  void set_size(const gfx::Size& size) { size_ = size; }

  // The client texture ids, i.e., those returned by Chrome's GL service.
  const TextureIds& client_texture_ids() const { return client_texture_ids_; }

  // The service texture ids, i.e., the real platform ids corresponding to
  // |client_texture_ids|.
  const TextureIds& service_texture_ids() const { return service_texture_ids_; }

  uint32_t texture_target() const { return texture_target_; }

  VideoPixelFormat pixel_format() const { return pixel_format_; }

  gpu::Mailbox texture_mailbox(size_t plane) const;

 private:
  int32_t id_;
  gfx::Size size_;
  TextureIds client_texture_ids_;
  TextureIds service_texture_ids_;
  std::vector<gpu::Mailbox> texture_mailboxes_;
  uint32_t texture_target_ = 0;
  VideoPixelFormat pixel_format_ = PIXEL_FORMAT_UNKNOWN;
};

// A decoded picture frame.
// This is the media-namespace equivalent of PP_Picture_Dev.
class MEDIA_EXPORT Picture {
 public:
  // Defaults |size_changed_| to false. Size changed is currently only used
  // by AVDA and is set via set_size_changd().
  Picture(int32_t picture_buffer_id,
          int32_t bitstream_buffer_id,
          const gfx::Rect& visible_rect,
          const gfx::ColorSpace& color_space,
          bool allow_overlay);
  Picture(const Picture&);
  ~Picture();

  // Returns the id of the picture buffer where this picture is contained.
  int32_t picture_buffer_id() const { return picture_buffer_id_; }

  // Returns the id of the bitstream buffer from which this frame was decoded.
  int32_t bitstream_buffer_id() const { return bitstream_buffer_id_; }

  void set_bitstream_buffer_id(int32_t bitstream_buffer_id) {
    bitstream_buffer_id_ = bitstream_buffer_id;
  }

  // Returns the color space of the picture.
  const gfx::ColorSpace& color_space() const { return color_space_; }

  // Returns the visible rectangle of the picture. Its size may be smaller
  // than the size of the PictureBuffer, as it is the only visible part of the
  // Picture contained in the PictureBuffer.
  gfx::Rect visible_rect() const { return visible_rect_; }

  bool allow_overlay() const { return allow_overlay_; }

  // Returns true when the VDA has adjusted the resolution of this Picture
  // without requesting new PictureBuffers. GpuVideoDecoder should read this
  // as a signal to update the size of the corresponding PicutreBuffer using
  // visible_rect() upon receiving this Picture from a VDA.
  bool size_changed() const { return size_changed_; };

  void set_size_changed(bool size_changed) { size_changed_ = size_changed; }

  bool texture_owner() const { return texture_owner_; }

  void set_texture_owner(bool texture_owner) { texture_owner_ = texture_owner; }

  bool wants_promotion_hint() const { return wants_promotion_hint_; }

  void set_wants_promotion_hint(bool wants_promotion_hint) {
    wants_promotion_hint_ = wants_promotion_hint;
  }

 private:
  int32_t picture_buffer_id_;
  int32_t bitstream_buffer_id_;
  gfx::Rect visible_rect_;
  gfx::ColorSpace color_space_;
  bool allow_overlay_;
  bool size_changed_;
  bool texture_owner_;
  bool wants_promotion_hint_;
};

}  // namespace media

#endif  // MEDIA_VIDEO_PICTURE_H_
