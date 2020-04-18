// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Needed on Windows to get |M_PI| from math.h.
#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <math.h>

#include <vector>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_input_event.h"
#include "ppapi/cpp/compositor.h"
#include "ppapi/cpp/compositor_layer.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/graphics_3d_client.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/input_event.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/examples/compositor/spinning_cube.h"
#include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"
#include "ppapi/lib/gl/include/GLES2/gl2.h"
#include "ppapi/lib/gl/include/GLES2/gl2ext.h"
#include "ppapi/utility/completion_callback_factory.h"

// Use assert as a makeshift CHECK, even in non-debug mode.
// Since <assert.h> redefines assert on every inclusion (it doesn't use
// include-guards), make sure this is the last file #include'd in this file.
#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// When compiling natively on Windows, PostMessage can be #define-d to
// something else.
#ifdef PostMessage
#undef PostMessage
#endif

// Assert |context_| isn't holding any GL Errors.  Done as a macro instead of a
// function to preserve line number information in the failure message.
#define AssertNoGLError() \
  PP_DCHECK(!glGetError());

namespace {

const int32_t kTextureWidth = 800;
const int32_t kTextureHeight = 800;
const int32_t kImageWidth = 256;
const int32_t kImageHeight = 256;

class DemoInstance : public pp::Instance, public pp::Graphics3DClient {
 public:
  DemoInstance(PP_Instance instance);
  virtual ~DemoInstance();

  // pp::Instance implementation (see PPP_Instance).
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void DidChangeView(const pp::Rect& position,
                             const pp::Rect& clip);
  virtual bool HandleInputEvent(const pp::InputEvent& event);

  // pp::Graphics3DClient implementation.
  virtual void Graphics3DContextLost();

 private:
  // GL-related functions.
  void InitGL(int32_t result);
  GLuint PrepareFramebuffer();
  pp::ImageData PrepareImage();
  void PrepareLayers(int32_t frame);
  void Paint(int32_t result, int32_t frame);
  void OnTextureReleased(int32_t result, GLuint texture);
  void OnImageReleased(int32_t result, const pp::ImageData& image);

  pp::CompletionCallbackFactory<DemoInstance> callback_factory_;

  // Owned data.
  pp::Graphics3D* context_;

  GLuint fbo_;
  GLuint rbo_;

  std::vector<GLuint> textures_;
  std::vector<pp::ImageData> images_;

  pp::Compositor compositor_;
  pp::CompositorLayer color_layer_;
  pp::CompositorLayer stable_texture_layer_;
  pp::CompositorLayer texture_layer_;
  pp::CompositorLayer image_layer_;

  bool rebuild_layers_;
  int32_t total_resource_;

  SpinningCube* cube_;
};

DemoInstance::DemoInstance(PP_Instance instance)
    : pp::Instance(instance),
      pp::Graphics3DClient(this),
      callback_factory_(this),
      context_(NULL),
      fbo_(0),
      rbo_(0),
      rebuild_layers_(true),
      total_resource_(0),
      cube_(new SpinningCube()) {
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
}

DemoInstance::~DemoInstance() {
  delete cube_;
  assert(glTerminatePPAPI());
  delete context_;
}

bool DemoInstance::Init(uint32_t /*argc*/,
                        const char* /*argn*/[],
                        const char* /*argv*/[]) {
  return !!glInitializePPAPI(pp::Module::Get()->get_browser_interface());
}

void DemoInstance::DidChangeView(
    const pp::Rect& position, const pp::Rect& /*clip*/) {
  if (position.width() == 0 || position.height() == 0)
    return;
  // Initialize graphics.
  InitGL(0);
}

bool DemoInstance::HandleInputEvent(const pp::InputEvent& event) {
  switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
      rebuild_layers_ = true;
      return true;
    default:
      break;
  }
  return false;
}

void DemoInstance::Graphics3DContextLost() {
  fbo_ = 0;
  rbo_ = 0;
  rebuild_layers_ = true;
  total_resource_ -= static_cast<int32_t>(textures_.size());
  textures_.clear();
  delete context_;
  context_ = NULL;
  cube_->OnGLContextLost();
  pp::CompletionCallback cb = callback_factory_.NewCallback(
      &DemoInstance::InitGL);
  pp::Module::Get()->core()->CallOnMainThread(0, cb, 0);
}

