// Copyright 2017 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/linux/memory_map.h"

#include <stdio.h>
#include <string.h>
#include <sys/sysmacros.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "util/file/delimited_file_reader.h"
#include "util/file/file_io.h"
#include "util/file/string_file.h"
#include "util/stdlib/string_number_conversion.h"

namespace crashpad {

namespace {

// This function is used in this file specfically for signed or unsigned longs.
// longs are typically either int or int64 sized, but pointers to longs are not
// automatically coerced to pointers to ints when they are the same size.
// Simply adding a StringToNumber for longs doesn't work since sometimes long
// and int64_t are actually the same type, resulting in a redefinition error.
template <typename Type>
bool LocalStringToNumber(const std::string& string, Type* number) {
  static_assert(sizeof(Type) == sizeof(int) || sizeof(Type) == sizeof(int64_t),
                "Unexpected Type size");

  if (sizeof(Type) == sizeof(int)) {
    return std::numeric_limits<Type>::is_signed
               ? StringToNumber(string, reinterpret_cast<int*>(number))
               : StringToNumber(string,
                                reinterpret_cast<unsigned int*>(number));
  } else {
    return std::numeric_limits<Type>::is_signed
               ? StringToNumber(string, reinterpret_cast<int64_t*>(number))
               : StringToNumber(string, reinterpret_cast<uint64_t*>(number));
  }
}

template <typename Type>
bool HexStringToNumber(const std::string& string, Type* number) {
  return LocalStringToNumber("0x" + string, number);
}

// The result from parsing a line from the maps file.
enum class ParseResult {
  // A line was successfully parsed.
  kSuccess = 0,

  // The end of the file was successfully reached.
  kEndOfFile,

  // There was an error in the file, likely because it was read non-atmoically.
  // We should try to read it again.
  kRetry,

  // An error with a message logged.
  kError
};

// Reads a line from a maps file being read by maps_file_reader and extends
// mappings with a new MemoryMap::Mapping describing the line.
ParseResult ParseMapsLine(DelimitedFileReader* maps_file_reader,
                          std::vector<MemoryMap::Mapping>* mappings) {
  std::string field;
  LinuxVMAddress start_address;
  switch (maps_file_reader->GetDelim('-', &field)) {
    case DelimitedFileReader::Result::kError:
      return ParseResult::kError;
    case DelimitedFileReader::Result::kEndOfFile:
      return ParseResult::kEndOfFile;
    case DelimitedFileReader::Result::kSuccess:
      field.pop_back();
      if (!HexStringToNumber(field, &start_address)) {
        LOG(ERROR) << "format error";
        return ParseResult::kError;
      }
      if (!mappings->empty() && start_address < mappings->back().range.End()) {
        return ParseResult::kRetry;
      }
  }

  LinuxVMAddress end_address;
  if (maps_file_reader->GetDelim(' ', &field) !=
          DelimitedFileReader::Result::kSuccess ||
      (field.pop_back(), !HexStringToNumber(field, &end_address))) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }
  if (end_address < start_address) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }
  // Skip zero-length mappings.
  if (end_address == start_address) {
    std::string rest_of_line;
    if (maps_file_reader->GetLine(&rest_of_line) !=
        DelimitedFileReader::Result::kSuccess) {
      LOG(ERROR) << "format error";
      return ParseResult::kError;
    }
    return ParseResult::kSuccess;
  }

  // TODO(jperaza): set bitness properly
#if defined(ARCH_CPU_64_BITS)
  constexpr bool is_64_bit = true;
#else
  constexpr bool is_64_bit = false;
#endif

  MemoryMap::Mapping mapping;
  mapping.range.SetRange(is_64_bit, start_address, end_address - start_address);

  if (maps_file_reader->GetDelim(' ', &field) !=
          DelimitedFileReader::Result::kSuccess ||
      (field.pop_back(), field.size() != 4)) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }
#define SET_FIELD(actual_c, outval, true_chars, false_chars) \
  do {                                                       \
    if (strchr(true_chars, actual_c)) {                      \
      *outval = true;                                        \
    } else if (strchr(false_chars, actual_c)) {              \
      *outval = false;                                       \
    } else {                                                 \
      LOG(ERROR) << "format error";                          \
      return ParseResult::kError;                            \
    }                                                        \
  } while (false)
  SET_FIELD(field[0], &mapping.readable, "r", "-");
  SET_FIELD(field[1], &mapping.writable, "w", "-");
  SET_FIELD(field[2], &mapping.executable, "x", "-");
  SET_FIELD(field[3], &mapping.shareable, "sS", "p");
