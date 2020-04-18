// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/messaging/blink_transferable_message_struct_traits.h"

#include "mojo/public/cpp/base/big_buffer_mojom_traits.h"
#include "third_party/blink/renderer/core/imagebitmap/image_bitmap.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace mojo {

namespace {

scoped_refptr<blink::StaticBitmapImage> ToStaticBitmapImage(
    const SkBitmap& sk_bitmap) {
  auto handle = WTF::ArrayBufferContents::CreateDataHandle(
      sk_bitmap.computeByteSize(), WTF::ArrayBufferContents::kZeroInitialize);
  if (!handle)
    return nullptr;

  WTF::ArrayBufferContents array_buffer_contents(
      std::move(handle), WTF::ArrayBufferContents::kNotShared);
  if (!array_buffer_contents.Data())
    return nullptr;

  SkImageInfo info = sk_bitmap.info();
  if (!sk_bitmap.readPixels(info, array_buffer_contents.Data(),
                            info.minRowBytes(), 0, 0,
                            SkTransferFunctionBehavior::kIgnore))
    return nullptr;

  return blink::StaticBitmapImage::Create(array_buffer_contents, info);
}

bool ToSkBitmap(
    const scoped_refptr<blink::StaticBitmapImage>& static_bitmap_image,
    SkBitmap& dest) {
  const sk_sp<SkImage> image =
      static_bitmap_image->PaintImageForCurrentFrame().GetSkImage();
  return image && image->asLegacyBitmap(
                      &dest, SkImage::LegacyBitmapMode::kRO_LegacyBitmapMode);
}

}  // namespace

Vector<SkBitmap>
StructTraits<blink::mojom::blink::TransferableMessage::DataView,
             blink::BlinkTransferableMessage>::
    image_bitmap_contents_array(const blink::BlinkCloneableMessage& input) {
  Vector<SkBitmap> out;
  out.ReserveInitialCapacity(
      input.message->GetImageBitmapContentsArray().size());
  for (auto& bitmap_contents : input.message->GetImageBitmapContentsArray()) {
    SkBitmap bitmap;
    if (!ToSkBitmap(bitmap_contents, bitmap)) {
      return Vector<SkBitmap>();
    }
    out.push_back(std::move(bitmap));
  }
  return out;
}

bool StructTraits<blink::mojom::blink::TransferableMessage::DataView,
                  blink::BlinkTransferableMessage>::
    Read(blink::mojom::blink::TransferableMessage::DataView data,
         blink::BlinkTransferableMessage* out) {
  Vector<mojo::ScopedMessagePipeHandle> ports;
  blink::SerializedScriptValue::ArrayBufferContentsArray
      array_buffer_contents_array;
  Vector<SkBitmap> sk_bitmaps;
  if (!data.ReadMessage(static_cast<blink::BlinkCloneableMessage*>(out)) ||
      !data.ReadArrayBufferContentsArray(&array_buffer_contents_array) ||
      !data.ReadImageBitmapContentsArray(&sk_bitmaps) ||
      !data.ReadPorts(&ports)) {
    return false;
  }

  out->ports.ReserveInitialCapacity(ports.size());
  out->ports.AppendRange(std::make_move_iterator(ports.begin()),
                         std::make_move_iterator(ports.end()));
  out->has_user_gesture = data.has_user_gesture();

  out->message->SetArrayBufferContentsArray(
      std::move(array_buffer_contents_array));
  array_buffer_contents_array.clear();

  // Bitmaps are serialized in mojo as SkBitmaps to leverage existing
  // serialization logic, but SerializedScriptValue uses StaticBitmapImage, so
  // the SkBitmaps need to be converted to StaticBitmapImages.
  blink::SerializedScriptValue::ImageBitmapContentsArray
      image_bitmap_contents_array;
  for (auto& sk_bitmap : sk_bitmaps) {
    const scoped_refptr<blink::StaticBitmapImage> bitmap_contents =
        ToStaticBitmapImage(sk_bitmap);
    if (!bitmap_contents) {
      return false;
    }
    image_bitmap_contents_array.push_back(bitmap_contents);
  }
  out->message->SetImageBitmapContentsArray(image_bitmap_contents_array);

  return true;
}

bool StructTraits<blink::mojom::blink::SerializedArrayBufferContents::DataView,
                  WTF::ArrayBufferContents>::
    Read(blink::mojom::blink::SerializedArrayBufferContents::DataView data,
         WTF::ArrayBufferContents* out) {
  mojo_base::BigBufferView contents_view;
  if (!data.ReadContents(&contents_view))
    return false;
  auto contents_data = contents_view.data();
  auto handle = WTF::ArrayBufferContents::CreateDataHandle(
      contents_data.size(), WTF::ArrayBufferContents::kZeroInitialize);
  if (!handle)
    return false;

  WTF::ArrayBufferContents array_buffer_contents(
      std::move(handle), WTF::ArrayBufferContents::kNotShared);
  memcpy(array_buffer_contents.Data(), contents_data.data(),
         contents_data.size());
  *out = std::move(array_buffer_contents);
  return true;
}

}  // namespace mojo
