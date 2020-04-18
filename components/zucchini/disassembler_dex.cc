// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/disassembler_dex.h"

#include <stddef.h>
#include <stdlib.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "components/zucchini/buffer_source.h"
#include "components/zucchini/buffer_view.h"
#include "components/zucchini/io_utils.h"

namespace zucchini {

namespace {

// Size of a Dalvik instruction unit. Need to cast to signed int because
// sizeof() gives size_t, which dominates when operated on ptrdiff_t, then
// wrecks havoc for base::checked_cast<int16_t>().
constexpr int kInstrUnitSize = static_cast<int>(sizeof(uint16_t));

/******** CodeItemParser ********/

// A parser to extract successive code items from a DEX image whose header has
// been parsed.
class CodeItemParser {
 public:
  using size_type = BufferSource::size_type;

  explicit CodeItemParser(ConstBufferView image) : image_(image) {}

  // Initializes the parser, returns true on success and false on error.
  bool Init(const dex::MapItem& code_map_item) {
    // Sanity check to quickly fail if |code_map_item.offset| or
    // |code_map_item.size| is too large. This is a heuristic because code item
    // sizes need to be parsed (sizeof(dex::CodeItem) is a lower bound).
    if (!image_.covers_array(code_map_item.offset, code_map_item.size,
                             sizeof(dex::CodeItem))) {
      return false;
    }
    source_ = std::move(BufferSource(image_).Skip(code_map_item.offset));
    return true;
  }

  // Extracts the header of the next code item, and skips the variable-length
  // data. Returns the offset of the code item if successful. Otherwise returns
  // kInvalidOffset, and thereafter the parser becomes valid. For reference,
  // here's a pseudo-struct of a complete code item:
  //
  // struct code_item {
  //   // 4-byte aligned here.
  //   // 16-byte header defined (dex::CodeItem).
  //   uint16_t registers_size;
  //   uint16_t ins_size;
  //   uint16_t outs_size;
  //   uint16_t tries_size;
  //   uint32_t debug_info_off;
  //   uint32_t insns_size;
  //
  //   // Variable-length data follow.
  //   uint16_t insns[insns_size];  // Instruction bytes.
  //   uint16_t padding[(tries_size > 0 && insns_size % 2 == 1) ? 1 : 0];
  //
  //   if (tries_size > 0) {
  //     // 4-byte aligned here.
  //     struct try_item {  // dex::TryItem.
  //       uint32_t start_addr;
  //       uint16_t insn_count;
  //       uint16_t handler_off;
  //     } tries[tries_size];
  //
  //     struct encoded_catch_handler_list {
  //       uleb128 handlers_size;
  //       struct encoded_catch_handler {
  //         sleb128 encoded_catch_handler_size;
  //         struct encoded_type_addr_pair {
  //           uleb128 type_idx;
  //           uleb128 addr;
  //         } handlers[abs(encoded_catch_handler_size)];
  //         if (encoded_catch_handler_size <= 0) {
  //           uleb128 catch_all_addr;
  //         }
  //       } handlers_list[handlers_size];
  //     } handlers_group;  // Confusingly called "handlers" in DEX doc.
  //   }
  //
  //   // Padding to 4-bytes align next code_item *only if more exist*.
  // }
  offset_t GetNext() {
    // Read header CodeItem.
    if (!source_.AlignOn(image_, 4U))
      return kInvalidOffset;
    const offset_t code_item_offset =
        base::checked_cast<offset_t>(source_.begin() - image_.begin());
    const auto* code_item = source_.GetPointer<const dex::CodeItem>();
    if (!code_item)
      return kInvalidOffset;
    DCHECK_EQ(0U, code_item_offset % 4U);

    // Skip instruction bytes.
    if (!source_.GetArray<uint16_t>(code_item->insns_size))
      return kInvalidOffset;
    // Skip padding if present.
    if (code_item->tries_size > 0 && !source_.AlignOn(image_, 4U))
      return kInvalidOffset;

    // Skip tries[] and handlers_group to arrive at the next code item. Parsing
    // is nontrivial due to use of uleb128 / sleb128.
    if (code_item->tries_size > 0) {
      // Skip (try_item) tries[].
      if (!source_.GetArray<dex::TryItem>(code_item->tries_size))
        return kInvalidOffset;

      // Skip handlers_group.
      uint32_t handlers_size = 0;
      if (!source_.GetUleb128(&handlers_size))
        return kInvalidOffset;
      // Sanity check to quickly reject excessively large |handlers_size|.
      if (source_.Remaining() < static_cast<size_type>(handlers_size))
        return kInvalidOffset;

      // Skip (encoded_catch_handler) handlers_list[].
      for (uint32_t k = 0; k < handlers_size; ++k) {
        int32_t encoded_catch_handler_size = 0;
        if (!source_.GetSleb128(&encoded_catch_handler_size))
          return kInvalidOffset;
        const size_type abs_size = std::abs(encoded_catch_handler_size);
        if (source_.Remaining() < abs_size)  // Sanity check.
          return kInvalidOffset;
        // Skip (encoded_type_addr_pair) handlers[].
        for (size_type j = 0; j < abs_size; ++j) {
          if (!source_.SkipLeb128() || !source_.SkipLeb128())
            return kInvalidOffset;
        }
        // Skip catch_all_addr.
        if (encoded_catch_handler_size <= 0) {
          if (!source_.SkipLeb128())
            return kInvalidOffset;
        }
      }
    }
    // Success! |code_item->insns_size| is validated, but its content is still
    // considered unsafe and requires validation.
    return code_item_offset;
  }

