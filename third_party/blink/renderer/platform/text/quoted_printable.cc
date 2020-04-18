/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/text/quoted_printable.h"

#include "third_party/blink/renderer/platform/wtf/ascii_ctype.h"

namespace blink {

static size_t LengthOfLineEndingAtIndex(const char* input,
                                        size_t input_length,
                                        size_t index) {
  SECURITY_DCHECK(index < input_length);
  if (input[index] == '\n')
    return 1;  // Single LF.

  if (input[index] == '\r') {
    if ((index + 1) == input_length || input[index + 1] != '\n')
      return 1;  // Single CR (Classic Mac OS).
    return 2;    // CR-LF.
  }

  return 0;
}

void QuotedPrintableEncode(const char* input,
                           size_t input_length,
                           QuotedPrintableEncodeDelegate* delegate,
                           Vector<char>& out) {
  out.clear();
  out.ReserveCapacity(input_length);
  delegate->DidStartLine(out);
  size_t current_line_length = 0;
  for (size_t i = 0; i < input_length; ++i) {
    bool is_last_character = (i == input_length - 1);
    char current_character = input[i];
    bool requires_encoding = false;
    // All non-printable ASCII characters and = require encoding.
    if ((current_character < ' ' || current_character > '~' ||
         current_character == '=') &&
        current_character != '\t')
      requires_encoding = true;

    // Decide if space and tab characters need to be encoded.
    if (!requires_encoding &&
        (current_character == '\t' || current_character == ' ')) {
      bool end_of_line = is_last_character ||
                         LengthOfLineEndingAtIndex(input, input_length, i + 1);
      requires_encoding =
          delegate->ShouldEncodeWhiteSpaceCharacters(end_of_line);
    }

    // End of line should be converted to CR-LF sequences.
    if (!is_last_character) {
      size_t length_of_line_ending =
          LengthOfLineEndingAtIndex(input, input_length, i);
      if (length_of_line_ending) {
        out.Append("\r\n", 2);
        current_line_length = 0;
        i += (length_of_line_ending -
              1);  // -1 because we'll ++ in the for() above.
        continue;
      }
    }

    size_t length_of_encoded_character = 1;
    if (requires_encoding)
      length_of_encoded_character += 2;
    if (!is_last_character)
      length_of_encoded_character += 1;  // + 1 for the = (soft line break).

    // Insert a soft line break if necessary.
    if (current_line_length + length_of_encoded_character >
        delegate->GetMaxLineLengthForEncodedContent()) {
      delegate->DidFinishLine(false /*last_line*/, out);
      current_line_length = 0;
      delegate->DidStartLine(out);
    }

    // Finally, insert the actual character(s).
    if (requires_encoding) {
      out.push_back('=');
      out.push_back(UpperNibbleToASCIIHexDigit(current_character));
      out.push_back(LowerNibbleToASCIIHexDigit(current_character));
      current_line_length += 3;
    } else {
      out.push_back(current_character);
      current_line_length++;
    }
  }
  delegate->DidFinishLine(true /*last_line*/, out);
}

void QuotedPrintableDecode(const Vector<char>& in, Vector<char>& out) {
  QuotedPrintableDecode(in.data(), in.size(), out);
}

void QuotedPrintableDecode(const char* data,
                           size_t data_length,
                           Vector<char>& out) {
  out.clear();
  if (!data_length)
    return;

  for (size_t i = 0; i < data_length; ++i) {
    char current_character = data[i];
    if (current_character != '=') {
      out.push_back(current_character);
      continue;
    }
    // We are dealing with a '=xx' sequence.
    if (data_length - i < 3) {
      // Unfinished = sequence, append as is.
      out.push_back(current_character);
      continue;
    }
    char upper_character = data[++i];
    char lower_character = data[++i];
    if (upper_character == '\r' && lower_character == '\n')
      continue;

    if (!IsASCIIHexDigit(upper_character) ||
        !IsASCIIHexDigit(lower_character)) {
      // Invalid sequence, = followed by non hex digits, just insert the
      // characters as is.
      out.push_back('=');
      out.push_back(upper_character);
      out.push_back(lower_character);
      continue;
    }
    out.push_back(
        static_cast<char>(ToASCIIHexValue(upper_character, lower_character)));
  }
}

}  // namespace blink
