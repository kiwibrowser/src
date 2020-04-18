/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_HEX_NUMBER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_HEX_NUMBER_H_

#include "third_party/blink/renderer/platform/wtf/text/string_concatenate.h"

namespace WTF {

namespace Internal {

const LChar kLowerHexDigits[17] = "0123456789abcdef";
const LChar kUpperHexDigits[17] = "0123456789ABCDEF";

}  // namespace Internal

class HexNumber final {
  STATIC_ONLY(HexNumber);

 public:
  enum HexConversionMode { kLowercase, kUppercase };

  template <typename T>
  static inline void AppendByteAsHex(unsigned char byte,
                                     T& destination,
                                     HexConversionMode mode = kUppercase) {
    const LChar* hex_digits = HexDigitsForMode(mode);
    destination.Append(hex_digits[byte >> 4]);
    destination.Append(hex_digits[byte & 0xF]);
  }

  static inline void AppendByteAsHex(unsigned char byte,
                                     Vector<LChar>& destination,
                                     HexConversionMode mode = kUppercase) {
    const LChar* hex_digits = HexDigitsForMode(mode);
    destination.push_back(hex_digits[byte >> 4]);
    destination.push_back(hex_digits[byte & 0xF]);
  }

  static inline void AppendByteAsHex(unsigned char byte,
                                     Vector<char>& destination,
                                     HexConversionMode mode = kUppercase) {
    const LChar* hex_digits = HexDigitsForMode(mode);
    destination.push_back(hex_digits[byte >> 4]);
    destination.push_back(hex_digits[byte & 0xF]);
  }

  template <typename T>
  static inline void AppendUnsignedAsHex(unsigned number,
                                         T& destination,
                                         HexConversionMode mode = kUppercase) {
    const LChar* hex_digits = HexDigitsForMode(mode);
    Vector<LChar, 8> result;
    do {
      result.push_front(hex_digits[number % 16]);
      number >>= 4;
    } while (number > 0);

    destination.Append(result.data(), result.size());
  }

  // Same as appendUnsignedAsHex, but using exactly 'desiredDigits' for the
  // conversion.
  template <typename T>
  static inline void AppendUnsignedAsHexFixedSize(
      unsigned number,
      T& destination,
      unsigned desired_digits,
      HexConversionMode mode = kUppercase) {
    DCHECK(desired_digits);

    const LChar* hex_digits = HexDigitsForMode(mode);
    Vector<LChar, 8> result;
    do {
      result.push_front(hex_digits[number % 16]);
      number >>= 4;
    } while (result.size() < desired_digits);

    DCHECK_EQ(result.size(), desired_digits);
    destination.Append(result.data(), result.size());
  }

 private:
  static inline const LChar* HexDigitsForMode(HexConversionMode mode) {
    return mode == kLowercase ? Internal::kLowerHexDigits
                              : Internal::kUpperHexDigits;
  }
};

}  // namespace WTF

using WTF::HexNumber;

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WTF_HEX_NUMBER_H_
