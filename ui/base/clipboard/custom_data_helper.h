// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Due to restrictions of most operating systems, we don't directly map each
// type of custom data to a native data transfer type. Instead, we serialize
// each key-value pair into the pickle as a pair of string objects, and then
// write the binary data in the pickle to the native data transfer object.

#ifndef UI_BASE_CLIPBOARD_CUSTOM_DATA_HELPER_H_
#define UI_BASE_CLIPBOARD_CUSTOM_DATA_HELPER_H_

#include <stddef.h>

#include <unordered_map>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "ui/base/ui_base_export.h"

namespace base {
class Pickle;
}

#if defined(OS_MACOSX)
#ifdef __OBJC__
@class NSString;
#else
class NSString;
#endif
#endif  // defined(OS_MACOSX)

namespace ui {

#if defined(OS_MACOSX) && !defined(USE_AURA)
UI_BASE_EXPORT extern NSString* const kWebCustomDataPboardType;
#endif

UI_BASE_EXPORT void ReadCustomDataTypes(const void* data,
                                        size_t data_length,
                                        std::vector<base::string16>* types);
UI_BASE_EXPORT void ReadCustomDataForType(const void* data,
                                          size_t data_length,
                                          const base::string16& type,
                                          base::string16* result);
UI_BASE_EXPORT void ReadCustomDataIntoMap(
    const void* data,
    size_t data_length,
    std::unordered_map<base::string16, base::string16>* result);

UI_BASE_EXPORT void WriteCustomDataToPickle(
    const std::unordered_map<base::string16, base::string16>& data,
    base::Pickle* pickle);

UI_BASE_EXPORT void WriteCustomDataToPickle(
    const base::flat_map<base::string16, base::string16>& data,
    base::Pickle* pickle);

}  // namespace ui

#endif  // UI_BASE_CLIPBOARD_CLIPBOARD_H_