void DemoInstance::InitGL(int32_t /*result*/) {
  if (context_)
    return;
  int32_t context_attributes[] = {
    PP_GRAPHICS3DATTRIB_ALPHA_SIZE, 8,
    PP_GRAPHICS3DATTRIB_BLUE_SIZE, 8,
    PP_GRAPHICS3DATTRIB_GREEN_SIZE, 8,
    PP_GRAPHICS3DATTRIB_RED_SIZE, 8,
    PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 0,
    PP_GRAPHICS3DATTRIB_STENCIL_SIZE, 0,
    PP_GRAPHICS3DATTRIB_SAMPLES, 0,
    PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS, 0,
    PP_GRAPHICS3DATTRIB_WIDTH, 32,
    PP_GRAPHICS3DATTRIB_HEIGHT, 32,
    PP_GRAPHICS3DATTRIB_NONE,
  };
  context_ = new pp::Graphics3D(this, context_attributes);
  assert(!context_->is_null());
  assert(BindGraphics(compositor_));

  glSetCurrentContextPPAPI(context_->pp_resource());

  cube_->Init(kTextureWidth, kTextureHeight);

  Paint(PP_OK, 0);
}

GLuint DemoInstance::PrepareFramebuffer() {
  GLuint texture = 0;
  if (textures_.empty()) {
    total_resource_++;
    // Create a texture object
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kTextureWidth, kTextureHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
  } else {
    texture = textures_.back();
    textures_.pop_back();
  }

  if (!rbo_) {
    // create a renderbuffer object to store depth info
    glGenRenderbuffers(1, &rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
        kTextureWidth, kTextureHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }

  if (!fbo_) {
    // create a framebuffer object
    glGenFramebuffers(1, &fbo_);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  // attach the texture to FBO color attachment point
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         texture,
                         0);

  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                            GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER,
                            rbo_);

  // check FBO status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  assert(status == GL_FRAMEBUFFER_COMPLETE);

  AssertNoGLError();
  return texture;
}

pp::ImageData DemoInstance::PrepareImage() {
  if (images_.empty()) {
    total_resource_++;
    return pp::ImageData(this,
                         PP_IMAGEDATAFORMAT_RGBA_PREMUL,
                         pp::Size(kImageWidth, kImageHeight),
                         false);
  }
  pp::ImageData image = images_.back();
  images_.pop_back();
  return image;
}

void DemoInstance::Paint(int32_t result, int32_t frame) {
  assert(result == PP_OK);
  if (result != PP_OK || !context_)
    return;

  if (rebuild_layers_) {
    compositor_ = pp::Compositor(this);
    assert(BindGraphics(compositor_));
    color_layer_ = pp::CompositorLayer();
    stable_texture_layer_ = pp::CompositorLayer();
    texture_layer_ = pp::CompositorLayer();
    image_layer_ = pp::CompositorLayer();
    frame = 0;
    rebuild_layers_ = false;
  }

  PrepareLayers(frame);

  int32_t rv = compositor_.CommitLayers(
      callback_factory_.NewCallback(&DemoInstance::Paint, ++frame));
  assert(rv == PP_OK_COMPLETIONPENDING);

  pp::VarDictionary dict;
  dict.Set("total_resource", total_resource_);
  size_t free_resource = textures_.size() + images_.size();
  dict.Set("free_resource", static_cast<int32_t>(free_resource));
  PostMessage(dict);
}

