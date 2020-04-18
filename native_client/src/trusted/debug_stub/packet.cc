/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <string>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/debug_stub/packet.h"
#include "native_client/src/trusted/debug_stub/util.h"
#include "native_client/src/trusted/debug_stub/platform.h"

using std::string;
using port::IPlatform;


namespace gdb_rsp {

#define MIN_PAD 1
#define GROW_SIZE  64

Packet::Packet() {
  seq_ = -1;
  Clear();
}

void Packet::Clear() {
  data_.clear();
  data_.resize(GROW_SIZE);
  data_[0] = 0;

  read_index_  = 0;
  write_index_ = 0;
}

void Packet::Rewind() {
  read_index_ = 0;
}

bool Packet::EndOfPacket() const {
  return (read_index_ >= write_index_);
}

void Packet::AddRawChar(char ch) {
  // Grow by a fixed amount whenever we are within the pad boundry.
  // The pad boundry allows for the addition of NUL termination.
  if (data_.size() <= (write_index_ + MIN_PAD)) {
    data_.resize(data_.size() + GROW_SIZE);
  }

  // Add character and always null terminate.
  data_[write_index_++] = ch;
  data_[write_index_] = 0;
}

void Packet::AddWord8(uint8_t ch) {
  char seq1, seq2;

  IntToNibble(ch >> 4, &seq1);
  IntToNibble(ch & 0xF, &seq2);

  AddRawChar(seq1);
  AddRawChar(seq2);
}

void Packet::AddBlock(const void *ptr, uint32_t len) {
  assert(ptr);

  const char *p = (const char *) ptr;

  for (uint32_t offs = 0; offs < len; offs++) {
    AddWord8(p[offs]);
  }
}

void Packet::AddWord16(uint16_t val) {
  AddBlock(&val, sizeof(val));
}

void Packet::AddWord32(uint32_t val) {
  AddBlock(&val, sizeof(val));
}

void Packet::AddWord64(uint64_t val) {
  AddBlock(&val, sizeof(val));
}

void Packet::AddString(const char *str) {
  assert(str);

  while (*str) {
    AddRawChar(*str);
    str++;
  }
}

void Packet::AddEscapedData(const char *data, size_t length) {
  while (length > 0) {
    char ch = *data;
    // Escape certain characters by sending 0x7d ('}') followed by the original
    // character xor-ed with 0x20.
    if (ch == '}' || ch == '#' || ch == '$' || ch == '*') {
      AddRawChar('}');
      AddRawChar(ch ^ 0x20);
    } else {
      AddRawChar(ch);
    }
    ++data;
    --length;
    // See if run length encoding can be used.
    // Limit runs to 97 copies, as character 126 is the highest that can be
    // used in the encoding.
    size_t count = 0;
    while (count < 97 && length > count && data[count] == ch) {
      count++;
    }
    // We can only use run length encoding if there are 3 or more of the same
    // character (not including the initial character). This is the minimum run
    // length allowed by the protocol.
    if (count >= 3) {
      // An odd quirk of the protocol is that because the characters
      // '#' and '$' cannot appear in a packet, they also are not valid as a
      // run length. Since these correspond to lengths 6 and 7, runs of this
      // size must be clipped down to length 5.
      if (count == 6 || count == 7) {
        count = 5;
      }
      AddRawChar('*');
      AddRawChar(static_cast<char>(count + 29));
      data += count;
      length -= count;
    }
  }
}

void Packet::AddHexString(const char *str) {
  assert(str);

  while (*str) {
    AddWord8(*str);
    str++;
  }
}

void Packet::AddNumberSep(uint64_t val, char sep) {
  char out[sizeof(val) * 2];
  int nibbles = 0;
  size_t a;

  // Check for -1 optimization
  if (val == static_cast<uint64_t>(-1)) {
    AddRawChar('-');
    AddRawChar('1');
  } else {
    // Assume we have the valuse 0x00001234
    for (a = 0; a < sizeof(val); a++) {
      uint8_t byte = static_cast<uint8_t>(val & 0xFF);

      // Stream in with bytes reverse, starting at least significant
      // So we store 4, then 3, 2, 1
      IntToNibble(byte & 0xF, &out[nibbles++]);
      IntToNibble(byte >> 4, &out[nibbles++]);

      // Get the next 8 bits;
      val >>= 8;

      // Supress leading zeros, so we are done when val hits zero
      if (val == 0) {
        break;
      }
    }

    // Strip the high zero for this byte if needed
    if ((nibbles > 1) && (out[nibbles-1] == '0')) nibbles--;

    // Now write it out reverse to correct the order
    while (nibbles) {
      nibbles--;
      AddRawChar(out[nibbles]);
    }
  }

  // If we asked for a sperator, insert it
  if (sep) AddRawChar(sep);
}

bool Packet::GetNumberSep(uint64_t *val, char *sep) {
  uint64_t out = 0;
  char ch;

  if (!GetRawChar(&ch)) {
    return false;
  }

  // Check for -1
  if (ch == '-') {
    if (!GetRawChar(&ch)) {
      return false;
    }

    if (ch == '1') {
      *val = (uint64_t) -1;

      ch = 0;
      GetRawChar(&ch);
      if (sep) {
        *sep = ch;
      }
      return true;
    }
    return false;
  }

  do {
    int nib;

    // Check for separator
    if (!NibbleToInt(ch, &nib)) {
      break;
    }

    // Add this nibble.
    out = (out << 4) + nib;

    // Get the next character (if availible)
    ch = 0;
    if (!GetRawChar(&ch)) {
      break;
    }
  } while (1);

  // Set the value;
  *val = out;

  // Add the separator if the user wants it...
  if (sep != NULL) *sep = ch;

  return true;
}

bool Packet::GetRawChar(char *ch) {
  assert(ch != NULL);

  if (read_index_ >= write_index_)
    return false;

  *ch = data_[read_index_++];

  // Check for RLE X*N, where X is the value, N is the reps.
  if (*ch == '*') {
    if (read_index_ < 2) {
      NaClLog(LOG_ERROR, "Unexpected RLE at start of packet.\n");
      return false;
    }

    if (read_index_ >= write_index_) {
      NaClLog(LOG_ERROR, "Unexpected EoP during RLE.\n");
      return false;
    }

    // GDB does not use "CTRL" characters in the stream, so the
    // number of reps is encoded as the ASCII value beyond 28
    // (which when you add a min rep size of 4, forces the rep
    // character to be ' ' (32) or greater).
    int32_t cnt = (data_[read_index_] - 28);
    if (cnt < 3) {
      NaClLog(LOG_ERROR, "Unexpected RLE length.\n");
      return false;
    }

    // We have just read '*' and incremented the read pointer,
    // so here is the old state, and expected new state.
    //
    //   Assume N = 5, we grow by N - size of encoding (3).
    //
    // OldP:       R  W
    // OldD:  012X*N89 = 8 chars
    // Size:  012X*N89__ = 10 chars
    // Move:  012X*__N89 = 10 chars
    // Fill:  012XXXXX89 = 10 chars
    // NewP:       R    W  (shifted 5 - 3)
    //
    // To accomplish this we must first, resize the vector then move
    // all remaining characters to the right, by the delta between
    // the run length, and encoding size. This moves one more char
    // than needed (the 'N'), but is easier to understand.
    // NOTE: We add one to the resize to allow for zero termination.
    data_.resize(write_index_ + cnt - 3 + 1);
    memmove(&data_[read_index_ + cnt - 3], &data_[read_index_],
            write_index_ - read_index_);

    // Now me must go back and fill over the previous '*' with the
    // repeated character for the length of the run minus the original
    // character which is already correct
    *ch = data_[read_index_ - 2];
    memset(&data_[read_index_ - 1], *ch, cnt - 1);

    // Now we update the write_index_, and reterminate the string.
    write_index_ = data_.size() - 1;
    data_[write_index_] = 0;
  }
  return true;
}

bool Packet::GetWord8(uint8_t *ch) {
  assert(ch);

  char seq1, seq2;
  int  val1, val2;

  // Get two ASCII hex values
  if (!GetRawChar(&seq1)) {
    return false;
  }
  if (!GetRawChar(&seq2)) {
    return false;
  }

  // Convert them to ints
  if (!NibbleToInt(seq1, &val1)) {
    return false;
  }
  if (!NibbleToInt(seq2, &val2)) {
    return false;
  }

  *ch = (val1 << 4) + val2;
  return true;
}

bool Packet::GetBlock(void *ptr, uint32_t len) {
  assert(ptr);

  uint8_t *p = reinterpret_cast<uint8_t *>(ptr);
  bool res = true;

  for (uint32_t offs = 0; offs < len; offs++) {
    res = GetWord8(&p[offs]);
    if (false == res) {
      break;
    }
  }

  return res;
}

bool Packet::GetWord16(uint16_t *ptr) {
  assert(ptr);
  return GetBlock(ptr, sizeof(*ptr));
}

bool Packet::GetWord32(uint32_t *ptr) {
  assert(ptr);
  return GetBlock(ptr, sizeof(*ptr));
}

bool Packet::GetWord64(uint64_t *ptr) {
  assert(ptr);
  return GetBlock(ptr, sizeof(*ptr));
}


bool Packet::GetString(string *str) {
  if (EndOfPacket()) {
    return false;
  }

  *str = &data_[read_index_];
  read_index_ = write_index_;
  return true;
}

bool Packet::GetHexString(string *str) {
  // Decode a string encoded as a series of 2-hex digit pairs.

  char ch1;
  char ch2;
  int nib1;
  int nib2;

  if (EndOfPacket()) {
    return false;
  }

  // Pull values until we hit a separator
  str->clear();
  while (GetRawChar(&ch1)) {
    if (!NibbleToInt(ch1, &nib1)) {
      read_index_--;
      break;
    }
    if (!GetRawChar(&ch2) ||
        !NibbleToInt(ch2, &nib2)) {
      return false;
    }
    *str += static_cast<char>((nib1 << 4) + nib2);
  }
  return true;
}

bool Packet::GetStringSep(std::string *str, char sep) {
  char ch;

  if (EndOfPacket()) {
    return false;
  }

  // Pull values until we hit a separator
  str->clear();
  while (GetRawChar(&ch)) {
    if (ch == sep) {
      return true;
    } else {
      *str += ch;
    }
  }
  return false;
}

const char *Packet::GetPayload() const {
  return &data_[0];
}

size_t Packet::GetPayloadSize() const {
  return write_index_;
}

bool Packet::GetSequence(int32_t *ch) const {
  assert(ch);

  if (seq_ != -1) {
    *ch = seq_;
    return true;
  }

  return false;
}

void Packet::ParseSequence() {
  size_t saved_read_index = read_index_;
  unsigned char seq;
  char ch;
  if (GetWord8(&seq) &&
      GetRawChar(&ch)) {
    if (ch == ':') {
      SetSequence(seq);
      return;
    }
  }
  // No sequence number present, so reset to original position.
  read_index_ = saved_read_index;
}

void Packet::SetSequence(int32_t val) {
  seq_ = val;
}

}  // namespace gdb_rsp
