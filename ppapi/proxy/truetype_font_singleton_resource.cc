// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/truetype_font_singleton_resource.h"

#include <stddef.h>

#include "base/bind.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_structs.h"
#include "ppapi/shared_impl/array_writer.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/shared_impl/var_tracker.h"

namespace ppapi {
namespace proxy {

TrueTypeFontSingletonResource::TrueTypeFontSingletonResource(
    Connection connection,
    PP_Instance instance)
    : PluginResource(connection, instance) {
  SendCreate(BROWSER, PpapiHostMsg_TrueTypeFontSingleton_Create());
}

TrueTypeFontSingletonResource::~TrueTypeFontSingletonResource() {
}

thunk::PPB_TrueTypeFont_Singleton_API*
TrueTypeFontSingletonResource::AsPPB_TrueTypeFont_Singleton_API() {
  return this;
}

int32_t TrueTypeFontSingletonResource::GetFontFamilies(
    PP_Instance instance,
    const PP_ArrayOutput& output,
    const scoped_refptr<TrackedCallback>& callback) {
  Call<PpapiPluginMsg_TrueTypeFontSingleton_GetFontFamiliesReply>(BROWSER,
      PpapiHostMsg_TrueTypeFontSingleton_GetFontFamilies(),
      base::Bind(
          &TrueTypeFontSingletonResource::OnPluginMsgGetFontFamiliesComplete,
          this, callback, output));
  return PP_OK_COMPLETIONPENDING;
}

int32_t TrueTypeFontSingletonResource::GetFontsInFamily(
      PP_Instance instance,
      PP_Var family,
      const PP_ArrayOutput& output,
      const scoped_refptr<TrackedCallback>& callback) {
  scoped_refptr<StringVar> family_var = StringVar::FromPPVar(family);
  const uint32_t kMaxFamilySizeInBytes = 1024;
  if (!family_var.get() || family_var->value().size() > kMaxFamilySizeInBytes)
    return PP_ERROR_BADARGUMENT;
  Call<PpapiPluginMsg_TrueTypeFontSingleton_GetFontsInFamilyReply>(BROWSER,
      PpapiHostMsg_TrueTypeFontSingleton_GetFontsInFamily(family_var->value()),
      base::Bind(
          &TrueTypeFontSingletonResource::OnPluginMsgGetFontsInFamilyComplete,
          this, callback, output));
  return PP_OK_COMPLETIONPENDING;
}

void TrueTypeFontSingletonResource::OnPluginMsgGetFontFamiliesComplete(
    scoped_refptr<TrackedCallback> callback,
    PP_ArrayOutput array_output,
    const ResourceMessageReplyParams& params,
    const std::vector<std::string>& font_families) {
  if (!TrackedCallback::IsPending(callback))
    return;
  // The result code should contain the data size if it's positive.
  int32_t result = params.result();
  DCHECK((result < 0 && font_families.size() == 0) ||
         result == static_cast<int32_t>(font_families.size()));

  ArrayWriter output;
  output.set_pp_array_output(array_output);
  if (output.is_valid()) {
    std::vector< scoped_refptr<Var> > font_family_vars;
    for (size_t i = 0; i < font_families.size(); i++)
      font_family_vars.push_back(
          scoped_refptr<Var>(new StringVar(font_families[i])));
    output.StoreVarVector(font_family_vars);
  } else {
    result = PP_ERROR_FAILED;
  }

  callback->Run(result);
}

void TrueTypeFontSingletonResource::OnPluginMsgGetFontsInFamilyComplete(
    scoped_refptr<TrackedCallback> callback,
    PP_ArrayOutput array_output,
    const ResourceMessageReplyParams& params,
    const std::vector<SerializedTrueTypeFontDesc>& fonts) {
  if (!TrackedCallback::IsPending(callback))
    return;
  // The result code should contain the data size if it's positive.
  int32_t result = params.result();
  DCHECK((result < 0 && fonts.size() == 0) ||
         result == static_cast<int32_t>(fonts.size()));
  ArrayWriter output;
  output.set_pp_array_output(array_output);
  if (output.is_valid()) {
    // Convert the message data to an array of PP_TrueTypeFontDesc_Dev structs.
    // Each desc has an embedded PP_Var containing the family name.
    std::vector<PP_TrueTypeFontDesc_Dev> pp_fonts(fonts.size());
    for (size_t i = 0; i < fonts.size(); i++)
      fonts[i].CopyToPPTrueTypeFontDesc(&pp_fonts[i]);

    if (!output.StoreVector(pp_fonts)) {
      for (size_t i = 0; i < pp_fonts.size(); i++)
        PpapiGlobals::Get()->GetVarTracker()->ReleaseVar(pp_fonts[i].family);
    }
  } else {
    result = PP_ERROR_FAILED;
  }

  callback->Run(result);
}

}  // namespace proxy
}  // namespace ppapi
