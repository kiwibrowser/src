// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/dev/truetype_font_dev.h"

#include <string.h>  // memcpy

#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_TrueTypeFont_Dev_0_1>() {
  return PPB_TRUETYPEFONT_DEV_INTERFACE_0_1;
}

}  // namespace

// TrueTypeFontDesc_Dev --------------------------------------------------------

TrueTypeFontDesc_Dev::TrueTypeFontDesc_Dev() {
  desc_.family = family_.pp_var();
  set_generic_family(PP_TRUETYPEFONTFAMILY_SERIF);
  set_style(PP_TRUETYPEFONTSTYLE_NORMAL);
  set_weight(PP_TRUETYPEFONTWEIGHT_NORMAL);
  set_width(PP_TRUETYPEFONTWIDTH_NORMAL);
  set_charset(PP_TRUETYPEFONTCHARSET_DEFAULT);
}

TrueTypeFontDesc_Dev::TrueTypeFontDesc_Dev(
    PassRef,
    const PP_TrueTypeFontDesc_Dev& pp_desc) {
  desc_ = pp_desc;
  set_family(Var(PASS_REF, pp_desc.family));
}

TrueTypeFontDesc_Dev::TrueTypeFontDesc_Dev(const TrueTypeFontDesc_Dev& other) {
  set_family(other.family());
  set_generic_family(other.generic_family());
  set_style(other.style());
  set_weight(other.weight());
  set_width(other.width());
  set_charset(other.charset());
}

TrueTypeFontDesc_Dev::~TrueTypeFontDesc_Dev() {
}

TrueTypeFontDesc_Dev& TrueTypeFontDesc_Dev::operator=(
    const TrueTypeFontDesc_Dev& other) {
  if (this == &other)
    return *this;

  set_family(other.family());
  set_generic_family(other.generic_family());
  set_style(other.style());
  set_weight(other.weight());
  set_width(other.width());
  set_charset(other.charset());

  return *this;
}

// TrueTypeFont_Dev ------------------------------------------------------------

TrueTypeFont_Dev::TrueTypeFont_Dev() {
}

TrueTypeFont_Dev::TrueTypeFont_Dev(const InstanceHandle& instance,
                                   const TrueTypeFontDesc_Dev& desc) {
  if (!has_interface<PPB_TrueTypeFont_Dev_0_1>())
    return;
  PassRefFromConstructor(get_interface<PPB_TrueTypeFont_Dev_0_1>()->Create(
      instance.pp_instance(), &desc.pp_desc()));
}

TrueTypeFont_Dev::TrueTypeFont_Dev(const TrueTypeFont_Dev& other)
    : Resource(other) {
}

TrueTypeFont_Dev::TrueTypeFont_Dev(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

// static
int32_t TrueTypeFont_Dev::GetFontFamilies(
    const InstanceHandle& instance,
    const CompletionCallbackWithOutput<std::vector<Var> >& cc) {
  if (has_interface<PPB_TrueTypeFont_Dev_0_1>()) {
    return get_interface<PPB_TrueTypeFont_Dev_0_1>()->GetFontFamilies(
        instance.pp_instance(),
        cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

// static
int32_t TrueTypeFont_Dev::GetFontsInFamily(
    const InstanceHandle& instance,
    const Var& family,
    const CompletionCallbackWithOutput<std::vector<TrueTypeFontDesc_Dev> >& cc)
        {
  if (has_interface<PPB_TrueTypeFont_Dev_0_1>()) {
    return get_interface<PPB_TrueTypeFont_Dev_0_1>()->GetFontsInFamily(
        instance.pp_instance(),
        family.pp_var(),
        cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t TrueTypeFont_Dev::Describe(
    const CompletionCallbackWithOutput<TrueTypeFontDesc_Dev>& cc) {
  if (has_interface<PPB_TrueTypeFont_Dev_0_1>()) {
    int32_t result =
        get_interface<PPB_TrueTypeFont_Dev_0_1>()->Describe(
            pp_resource(), cc.output(), cc.pp_completion_callback());
    return result;
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t TrueTypeFont_Dev::GetTableTags(
    const CompletionCallbackWithOutput<std::vector<uint32_t> >& cc) {
  if (has_interface<PPB_TrueTypeFont_Dev_0_1>()) {
    return get_interface<PPB_TrueTypeFont_Dev_0_1>()->GetTableTags(
        pp_resource(),
        cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t TrueTypeFont_Dev::GetTable(
    uint32_t table,
    int32_t offset,
    int32_t max_data_length,
    const CompletionCallbackWithOutput<std::vector<char> >& cc) {
  if (has_interface<PPB_TrueTypeFont_Dev_0_1>()) {
    return get_interface<PPB_TrueTypeFont_Dev_0_1>()->GetTable(
        pp_resource(),
        table, offset, max_data_length,
        cc.output(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace pp
