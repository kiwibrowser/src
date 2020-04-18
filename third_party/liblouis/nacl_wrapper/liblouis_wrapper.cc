// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "third_party/liblouis/nacl_wrapper/liblouis_wrapper.h"

#include <cstddef>

#include "third_party/liblouis/overrides/liblouis/liblouis.h"

namespace {

// Decodes UTF-8 into 16-bit wide characters.
// This implementation is very permissive and may miss encoding errors.
// It ignores charaters which are not in the Unicode Basic Multilingual Plane.
// TODO(jbroman): Handle more than BMP if liblouis changes to accept UTF-16.
static bool DecodeUtf8(const std::string& in, std::vector<widechar>* out) {
  int len = in.length();
  std::vector<widechar> result;
  result.reserve(len);
  int i = 0;
  while (i < len) {
    int ch = static_cast<unsigned char>(in[i++]);
    widechar cp;
    if ((ch & 0x80) == 0x00) {                      // U+0000 - U+007F
      cp = ch;
    } else if ((ch & 0xe0) == 0xc0 && i < len) {    // U+0080 - U+07FF
      cp = (ch & 0x1f) << 6;
      ch = static_cast<unsigned char>(in[i++]);
      cp |= (ch & 0x3f);
    } else if ((ch & 0xf0) == 0xe0 && i+1 < len) {  // U+0800 - U+FFFF
      cp = (ch & 0x0f) << 12;
      ch = static_cast<unsigned char>(in[i++]);
      cp |= (ch & 0x3f) << 6;
      ch = static_cast<unsigned char>(in[i++]);
      cp |= (ch & 0x3f);
    } else if ((ch & 0xf8) == 0xf0 && i+2 < len) {  // U+10000 - U+1FFFFF
      i += 3;
      continue;
    } else if ((ch & 0xfc) == 0xf8 && i+3 < len) {  // U+200000 - U+3FFFFFF
      i += 4;
      continue;
    } else if ((ch & 0xfe) == 0xfc && i+4 < len) {  // U+4000000 - U+7FFFFFFF
      i += 5;
      continue;
    } else {
      // Invalid first code point.
      return false;
    }
    result.push_back(cp);
  }
  out->swap(result);
  return true;
}

// Encodes 16-bit wide characters into UTF-8.
// This implementation is very permissive and may miss invalid code points in
// its input.
// TODO(jbroman): Handle more than BMP if widechar ever becomes larger.
static bool EncodeUtf8(const std::vector<widechar>& in, std::string* out) {
  std::string result;
  result.reserve(in.size() * 2);
  for (std::vector<widechar>::const_iterator it = in.begin(); it != in.end();
      ++it) {
    unsigned int cp = *it;
    if (cp <= 0x007f) {         // U+0000 - U+007F
      result.push_back(static_cast<char>(cp));
    } else if (cp <= 0x07ff) {  // U+0080 - U+07FF
      result.push_back(static_cast<char>(0xc0 | ((cp >> 6) & 0x1f)));
      result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else if (cp <= 0xffff) {  // U+0800 - U+FFFF
      result.push_back(static_cast<char>(0xe0 | ((cp >> 12) & 0x0f)));
      result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
      result.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else {
      // This can't happen if widechar is 16 bits wide.
      // TODO(jbroman): assert this
    }
  }
  out->swap(result);
  return true;
}

}  // namespace


namespace liblouis_nacl {

LibLouisWrapper::LibLouisWrapper() {
  char data_path[] = "/";  // Needed because lou_setDataPath takes a char*.
  lou_setDataPath(data_path);
}

LibLouisWrapper::~LibLouisWrapper() {
  lou_free();
}

const char* LibLouisWrapper::tables_dir() const {
  return "/liblouis/tables";
}

bool LibLouisWrapper::CheckTable(const std::string& table_names) {
  return lou_getTable(table_names.c_str()) != NULL;
}

bool LibLouisWrapper::Translate(const TranslationParams& params,
    TranslationResult* out) {
  // Convert the character set of the input text.
  std::vector<widechar> inbuf;
  if (!DecodeUtf8(params.text, &inbuf)) {
    // TODO(jbroman): log this
    return false;
  }
  // To avoid unsigned/signed comparison warnings.
  int inbufsize = inbuf.size();

  std::vector<widechar> outbuf;
  std::vector<int> text_to_braille(inbuf.size());
  std::vector<int> braille_to_text;
  int outlen;

  // Compute the cursor position pointer to pass to liblouis.
  int out_cursor_position;
  int* out_cursor_position_ptr;
  if (params.cursor_position < 0) {
    out_cursor_position = -1;
    out_cursor_position_ptr = NULL;
  } else {
    out_cursor_position = params.cursor_position;
    out_cursor_position_ptr = &out_cursor_position;
  }

  std::vector<unsigned char> form_type_map(params.form_type_map);

  // Invoke liblouis.  Do this in a loop since we can't precalculate the
  // translated size.  We add an extra slot in the output buffer so that
  // common cases like single digits or capital letters won't always trigger
  // retranslations (see the comments above the second exit condition inside
  // the loop).  We also set an arbitrary upper bound for the allocation
  // to make sure the loop exits without running out of memory.
  for (int outalloc = (inbufsize + 1) * 2, maxoutalloc = (inbufsize + 1) * 8;
       outalloc <= maxoutalloc; outalloc *= 2) {
    int inlen = inbufsize;
    outlen = outalloc;
    outbuf.resize(outalloc);
    braille_to_text.resize(outalloc);
    form_type_map.resize(outalloc);
    int result = lou_translate(
        params.table_names.c_str(), &inbuf[0], &inlen, &outbuf[0], &outlen,
        &form_type_map[0], NULL /* spacing */, &text_to_braille[0],
        &braille_to_text[0], out_cursor_position_ptr, dotsIO /* mode */);
    if (result == 0) {
      // TODO(jbroman): log this
      return false;
    }
    // If all of inbuf was not consumed, the output buffer must be too small
    // and we have to retry with a larger buffer.
    // In addition, if all of outbuf was exhausted, there's no way to know if
    // more space was needed, so we'll have to retry the translation in that
    // corner case as well.
    if (inlen == inbufsize && outlen < outalloc)
      break;
    outbuf.clear();
    braille_to_text.clear();
  }

  // Massage the result.
  std::vector<unsigned char> cells;
  cells.reserve(outlen);
  for (int i = 0; i < outlen; i++) {
    cells.push_back(outbuf[i]);
  }
  braille_to_text.resize(outlen);

  // Return the translation result.
  out->cells.swap(cells);
  out->text_to_braille.swap(text_to_braille);
  out->braille_to_text.swap(braille_to_text);
  out->cursor_position = out_cursor_position;
  return true;
}

bool LibLouisWrapper::BackTranslate(const std::string& table_names,
    const std::vector<unsigned char>& cells, std::string* out) {
  std::vector<widechar> inbuf;
  inbuf.reserve(cells.size());
  for (std::vector<unsigned char>::const_iterator it = cells.begin();
      it != cells.end(); ++it) {
    // Set the high-order bit to prevent liblouis from dropping empty cells.
    inbuf.push_back(*it | 0x8000);
  }
  // To avoid unsigned/signed comparison warnings.
  int inbufsize = inbuf.size();
  std::vector<widechar> outbuf;
  int outlen;

  // Invoke liblouis.  Do this in a loop since we can't precalculate the
  // translated size.  We add an extra slot in the output buffer so that
  // common cases like single digits or capital letters won't always trigger
  // retranslations (see the comments above the second exit condition inside
  // the loop).  We also set an arbitrary upper bound for the allocation
  // to make sure the loop exits without running out of memory.
  for (int outalloc = (inbufsize + 1) * 2, maxoutalloc = (inbufsize + 1) * 8;
       outalloc <= maxoutalloc; outalloc *= 2) {
    int inlen = inbufsize;
    outlen = outalloc;
    outbuf.resize(outalloc);

    int result = lou_backTranslateString(
        table_names.c_str(), &inbuf[0], &inlen, &outbuf[0], &outlen,
      NULL /* typeform */, NULL /* spacing */, dotsIO /* mode */);
    if (result == 0) {
      // TODO(jbroman): log this
      return false;
    }

    // If all of inbuf was not consumed, the output buffer must be too small
    // and we have to retry with a larger buffer.
    // In addition, if all of outbuf was exhausted, there's no way to know if
    // more space was needed, so we'll have to retry the translation in that
    // corner case as well.
    if (inlen == inbufsize && outlen < outalloc)
      break;
    outbuf.clear();
  }

  // Massage the result.
  outbuf.resize(outlen);
  std::string text;
  if (!EncodeUtf8(outbuf, &text)) {
    // TODO(jbroman): log this
    return false;
  }

  // Return the back translation result.
  out->swap(text);
  return true;
}

}  // namespace liblouis_nacl