  // Given |code_item_offset| that points to the start of a valid code item in
  // |image|, returns |insns| bytes as ConstBufferView.
  static ConstBufferView GetCodeItemInsns(ConstBufferView image,
                                          offset_t code_item_offset) {
    BufferSource source(BufferSource(image).Skip(code_item_offset));
    const auto* code_item = source.GetPointer<const dex::CodeItem>();
    DCHECK(code_item);
    BufferRegion insns{0, code_item->insns_size * kInstrUnitSize};
    DCHECK(source.covers(insns));
    return source[insns];
  }

 private:
  ConstBufferView image_;
  BufferSource source_;
};

/******** InstructionParser ********/

// A class that successively reads |code_item| for Dalvik instructions, which
// are found at |insns|, spanning |insns_size| uint16_t "units". These units
// store instructions followed by optional non-instruction "payload". Finding
// payload boundary requires parsing: On finding an instruction that uses (and
// points to) payload, the boundary is updated.
class InstructionParser {
 public:
  struct Value {
    offset_t instr_offset;
    const dex::Instruction* instr = nullptr;  // null for unknown instructions.
  };

  // Returns pointer to DEX Instruction data for |opcode|, or null if |opcode|
  // is unknown. An internal initialize-on-first-use table is used for fast
  // lookup.
  const dex::Instruction* FindDalvikInstruction(uint8_t opcode) {
    static bool is_init = false;
    static const dex::Instruction* instruction_table[256];
    if (!is_init) {
      is_init = true;
      std::fill(std::begin(instruction_table), std::end(instruction_table),
                nullptr);
      for (const dex::Instruction& instr : dex::kByteCode) {
        std::fill(instruction_table + instr.opcode,
                  instruction_table + instr.opcode + instr.variant, &instr);
      }
    }
    return instruction_table[opcode];
  }

  InstructionParser() = default;

  InstructionParser(ConstBufferView image, offset_t base_offset)
      : image_begin_(image.begin()),
        insns_(CodeItemParser::GetCodeItemInsns(image, base_offset)),
        payload_boundary_(insns_.end()) {}

  // Reads the next instruction. On success, makes the data read available via
  // value() and returns true. Otherwise (done or found error) returns false.
  bool ReadNext() {
    // Do not scan past payload boundary.
    if (insns_.begin() >= payload_boundary_)
      return false;

    const offset_t instr_offset =
        base::checked_cast<offset_t>(insns_.begin() - image_begin_);
    const uint8_t op = insns_.read<uint8_t>(0);
    const dex::Instruction* instr = FindDalvikInstruction(op);

    // Stop on finding unknown instructions. ODEX files might trigger this.
    if (!instr) {
      LOG(WARNING) << "Unknown Dalvik instruction detected at "
                   << AsHex<8>(instr_offset) << ".";
      return false;
    }

    const int instr_length_units = instr->layout;
    const size_t instr_length_bytes = instr_length_units * kInstrUnitSize;
    if (insns_.size() < instr_length_bytes)
      return false;

    // Handle instructions with variable-length data payload (31t).
    if (instr->opcode == 0x26 ||  // fill-array-data
        instr->opcode == 0x2B ||  // packed-switch
        instr->opcode == 0x2C) {  // sparse-switch
      const int32_t unsafe_payload_rel_units = insns_.read<int32_t>(2);
      // Payload must be in current code item, after current instruction.
      if (unsafe_payload_rel_units < instr_length_units ||
          static_cast<uint32_t>(unsafe_payload_rel_units) >=
              insns_.size() / kInstrUnitSize) {
        LOG(WARNING) << "Invalid payload found.";
        return false;
      }
      // Update boundary between instructions and payload.
      const ConstBufferView::const_iterator payload_it =
          insns_.begin() + unsafe_payload_rel_units * kInstrUnitSize;
      payload_boundary_ = std::min(payload_boundary_, payload_it);
    }

    insns_.remove_prefix(instr_length_bytes);
    value_ = {instr_offset, instr};
    return true;
  }

