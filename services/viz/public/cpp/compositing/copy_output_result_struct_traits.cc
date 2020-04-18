// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/viz/public/cpp/compositing/copy_output_result_struct_traits.h"

#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

// This class retains the SingleReleaseCallback of the CopyOutputResult that is
// being sent over mojo. A TextureReleaserPtr that talks to this impl
// object will be sent over mojo instead of the release_callback_ (which is not
// serializable). Once the client calls Release, the release_callback_ will be
// called. An object of this class will remain alive until the MessagePipe
// attached to it goes away (i.e. StrongBinding is used).
class TextureReleaserImpl : public viz::mojom::TextureReleaser {
 public:
  explicit TextureReleaserImpl(
      std::unique_ptr<viz::SingleReleaseCallback> release_callback)
      : release_callback_(std::move(release_callback)) {}

  // mojom::TextureReleaser implementation:
  void Release(const gpu::SyncToken& sync_token, bool is_lost) override {
    release_callback_->Run(sync_token, is_lost);
  }

 private:
  std::unique_ptr<viz::SingleReleaseCallback> release_callback_;
};

void Release(viz::mojom::TextureReleaserPtr ptr,
             const gpu::SyncToken& sync_token,
             bool is_lost) {
  ptr->Release(sync_token, is_lost);
}

}  // namespace

