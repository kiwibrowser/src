// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/compositor.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_Compositor_0_1>() {
  return PPB_COMPOSITOR_INTERFACE_0_1;
}

}  // namespace

Compositor::Compositor() {
}

Compositor::Compositor(const InstanceHandle& instance) {
  if (has_interface<PPB_Compositor_0_1>()) {
    PassRefFromConstructor(get_interface<PPB_Compositor_0_1>()->Create(
        instance.pp_instance()));
  }
}

Compositor::Compositor(
    const Compositor& other) : Resource(other) {
}

Compositor::Compositor(const Resource& resource)
    : Resource(resource) {
  PP_DCHECK(IsCompositor(resource));
}

Compositor::Compositor(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

Compositor::~Compositor() {
}

CompositorLayer Compositor::AddLayer() {
  PP_Resource layer = 0;
  if (has_interface<PPB_Compositor_0_1>()) {
    layer = get_interface<PPB_Compositor_0_1>()->AddLayer(pp_resource());
  }
  return CompositorLayer(PASS_REF, layer);
}

int32_t Compositor::CommitLayers(const CompletionCallback& cc) {
  if (has_interface<PPB_Compositor_0_1>()) {
    return get_interface<PPB_Compositor_0_1>()->CommitLayers(
        pp_resource(), cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t Compositor::ResetLayers() {
  if (has_interface<PPB_Compositor_0_1>()) {
    return get_interface<PPB_Compositor_0_1>()->ResetLayers(pp_resource());
  }
  return PP_ERROR_NOINTERFACE;
}

bool Compositor::IsCompositor(const Resource& resource) {
  if (has_interface<PPB_Compositor_0_1>()) {
    return PP_ToBool(get_interface<PPB_Compositor_0_1>()->
        IsCompositor(resource.pp_resource()));
  }
  return false;
}

}  // namespace pp