#undef SET_FIELD

  if (maps_file_reader->GetDelim(' ', &field) !=
          DelimitedFileReader::Result::kSuccess ||
      (field.pop_back(), !HexStringToNumber(field, &mapping.offset))) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }

  uint32_t major;
  if (maps_file_reader->GetDelim(':', &field) !=
          DelimitedFileReader::Result::kSuccess ||
      (field.pop_back(), field.size()) < 2 ||
      !HexStringToNumber(field, &major)) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }

  uint32_t minor;
  if (maps_file_reader->GetDelim(' ', &field) !=
          DelimitedFileReader::Result::kSuccess ||
      (field.pop_back(), field.size()) < 2 ||
      !HexStringToNumber(field, &minor)) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }

  mapping.device = makedev(major, minor);

  if (maps_file_reader->GetDelim(' ', &field) !=
          DelimitedFileReader::Result::kSuccess ||
      (field.pop_back(), !LocalStringToNumber(field, &mapping.inode))) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }

  if (maps_file_reader->GetDelim('\n', &field) !=
      DelimitedFileReader::Result::kSuccess) {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }
  if (field.back() != '\n') {
    LOG(ERROR) << "format error";
    return ParseResult::kError;
  }
  field.pop_back();

  mappings->push_back(mapping);

  size_t path_start = field.find_first_not_of(' ');
  if (path_start != std::string::npos) {
    mappings->back().name = field.substr(path_start);
  }
  return ParseResult::kSuccess;
}

}  // namespace

MemoryMap::Mapping::Mapping()
    : name(),
      range(false, 0, 0),
      offset(0),
      device(0),
      inode(0),
      readable(false),
      writable(false),
      executable(false),
      shareable(false) {}

MemoryMap::MemoryMap() : mappings_(), initialized_() {}

MemoryMap::~MemoryMap() {}

bool MemoryMap::Mapping::Equals(const Mapping& other) const {
  DCHECK_EQ(range.Is64Bit(), other.range.Is64Bit());
  return range.Base() == other.range.Base() &&
         range.Size() == other.range.Size() && name == other.name &&
         offset == other.offset && device == other.device &&
         inode == other.inode && readable == other.readable &&
         writable == other.writable && executable == other.executable &&
         shareable == other.shareable;
}

bool MemoryMap::Initialize(PtraceConnection* connection) {
  INITIALIZATION_STATE_SET_INITIALIZING(initialized_);

  // If the maps file is not read atomically, entries can be read multiple times
  // or missed entirely. The kernel reads entries from this file into a page
  // sized buffer, so maps files larger than a page require multiple reads.
  // Attempt to reduce the time between reads by reading the entire file into a
  // StringFile before attempting to parse it. If ParseMapsLine detects
  // duplicate, overlapping, or out-of-order entries, it will trigger restarting
  // the read up to |attempts| times.
  int attempts = 3;
  do {
    std::string contents;
    char path[32];
    snprintf(path, sizeof(path), "/proc/%d/maps", connection->GetProcessID());
    if (!connection->ReadFileContents(base::FilePath(path), &contents)) {
      return false;
    }

    StringFile maps_file;
    maps_file.SetString(contents);
    DelimitedFileReader maps_file_reader(&maps_file);

    ParseResult result;
    while ((result = ParseMapsLine(&maps_file_reader, &mappings_)) ==
           ParseResult::kSuccess) {
    }
    if (result == ParseResult::kEndOfFile) {
      INITIALIZATION_STATE_SET_VALID(initialized_);
      return true;
    }
    if (result == ParseResult::kError) {
      return false;
    }

    DCHECK(result == ParseResult::kRetry);
  } while (--attempts > 0);

  LOG(ERROR) << "retry count exceeded";
  return false;
}

const MemoryMap::Mapping* MemoryMap::FindMapping(LinuxVMAddress address) const {
  INITIALIZATION_STATE_DCHECK_VALID(initialized_);

  for (const auto& mapping : mappings_) {
    if (mapping.range.Base() <= address && mapping.range.End() > address) {
      return &mapping;
    }
  }
  return nullptr;
}

const MemoryMap::Mapping* MemoryMap::FindMappingWithName(
    const std::string& name) const {
  INITIALIZATION_STATE_DCHECK_VALID(initialized_);

  for (const auto& mapping : mappings_) {
    if (mapping.name == name) {
      return &mapping;
    }
  }
  return nullptr;
}

const MemoryMap::Mapping* MemoryMap::FindFileMmapStart(
    const Mapping& mapping) const {
  INITIALIZATION_STATE_DCHECK_VALID(initialized_);

  size_t index = 0;
  for (; index < mappings_.size(); ++index) {
    if (mappings_[index].Equals(mapping)) {
      break;
    }
  }
  if (index >= mappings_.size()) {
    LOG(ERROR) << "mapping not found";
    return nullptr;
  }

  // If the mapping is anonymous, as is for the VDSO, there is no mapped file to
  // find the start of, so just return the input mapping.
  if (mapping.device == 0 && mapping.inode == 0) {
    return &mappings_[index];
  }

  do {
    // There may by anonymous mappings or other files mapped into the holes,
    // so check that the mapping uses the same file as the input, but keep
    // searching if it doesn't.
    if (mappings_[index].device == mapping.device &&
        mappings_[index].inode == mapping.inode &&
        mappings_[index].offset == 0) {
      return &mappings_[index];
    }
  } while (index--);

  LOG(ERROR) << "mapping not found";
  return nullptr;
}

}  // namespace crashpad
