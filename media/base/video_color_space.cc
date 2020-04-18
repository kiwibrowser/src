// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/video_color_space.h"

namespace media {

VideoColorSpace::PrimaryID VideoColorSpace::GetPrimaryID(int primary) {
  if (primary < 1 || primary > 22 || primary == 3)
    return PrimaryID::INVALID;
  if (primary > 12 && primary < 22)
    return PrimaryID::INVALID;
  return static_cast<PrimaryID>(primary);
}

VideoColorSpace::TransferID VideoColorSpace::GetTransferID(int transfer) {
  if (transfer < 1 || transfer > 18 || transfer == 3)
    return TransferID::INVALID;
  return static_cast<TransferID>(transfer);
}

VideoColorSpace::MatrixID VideoColorSpace::GetMatrixID(int matrix) {
  if (matrix < 0 || matrix > 11 || matrix == 3)
    return MatrixID::INVALID;
  return static_cast<MatrixID>(matrix);
}

VideoColorSpace::VideoColorSpace() = default;

VideoColorSpace::VideoColorSpace(PrimaryID primaries,
                                 TransferID transfer,
                                 MatrixID matrix,
                                 gfx::ColorSpace::RangeID range)
    : primaries(primaries), transfer(transfer), matrix(matrix), range(range) {}

VideoColorSpace::VideoColorSpace(int primaries,
                                 int transfer,
                                 int matrix,
                                 gfx::ColorSpace::RangeID range)
    : primaries(GetPrimaryID(primaries)),
      transfer(GetTransferID(transfer)),
      matrix(GetMatrixID(matrix)),
      range(range) {}

bool VideoColorSpace::operator==(const VideoColorSpace& other) const {
  return primaries == other.primaries && transfer == other.transfer &&
         matrix == other.matrix && range == other.range;
}

bool VideoColorSpace::operator!=(const VideoColorSpace& other) const {
  return primaries != other.primaries || transfer != other.transfer ||
         matrix != other.matrix || range != other.range;
}

bool VideoColorSpace::IsSpecified() const {
  if (primaries != PrimaryID::INVALID && primaries != PrimaryID::UNSPECIFIED)
    return true;
  if (transfer != TransferID::INVALID && transfer != TransferID::UNSPECIFIED)
    return true;
  if (matrix != MatrixID::INVALID && matrix != MatrixID::UNSPECIFIED)
    return true;
  // Note that it's not enough to have a range for a video color space to
  // be considered valid, because often the range is just specified with
  // a bool, so there is no way to know if it was set specifically or not.
  return false;
}

gfx::ColorSpace VideoColorSpace::ToGfxColorSpace() const {
  gfx::ColorSpace::PrimaryID primary_id = gfx::ColorSpace::PrimaryID::INVALID;
  gfx::ColorSpace::TransferID transfer_id =
      gfx::ColorSpace::TransferID::INVALID;
  gfx::ColorSpace::MatrixID matrix_id = gfx::ColorSpace::MatrixID::INVALID;

  // Bitfield, note that guesses with higher values take precedence over
  // guesses with lower values.
  enum Guess {
    GUESS_BT709 = 1 << 4,
    GUESS_BT470M = 1 << 3,
    GUESS_BT470BG = 1 << 2,
    GUESS_SMPTE170M = 1 << 1,
    GUESS_SMPTE240M = 1 << 0,
  };

  uint32_t guess = 0;

  switch (primaries) {
    case PrimaryID::BT709:
      primary_id = gfx::ColorSpace::PrimaryID::BT709;
      guess |= GUESS_BT709;
      break;
    case PrimaryID::BT470M:
      primary_id = gfx::ColorSpace::PrimaryID::BT470M;
      guess |= GUESS_BT470M;
      break;
    case PrimaryID::BT470BG:
      primary_id = gfx::ColorSpace::PrimaryID::BT470BG;
      guess |= GUESS_BT470BG;
      break;
    case PrimaryID::SMPTE170M:
      primary_id = gfx::ColorSpace::PrimaryID::SMPTE170M;
      guess |= GUESS_SMPTE170M;
      break;
    case PrimaryID::SMPTE240M:
      primary_id = gfx::ColorSpace::PrimaryID::SMPTE240M;
      guess |= GUESS_SMPTE240M;
      break;
    case PrimaryID::FILM:
      primary_id = gfx::ColorSpace::PrimaryID::FILM;
      break;
    case PrimaryID::BT2020:
      primary_id = gfx::ColorSpace::PrimaryID::BT2020;
      break;
    case PrimaryID::SMPTEST428_1:
      primary_id = gfx::ColorSpace::PrimaryID::SMPTEST428_1;
      break;
    case PrimaryID::SMPTEST431_2:
      primary_id = gfx::ColorSpace::PrimaryID::SMPTEST431_2;
      break;
    case PrimaryID::SMPTEST432_1:
      primary_id = gfx::ColorSpace::PrimaryID::SMPTEST432_1;
      break;
    case PrimaryID::EBU_3213_E:
      // TODO(uzair.jaleel) Need to check this once.
      primary_id = gfx::ColorSpace::PrimaryID::INVALID;
      break;
    case PrimaryID::INVALID:
    case PrimaryID::UNSPECIFIED:
      break;
  }

  switch (transfer) {
    case TransferID::BT709:
      transfer_id = gfx::ColorSpace::TransferID::BT709;
      guess |= GUESS_BT709;
      break;
    case TransferID::GAMMA22:
      transfer_id = gfx::ColorSpace::TransferID::GAMMA22;
      break;
    case TransferID::GAMMA28:
      transfer_id = gfx::ColorSpace::TransferID::GAMMA28;
      break;
    case TransferID::SMPTE170M:
      transfer_id = gfx::ColorSpace::TransferID::SMPTE170M;
      guess |= GUESS_SMPTE170M;
      break;
    case TransferID::SMPTE240M:
      transfer_id = gfx::ColorSpace::TransferID::SMPTE240M;
      guess |= GUESS_SMPTE240M;
      break;
    case TransferID::LINEAR:
      transfer_id = gfx::ColorSpace::TransferID::LINEAR;
      break;
    case TransferID::LOG:
      transfer_id = gfx::ColorSpace::TransferID::LOG;
      break;
    case TransferID::LOG_SQRT:
      transfer_id = gfx::ColorSpace::TransferID::LOG_SQRT;
      break;
    case TransferID::IEC61966_2_4:
      transfer_id = gfx::ColorSpace::TransferID::IEC61966_2_4;
      break;
    case TransferID::BT1361_ECG:
      transfer_id = gfx::ColorSpace::TransferID::BT1361_ECG;
      break;
    case TransferID::IEC61966_2_1:
      transfer_id = gfx::ColorSpace::TransferID::IEC61966_2_1;
      break;
    case TransferID::BT2020_10:
      transfer_id = gfx::ColorSpace::TransferID::BT2020_10;
      break;
    case TransferID::BT2020_12:
      transfer_id = gfx::ColorSpace::TransferID::BT2020_12;
      break;
    case TransferID::SMPTEST2084:
      transfer_id = gfx::ColorSpace::TransferID::SMPTEST2084;
      break;
    case TransferID::SMPTEST428_1:
      transfer_id = gfx::ColorSpace::TransferID::SMPTEST428_1;
      break;
    case TransferID::ARIB_STD_B67:
      transfer_id = gfx::ColorSpace::TransferID::ARIB_STD_B67;
      break;
    case TransferID::INVALID:
    case TransferID::UNSPECIFIED:
      break;
  }

  switch (matrix) {
    case MatrixID::RGB:
      matrix_id = gfx::ColorSpace::MatrixID::RGB;
      break;
    case MatrixID::BT709:
      matrix_id = gfx::ColorSpace::MatrixID::BT709;
      guess |= GUESS_BT709;
      break;
    case MatrixID::FCC:
      matrix_id = gfx::ColorSpace::MatrixID::FCC;
      break;
    case MatrixID::BT470BG:
      matrix_id = gfx::ColorSpace::MatrixID::BT470BG;
      guess |= GUESS_BT470BG;
      break;
    case MatrixID::SMPTE170M:
      matrix_id = gfx::ColorSpace::MatrixID::SMPTE170M;
      guess |= GUESS_SMPTE170M;
      break;
    case MatrixID::SMPTE240M:
      matrix_id = gfx::ColorSpace::MatrixID::SMPTE240M;
      guess |= GUESS_SMPTE240M;
      break;
    case MatrixID::YCOCG:
      matrix_id = gfx::ColorSpace::MatrixID::YCOCG;
      break;
    case MatrixID::BT2020_NCL:
      matrix_id = gfx::ColorSpace::MatrixID::BT2020_NCL;
      break;
    case MatrixID::BT2020_CL:
      matrix_id = gfx::ColorSpace::MatrixID::BT2020_CL;
      break;
    case MatrixID::YDZDX:
      matrix_id = gfx::ColorSpace::MatrixID::YDZDX;
      break;
    case MatrixID::INVALID:
    case MatrixID::UNSPECIFIED:
      break;
  }
  // Removes lowest bit until only a single bit remains.
  while (guess & (guess - 1)) {
    guess &= guess - 1;
  }
  if (!guess)
    guess = GUESS_BT709;

  if (primary_id == gfx::ColorSpace::PrimaryID::INVALID) {
    switch (guess) {
      case GUESS_BT709:
        primary_id = gfx::ColorSpace::PrimaryID::BT709;
        break;
      case GUESS_BT470M:
        primary_id = gfx::ColorSpace::PrimaryID::BT470M;
        break;
      case GUESS_BT470BG:
        primary_id = gfx::ColorSpace::PrimaryID::BT470BG;
        break;
      case GUESS_SMPTE170M:
        primary_id = gfx::ColorSpace::PrimaryID::SMPTE170M;
        break;
      case GUESS_SMPTE240M:
        primary_id = gfx::ColorSpace::PrimaryID::SMPTE240M;
        break;
    }
  }

  if (transfer_id == gfx::ColorSpace::TransferID::INVALID) {
    switch (guess) {
      case GUESS_BT709:
        transfer_id = gfx::ColorSpace::TransferID::BT709;
        break;
      case GUESS_BT470M:
      case GUESS_BT470BG:
      case GUESS_SMPTE170M:
        transfer_id = gfx::ColorSpace::TransferID::SMPTE170M;
        break;
      case GUESS_SMPTE240M:
        transfer_id = gfx::ColorSpace::TransferID::SMPTE240M;
        break;
    }
  }

  if (matrix_id == gfx::ColorSpace::MatrixID::INVALID) {
    switch (guess) {
      case GUESS_BT709:
        matrix_id = gfx::ColorSpace::MatrixID::BT709;
        break;
      case GUESS_BT470M:
      case GUESS_BT470BG:
      case GUESS_SMPTE170M:
        matrix_id = gfx::ColorSpace::MatrixID::SMPTE170M;
        break;
      case GUESS_SMPTE240M:
        matrix_id = gfx::ColorSpace::MatrixID::SMPTE240M;
        break;
    }
  }

  return gfx::ColorSpace(primary_id, transfer_id, matrix_id, range);
}

VideoColorSpace VideoColorSpace::REC709() {
  return VideoColorSpace(PrimaryID::BT709, TransferID::BT709, MatrixID::BT709,
                         gfx::ColorSpace::RangeID::LIMITED);
}
VideoColorSpace VideoColorSpace::REC601() {
  return VideoColorSpace(PrimaryID::SMPTE170M, TransferID::SMPTE170M,
                         MatrixID::SMPTE170M,
                         gfx::ColorSpace::RangeID::LIMITED);
}
VideoColorSpace VideoColorSpace::JPEG() {
  // TODO(ccameron): Determine which primaries and transfer function were
  // intended here.
  return VideoColorSpace(PrimaryID::BT709, TransferID::IEC61966_2_1,
                         MatrixID::SMPTE170M, gfx::ColorSpace::RangeID::FULL);
}

}  // namespace
