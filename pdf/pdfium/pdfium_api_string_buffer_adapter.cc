// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pdf/pdfium/pdfium_api_string_buffer_adapter.h"

#include <stddef.h>

#include <string>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"

namespace chrome_pdf {

template <class StringType>
PDFiumAPIStringBufferAdapter<StringType>::PDFiumAPIStringBufferAdapter(
    StringType* str,
    size_t expected_size,
    bool check_expected_size)
    : str_(str),
      data_(base::WriteInto(str, expected_size + 1)),
      expected_size_(expected_size),
      check_expected_size_(check_expected_size),
      is_closed_(false) {}

template <class StringType>
PDFiumAPIStringBufferAdapter<StringType>::~PDFiumAPIStringBufferAdapter() {
  DCHECK(is_closed_);
}

template <class StringType>
void* PDFiumAPIStringBufferAdapter<StringType>::GetData() {
  DCHECK(!is_closed_);
  return data_;
}

template <class StringType>
void PDFiumAPIStringBufferAdapter<StringType>::Close(size_t actual_size) {
  DCHECK(!is_closed_);
  is_closed_ = true;

  if (check_expected_size_)
    DCHECK_EQ(expected_size_, actual_size);

  if (actual_size > 0) {
    DCHECK((*str_)[actual_size - 1] == 0);
    str_->resize(actual_size - 1);
  } else {
    str_->clear();
  }
}

template <class StringType>
PDFiumAPIStringBufferSizeInBytesAdapter<StringType>::
    PDFiumAPIStringBufferSizeInBytesAdapter(StringType* str,
                                            size_t expected_size,
                                            bool check_expected_size)
    : adapter_(str,
               expected_size / sizeof(typename StringType::value_type),
               check_expected_size) {
  DCHECK(expected_size % sizeof(typename StringType::value_type) == 0);
}

template <class StringType>
PDFiumAPIStringBufferSizeInBytesAdapter<
    StringType>::~PDFiumAPIStringBufferSizeInBytesAdapter() = default;

template <class StringType>
void* PDFiumAPIStringBufferSizeInBytesAdapter<StringType>::GetData() {
  return adapter_.GetData();
}

template <class StringType>
void PDFiumAPIStringBufferSizeInBytesAdapter<StringType>::Close(
    size_t actual_size) {
  DCHECK(actual_size % sizeof(typename StringType::value_type) == 0);
  adapter_.Close(actual_size / sizeof(typename StringType::value_type));
}

// explicit instantiations
template class PDFiumAPIStringBufferAdapter<std::string>;
template class PDFiumAPIStringBufferAdapter<base::string16>;
template class PDFiumAPIStringBufferSizeInBytesAdapter<base::string16>;

}  // namespace chrome_pdf
