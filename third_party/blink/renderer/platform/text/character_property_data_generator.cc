// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/text/character_property_data_generator.h"

#include <stdio.h>
#include <cassert>
#include <cstring>
#include <memory>
#include "third_party/blink/renderer/platform/text/character_property.h"
#if !defined(USING_SYSTEM_ICU)
#define MUTEX_H  // Prevent compile failure of utrie2.h on Windows
#include <utrie2.h>
#endif

namespace blink {

#if defined(USING_SYSTEM_ICU)
static void Generate(FILE*) {}
#else

const UChar32 kMaxCodepoint = 0x10FFFF;
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))

static void SetRanges(CharacterProperty* values,
                      const UChar32* ranges,
                      size_t length,
                      CharacterProperty value) {
  assert(length % 2 == 0);
  const UChar32* end = ranges + length;
  for (; ranges != end; ranges += 2) {
    assert(ranges[0] <= ranges[1] && ranges[1] <= kMaxCodepoint);
    for (UChar32 c = ranges[0]; c <= ranges[1]; c++)
      values[c] |= value;
  }
}

static void SetValues(CharacterProperty* values,
                      const UChar32* begin,
                      size_t length,
                      CharacterProperty value) {
  const UChar32* end = begin + length;
  for (; begin != end; begin++) {
    assert(*begin <= kMaxCodepoint);
    values[*begin] |= value;
  }
}

static void GenerateUTrieSerialized(FILE* fp, int32_t size, uint8_t* array) {
  fprintf(fp,
          "#include <cstdint>\n\n"
          "namespace blink {\n\n"
          "extern const int32_t kSerializedCharacterDataSize = %d;\n"
          // The utrie2_openFromSerialized function requires character data to
          // be aligned to 4 bytes.
          "alignas(4) extern const uint8_t kSerializedCharacterData[] = {",
          size);
  for (int32_t i = 0; i < size;) {
    fprintf(fp, "\n   ");
    for (int col = 0; col < 16 && i < size; col++, i++)
      fprintf(fp, " 0x%02X,", array[i]);
  }
  fprintf(fp,
          "\n};\n\n"
          "} // namespace blink\n");
}

static void Generate(FILE* fp) {
  // Create a value array of all possible code points.
  const UChar32 kSize = kMaxCodepoint + 1;
  std::unique_ptr<CharacterProperty[]> values(new CharacterProperty[kSize]);
  memset(values.get(), 0, sizeof(CharacterProperty) * kSize);

#define SET(name)                                                   \
  SetRanges(values.get(), name##Ranges, ARRAY_LENGTH(name##Ranges), \
            CharacterProperty::name);                               \
  SetValues(values.get(), name##Array, ARRAY_LENGTH(name##Array),   \
            CharacterProperty::name);

  SET(kIsCJKIdeographOrSymbol);
  SET(kIsUprightInMixedVertical);
  SET(kIsPotentialCustomElementNameChar);
  SET(kIsBidiControl);

  // Create a trie from the value array.
  UErrorCode error = U_ZERO_ERROR;
  UTrie2* trie = utrie2_open(0, 0, &error);
  assert(error == U_ZERO_ERROR);
  UChar32 start = 0;
  CharacterProperty value = values[0];
  for (UChar32 c = 1;; c++) {
    if (c < kSize && values[c] == value)
      continue;
    if (static_cast<uint32_t>(value)) {
      utrie2_setRange32(trie, start, c - 1, static_cast<uint32_t>(value), TRUE,
                        &error);
      assert(error == U_ZERO_ERROR);
    }
    if (c >= kSize)
      break;
    start = c;
    value = values[start];
  }

  // Freeze and serialize the trie to a byte array.
  utrie2_freeze(trie, UTrie2ValueBits::UTRIE2_16_VALUE_BITS, &error);
  assert(error == U_ZERO_ERROR);
  int32_t serialized_size = utrie2_serialize(trie, nullptr, 0, &error);
  error = U_ZERO_ERROR;
  std::unique_ptr<uint8_t[]> serialized(new uint8_t[serialized_size]);
  serialized_size =
      utrie2_serialize(trie, serialized.get(), serialized_size, &error);
  assert(error == U_ZERO_ERROR);

  GenerateUTrieSerialized(fp, serialized_size, serialized.get());

  utrie2_close(trie);
}
#endif

}  // namespace blink

int main(int argc, char** argv) {
  // Write the serialized array to the source file.
  if (argc <= 1) {
    blink::Generate(stdout);
  } else {
    FILE* fp = fopen(argv[1], "wb");
    blink::Generate(fp);
    fclose(fp);
  }

  return 0;
}
