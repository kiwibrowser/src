// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_SIZES_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_SIZES_H_

#include <stddef.h>

#include <limits>

#include "base/macros.h"
#include "base/numerics/safe_math.h"
#include "cc/base/math_util.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "components/viz/common/viz_common_export.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

class VIZ_COMMON_EXPORT ResourceSizes {
 public:
  // Returns true if the width is valid and fits in bytes, false otherwise.
  template <typename T>
  static bool VerifyWidthInBytes(int width, ResourceFormat format);
  // Returns true if the size is valid and fits in bytes, false otherwise.
  template <typename T>
  static bool VerifySizeInBytes(const gfx::Size& size, ResourceFormat format);

  // Returns true if the width is valid and fits in bytes, false otherwise.
  // Sets the bytes result in the out parameter |bytes|.
  template <typename T>
  static bool MaybeWidthInBytes(int width, ResourceFormat format, T* bytes);
  // Returns true if the size is valid and fits in bytes, false otherwise.
  // Sets the bytes result in the out parameter |bytes|.
  template <typename T>
  static bool MaybeSizeInBytes(const gfx::Size& size,
                               ResourceFormat format,
                               T* bytes);

  // Dies with a CRASH() if the width can not be represented as a positive
  // number of bytes.
  template <typename T>
  static T CheckedWidthInBytes(int width, ResourceFormat format);
  // Dies with a CRASH() if the size can not be represented as a positive
  // number of bytes.
  template <typename T>
  static T CheckedSizeInBytes(const gfx::Size& size, ResourceFormat format);

  // Returns the width in bytes but may overflow or return 0. Only do this for
  // computing widths for sizes that have already been checked.
  template <typename T>
  static T UncheckedWidthInBytes(int width, ResourceFormat format);
  // Returns the size in bytes but may overflow or return 0. Only do this for
  // sizes that have already been checked.
  template <typename T>
  static T UncheckedSizeInBytes(const gfx::Size& size, ResourceFormat format);
  // Returns the width in bytes aligned but may overflow or return 0. Only do
  // this for computing widths for sizes that have already been checked.
  template <typename T>
  static T UncheckedWidthInBytesAligned(int width, ResourceFormat format);
  // Returns the size in bytes aligned but may overflow or return 0. Only do
  // this for sizes that have already been checked.
  template <typename T>
  static T UncheckedSizeInBytesAligned(const gfx::Size& size,
                                       ResourceFormat format);

 private:
  template <typename T>
  static inline void VerifyType();

  template <typename T>
  static bool VerifyFitsInBytesInternal(int width,
                                        int height,
                                        ResourceFormat format,
                                        bool verify_size,
                                        bool aligned);

  template <typename T>
  static T BytesInternal(int width,
                         int height,
                         ResourceFormat format,
                         bool verify_size,
                         bool aligned);

  // Not instantiable.
  ResourceSizes() = delete;
};

template <typename T>
bool ResourceSizes::VerifyWidthInBytes(int width, ResourceFormat format) {
  VerifyType<T>();
  if (width <= 0)
    return false;
  return VerifyFitsInBytesInternal<T>(width, 0, format, false, false);
}

template <typename T>
bool ResourceSizes::VerifySizeInBytes(const gfx::Size& size,
                                      ResourceFormat format) {
  VerifyType<T>();
  if (size.IsEmpty())
    return false;
  return VerifyFitsInBytesInternal<T>(size.width(), size.height(), format, true,
                                      false);
}

template <typename T>
bool ResourceSizes::MaybeWidthInBytes(int width,
                                      ResourceFormat format,
                                      T* bytes) {
  VerifyType<T>();
  if (width <= 0)
    return false;
  base::CheckedNumeric<T> checked_value = BitsPerPixel(format);
  checked_value *= width;
  checked_value =
      cc::MathUtil::CheckedRoundUp<T>(checked_value.ValueOrDie(), 8);
  checked_value /= 8;
  if (!checked_value.IsValid())
    return false;
  *bytes = checked_value.ValueOrDie();
  return true;
}

template <typename T>
bool ResourceSizes::MaybeSizeInBytes(const gfx::Size& size,
                                     ResourceFormat format,
                                     T* bytes) {
  VerifyType<T>();
  if (size.IsEmpty())
    return false;
  base::CheckedNumeric<T> checked_value = BitsPerPixel(format);
  checked_value *= size.width();
  checked_value =
      cc::MathUtil::CheckedRoundUp<T>(checked_value.ValueOrDie(), 8);
  checked_value /= 8;
  checked_value *= size.height();
  if (!checked_value.IsValid())
    return false;
  *bytes = checked_value.ValueOrDie();
  return true;
}

