// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Parse the data returned from the SafeBrowsing v2.1 protocol response.

// TODOv3(shess): Review these changes carefully.

#include "chrome/browser/safe_browsing/protocol_parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <utility>

#include "base/format_macros.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/sys_byteorder.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/safe_browsing/db/metadata.pb.h"

namespace safe_browsing {

namespace {

// Helper class for scanning a buffer.
class BufferReader {
 public:
  BufferReader(const char* data, size_t length)
      : data_(data),
        length_(length) {
  }

  // Return info about remaining buffer data.
  size_t length() const {
    return length_;
  }
  const char* data() const {
    return data_;
  }
  bool empty() const {
    return length_ == 0;
  }

  // Remove |l| characters from the buffer.
  void Advance(size_t l) {
    DCHECK_LE(l, length());
    data_ += l;
    length_ -= l;
  }

  // Get a reference to data in the buffer.
  // TODO(shess): I'm not sure I like this.  Fill out a StringPiece instead?
  bool RefData(const void** pptr, size_t l) {
    if (length() < l) {
      Advance(length());  // poison
      return false;
    }

    *pptr = data();
    Advance(l);
    return true;
  }

  // Copy data out of the buffer.
  bool GetData(void* ptr, size_t l) {
    const void* buf_ptr;
    if (!RefData(&buf_ptr, l))
      return false;

    memcpy(ptr, buf_ptr, l);
    return true;
  }

  // Read a 32-bit integer in network byte order into a local uint32_t.
  bool GetNet32(uint32_t* i) {
    if (!GetData(i, sizeof(*i)))
      return false;

    *i = base::NetToHost32(*i);
    return true;
  }

  // Returns false if there is no data, otherwise fills |*line| with a reference
  // to the next line of data in the buffer.
  bool GetLine(base::StringPiece* line) {
    if (!length_)
      return false;

    // Find the end of the line, or the end of the input.
    size_t eol = 0;
    while (eol < length_ && data_[eol] != '\n') {
      ++eol;
    }
    line->set(data_, eol);
    Advance(eol);

    // Skip the newline if present.
    if (length_ && data_[0] == '\n')
      Advance(1);

    return true;
  }

  // Read out |c| colon-separated pieces from the next line.  The resulting
  // pieces point into the original data buffer.
  bool GetPieces(size_t c, std::vector<base::StringPiece>* pieces) {
    base::StringPiece line;
    if (!GetLine(&line))
      return false;

    // Find the parts separated by ':'.
    while (pieces->size() + 1 < c) {
      size_t colon_ofs = line.find(':');
      if (colon_ofs == base::StringPiece::npos) {
        Advance(length_);
        return false;
      }

      pieces->push_back(line.substr(0, colon_ofs));
      line.remove_prefix(colon_ofs + 1);
    }

    // The last piece runs to the end of the line.
    pieces->push_back(line);
    return true;
  }

 private:
  const char* data_;
  size_t length_;

