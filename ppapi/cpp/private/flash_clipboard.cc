// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/flash_clipboard.h"

#include <vector>

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Flash_Clipboard_4_0>() {
  return PPB_FLASH_CLIPBOARD_INTERFACE_4_0;
}

template <> const char* interface_name<PPB_Flash_Clipboard_5_0>() {
  return PPB_FLASH_CLIPBOARD_INTERFACE_5_0;
}

template <> const char* interface_name<PPB_Flash_Clipboard_5_1>() {
  return PPB_FLASH_CLIPBOARD_INTERFACE_5_1;
}

}  // namespace

namespace flash {

// static
bool Clipboard::IsAvailable() {
  return has_interface<PPB_Flash_Clipboard_5_1>() ||
      has_interface<PPB_Flash_Clipboard_5_0>() ||
      has_interface<PPB_Flash_Clipboard_4_0>() ;
}

// static
uint32_t Clipboard::RegisterCustomFormat(const InstanceHandle& instance,
                                         const std::string& format_name) {
  uint32_t rv = PP_FLASH_CLIPBOARD_FORMAT_INVALID;
  if (has_interface<PPB_Flash_Clipboard_5_1>()) {
    rv = get_interface<PPB_Flash_Clipboard_5_1>()->RegisterCustomFormat(
        instance.pp_instance(), format_name.c_str());
  } else if (has_interface<PPB_Flash_Clipboard_5_0>()) {
    rv = get_interface<PPB_Flash_Clipboard_5_0>()->RegisterCustomFormat(
        instance.pp_instance(), format_name.c_str());
  }
  return rv;
}

// static
bool Clipboard::IsFormatAvailable(const InstanceHandle& instance,
                                  PP_Flash_Clipboard_Type clipboard_type,
                                  uint32_t format) {
  bool rv = false;
  if (has_interface<PPB_Flash_Clipboard_5_1>()) {
    rv = PP_ToBool(get_interface<PPB_Flash_Clipboard_5_1>()->IsFormatAvailable(
        instance.pp_instance(), clipboard_type, format));
  } else if (has_interface<PPB_Flash_Clipboard_5_0>()) {
    rv = PP_ToBool(get_interface<PPB_Flash_Clipboard_5_0>()->IsFormatAvailable(
        instance.pp_instance(), clipboard_type, format));
  } else if (has_interface<PPB_Flash_Clipboard_4_0>()) {
    rv = PP_ToBool(get_interface<PPB_Flash_Clipboard_4_0>()->IsFormatAvailable(
        instance.pp_instance(), clipboard_type,
        static_cast<PP_Flash_Clipboard_Format>(format)));
  }
  return rv;
}

// static
bool Clipboard::ReadData(
    const InstanceHandle& instance,
    PP_Flash_Clipboard_Type clipboard_type,
    uint32_t format,
    Var* out) {
  bool rv = false;
  if (has_interface<PPB_Flash_Clipboard_5_1>()) {
    PP_Var result = get_interface<PPB_Flash_Clipboard_5_1>()->ReadData(
        instance.pp_instance(),
        clipboard_type,
        format);
    *out = Var(PASS_REF, result);
    rv = true;
  } else if (has_interface<PPB_Flash_Clipboard_5_0>()) {
    PP_Var result = get_interface<PPB_Flash_Clipboard_5_0>()->ReadData(
        instance.pp_instance(),
        clipboard_type,
        format);
    *out = Var(PASS_REF, result);
    rv = true;
  } else if (has_interface<PPB_Flash_Clipboard_4_0>()) {
    PP_Var result = get_interface<PPB_Flash_Clipboard_4_0>()->ReadData(
        instance.pp_instance(),
        clipboard_type,
        static_cast<PP_Flash_Clipboard_Format>(format));
    *out = Var(PASS_REF, result);
    rv = true;
  }
  return rv;
}

// static
bool Clipboard::WriteData(
    const InstanceHandle& instance,
    PP_Flash_Clipboard_Type clipboard_type,
    const std::vector<uint32_t>& formats,
    const std::vector<Var>& data_items) {
  if (formats.size() != data_items.size())
    return false;

  bool rv = false;
  if (has_interface<PPB_Flash_Clipboard_5_1>()) {
    // Convert vector of pp::Var into a vector of PP_Var.
    std::vector<PP_Var> data_items_vector;
    for (uint32_t i = 0; i < data_items.size(); ++i)
      data_items_vector.push_back(data_items[i].pp_var());

    // Ensure that we don't dereference the memory in empty vectors. We still
    // want to call WriteData because it has the effect of clearing the
    // clipboard.
    const uint32_t* formats_ptr(NULL);
    const PP_Var* data_items_ptr(NULL);
    if (data_items.size() > 0) {
      formats_ptr = &formats[0];
      data_items_ptr = &data_items_vector[0];
    }

    rv = (get_interface<PPB_Flash_Clipboard_5_1>()->WriteData(
        instance.pp_instance(),
        clipboard_type,
        static_cast<uint32_t>(data_items.size()),
        formats_ptr,
            data_items_ptr) == PP_OK);
  } else if (has_interface<PPB_Flash_Clipboard_5_0>()) {
    // Convert vector of pp::Var into a vector of PP_Var.
    std::vector<PP_Var> data_items_vector;
    for (uint32_t i = 0; i < data_items.size(); ++i)
      data_items_vector.push_back(data_items[i].pp_var());

    // Ensure that we don't dereference the memory in empty vectors. We still
    // want to call WriteData because it has the effect of clearing the
    // clipboard.
    const uint32_t* formats_ptr(NULL);
    const PP_Var* data_items_ptr(NULL);
    if (data_items.size() > 0) {
      formats_ptr = &formats[0];
      data_items_ptr = &data_items_vector[0];
    }

    rv = (get_interface<PPB_Flash_Clipboard_5_0>()->WriteData(
        instance.pp_instance(),
        clipboard_type,
        static_cast<uint32_t>(data_items.size()),
        formats_ptr,
        data_items_ptr) == PP_OK);
  } else if (has_interface<PPB_Flash_Clipboard_4_0>()) {
    // Convert vector of pp::Var into a vector of PP_Var.
    std::vector<PP_Var> data_items_vector;
    std::vector<PP_Flash_Clipboard_Format> old_formats;
    for (uint32_t i = 0; i < data_items.size(); ++i) {
      data_items_vector.push_back(data_items[i].pp_var());
      old_formats.push_back(static_cast<PP_Flash_Clipboard_Format>(formats[i]));
    }

    // Ensure that we don't dereference the memory in empty vectors. We still
    // want to call WriteData because it has the effect of clearing the
    // clipboard.
    const PP_Flash_Clipboard_Format* formats_ptr(NULL);
    const PP_Var* data_items_ptr(NULL);
    if (data_items.size() > 0) {
      formats_ptr = &old_formats[0];
      data_items_ptr = &data_items_vector[0];
    }

    rv = (get_interface<PPB_Flash_Clipboard_4_0>()->WriteData(
        instance.pp_instance(),
        clipboard_type,
        static_cast<uint32_t>(data_items.size()),
        formats_ptr,
        data_items_ptr) == PP_OK);
  }

  return rv;
}

// static
bool Clipboard::GetSequenceNumber(const InstanceHandle& instance,
                                  PP_Flash_Clipboard_Type clipboard_type,
                                  uint64_t* sequence_number) {
  if (has_interface<PPB_Flash_Clipboard_5_1>()) {
    return PP_ToBool(
        get_interface<PPB_Flash_Clipboard_5_1>()->GetSequenceNumber(
            instance.pp_instance(), clipboard_type, sequence_number));
  }
  return false;
}

}  // namespace flash
}  // namespace pp
