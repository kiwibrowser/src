// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/image-encoders/image_encoder_utils.h"

#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

const char ImageEncoderUtils::kDefaultMimeType[] = "image/png";

// This enum is used in a UMA histogram; the values should not be changed.
enum RequestedImageMimeType {
  kRequestedImageMimeTypePng = 0,
  kRequestedImageMimeTypeJpeg = 1,
  kRequestedImageMimeTypeWebp = 2,
  kRequestedImageMimeTypeGif = 3,
  kRequestedImageMimeTypeBmp = 4,
  kRequestedImageMimeTypeIco = 5,
  kRequestedImageMimeTypeTiff = 6,
  kRequestedImageMimeTypeUnknown = 7,
  kNumberOfRequestedImageMimeTypes
};

String ImageEncoderUtils::ToEncodingMimeType(const String& mime_type,
                                             const EncodeReason encode_reason) {
  String lowercase_mime_type = mime_type.DeprecatedLower();

  if (mime_type.IsNull())
    lowercase_mime_type = kDefaultMimeType;

  RequestedImageMimeType image_format;
  if (lowercase_mime_type == "image/png") {
    image_format = kRequestedImageMimeTypePng;
  } else if (lowercase_mime_type == "image/jpeg") {
    image_format = kRequestedImageMimeTypeJpeg;
  } else if (lowercase_mime_type == "image/webp") {
    image_format = kRequestedImageMimeTypeWebp;
  } else if (lowercase_mime_type == "image/gif") {
    image_format = kRequestedImageMimeTypeGif;
  } else if (lowercase_mime_type == "image/bmp" ||
             lowercase_mime_type == "image/x-windows-bmp") {
    image_format = kRequestedImageMimeTypeBmp;
  } else if (lowercase_mime_type == "image/x-icon") {
    image_format = kRequestedImageMimeTypeIco;
  } else if (lowercase_mime_type == "image/tiff" ||
             lowercase_mime_type == "image/x-tiff") {
    image_format = kRequestedImageMimeTypeTiff;
  } else {
    image_format = kRequestedImageMimeTypeUnknown;
  }

  if (encode_reason == kEncodeReasonToDataURL) {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(EnumerationHistogram,
                                    to_data_url_image_format_histogram,
                                    ("Canvas.RequestedImageMimeTypes_toDataURL",
                                     kNumberOfRequestedImageMimeTypes));
    to_data_url_image_format_histogram.Count(image_format);
  } else if (encode_reason == kEncodeReasonToBlobCallback) {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        EnumerationHistogram, to_blob_callback_image_format_histogram,
        ("Canvas.RequestedImageMimeTypes_toBlobCallback",
         kNumberOfRequestedImageMimeTypes));
    to_blob_callback_image_format_histogram.Count(image_format);
  } else if (encode_reason == kEncodeReasonConvertToBlobPromise) {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        EnumerationHistogram, convert_to_blob_promise_image_format_histogram,
        ("Canvas.RequestedImageMimeTypes_convertToBlobPromise",
         kNumberOfRequestedImageMimeTypes));
    convert_to_blob_promise_image_format_histogram.Count(image_format);
  }

  // FIXME: Make isSupportedImageMIMETypeForEncoding threadsafe (to allow this
  // method to be used on a worker thread).
  if (!MIMETypeRegistry::IsSupportedImageMIMETypeForEncoding(
          lowercase_mime_type))
    lowercase_mime_type = kDefaultMimeType;
  return lowercase_mime_type;
}

}  // namespace blink