template <typename T>
T ResourceSizes::CheckedWidthInBytes(int width, ResourceFormat format) {
  VerifyType<T>();
  CHECK_GT(width, 0);
  DCHECK(VerifyFitsInBytesInternal<T>(width, 0, format, false, false));
  base::CheckedNumeric<T> checked_value = BitsPerPixel(format);
  checked_value *= width;
  checked_value =
      cc::MathUtil::CheckedRoundUp<T>(checked_value.ValueOrDie(), 8);
  checked_value /= 8;
  return checked_value.ValueOrDie();
}

template <typename T>
T ResourceSizes::CheckedSizeInBytes(const gfx::Size& size,
                                    ResourceFormat format) {
  VerifyType<T>();
  CHECK(!size.IsEmpty());
  DCHECK(VerifyFitsInBytesInternal<T>(size.width(), size.height(), format, true,
                                      false));
  base::CheckedNumeric<T> checked_value = BitsPerPixel(format);
  checked_value *= size.width();
  checked_value =
      cc::MathUtil::CheckedRoundUp<T>(checked_value.ValueOrDie(), 8);
  checked_value /= 8;
  checked_value *= size.height();
  return checked_value.ValueOrDie();
}

template <typename T>
T ResourceSizes::UncheckedWidthInBytes(int width, ResourceFormat format) {
  VerifyType<T>();
  DCHECK_GT(width, 0);
  DCHECK(VerifyFitsInBytesInternal<T>(width, 0, format, false, false));
  return BytesInternal<T>(width, 0, format, false, false);
}

template <typename T>
T ResourceSizes::UncheckedSizeInBytes(const gfx::Size& size,
                                      ResourceFormat format) {
  VerifyType<T>();
  DCHECK(!size.IsEmpty());
  DCHECK(VerifyFitsInBytesInternal<T>(size.width(), size.height(), format, true,
                                      false));
  return BytesInternal<T>(size.width(), size.height(), format, true, false);
}

template <typename T>
T ResourceSizes::UncheckedWidthInBytesAligned(int width,
                                              ResourceFormat format) {
  VerifyType<T>();
  DCHECK_GT(width, 0);
  DCHECK(VerifyFitsInBytesInternal<T>(width, 0, format, false, true));
  return BytesInternal<T>(width, 0, format, false, true);
}

template <typename T>
T ResourceSizes::UncheckedSizeInBytesAligned(const gfx::Size& size,
                                             ResourceFormat format) {
  VerifyType<T>();
  CHECK(!size.IsEmpty());
  DCHECK(VerifyFitsInBytesInternal<T>(size.width(), size.height(), format, true,
                                      true));
  return BytesInternal<T>(size.width(), size.height(), format, true, true);
}

template <typename T>
void ResourceSizes::VerifyType() {
  static_assert(
      std::numeric_limits<T>::is_integer && !std::is_same<T, bool>::value,
      "T must be non-bool integer type. Preferred type is size_t.");
}

template <typename T>
bool ResourceSizes::VerifyFitsInBytesInternal(int width,
                                              int height,
                                              ResourceFormat format,
                                              bool verify_size,
                                              bool aligned) {
  base::CheckedNumeric<T> checked_value = BitsPerPixel(format);
  checked_value *= width;
  if (!checked_value.IsValid())
    return false;

  // Roundup bits to byte (8 bits) boundary. If width is 3 and BitsPerPixel is
  // 4, then it should return 16, so that row pixels do not get truncated.
  checked_value =
      cc::MathUtil::UncheckedRoundUp<T>(checked_value.ValueOrDie(), 8);

  // If aligned is true, bytes are aligned on 4-bytes boundaries for upload
  // performance, assuming that GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT have
  // not changed from default.
  if (aligned) {
    checked_value /= 8;
    if (!checked_value.IsValid())
      return false;
    checked_value =
        cc::MathUtil::UncheckedRoundUp<T>(checked_value.ValueOrDie(), 4);
    checked_value *= 8;
  }

  if (verify_size)
    checked_value *= height;
  if (!checked_value.IsValid())
    return false;
  T value = checked_value.ValueOrDie();
  if ((value % 8) != 0)
    return false;
  return true;
}

template <typename T>
T ResourceSizes::BytesInternal(int width,
                               int height,
                               ResourceFormat format,
                               bool verify_size,
                               bool aligned) {
  T bytes = BitsPerPixel(format);
  bytes *= width;
  bytes = cc::MathUtil::UncheckedRoundUp<T>(bytes, 8);
  bytes /= 8;
  if (aligned)
    bytes = cc::MathUtil::UncheckedRoundUp<T>(bytes, 4);
  if (verify_size)
    bytes *= height;

  return bytes;
}

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_SIZES_H_
