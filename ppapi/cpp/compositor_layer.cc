// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/compositor_layer.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_CompositorLayer_0_1>() {
  return PPB_COMPOSITORLAYER_INTERFACE_0_1;
}

template <> const char* interface_name<PPB_CompositorLayer_0_2>() {
  return PPB_COMPOSITORLAYER_INTERFACE_0_2;
}

}  // namespace

CompositorLayer::CompositorLayer() {
}

CompositorLayer::CompositorLayer(
    const CompositorLayer& other) : Resource(other) {
}

CompositorLayer::CompositorLayer(const Resource& resource)
    : Resource(resource) {
  PP_DCHECK(IsCompositorLayer(resource));
}

CompositorLayer::CompositorLayer(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

CompositorLayer::~CompositorLayer() {
}

int32_t CompositorLayer::SetColor(float red,
                                  float green,
                                  float blue,
                                  float alpha,
                                  const Size& size) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetColor(
        pp_resource(), red, green, blue, alpha, &size.pp_size());
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetColor(
        pp_resource(), red, green, blue, alpha, &size.pp_size());
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t CompositorLayer::SetTexture(const Graphics3D& context,
                                    uint32_t target,
                                    uint32_t texture,
                                    const Size& size,
                                    const CompletionCallback& cc) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetTexture(
        pp_resource(), context.pp_resource(), target, texture, &size.pp_size(),
        cc.pp_completion_callback());
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    if (target != 0x0DE1) // 0x0DE1 GL_TEXTURE_2D
      return cc.MayForce(PP_ERROR_NOTSUPPORTED);
    return get_interface<PPB_CompositorLayer_0_1>()->SetTexture(
        pp_resource(), context.pp_resource(), texture, &size.pp_size(),
        cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t CompositorLayer::SetImage(const ImageData& image,
                                  const CompletionCallback& cc) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetImage(
        pp_resource(), image.pp_resource(), NULL,
        cc.pp_completion_callback());
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetImage(
        pp_resource(), image.pp_resource(), NULL,
        cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t CompositorLayer::SetImage(const ImageData& image,
                                  const Size& size,
                                  const CompletionCallback& cc) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetImage(
        pp_resource(), image.pp_resource(), &size.pp_size(),
        cc.pp_completion_callback());
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetImage(
        pp_resource(), image.pp_resource(), &size.pp_size(),
        cc.pp_completion_callback());
  }
  return cc.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t CompositorLayer::SetClipRect(const Rect& rect) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetClipRect(
        pp_resource(), &rect.pp_rect());
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetClipRect(
        pp_resource(), &rect.pp_rect());
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t CompositorLayer::SetTransform(const float matrix[16]) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetTransform(
        pp_resource(), matrix);
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetTransform(
        pp_resource(), matrix);
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t CompositorLayer::SetOpacity(float opacity) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetOpacity(
        pp_resource(), opacity);
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetOpacity(
        pp_resource(), opacity);
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t CompositorLayer::SetBlendMode(PP_BlendMode mode) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetBlendMode(
        pp_resource(), mode);
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetBlendMode(
        pp_resource(), mode);
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t CompositorLayer::SetSourceRect(const FloatRect& rect) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetSourceRect(
        pp_resource(), &rect.pp_float_rect());
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetSourceRect(
        pp_resource(), &rect.pp_float_rect());
  }
  return PP_ERROR_NOINTERFACE;
}

int32_t CompositorLayer::SetPremultipliedAlpha(bool premult) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return get_interface<PPB_CompositorLayer_0_2>()->SetPremultipliedAlpha(
        pp_resource(), PP_FromBool(premult));
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return get_interface<PPB_CompositorLayer_0_1>()->SetPremultipliedAlpha(
        pp_resource(), PP_FromBool(premult));
  }
  return PP_ERROR_NOINTERFACE;
}

bool CompositorLayer::IsCompositorLayer(const Resource& resource) {
  if (has_interface<PPB_CompositorLayer_0_2>()) {
    return PP_ToBool(get_interface<PPB_CompositorLayer_0_2>()->
        IsCompositorLayer(resource.pp_resource()));
  }
  if (has_interface<PPB_CompositorLayer_0_1>()) {
    return PP_ToBool(get_interface<PPB_CompositorLayer_0_1>()->
        IsCompositorLayer(resource.pp_resource()));
  }
  return false;
}

}  // namespace pp
