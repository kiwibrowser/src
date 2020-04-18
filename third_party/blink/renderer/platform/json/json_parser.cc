// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/json/json_parser.h"

#include "third_party/blink/renderer/platform/decimal.h"
#include "third_party/blink/renderer/platform/json/json_values.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/string_to_number.h"

namespace blink {

namespace {

const int kMaxStackLimit = 1000;

enum Token {
  kObjectBegin,
  kObjectEnd,
  kArrayBegin,
  kArrayEnd,
  kStringLiteral,
  kNumber,
  kBoolTrue,
  kBoolFalse,
  kNullToken,
  kListSeparator,
  kObjectPairSeparator,
  kInvalidToken,
};

template <typename CharType>
bool ParseConstToken(const CharType* start,
                     const CharType* end,
                     const CharType** token_end,
                     const char* token) {
  while (start < end && *token != '\0' && *start++ == *token++) {
  }
  if (*token != '\0')
    return false;
  *token_end = start;
  return true;
}

template <typename CharType>
bool ReadInt(const CharType* start,
             const CharType* end,
             const CharType** token_end,
             bool can_have_leading_zeros) {
  if (start == end)
    return false;
  bool have_leading_zero = '0' == *start;
  int length = 0;
  while (start < end && '0' <= *start && *start <= '9') {
    ++start;
    ++length;
  }
  if (!length)
    return false;
  if (!can_have_leading_zeros && length > 1 && have_leading_zero)
    return false;
  *token_end = start;
  return true;
}

template <typename CharType>
bool ParseNumberToken(const CharType* start,
                      const CharType* end,
                      const CharType** token_end) {
  // We just grab the number here. We validate the size in DecodeNumber.
  // According to RFC4627, a valid number is: [minus] int [frac] [exp]
  if (start == end)
    return false;
  CharType c = *start;
  if ('-' == c)
    ++start;

  if (!ReadInt(start, end, &start, false))
    return false;
  if (start == end) {
    *token_end = start;
    return true;
  }

  // Optional fraction part
  c = *start;
  if ('.' == c) {
    ++start;
    if (!ReadInt(start, end, &start, true))
      return false;
    if (start == end) {
      *token_end = start;
      return true;
    }
    c = *start;
  }

  // Optional exponent part
  if ('e' == c || 'E' == c) {
    ++start;
    if (start == end)
      return false;
    c = *start;
    if ('-' == c || '+' == c) {
      ++start;
      if (start == end)
        return false;
    }
    if (!ReadInt(start, end, &start, true))
      return false;
  }

  *token_end = start;
  return true;
}

template <typename CharType>
bool ReadHexDigits(const CharType* start,
                   const CharType* end,
                   const CharType** token_end,
                   int digits) {
  if (end - start < digits)
    return false;
  for (int i = 0; i < digits; ++i) {
    CharType c = *start++;
    if (!(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
          ('A' <= c && c <= 'F')))
      return false;
  }
  *token_end = start;
  return true;
}

template <typename CharType>
bool ParseStringToken(const CharType* start,
                      const CharType* end,
                      const CharType** token_end) {
  while (start < end) {
    CharType c = *start++;
    if ('\\' == c) {
      if (start == end)
        return false;
      c = *start++;
      // Make sure the escaped char is valid.
      switch (c) {
        case 'x':
          if (!ReadHexDigits(start, end, &start, 2))
            return false;
          break;
        case 'u':
          if (!ReadHexDigits(start, end, &start, 4))
            return false;
          break;
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '"':
          break;
        default:
          return false;
      }
    } else if ('"' == c) {
      *token_end = start;
      return true;
    }
  }
  return false;
}

template <typename CharType>
bool SkipComment(const CharType* start,
                 const CharType* end,
                 const CharType** comment_end) {
  if (start == end)
    return false;

  if (*start != '/' || start + 1 >= end)
    return false;
  ++start;

  if (*start == '/') {
    // Single line comment, read to newline.
    for (++start; start < end; ++start) {
      if (*start == '\n' || *start == '\r') {
        *comment_end = start + 1;
        return true;
      }
    }
    *comment_end = end;
    // Comment reaches end-of-input, which is fine.
    return true;
  }

  if (*start == '*') {
    CharType previous = '\0';
    // Block comment, read until end marker.
    for (++start; start < end; previous = *start++) {
      if (previous == '*' && *start == '/') {
        *comment_end = start + 1;
        return true;
      }
    }
    // Block comment must close before end-of-input.
    return false;
  }

  return false;
}

template <typename CharType>
void SkipWhitespaceAndComments(const CharType* start,
                               const CharType* end,
                               const CharType** whitespace_end) {
  while (start < end) {
    if (IsSpaceOrNewline(*start)) {
      ++start;
    } else if (*start == '/') {
      const CharType* comment_end;
      if (!SkipComment(start, end, &comment_end))
        break;
      start = comment_end;
    } else {
      break;
    }
  }
  *whitespace_end = start;
}

template <typename CharType>
Token ParseToken(const CharType* start,
                 const CharType* end,
                 const CharType** token_start,
                 const CharType** token_end) {
  SkipWhitespaceAndComments(start, end, token_start);
  start = *token_start;

  if (start == end)
    return kInvalidToken;

  switch (*start) {
    case 'n':
      if (ParseConstToken(start, end, token_end, kJSONNullString))
        return kNullToken;
      break;
    case 't':
      if (ParseConstToken(start, end, token_end, kJSONTrueString))
        return kBoolTrue;
      break;
    case 'f':
      if (ParseConstToken(start, end, token_end, kJSONFalseString))
        return kBoolFalse;
      break;
    case '[':
      *token_end = start + 1;
      return kArrayBegin;
    case ']':
      *token_end = start + 1;
      return kArrayEnd;
    case ',':
      *token_end = start + 1;
      return kListSeparator;
    case '{':
      *token_end = start + 1;
      return kObjectBegin;
    case '}':
      *token_end = start + 1;
      return kObjectEnd;
    case ':':
      *token_end = start + 1;
      return kObjectPairSeparator;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      if (ParseNumberToken(start, end, token_end))
        return kNumber;
      break;
    case '"':
      if (ParseStringToken(start + 1, end, token_end))
        return kStringLiteral;
      break;
  }
  return kInvalidToken;
}

template <typename CharType>
inline int HexToInt(CharType c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('A' <= c && c <= 'F')
    return c - 'A' + 10;
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  NOTREACHED();
  return 0;
}

template <typename CharType>
bool DecodeString(const CharType* start,
                  const CharType* end,
                  StringBuilder* output) {
  while (start < end) {
    UChar c = *start++;
    if ('\\' != c) {
      output->Append(c);
      continue;
    }
    if (start == end)
      return false;
    c = *start++;

    if (c == 'x') {
      // \x is not supported.
      return false;
    }

    switch (c) {
      case '"':
      case '/':
      case '\\':
        break;
      case 'b':
        c = '\b';
        break;
      case 'f':
        c = '\f';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      case 'v':
        c = '\v';
        break;
      case 'u':
        c = (HexToInt(*start) << 12) + (HexToInt(*(start + 1)) << 8) +
            (HexToInt(*(start + 2)) << 4) + HexToInt(*(start + 3));
        start += 4;
        break;
      default:
        return false;
    }
    output->Append(c);
  }
  return true;
}

template <typename CharType>
bool DecodeString(const CharType* start, const CharType* end, String* output) {
  if (start == end) {
    *output = "";
    return true;
  }
  if (start > end)
    return false;
  StringBuilder buffer;
  buffer.ReserveCapacity(end - start);
  if (!DecodeString(start, end, &buffer))
    return false;
  *output = buffer.ToString();
  // Validate constructed utf16 string.
  if (output->Utf8(kStrictUTF8Conversion).IsNull())
    return false;
  return true;
}

template <typename CharType>
std::unique_ptr<JSONValue> BuildValue(const CharType* start,
                                      const CharType* end,
                                      const CharType** value_token_end,
                                      int max_depth) {
  if (max_depth == 0)
    return nullptr;

  std::unique_ptr<JSONValue> result;
  const CharType* token_start;
  const CharType* token_end;
  Token token = ParseToken(start, end, &token_start, &token_end);
  switch (token) {
    case kInvalidToken:
      return nullptr;
    case kNullToken:
      result = JSONValue::Null();
      break;
    case kBoolTrue:
      result = JSONBasicValue::Create(true);
      break;
    case kBoolFalse:
      result = JSONBasicValue::Create(false);
      break;
    case kNumber: {
      bool ok;
      double value =
          CharactersToDouble(token_start, token_end - token_start, &ok);
      if (Decimal::FromDouble(value).IsInfinity())
        ok = false;
      if (!ok)
        return nullptr;
      int number = static_cast<int>(value);
      if (number == value)
        result = JSONBasicValue::Create(number);
      else
        result = JSONBasicValue::Create(value);
      break;
    }
    case kStringLiteral: {
      String value;
      bool ok = DecodeString(token_start + 1, token_end - 1, &value);
      if (!ok)
        return nullptr;
      result = JSONString::Create(value);
      break;
    }
    case kArrayBegin: {
      std::unique_ptr<JSONArray> array = JSONArray::Create();
      start = token_end;
      token = ParseToken(start, end, &token_start, &token_end);
      while (token != kArrayEnd) {
        std::unique_ptr<JSONValue> array_node =
            BuildValue(start, end, &token_end, max_depth - 1);
        if (!array_node)
          return nullptr;
        array->PushValue(std::move(array_node));

        // After a list value, we expect a comma or the end of the list.
        start = token_end;
        token = ParseToken(start, end, &token_start, &token_end);
        if (token == kListSeparator) {
          start = token_end;
          token = ParseToken(start, end, &token_start, &token_end);
          if (token == kArrayEnd)
            return nullptr;
        } else if (token != kArrayEnd) {
          // Unexpected value after list value. Bail out.
          return nullptr;
        }
      }
      if (token != kArrayEnd)
        return nullptr;
      result = std::move(array);
      break;
    }
    case kObjectBegin: {
      std::unique_ptr<JSONObject> object = JSONObject::Create();
      start = token_end;
      token = ParseToken(start, end, &token_start, &token_end);
      while (token != kObjectEnd) {
        if (token != kStringLiteral)
          return nullptr;
        String key;
        if (!DecodeString(token_start + 1, token_end - 1, &key))
          return nullptr;
        start = token_end;

        token = ParseToken(start, end, &token_start, &token_end);
        if (token != kObjectPairSeparator)
          return nullptr;
        start = token_end;

        std::unique_ptr<JSONValue> value =
            BuildValue(start, end, &token_end, max_depth - 1);
        if (!value)
          return nullptr;
        object->SetValue(key, std::move(value));
        start = token_end;

        // After a key/value pair, we expect a comma or the end of the
        // object.
        token = ParseToken(start, end, &token_start, &token_end);
        if (token == kListSeparator) {
          start = token_end;
          token = ParseToken(start, end, &token_start, &token_end);
          if (token == kObjectEnd)
            return nullptr;
        } else if (token != kObjectEnd) {
          // Unexpected value after last object value. Bail out.
          return nullptr;
        }
      }
      if (token != kObjectEnd)
        return nullptr;
      result = std::move(object);
      break;
    }

    default:
      // We got a token that's not a value.
      return nullptr;
  }

  SkipWhitespaceAndComments(token_end, end, value_token_end);
  return result;
}

template <typename CharType>
std::unique_ptr<JSONValue> ParseJSONInternal(const CharType* start,
                                             unsigned length,
                                             int max_depth) {
  const CharType* end = start + length;
  const CharType* token_end;
  std::unique_ptr<JSONValue> value =
      BuildValue(start, end, &token_end, max_depth);
  if (!value || token_end != end)
    return nullptr;
  return value;
}

}  // anonymous namespace

std::unique_ptr<JSONValue> ParseJSON(const String& json) {
  return ParseJSON(json, kMaxStackLimit);
}

std::unique_ptr<JSONValue> ParseJSON(const String& json, int max_depth) {
  if (json.IsEmpty())
    return nullptr;
  if (max_depth < 0)
    max_depth = 0;
  if (max_depth > kMaxStackLimit)
    max_depth = kMaxStackLimit;
  if (json.Is8Bit())
    return ParseJSONInternal(json.Characters8(), json.length(), max_depth);
  return ParseJSONInternal(json.Characters16(), json.length(), max_depth);
}

}  // namespace blink