  const Value& value() const { return value_; }

 private:
  ConstBufferView::const_iterator image_begin_;
  ConstBufferView insns_;
  ConstBufferView::const_iterator payload_boundary_;
  Value value_;
};

/******** InstructionReferenceReader ********/

// A class to visit |code_items|, parse instructions, and emit embedded
// References of a type determined by |filter_| and |mapper_|. Only References
// located in |[lo, hi)| are emitted. |lo| and |hi| are assumed to never
// straddle the body of a Reference.
class InstructionReferenceReader : public ReferenceReader {
 public:
  // A function that takes a parsed Dalvik instruction and decides whether it
  // contains a specific type of Reference. If true, then returns the Reference
  // location. Otherwise returns kInvalidOffset.
  using Filter =
      base::RepeatingCallback<offset_t(const InstructionParser::Value&)>;
  // A function that takes Reference location from |filter_| to extract the
  // stored target. If valid, returns it. Otherwise returns kInvalidOffset.
  using Mapper = base::RepeatingCallback<offset_t(offset_t)>;

  InstructionReferenceReader(ConstBufferView image,
                             offset_t lo,
                             offset_t hi,
                             const std::vector<offset_t>& code_item_offsets,
                             Filter&& filter,
                             Mapper&& mapper)
      : image_(image),
        lo_(lo),
        hi_(hi),
        end_it_(code_item_offsets.end()),
        filter_(std::move(filter)),
        mapper_(std::move(mapper)) {
    const auto begin_it = code_item_offsets.begin();
    // Use binary search to find the code item that contains |lo_|.
    auto comp = [](offset_t test_offset, offset_t code_item_offset) {
      return test_offset < code_item_offset;
    };
    cur_it_ = std::upper_bound(begin_it, end_it_, lo_, comp);
    if (cur_it_ != begin_it)
      --cur_it_;
    parser_ = InstructionParser(image_, *cur_it_);
  }

  // ReferenceReader:
  base::Optional<Reference> GetNext() override {
    for (;;) {
      while (parser_.ReadNext()) {
        const auto& v = parser_.value();
        DCHECK_NE(v.instr, nullptr);
        if (v.instr_offset >= hi_)
          return base::nullopt;
        const offset_t location = filter_.Run(v);
        if (location == kInvalidOffset || location < lo_)
          continue;
        // The general check is |location + reference_width > hi_|. However, by
        // assumption |hi_| and |lo_| do not straddle the body of a Reference.
        // So |reference_width| is unneeded.
        if (location >= hi_)
          return base::nullopt;
        offset_t target = mapper_.Run(location);
        if (target != kInvalidOffset)
          return Reference{location, target};
        else
          LOG(WARNING) << "Invalid target at " << AsHex<8>(location) << ".";
      }
      ++cur_it_;
      if (cur_it_ == end_it_)
        return base::nullopt;
      parser_ = InstructionParser(image_, *cur_it_);
    }
  }

 private:
  const ConstBufferView image_;
  const offset_t lo_;
  const offset_t hi_;
  const std::vector<offset_t>::const_iterator end_it_;
  const Filter filter_;
  const Mapper mapper_;
  std::vector<offset_t>::const_iterator cur_it_;
  InstructionParser parser_;
};

/******** ItemReferenceReader ********/

// A class to visit fixed-size item elements (determined by |item_size|) and
// emit a "member variable of interest" (MVI, determined by |rel_location| and
// |mapper|) as Reference. Only MVIs lying in |[lo, hi)| are emitted. |lo| and
// |hi| are assumed to never straddle the body of a Reference.
class ItemReferenceReader : public ReferenceReader {
 public:
  // A function that takes an MVI's location and emit its target offset.
  using Mapper = base::RepeatingCallback<offset_t(offset_t)>;