namespace mojo {

// static
viz::mojom::CopyOutputResultFormat
EnumTraits<viz::mojom::CopyOutputResultFormat, viz::CopyOutputResult::Format>::
    ToMojom(viz::CopyOutputResult::Format format) {
  switch (format) {
    case viz::CopyOutputResult::Format::RGBA_BITMAP:
      return viz::mojom::CopyOutputResultFormat::RGBA_BITMAP;
    case viz::CopyOutputResult::Format::RGBA_TEXTURE:
      return viz::mojom::CopyOutputResultFormat::RGBA_TEXTURE;
    case viz::CopyOutputResult::Format::I420_PLANES:
      break;  // Not intended for transport across service boundaries.
  }
  NOTREACHED();
  return viz::mojom::CopyOutputResultFormat::RGBA_BITMAP;
}

// static
bool EnumTraits<viz::mojom::CopyOutputResultFormat,
                viz::CopyOutputResult::Format>::
    FromMojom(viz::mojom::CopyOutputResultFormat input,
              viz::CopyOutputResult::Format* out) {
  switch (input) {
    case viz::mojom::CopyOutputResultFormat::RGBA_BITMAP:
      *out = viz::CopyOutputResult::Format::RGBA_BITMAP;
      return true;
    case viz::mojom::CopyOutputResultFormat::RGBA_TEXTURE:
      *out = viz::CopyOutputResult::Format::RGBA_TEXTURE;
      return true;
  }
  return false;
}

// static
viz::CopyOutputResult::Format
StructTraits<viz::mojom::CopyOutputResultDataView,
             std::unique_ptr<viz::CopyOutputResult>>::
    format(const std::unique_ptr<viz::CopyOutputResult>& result) {
  return result->format();
}

// static
const gfx::Rect& StructTraits<viz::mojom::CopyOutputResultDataView,
                              std::unique_ptr<viz::CopyOutputResult>>::
    rect(const std::unique_ptr<viz::CopyOutputResult>& result) {
  return result->rect();
}

// static
const SkBitmap& StructTraits<viz::mojom::CopyOutputResultDataView,
                             std::unique_ptr<viz::CopyOutputResult>>::
    bitmap(const std::unique_ptr<viz::CopyOutputResult>& result) {
  // This will return a non-drawable bitmap if the result was not
  // RGBA_BITMAP or if the result is empty.
  return result->AsSkBitmap();
}

// static
base::Optional<gpu::Mailbox>
StructTraits<viz::mojom::CopyOutputResultDataView,
             std::unique_ptr<viz::CopyOutputResult>>::
    mailbox(const std::unique_ptr<viz::CopyOutputResult>& result) {
  if (result->format() != viz::CopyOutputResult::Format::RGBA_TEXTURE)
    return base::nullopt;
  return result->GetTextureResult()->mailbox;
}

// static
base::Optional<gpu::SyncToken>
StructTraits<viz::mojom::CopyOutputResultDataView,
             std::unique_ptr<viz::CopyOutputResult>>::
    sync_token(const std::unique_ptr<viz::CopyOutputResult>& result) {
  if (result->format() != viz::CopyOutputResult::Format::RGBA_TEXTURE)
    return base::nullopt;
  return result->GetTextureResult()->sync_token;
}

// static
base::Optional<gfx::ColorSpace>
StructTraits<viz::mojom::CopyOutputResultDataView,
             std::unique_ptr<viz::CopyOutputResult>>::
    color_space(const std::unique_ptr<viz::CopyOutputResult>& result) {
  if (result->format() != viz::CopyOutputResult::Format::RGBA_TEXTURE)
    return base::nullopt;
  return result->GetTextureResult()->color_space;
}

// static
viz::mojom::TextureReleaserPtr
StructTraits<viz::mojom::CopyOutputResultDataView,
             std::unique_ptr<viz::CopyOutputResult>>::
    releaser(const std::unique_ptr<viz::CopyOutputResult>& result) {
  if (result->format() != viz::CopyOutputResult::Format::RGBA_TEXTURE)
    return nullptr;

  viz::mojom::TextureReleaserPtr releaser;
  MakeStrongBinding(
      std::make_unique<TextureReleaserImpl>(result->TakeTextureOwnership()),
      MakeRequest(&releaser));
  return releaser;
}

// static
bool StructTraits<viz::mojom::CopyOutputResultDataView,
                  std::unique_ptr<viz::CopyOutputResult>>::
    Read(viz::mojom::CopyOutputResultDataView data,
         std::unique_ptr<viz::CopyOutputResult>* out_p) {
  // First read into local variables, and then instantiate an appropriate
  // implementation of viz::CopyOutputResult.
  viz::CopyOutputResult::Format format;
  gfx::Rect rect;

  if (!data.ReadFormat(&format) || !data.ReadRect(&rect))
    return false;

  switch (format) {
    case viz::CopyOutputResult::Format::RGBA_BITMAP: {
      SkBitmap bitmap;
      if (!data.ReadBitmap(&bitmap))
        return false;

      bool has_bitmap = bitmap.readyToDraw();

      // The rect should be empty iff there is no bitmap.
      if (!(has_bitmap == !rect.IsEmpty()))
        return false;

      *out_p = std::make_unique<viz::CopyOutputSkBitmapResult>(
          rect, std::move(bitmap));
      return true;
    }

    case viz::CopyOutputResult::Format::RGBA_TEXTURE: {
      base::Optional<gpu::Mailbox> mailbox;
      if (!data.ReadMailbox(&mailbox) || !mailbox)
        return false;
      base::Optional<gpu::SyncToken> sync_token;
      if (!data.ReadSyncToken(&sync_token) || !sync_token)
        return false;
      base::Optional<gfx::ColorSpace> color_space;
      if (!data.ReadColorSpace(&color_space) || !color_space)
        return false;

      bool has_mailbox = !mailbox->IsZero();

      // The rect should be empty iff there is no texture.
      if (!(has_mailbox == !rect.IsEmpty()))
        return false;

      if (!has_mailbox) {
        // Returns an empty result.
        *out_p = std::make_unique<viz::CopyOutputResult>(
            viz::CopyOutputResult::Format::RGBA_TEXTURE, gfx::Rect());
        return true;
      }

      viz::mojom::TextureReleaserPtr releaser =
          data.TakeReleaser<viz::mojom::TextureReleaserPtr>();
      if (!releaser)
        return false;  // Illegal to provide texture without Releaser.

      // Returns a result with a SingleReleaseCallback that will return
      // here and proxy the callback over mojo to the CopyOutputResult's
      // origin via the TextureReleaserPtr.
      *out_p = std::make_unique<viz::CopyOutputTextureResult>(
          rect, *mailbox, *sync_token, *color_space,
          viz::SingleReleaseCallback::Create(
              base::Bind(&Release, base::Passed(&releaser))));
      return true;
    }

    case viz::CopyOutputResult::Format::I420_PLANES:
      break;  // Not intended for transport across service boundaries.
  }

  NOTREACHED();
  return false;
}

}  // namespace mojo