  DISALLOW_COPY_AND_ASSIGN(BufferReader);
};

// Helper function to parse malware metadata string
void InterpretMalwareMetadataString(const std::string& raw_metadata,
                                    ThreatMetadata* metadata) {
  MalwarePatternType proto;
  if (!proto.ParseFromString(raw_metadata)) {
    DCHECK(false) << "Bad MalwarePatternType";
    return;
  }

  // Convert proto enum to internal enum (we'll move away from this
  // proto in Pver4).
  switch (proto.pattern_type()) {
    case MalwarePatternType::LANDING:
      metadata->threat_pattern_type = ThreatPatternType::MALWARE_LANDING;
      break;
    case MalwarePatternType::DISTRIBUTION:
      metadata->threat_pattern_type = ThreatPatternType::MALWARE_DISTRIBUTION;
      break;
    default:
      metadata->threat_pattern_type = ThreatPatternType::NONE;
  }
}

// Helper function to parse social engineering metadata string.
void InterpretSocEngMetadataString(const std::string& raw_metadata,
                                   ThreatMetadata* metadata) {
  SocialEngineeringPatternType proto;
  if (!proto.ParseFromString(raw_metadata)) {
    DCHECK(false) << "Bad SocialEngineeringPatternType";
    return;
  }

  switch (proto.pattern_type()) {
    case SocialEngineeringPatternType::SOCIAL_ENGINEERING_ADS:
      metadata->threat_pattern_type = ThreatPatternType::SOCIAL_ENGINEERING_ADS;
      break;
    case SocialEngineeringPatternType::SOCIAL_ENGINEERING_LANDING:
      metadata->threat_pattern_type =
          ThreatPatternType::SOCIAL_ENGINEERING_LANDING;
      break;
    case SocialEngineeringPatternType::PHISHING:
      metadata->threat_pattern_type = ThreatPatternType::PHISHING;
      break;
    default:
      metadata->threat_pattern_type = ThreatPatternType::NONE;
  }
}

// Parse the |raw_metadata| string based on the |list_type| and populate
// the appropriate field of |metadata|.  For Pver3 (which this file implements),
// we only fill in threat_pattern_type.  Others are populated for Pver4.
void InterpretMetadataString(const std::string& raw_metadata,
                             ListType list_type,
                             ThreatMetadata* metadata) {
  if (raw_metadata.empty())
    return;
  if (list_type == MALWARE)
    InterpretMalwareMetadataString(raw_metadata, metadata);
  else if (list_type == PHISH)
    InterpretSocEngMetadataString(raw_metadata, metadata);
}

bool ParseGetHashMetadata(
    size_t hash_count,
    BufferReader* reader,
    std::vector<SBFullHashResult>* full_hashes) {
  for (size_t i = 0; i < hash_count; ++i) {
    base::StringPiece line;
    if (!reader->GetLine(&line))
      return false;

    size_t meta_data_len;
    if (!base::StringToSizeT(line, &meta_data_len))
      return false;

    const void* meta_data;
    if (!reader->RefData(&meta_data, meta_data_len))
      return false;

    if (full_hashes) {
      const std::string raw_metadata(reinterpret_cast<const char*>(meta_data),
                                     meta_data_len);
      // Update the i'th entry in the last hash_count elements of the list.
      SBFullHashResult* full_hash =
          &((*full_hashes)[full_hashes->size() - hash_count + i]);
      InterpretMetadataString(raw_metadata,
                              static_cast<ListType>(full_hash->list_id),
                              &full_hash->metadata);
    }
  }
  return true;
}

}  // namespace

// BODY          = CACHELIFETIME LF HASHENTRY* EOF
// CACHELIFETIME = DIGIT+
// HASHENTRY     = LISTNAME ":" HASHSIZE ":" NUMRESPONSES [":m"] LF
//                 HASHDATA (METADATALEN LF METADATA)*
// HASHSIZE      = DIGIT+                  # Length of each full hash
// NUMRESPONSES  = DIGIT+                  # Number of full hashes in HASHDATA
// HASHDATA      = <HASHSIZE*NUMRESPONSES number of unsigned bytes>
// METADATALEN   = DIGIT+
// METADATA      = <METADATALEN number of unsigned bytes>
bool ParseGetHash(const char* chunk_data,
                  size_t chunk_len,
                  base::TimeDelta* cache_lifetime,
                  std::vector<SBFullHashResult>* full_hashes) {
  full_hashes->clear();
  BufferReader reader(chunk_data, chunk_len);

  // Parse out cache lifetime.
  {
    base::StringPiece line;
    if (!reader.GetLine(&line))
      return false;

    int64_t cache_lifetime_seconds;
    if (!base::StringToInt64(line, &cache_lifetime_seconds))
      return false;

    // TODO(shess): Zero also doesn't make sense, but isn't clearly forbidden,
    // either.  Maybe there should be a threshold involved.
    if (cache_lifetime_seconds < 0)
      return false;

    *cache_lifetime = base::TimeDelta::FromSeconds(cache_lifetime_seconds);
  }

  while (!reader.empty()) {
    std::vector<base::StringPiece> cmd_parts;
    if (!reader.GetPieces(3, &cmd_parts))
      return false;

    SBFullHashResult full_hash;
    full_hash.list_id = GetListId(cmd_parts[0]);

    size_t hash_len;
    if (!base::StringToSizeT(cmd_parts[1], &hash_len))
      return false;

    // TODO(shess): Is this possible?  If not, why the length present?
    if (hash_len != sizeof(SBFullHash))
      return false;

    // Metadata is indicated by an optional ":m" at the end of the line.
    bool has_metadata = false;
    base::StringPiece hash_count_string = cmd_parts[2];
    size_t optional_colon = hash_count_string.find(':', 0);
    if (optional_colon != base::StringPiece::npos) {
      if (hash_count_string.substr(optional_colon) != ":m")
        return false;
      has_metadata = true;
      hash_count_string.remove_suffix(2);
    }

    size_t hash_count;
    if (!base::StringToSizeT(hash_count_string, &hash_count))
      return false;

    if (hash_len * hash_count > reader.length())
      return false;

    // Ignore hash results from lists we don't recognize.
    if (full_hash.list_id < 0) {
      reader.Advance(hash_len * hash_count);
      if (has_metadata && !ParseGetHashMetadata(hash_count, &reader, NULL))
        return false;
      continue;
    }

    for (size_t i = 0; i < hash_count; ++i) {
      if (!reader.GetData(&full_hash.hash, hash_len))
        return false;
      full_hashes->push_back(full_hash);
    }

    if (has_metadata && !ParseGetHashMetadata(hash_count, &reader, full_hashes))
      return false;
  }

  return reader.empty();
}

// BODY       = HEADER LF PREFIXES EOF
// HEADER     = PREFIXSIZE ":" LENGTH
// PREFIXSIZE = DIGIT+         # Size of each prefix in bytes
// LENGTH     = DIGIT+         # Size of PREFIXES in bytes
std::string FormatGetHash(const std::vector<SBPrefix>& prefixes) {
  std::string request;
  request.append(base::NumberToString(sizeof(SBPrefix)));
  request.append(":");
  request.append(base::NumberToString(sizeof(SBPrefix) * prefixes.size()));
  request.append("\n");

  // SBPrefix values are read without concern for byte order, so write back the
  // same way.
  for (size_t i = 0; i < prefixes.size(); ++i) {
    request.append(reinterpret_cast<const char*>(&prefixes[i]),
                   sizeof(SBPrefix));
  }

  return request;
}

bool ParseUpdate(const char* chunk_data,
                 size_t chunk_len,
                 size_t* next_update_sec,
                 bool* reset,
                 std::vector<SBChunkDelete>* deletes,
                 std::vector<ChunkUrl>* chunk_urls) {
  DCHECK(next_update_sec);
  DCHECK(deletes);
  DCHECK(chunk_urls);

  BufferReader reader(chunk_data, chunk_len);

  // Populated below.
  std::string list_name;

  while (!reader.empty()) {
    std::vector<base::StringPiece> pieces;
    if (!reader.GetPieces(2, &pieces))
      return false;

    base::StringPiece& command = pieces[0];

    // Differentiate on the first character of the command (which is usually
    // only one character, with the exception of the 'ad' and 'sd' commands).
    switch (command[0]) {
      case 'a':
      case 's': {
        // Must be either an 'ad' (add-del) or 'sd' (sub-del) chunk. We must
        // have also parsed the list name before getting here, or the add-del
        // or sub-del will have no context.
        if (list_name.empty() || (command != "ad" && command != "sd"))
          return false;
        SBChunkDelete chunk_delete;
        chunk_delete.is_sub_del = command[0] == 's';
        StringToRanges(pieces[1].as_string(), &chunk_delete.chunk_del);
        chunk_delete.list_name = list_name;
        deletes->push_back(chunk_delete);
        break;
      }

      case 'i':
        // The line providing the name of the list (i.e. 'goog-phish-shavar').
        list_name = pieces[1].as_string();
        break;

      case 'n':
        // The line providing the next earliest time (in seconds) to re-query.
        if (!base::StringToSizeT(pieces[1], next_update_sec))
          return false;
        break;

      case 'u': {
        ChunkUrl chunk_url;
        chunk_url.url = pieces[1].as_string();  // Skip the initial "u:".
        chunk_url.list_name = list_name;
        chunk_urls->push_back(chunk_url);
        break;
      }

      case 'r':
        if (pieces[1] != "pleasereset")
          return false;
        *reset = true;
        break;

      default:
        // According to the spec, we ignore commands we don't understand.
        // TODO(shess): Does this apply to r:unknown or n:not-integer?
        break;
    }
  }

  return true;
}

// BODY      = (UINT32 CHUNKDATA)+
// UINT32    = Unsigned 32-bit integer in network byte order
// CHUNKDATA = Encoded ChunkData protocol message
bool ParseChunk(const char* data,
                size_t length,
                std::vector<std::unique_ptr<SBChunkData>>* chunks) {
  BufferReader reader(data, length);

  while (!reader.empty()) {
    uint32_t l = 0;
    if (!reader.GetNet32(&l) || l == 0 || l > reader.length())
      return false;

    const void* p = NULL;
    if (!reader.RefData(&p, l))
      return false;

    std::unique_ptr<SBChunkData> chunk(new SBChunkData());
    if (!chunk->ParseFrom(reinterpret_cast<const unsigned char*>(p), l))
      return false;

    chunks->push_back(std::move(chunk));
  }

  DCHECK(reader.empty());
  return true;
}

// LIST      = LISTNAME ";" LISTINFO (":" LISTINFO)*
// LISTINFO  = CHUNKTYPE ":" CHUNKLIST
// CHUNKTYPE = "a" | "s"
// CHUNKLIST = (RANGE | NUMBER) ["," CHUNKLIST]
// NUMBER    = DIGIT+
// RANGE     = NUMBER "-" NUMBER
std::string FormatList(const SBListChunkRanges& list) {
  std::string formatted_results = list.name;
  formatted_results.append(";");

  if (!list.adds.empty())
    formatted_results.append("a:").append(list.adds);
  if (!list.adds.empty() && !list.subs.empty())
    formatted_results.append(":");
  if (!list.subs.empty())
    formatted_results.append("s:").append(list.subs);
  formatted_results.append("\n");

  return formatted_results;
}

}  // namespace safe_browsing