  // |item_size| is the size of a fixed-size item. |rel_location| is the
  // relative location of MVI from the start of the item containing it.
  ItemReferenceReader(offset_t lo,
                      offset_t hi,
                      const dex::MapItem& map_item,
                      size_t item_size,
                      size_t rel_location,
                      Mapper&& mapper)
      : hi_(hi),
        item_base_offset_(base::checked_cast<offset_t>(map_item.offset)),
        num_items_(base::checked_cast<uint32_t>(map_item.size)),
        item_size_(base::checked_cast<uint32_t>(item_size)),
        rel_location_(base::checked_cast<uint32_t>(rel_location)),
        mapper_(std::move(mapper)) {
    static_assert(sizeof(decltype(map_item.offset)) <= sizeof(offset_t),
                  "map_item.offset too large.");
    static_assert(sizeof(decltype(map_item.size)) <= sizeof(offset_t),
                  "map_item.size too large.");
    if (lo < item_base_offset_) {
      cur_idx_ = 0;
    } else if (lo < OffsetOfIndex(num_items_)) {
      cur_idx_ = (lo - item_base_offset_) / item_size_;
      // Fine-tune: Advance if |lo| lies beyond the MVI.
      if (lo > OffsetOfIndex(cur_idx_) + rel_location_)
        ++cur_idx_;
    } else {
      cur_idx_ = num_items_;
    }
  }

  // ReferenceReader:
  base::Optional<Reference> GetNext() override {
    if (cur_idx_ >= num_items_)
      return base::nullopt;

    const offset_t item_offset = OffsetOfIndex(cur_idx_);
    const offset_t location = item_offset + rel_location_;
    // The general check is |location + reference_width > hi_|. However, by
    // assumption |hi_| and |lo_| do not straddle the body of a Reference. So
    // |reference_width| is unneeded.
    if (location >= hi_)
      return base::nullopt;
    const offset_t target = mapper_.Run(location);
    if (target == kInvalidOffset) {
      LOG(WARNING) << "Invalid item target at " << AsHex<8>(location) << ".";
      return base::nullopt;
    }
    ++cur_idx_;
    return Reference{location, target};
  }

 private:
  offset_t OffsetOfIndex(uint32_t idx) {
    return base::checked_cast<uint32_t>(item_base_offset_ + idx * item_size_);
  }

  const offset_t hi_;
  const offset_t item_base_offset_;
  const uint32_t num_items_;
  const uint32_t item_size_;
  const uint32_t rel_location_;
  const Mapper mapper_;
  offset_t cur_idx_ = 0;
};

// Reads an INT index at |location| in |image| and translates the index to the
// offset of a fixed-size item specified by |target_map_item| and
// |target_item_size|. Returns the target offset if valid, or kInvalidOffset
// otherwise. This is compatible with InstructionReferenceReader::Mapper and
// ItemReferenceReader::Mapper.
template <typename INT>
static offset_t ReadTargetIndex(ConstBufferView image,
                                const dex::MapItem& target_map_item,
                                size_t target_item_size,
                                offset_t location) {
  static_assert(sizeof(INT) <= sizeof(offset_t),
                "INT may not fit into offset_t.");
  const offset_t unsafe_idx = image.read<INT>(location);
  if (unsafe_idx >= target_map_item.size)
    return kInvalidOffset;
  return target_map_item.offset +
         base::checked_cast<offset_t>(unsafe_idx * target_item_size);
}

/******** ReferenceWriterAdaptor ********/

// A ReferenceWriter that adapts a callback that performs type-specific
// Reference writes.
class ReferenceWriterAdaptor : public ReferenceWriter {
 public:
  using Writer = base::RepeatingCallback<void(Reference, MutableBufferView)>;

  ReferenceWriterAdaptor(MutableBufferView image, Writer&& writer)
      : image_(image), writer_(std::move(writer)) {}

  // ReferenceWriter:
  void PutNext(Reference ref) override { writer_.Run(ref, image_); }

