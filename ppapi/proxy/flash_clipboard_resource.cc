// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/flash_clipboard_resource.h"

#include <stddef.h>

#include "base/numerics/safe_conversions.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/shared_impl/var_tracker.h"

namespace ppapi {
namespace proxy {

namespace {

// Returns whether the given clipboard type is valid.
bool IsValidClipboardType(PP_Flash_Clipboard_Type type) {
  return type == PP_FLASH_CLIPBOARD_TYPE_STANDARD ||
         type == PP_FLASH_CLIPBOARD_TYPE_SELECTION;
}

// Convert a PP_Var to/from a string which is transmitted to the pepper host.
// These functions assume the format is valid.
bool PPVarToClipboardString(int32_t format,
                            const PP_Var& var,
                            std::string* string_out) {
  if (format == PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT ||
      format == PP_FLASH_CLIPBOARD_FORMAT_HTML) {
    StringVar* string_var = StringVar::FromPPVar(var);
    if (!string_var)
      return false;
    *string_out = string_var->value();
    return true;
  } else {
    // All other formats are expected to be array buffers.
    ArrayBufferVar* array_buffer_var = ArrayBufferVar::FromPPVar(var);
    if (!array_buffer_var)
      return false;
    *string_out = std::string(static_cast<const char*>(array_buffer_var->Map()),
                              array_buffer_var->ByteLength());
    return true;
  }
}

PP_Var ClipboardStringToPPVar(int32_t format,
                              const std::string& string) {
  if (format == PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT ||
      format == PP_FLASH_CLIPBOARD_FORMAT_HTML) {
    return StringVar::StringToPPVar(string);
  } else {
    // All other formats are expected to be array buffers.
    return PpapiGlobals::Get()->GetVarTracker()->MakeArrayBufferPPVar(
        base::checked_cast<uint32_t>(string.size()), string.data());
  }
}
}  // namespace

FlashClipboardResource::FlashClipboardResource(
    Connection connection, PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_FlashClipboard_Create());
}

FlashClipboardResource::~FlashClipboardResource() {
}

thunk::PPB_Flash_Clipboard_API*
FlashClipboardResource::AsPPB_Flash_Clipboard_API() {
  return this;
}

uint32_t FlashClipboardResource::RegisterCustomFormat(
    PP_Instance instance,
    const char* format_name) {
  // Check to see if the format has already been registered.
  uint32_t format = clipboard_formats_.GetFormatID(format_name);
  if (format != PP_FLASH_CLIPBOARD_FORMAT_INVALID)
    return format;
  int32_t result =
      SyncCall<PpapiPluginMsg_FlashClipboard_RegisterCustomFormatReply>(
          BROWSER,
          PpapiHostMsg_FlashClipboard_RegisterCustomFormat(format_name),
          &format);
  if (result != PP_OK || format == PP_FLASH_CLIPBOARD_FORMAT_INVALID)
    return PP_FLASH_CLIPBOARD_FORMAT_INVALID;
  clipboard_formats_.SetRegisteredFormat(format_name, format);
  return format;
}

PP_Bool FlashClipboardResource::IsFormatAvailable(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    uint32_t format) {
  if (IsValidClipboardType(clipboard_type) &&
      (FlashClipboardFormatRegistry::IsValidPredefinedFormat(format) ||
       clipboard_formats_.IsFormatRegistered(format))) {
    int32_t result = SyncCall<IPC::Message>(BROWSER,
        PpapiHostMsg_FlashClipboard_IsFormatAvailable(clipboard_type, format));
    return result == PP_OK ? PP_TRUE : PP_FALSE;
  }
  return PP_FALSE;
}

PP_Var FlashClipboardResource::ReadData(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    uint32_t format) {
  std::string value;
  int32_t result =
      SyncCall<PpapiPluginMsg_FlashClipboard_ReadDataReply>(
          BROWSER,
          PpapiHostMsg_FlashClipboard_ReadData(clipboard_type, format),
          &value);
  if (result != PP_OK)
    return PP_MakeUndefined();

  return ClipboardStringToPPVar(format, value);
}

int32_t FlashClipboardResource::WriteData(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    uint32_t data_item_count,
    const uint32_t formats[],
    const PP_Var data_items[]) {
  if (!IsValidClipboardType(clipboard_type))
      return PP_ERROR_BADARGUMENT;
  std::vector<uint32_t> formats_vector;
  std::vector<std::string> data_items_vector;
  for (size_t i = 0; i < data_item_count; ++i) {
    if (!clipboard_formats_.IsFormatRegistered(formats[i]) &&
        !FlashClipboardFormatRegistry::IsValidPredefinedFormat(formats[i])) {
      return PP_ERROR_BADARGUMENT;
    }
    formats_vector.push_back(formats[i]);
    std::string string;
    if (!PPVarToClipboardString(formats[i], data_items[i], &string))
      return PP_ERROR_BADARGUMENT;
    data_items_vector.push_back(string);
  }

  Post(BROWSER,
       PpapiHostMsg_FlashClipboard_WriteData(
           static_cast<uint32_t>(clipboard_type),
           formats_vector,
           data_items_vector));

  // Assume success, since it allows us to avoid a sync IPC.
  return PP_OK;
}

PP_Bool FlashClipboardResource::GetSequenceNumber(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    uint64_t* sequence_number) {
  int32_t result =
      SyncCall<PpapiPluginMsg_FlashClipboard_GetSequenceNumberReply>(
          BROWSER,
          PpapiHostMsg_FlashClipboard_GetSequenceNumber(clipboard_type),
          sequence_number);
  return PP_FromBool(result == PP_OK);
}

}  // namespace proxy
}  // namespace ppapi
