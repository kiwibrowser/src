// Copyright 2016 The PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_hint_tables.h"
#include "core/fpdfapi/parser/cpdf_linearized_header.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fxcrt/cfx_bitstream.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/span.h"

int32_t GetData(const int32_t** data32, const uint8_t** data, size_t* size) {
  const int32_t* ret = *data32;
  ++(*data32);
  *data += 4;
  *size -= 4;
  return *ret;
}

class HintTableForFuzzing : public CPDF_HintTables {
 public:
  HintTableForFuzzing(CPDF_LinearizedHeader* pLinearized,
                      int shared_hint_table_offset)
      : CPDF_HintTables(nullptr, pLinearized),
        shared_hint_table_offset_(shared_hint_table_offset) {}
  ~HintTableForFuzzing() {}

  void Fuzz(const uint8_t* data, size_t size) {
    if (shared_hint_table_offset_ <= 0)
      return;

    if (size < static_cast<size_t>(shared_hint_table_offset_))
      return;

    CFX_BitStream bs(pdfium::make_span(data, size));
    if (!ReadPageHintTable(&bs))
      return;
    ReadSharedObjHintTable(&bs, shared_hint_table_offset_);
  }

 private:
  int shared_hint_table_offset_;
};

class FakeLinearized : public CPDF_LinearizedHeader {
 public:
  explicit FakeLinearized(CPDF_Dictionary* linearized_dict)
      : CPDF_LinearizedHeader(linearized_dict, 0) {}
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Need 28 bytes for |linearized_dict|.
  // The header section of page offset hint table is 36 bytes.
  // The header section of shared object hint table is 24 bytes.
  if (size < 28 + 36 + 24)
    return 0;

  const int32_t* data32 = reinterpret_cast<const int32_t*>(data);

  auto linearized_dict = pdfium::MakeUnique<CPDF_Dictionary>();
  // Set initial value.
  linearized_dict->SetNewFor<CPDF_Boolean>("Linearized", true);
  // Set first page end offset
  linearized_dict->SetNewFor<CPDF_Number>("E", GetData(&data32, &data, &size));
  // Set page count
  linearized_dict->SetNewFor<CPDF_Number>("N", GetData(&data32, &data, &size));
  // Set first page obj num
  linearized_dict->SetNewFor<CPDF_Number>("O", GetData(&data32, &data, &size));
  // Set first page no
  linearized_dict->SetNewFor<CPDF_Number>("P", GetData(&data32, &data, &size));

  auto hint_info = pdfium::MakeUnique<CPDF_Array>();
  // Add primary hint stream offset
  hint_info->AddNew<CPDF_Number>(GetData(&data32, &data, &size));
  // Add primary hint stream size
  hint_info->AddNew<CPDF_Number>(GetData(&data32, &data, &size));
  // Set hint stream info.
  linearized_dict->SetFor("H", std::move(hint_info));

  const int shared_hint_table_offset = GetData(&data32, &data, &size);

  {
    FakeLinearized linearized(linearized_dict.get());
    HintTableForFuzzing hint_table(&linearized, shared_hint_table_offset);
    hint_table.Fuzz(data, size);
  }
  return 0;
}