 private:
  MutableBufferView image_;
  Writer writer_;
};

// Helper that's compatible with ReferenceWriterAdaptor::Writer.
// Given that |ref.target| points to the start of a fixed size DEX item (e.g.,
// FieldIdItem), translates |ref.target| to item index, and writes the result to
// |ref.location| as |INT|.
template <typename INT>
static void WriteTargetIndex(const dex::MapItem& target_map_item,
                             size_t target_item_size,
                             Reference ref,
                             MutableBufferView image) {
  const size_t idx = (ref.target - target_map_item.offset) / target_item_size;
  // Verify that index is within bound.
  DCHECK_LT(idx, target_map_item.size);
  // Verify that |ref.target| points to start of item.
  DCHECK_EQ(ref.target, target_map_item.offset + idx * target_item_size);
  image.write<INT>(ref.location, base::checked_cast<INT>(idx));
}

// Buffer for ReadDexHeader() to optionally return results.
struct ReadDexHeaderResults {
  BufferSource source;
  const dex::HeaderItem* header;
  int dex_version;
};

// Returns whether |image| points to a DEX file. If this is a possibility and
// |opt_results| is not null, then uses it to pass extracted data to enable
// further parsing.
bool ReadDexHeader(ConstBufferView image, ReadDexHeaderResults* opt_results) {
  // This part needs to be fairly efficient since it may be called many times.
  BufferSource source(image);
  const dex::HeaderItem* header = source.GetPointer<dex::HeaderItem>();
  if (!header)
    return false;
  if (header->magic[0] != 'd' || header->magic[1] != 'e' ||
      header->magic[2] != 'x' || header->magic[3] != '\n' ||
      header->magic[7] != '\0') {
    return false;
  }

  // Magic matches: More detailed tests can be conducted.
  int dex_version = 0;
  for (int i = 4; i < 7; ++i) {
    if (!isdigit(header->magic[i]))
      return false;
    dex_version = dex_version * 10 + (header->magic[i] - '0');
  }

  // Only support DEX versions 35 and 37.
  // TODO(huangs): Handle version 38.
  if (dex_version != 35 && dex_version != 37)
    return false;

  if (header->file_size > image.size() ||
      header->file_size < sizeof(dex::HeaderItem) ||
      header->map_off < sizeof(dex::HeaderItem)) {
    return false;
  }

  if (opt_results)
    *opt_results = {source, header, dex_version};
  return true;
}

}  // namespace

/******** DisassemblerDex ********/

DisassemblerDex::DisassemblerDex() : Disassembler(4) {}

DisassemblerDex::~DisassemblerDex() = default;

// static.
bool DisassemblerDex::QuickDetect(ConstBufferView image) {
  return ReadDexHeader(image, nullptr);
}

ExecutableType DisassemblerDex::GetExeType() const {
  return kExeTypeDex;
}

std::string DisassemblerDex::GetExeTypeString() const {
  return base::StringPrintf("DEX (version %d)", dex_version_);
}

std::vector<ReferenceGroup> DisassemblerDex::MakeReferenceGroups() const {
  // Must follow DisassemblerDex::ReferenceType order. Initialized on first use.
  return {
      {{4, TypeTag(kFieldIdToNameStringId), PoolTag(kStringId)},
       &DisassemblerDex::MakeReadFieldToNameStringId32,
       &DisassemblerDex::MakeWriteStringId32},
      {{2, TypeTag(kCodeToStringId16), PoolTag(kStringId)},
       &DisassemblerDex::MakeReadCodeToStringId16,
       &DisassemblerDex::MakeWriteStringId16},
      {{4, TypeTag(kCodeToStringId32), PoolTag(kStringId)},
       &DisassemblerDex::MakeReadCodeToStringId32,
       &DisassemblerDex::MakeWriteStringId32},
      {{2, TypeTag(kFieldIdToClassTypeId), PoolTag(kTypeId)},
       &DisassemblerDex::MakeReadFieldToClassTypeId16,
       &DisassemblerDex::MakeWriteTypeId16},
      {{2, TypeTag(kFieldIdToTypeId), PoolTag(kTypeId)},
       &DisassemblerDex::MakeReadFieldToTypeId16,
       &DisassemblerDex::MakeWriteTypeId16},
      {{2, TypeTag(kCodeToTypeId), PoolTag(kTypeId)},
       &DisassemblerDex::MakeReadCodeToTypeId16,
       &DisassemblerDex::MakeWriteTypeId16},
      {{2, TypeTag(kCodeToFieldId), PoolTag(kFieldId)},
       &DisassemblerDex::MakeReadCodeToFieldId16,
       &DisassemblerDex::MakeWriteFieldId16},
      {{2, TypeTag(kCodeToMethodId), PoolTag(kMethodId)},
       &DisassemblerDex::MakeReadCodeToMethodId16,
       &DisassemblerDex::MakeWriteMethodId16},
      {{2, TypeTag(kCodeToRelCode16), PoolTag(kCode)},
       &DisassemblerDex::MakeReadCodeToRelCode16,
       &DisassemblerDex::MakeWriteRelCode16},
      {{4, TypeTag(kCodeToRelCode32), PoolTag(kCode)},
       &DisassemblerDex::MakeReadCodeToRelCode32,
       &DisassemblerDex::MakeWriteRelCode32},
      {{4, TypeTag(kStringIdToStringData), PoolTag(kStringData)},
       &DisassemblerDex::MakeReadStringIdToStringData,
       &DisassemblerDex::MakeWriteAbs32},
  };
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadStringIdToStringData(
    offset_t lo,
    offset_t hi) {
  auto mapper = base::BindRepeating(
      [](ConstBufferView image, offset_t location) -> offset_t {
        const offset_t unsafe_target =
            image.read<decltype(dex::StringIdItem::string_data_off)>(location);
        // TODO(huangs): Check that |unsafe_target| lies in string data item.
        if (unsafe_target >= image.size())
          return kInvalidOffset;
        return unsafe_target;
      },
      image_);
  return std::make_unique<ItemReferenceReader>(
      lo, hi, string_map_item_, sizeof(dex::StringIdItem),
      offsetof(dex::StringIdItem, string_data_off), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadFieldToClassTypeId16(
    offset_t lo,
    offset_t hi) {
  auto mapper = base::BindRepeating(
      ReadTargetIndex<decltype(dex::FieldIdItem::class_idx)>, image_,
      type_map_item_, sizeof(dex::TypeIdItem));
  return std::make_unique<ItemReferenceReader>(
      lo, hi, field_map_item_, sizeof(dex::FieldIdItem),
      offsetof(dex::FieldIdItem, class_idx), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadFieldToTypeId16(
    offset_t lo,
    offset_t hi) {
  auto mapper =
      base::BindRepeating(ReadTargetIndex<decltype(dex::FieldIdItem::type_idx)>,
                          image_, type_map_item_, sizeof(dex::TypeIdItem));
  return std::make_unique<ItemReferenceReader>(
      lo, hi, field_map_item_, sizeof(dex::FieldIdItem),
      offsetof(dex::FieldIdItem, type_idx), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadFieldToNameStringId32(
    offset_t lo,
    offset_t hi) {
  auto mapper =
      base::BindRepeating(ReadTargetIndex<decltype(dex::FieldIdItem::name_idx)>,
                          image_, string_map_item_, sizeof(dex::StringIdItem));
  return std::make_unique<ItemReferenceReader>(
      lo, hi, field_map_item_, sizeof(dex::FieldIdItem),
      offsetof(dex::FieldIdItem, name_idx), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToStringId16(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::c &&
            (value.instr->opcode == 0x1A)) {  // const-string
          // BBBB from e.g., const-string vAA, string@BBBB.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper =
      base::BindRepeating(ReadTargetIndex<uint16_t>, image_, string_map_item_,
                          sizeof(dex::StringIdItem));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToStringId32(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::c &&
            (value.instr->opcode == 0x1B)) {  // const-string/jumbo
          // BBBBBBBB from e.g., const-string/jumbo vAA, string@BBBBBBBB.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper =
      base::BindRepeating(ReadTargetIndex<uint32_t>, image_, string_map_item_,
                          sizeof(dex::StringIdItem));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToTypeId16(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::c &&
            (value.instr->opcode == 0x1C ||   // const-class
             value.instr->opcode == 0x1F ||   // check-cast
             value.instr->opcode == 0x20 ||   // instance-of
             value.instr->opcode == 0x22 ||   // new-instance
             value.instr->opcode == 0x23 ||   // new-array
             value.instr->opcode == 0x24 ||   // filled-new-array
             value.instr->opcode == 0x25)) {  // filled-new-array/range
          // BBBB from e.g., const-class vAA, type@BBBB.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper = base::BindRepeating(ReadTargetIndex<uint16_t>, image_,
                                    type_map_item_, sizeof(dex::TypeIdItem));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToFieldId16(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::c &&
            (value.instr->opcode == 0x52 ||   // iinstanceop (iget-*, iput-*)
             value.instr->opcode == 0x60)) {  // sstaticop (sget-*, sput-*)
          // CCCC from e.g., iget vA, vB, field@CCCC.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper = base::BindRepeating(ReadTargetIndex<uint16_t>, image_,
                                    field_map_item_, sizeof(dex::FieldIdItem));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToMethodId16(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::c &&
            (value.instr->opcode == 0x6E ||   // invoke-kind
             value.instr->opcode == 0x74)) {  // invoke-kind/range
          // BBBB from e.g., invoke-virtual {vC, vD, vE, vF, vG}, meth@BBBB.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper =
      base::BindRepeating(ReadTargetIndex<uint16_t>, image_, method_map_item_,
                          sizeof(dex::MethodIdItem));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToRelCode16(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::t &&
            (value.instr->opcode == 0x29 ||   // goto/16
             value.instr->opcode == 0x32 ||   // if-test
             value.instr->opcode == 0x38)) {  // if-testz
          // +AAAA from e.g., goto/16 +AAAA.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper = base::BindRepeating(
      [](DisassemblerDex* dis, offset_t location) {
        // Address is relative to the current instruction, which begins 1 unit
        // before |location|. This needs to be subtracted out. Also, store as
        // int32_t so |unsafe_delta - 1| won't underflow!
        int32_t unsafe_delta = dis->image_.read<int16_t>(location);
        offset_t unsafe_target = static_cast<offset_t>(
            location + (unsafe_delta - 1) * kInstrUnitSize);
        // TODO(huangs): Check that |unsafe_target| stays within code item.
        return unsafe_target;
      },
      base::Unretained(this));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeReadCodeToRelCode32(
    offset_t lo,
    offset_t hi) {
  auto filter = base::BindRepeating(
      [](const InstructionParser::Value& value) -> offset_t {
        if (value.instr->format == dex::FormatId::t &&
            (value.instr->opcode == 0x26 ||   // fill-array-data
             value.instr->opcode == 0x2A ||   // goto/32
             value.instr->opcode == 0x2B ||   // packed-switch
             value.instr->opcode == 0x2C)) {  // sparse-switch
          // +BBBBBBBB from e.g., fill-array-data vAA, +BBBBBBBB.
          // +AAAAAAAA from e.g., goto/32 +AAAAAAAA.
          return value.instr_offset + 2;
        }
        return kInvalidOffset;
      });
  auto mapper = base::BindRepeating(
      [](DisassemblerDex* dis, offset_t location) {
        // Address is relative to the current instruction, which begins 1 unit
        // before |location|. This needs to be subtracted out.
        int32_t unsafe_delta = dis->image_.read<int32_t>(location);
        offset_t unsafe_target = static_cast<offset_t>(
            location + (unsafe_delta - 1) * kInstrUnitSize);
        // TODO(huangs): Check that |unsafe_target| stays within code item.
        return unsafe_target;
      },
      base::Unretained(this));
  return std::make_unique<InstructionReferenceReader>(
      image_, lo, hi, code_item_offsets_, std::move(filter), std::move(mapper));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteStringId16(
    MutableBufferView image) {
  auto writer = base::BindRepeating(
      WriteTargetIndex<uint16_t>, string_map_item_, sizeof(dex::StringIdItem));
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteStringId32(
    MutableBufferView image) {
  auto writer = base::BindRepeating(
      WriteTargetIndex<uint32_t>, string_map_item_, sizeof(dex::StringIdItem));
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteTypeId16(
    MutableBufferView image) {
  auto writer = base::BindRepeating(WriteTargetIndex<uint16_t>, type_map_item_,
                                    sizeof(dex::TypeIdItem));
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteFieldId16(
    MutableBufferView image) {
  auto writer = base::BindRepeating(WriteTargetIndex<uint16_t>, field_map_item_,
                                    sizeof(dex::FieldIdItem));
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteMethodId16(
    MutableBufferView image) {
  auto writer = base::BindRepeating(
      WriteTargetIndex<uint16_t>, method_map_item_, sizeof(dex::MethodIdItem));
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteRelCode16(
    MutableBufferView image) {
  auto writer = base::BindRepeating([](Reference ref, MutableBufferView image) {
    ptrdiff_t byte_diff = static_cast<ptrdiff_t>(ref.target) - ref.location;
    DCHECK_EQ(0, byte_diff % kInstrUnitSize);
    // |delta| is relative to start of instruction, which is 1 unit before
    // |ref.location|. The subtraction above removed too much, so +1 to fix.
    ptrdiff_t delta = (byte_diff / kInstrUnitSize) + 1;
    image.write<int16_t>(ref.location, base::checked_cast<int16_t>(delta));
  });
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteRelCode32(
    MutableBufferView image) {
  auto writer = base::BindRepeating([](Reference ref, MutableBufferView image) {
    ptrdiff_t byte_diff = static_cast<ptrdiff_t>(ref.target) - ref.location;
    DCHECK_EQ(0, byte_diff % kInstrUnitSize);
    ptrdiff_t delta = (byte_diff / kInstrUnitSize) + 1;
    image.write<int32_t>(ref.location, base::checked_cast<int32_t>(delta));
  });
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeWriteAbs32(
    MutableBufferView image) {
  auto writer = base::BindRepeating([](Reference ref, MutableBufferView image) {
    image.write<uint32_t>(ref.location, ref.target);
  });
  return std::make_unique<ReferenceWriterAdaptor>(image, std::move(writer));
}

bool DisassemblerDex::Parse(ConstBufferView image) {
  image_ = image;
  return ParseHeader();
}

bool DisassemblerDex::ParseHeader() {
  ReadDexHeaderResults results;
  if (!ReadDexHeader(image_, &results))
    return false;

  header_ = results.header;
  dex_version_ = results.dex_version;
  BufferSource source = results.source;

  // DEX header contains file size, so use it to resize |image_| right away.
  image_.shrink(header_->file_size);

  // Read map list. This is not a fixed-size array, so instead of reading
  // MapList directly, read |MapList::size| first, then visit elements in
  // |MapList::list|.
  static_assert(
      offsetof(dex::MapList, list) == sizeof(decltype(dex::MapList::size)),
      "MapList size error.");
  source = std::move(BufferSource(image_).Skip(header_->map_off));
  decltype(dex::MapList::size) list_size = 0;
  if (!source.GetValue(&list_size) || list_size > dex::kMaxItemListSize)
    return false;
  const auto* item_list = source.GetArray<const dex::MapItem>(list_size);
  if (!item_list)
    return false;

  // Read and validate map list, ensuring that required item types are present.
  std::set<uint16_t> required_item_types = {
      dex::kTypeStringIdItem, dex::kTypeTypeIdItem, dex::kTypeFieldIdItem,
      dex::kTypeMethodIdItem, dex::kTypeCodeItem};
  for (offset_t i = 0; i < list_size; ++i) {
    const dex::MapItem* item = &item_list[i];
    // Sanity check to reject unreasonably large |item->size|.
    // TODO(huangs): Implement a more stringent check.
    if (!image_.covers({item->offset, item->size}))
      return false;
    if (!map_item_map_.insert(std::make_pair(item->type, item)).second)
      return false;  // A given type must appear at most once.
    required_item_types.erase(item->type);
  }
  if (!required_item_types.empty())
    return false;

  // Make local copies of main map items.
  string_map_item_ = *map_item_map_[dex::kTypeStringIdItem];
  type_map_item_ = *map_item_map_[dex::kTypeTypeIdItem];
  field_map_item_ = *map_item_map_[dex::kTypeFieldIdItem];
  method_map_item_ = *map_item_map_[dex::kTypeMethodIdItem];
  code_map_item_ = *map_item_map_[dex::kTypeCodeItem];

  // Iteratively extract variable-length code items blocks. Any failure would
  // indicate invalid DEX. Success indicates that no structural problem is
  // found. However, contained instructions still need validation on use.
  CodeItemParser code_item_parser(image_);
  if (!code_item_parser.Init(code_map_item_))
    return false;
  code_item_offsets_.resize(code_map_item_.size);
  for (size_t i = 0; i < code_map_item_.size; ++i) {
    const offset_t code_item_offset = code_item_parser.GetNext();
    if (code_item_offset == kInvalidOffset)
      return false;
    code_item_offsets_[i] = code_item_offset;
  }
  return true;
}

}  // namespace zucchini
