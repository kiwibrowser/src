// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/find_private.h"

#include "ppapi/c/private/ppb_find_private.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/rect.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Find_Private>() {
  return PPB_FIND_PRIVATE_INTERFACE;
}

static const char kPPPFindInterface[] = PPP_FIND_PRIVATE_INTERFACE;

PP_Bool StartFind(PP_Instance instance,
                  const char* text,
                  PP_Bool case_sensitive) {
  void* object = Instance::GetPerInstanceObject(instance, kPPPFindInterface);
  if (!object)
    return PP_FALSE;
  if (!text || text[0] == '\0') {
    PP_NOTREACHED();
    return PP_FALSE;
  }
  bool return_value = static_cast<Find_Private*>(object)->StartFind(
      text, PP_ToBool(case_sensitive));
  return PP_FromBool(return_value);
}

void SelectFindResult(PP_Instance instance, PP_Bool forward) {
  void* object = Instance::GetPerInstanceObject(instance, kPPPFindInterface);
  if (object)
    static_cast<Find_Private*>(object)->SelectFindResult(PP_ToBool(forward));
}

void StopFind(PP_Instance instance) {
  void* object = Instance::GetPerInstanceObject(instance, kPPPFindInterface);
  if (object)
    static_cast<Find_Private*>(object)->StopFind();
}

const PPP_Find_Private ppp_find = {
  &StartFind,
  &SelectFindResult,
  &StopFind
};

}  // namespace

Find_Private::Find_Private(Instance* instance)
      : associated_instance_(instance) {
  Module::Get()->AddPluginInterface(kPPPFindInterface, &ppp_find);
  instance->AddPerInstanceObject(kPPPFindInterface, this);
}

Find_Private::~Find_Private() {
  Instance::RemovePerInstanceObject(associated_instance_,
                                    kPPPFindInterface, this);
}

void Find_Private::SetPluginToHandleFindRequests() {
  if (has_interface<PPB_Find_Private>()) {
    get_interface<PPB_Find_Private>()->SetPluginToHandleFindRequests(
        associated_instance_.pp_instance());
  }
}

void Find_Private::NumberOfFindResultsChanged(int32_t total,
                                              bool final_result) {
  if (has_interface<PPB_Find_Private>()) {
    get_interface<PPB_Find_Private>()->NumberOfFindResultsChanged(
        associated_instance_.pp_instance(), total, PP_FromBool(final_result));
  }
}

void Find_Private::SelectedFindResultChanged(int32_t index) {
  if (has_interface<PPB_Find_Private>()) {
    get_interface<PPB_Find_Private>()->SelectedFindResultChanged(
        associated_instance_.pp_instance(), index);
  }
}

void Find_Private::SetTickmarks(const std::vector<pp::Rect>& tickmarks) {
  if (has_interface<PPB_Find_Private>()) {
    std::vector<PP_Rect> tickmarks_converted(tickmarks.begin(),
                                             tickmarks.end());
    PP_Rect* array =
        tickmarks_converted.empty() ? NULL : &tickmarks_converted[0];
    get_interface<PPB_Find_Private>()->SetTickmarks(
        associated_instance_.pp_instance(), array,
        static_cast<uint32_t>(tickmarks.size()));
  }
}

}  // namespace pp
