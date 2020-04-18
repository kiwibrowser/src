// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/truetype_font_resource.h"

#include "base/bind.h"
#include "ipc/ipc_message.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/array_writer.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"

using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_TrueTypeFont_API;

namespace {

}  // namespace

namespace ppapi {
namespace proxy {

TrueTypeFontResource::TrueTypeFontResource(Connection connection,
                                           PP_Instance instance,
                                           const PP_TrueTypeFontDesc_Dev& desc)
    : PluginResource(connection, instance),
      create_result_(PP_OK_COMPLETIONPENDING),
      describe_desc_(NULL) {
  SerializedTrueTypeFontDesc serialized_desc;
  serialized_desc.SetFromPPTrueTypeFontDesc(desc);
  SendCreate(BROWSER, PpapiHostMsg_TrueTypeFont_Create(serialized_desc));
}

TrueTypeFontResource::~TrueTypeFontResource() {
}

PPB_TrueTypeFont_API* TrueTypeFontResource::AsPPB_TrueTypeFont_API() {
  return this;
}

int32_t TrueTypeFontResource::Describe(
    PP_TrueTypeFontDesc_Dev* desc,
    scoped_refptr<TrackedCallback> callback) {
  if (describe_callback_.get())
    return PP_ERROR_INPROGRESS;

  if (create_result_ == PP_OK) {
    desc_.CopyToPPTrueTypeFontDesc(desc);
  } else if (create_result_ == PP_OK_COMPLETIONPENDING) {
    describe_desc_ = desc;
    describe_callback_ = callback;
  }

  return create_result_;
}

int32_t TrueTypeFontResource::GetTableTags(
    const PP_ArrayOutput& output,
    scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_TrueTypeFont_GetTableTagsReply>(
      BROWSER,
      PpapiHostMsg_TrueTypeFont_GetTableTags(),
      base::Bind(&TrueTypeFontResource::OnPluginMsgGetTableTagsComplete,
                 this,
                 callback,
                 output));

  return PP_OK_COMPLETIONPENDING;
}

int32_t TrueTypeFontResource::GetTable(
    uint32_t table,
    int32_t offset,
    int32_t max_data_length,
    const PP_ArrayOutput& output,
    scoped_refptr<TrackedCallback> callback) {
  Call<PpapiPluginMsg_TrueTypeFont_GetTableReply>(
      BROWSER,
      PpapiHostMsg_TrueTypeFont_GetTable(table, offset, max_data_length),
      base::Bind(&TrueTypeFontResource::OnPluginMsgGetTableComplete,
                 this,
                 callback,
                 output));

  return PP_OK_COMPLETIONPENDING;
}

void TrueTypeFontResource::OnReplyReceived(
    const ResourceMessageReplyParams& params,
    const IPC::Message& msg) {
  PPAPI_BEGIN_MESSAGE_MAP(TrueTypeFontResource, msg)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL(PpapiPluginMsg_TrueTypeFont_CreateReply,
                                      OnPluginMsgCreateComplete)
  PPAPI_DISPATCH_PLUGIN_RESOURCE_CALL_UNHANDLED(
      PluginResource::OnReplyReceived(params, msg))
  PPAPI_END_MESSAGE_MAP()
}

void TrueTypeFontResource::OnPluginMsgCreateComplete(
    const ResourceMessageReplyParams& params,
    const ppapi::proxy::SerializedTrueTypeFontDesc& desc,
    int32_t result) {
  DCHECK(result != PP_OK_COMPLETIONPENDING);
  DCHECK(create_result_ == PP_OK_COMPLETIONPENDING);
  create_result_ = result;
  if (create_result_ == PP_OK)
    desc_ = desc;

  // Now complete any pending Describe operation.
  if (TrackedCallback::IsPending(describe_callback_)) {
    desc_.CopyToPPTrueTypeFontDesc(describe_desc_);
    describe_desc_ = NULL;
    scoped_refptr<TrackedCallback> callback;
    callback.swap(describe_callback_);
    callback->Run(create_result_ == PP_OK ? PP_OK : PP_ERROR_FAILED);
  }
}

void TrueTypeFontResource::OnPluginMsgGetTableTagsComplete(
    scoped_refptr<TrackedCallback> callback,
    PP_ArrayOutput array_output,
    const ResourceMessageReplyParams& params,
    const std::vector<uint32_t>& tag_array) {
  // The result code should contain the data size if it's positive.
  int32_t result = params.result();
  DCHECK((result < 0 && tag_array.size() == 0) ||
         result == static_cast<int32_t>(tag_array.size()));

  ArrayWriter output;
  output.set_pp_array_output(array_output);
  if (output.is_valid())
    output.StoreArray(&tag_array[0], std::max(0, result));
  else
    result = PP_ERROR_FAILED;

  callback->Run(result);
}

void TrueTypeFontResource::OnPluginMsgGetTableComplete(
    scoped_refptr<TrackedCallback> callback,
    PP_ArrayOutput array_output,
    const ResourceMessageReplyParams& params,
    const std::string& data) {
  // The result code should contain the data size if it's positive.
  int32_t result = params.result();
  DCHECK((result < 0 && data.size() == 0) ||
         result == static_cast<int32_t>(data.size()));

  ArrayWriter output;
  output.set_pp_array_output(array_output);
  if (output.is_valid())
    output.StoreArray(data.data(), std::max(0, result));
  else
    result = PP_ERROR_FAILED;

  callback->Run(result);
}

}  // namespace proxy
}  // namespace ppapi