void DemoInstance::PrepareLayers(int32_t frame) {
  int32_t rv;
  float factor_sin = sin(static_cast<float>(M_PI) / 180 * frame);
  float factor_cos = cos(static_cast<float>(M_PI) / 180 * frame);
  {
    // Set the background color layer.
    if (color_layer_.is_null()) {
      color_layer_ = compositor_.AddLayer();
      assert(!color_layer_.is_null());
      static const float transform[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
      };
      rv = color_layer_.SetTransform(transform);
      assert(rv == PP_OK);
    }
    rv = color_layer_.SetColor(fabs(factor_sin),
                               fabs(factor_cos),
                               fabs(factor_sin * factor_cos),
                               1.0f,
                               pp::Size(800, 600));
    assert(rv == PP_OK);
  }

  {
    // Set the image layer
    if (image_layer_.is_null()) {
      image_layer_ = compositor_.AddLayer();
      assert(!image_layer_.is_null());
    }
    float x = static_cast<float>(frame % 800);
    float y = 200 - 200 * factor_sin;
    const float transform[16] = {
      fabsf(factor_sin) + 0.2f, 0.0f, 0.0f, 0.0f,
      0.0f, fabsf(factor_sin) + 0.2f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
         x,    y, 0.0f, 1.0f,
    };
    rv = image_layer_.SetTransform(transform);
    assert(rv == PP_OK);

    pp::ImageData image = PrepareImage();
    uint8_t *p = static_cast<uint8_t*>(image.data());
    for (int x = 0; x < kImageWidth; ++x) {
      for (int y = 0; y < kImageHeight; ++y) {
        *(p++) = static_cast<uint8_t>(frame);
        *(p++) = static_cast<uint8_t>(frame * x);
        *(p++) = static_cast<uint8_t>(frame * y);
        *(p++) = 255;
      }
    }
    rv = image_layer_.SetImage(image, pp::Size(kImageWidth, kImageHeight),
        callback_factory_.NewCallback(&DemoInstance::OnImageReleased, image));
    assert(rv == PP_OK_COMPLETIONPENDING);
  }

  {
    // Set the stable texture layer
    if (stable_texture_layer_.is_null()) {
      stable_texture_layer_ = compositor_.AddLayer();
      assert(!stable_texture_layer_.is_null());
      GLuint texture = PrepareFramebuffer();
      cube_->UpdateForTimeDelta(0.02f);
      cube_->Draw();
      rv = stable_texture_layer_.SetTexture(
          *context_,
          GL_TEXTURE_2D,
          texture,
          pp::Size(600, 600),
          callback_factory_.NewCallback(&DemoInstance::OnTextureReleased,
                                        texture));
      assert(rv == PP_OK_COMPLETIONPENDING);
      rv = stable_texture_layer_.SetPremultipliedAlpha(PP_FALSE);
      assert(rv == PP_OK);
    }

    int32_t delta = static_cast<int32_t>(200 * fabsf(factor_sin));
    if (delta != 0) {
      int32_t x_y = 25 + delta;
      int32_t w_h =  650 - delta - delta;
      rv = stable_texture_layer_.SetClipRect(pp::Rect(x_y, x_y, w_h, w_h));
    } else {
      rv = stable_texture_layer_.SetClipRect(pp::Rect());
    }
    assert(rv == PP_OK);

    const float transform[16] = {
      factor_cos, -factor_sin, 0.0f, 0.0f,
      factor_sin, factor_cos, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      50.0f, 50.0f, 0.0f, 1.0f,
    };
    rv = stable_texture_layer_.SetTransform(transform);
    assert(rv == PP_OK);
  }

  {
    // Set the dynamic texture layer.
    if (texture_layer_.is_null()) {
      texture_layer_ = compositor_.AddLayer();
      assert(!texture_layer_.is_null());
      static const float transform[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        200.0f, 0.0f, 0.0f, 1.0f,
      };
      rv = texture_layer_.SetTransform(transform);
      assert(rv == PP_OK);
     }

    GLuint texture = PrepareFramebuffer();
    cube_->UpdateForTimeDelta(0.02f);
    cube_->Draw();
    rv = texture_layer_.SetTexture(
        *context_,
        GL_TEXTURE_2D,
        texture,
        pp::Size(400, 400),
        callback_factory_.NewCallback(&DemoInstance::OnTextureReleased,
                                      texture));
    assert(rv == PP_OK_COMPLETIONPENDING);
    rv = texture_layer_.SetPremultipliedAlpha(PP_FALSE);
    assert(rv == PP_OK);
  }
}

void DemoInstance::OnTextureReleased(int32_t result, GLuint texture) {
  if (result == PP_OK) {
    textures_.push_back(texture);
  } else {
    glDeleteTextures(1, &texture);
    total_resource_--;
  }
}

void DemoInstance::OnImageReleased(int32_t result, const pp::ImageData& image) {
  if (result == PP_OK) {
    images_.push_back(image);
  } else {
    total_resource_--;
  }
}

// This object is the global object representing this plugin library as long
// as it is loaded.
class DemoModule : public pp::Module {
 public:
  DemoModule() : Module() {}
  virtual ~DemoModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new DemoInstance(instance);
  }
};

}  // anonymous namespace

namespace pp {
// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new DemoModule();
}
}  // namespace pp
