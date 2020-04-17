// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file contains various utility methods related to SDL and OpenGL,
// primarily RAII wrappers around the C types used by those.

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_SDL_UTIL_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_SDL_UTIL_H_

#include <GL/glew.h>
#include <SDL.h>

#include <memory>

#include <glog/logging.h>
#include <glog/stl_logging.h>
#include "absl/memory/memory.h"

namespace quic_trace {
namespace render {

// Scoped global initialization for SDL.
class ScopedSDL {
 public:
  ScopedSDL() { CHECK_EQ(0, SDL_Init(SDL_INIT_VIDEO)); }
  ScopedSDL(const ScopedSDL&) = delete;
  ScopedSDL& operator=(const ScopedSDL&) = delete;

  ~ScopedSDL() { SDL_Quit(); }
};

// A helper class to automatically delete SDL objects once they go out of scope.
template <typename T, void (*D)(T*)>
class ScopedSdlType {
 public:
  ScopedSdlType() : value_(nullptr, D) {}
  ScopedSdlType(T* value) : value_(value, D) {}

  T* get() const { return value_.get(); }
  T* operator*() const { return value_.get(); }
  T* operator->() const { return value_.get(); }

 private:
  std::unique_ptr<T, void (*)(T*)> value_;
};

using ScopedSdlWindow = ScopedSdlType<SDL_Window, SDL_DestroyWindow>;
using ScopedSdlSurface = ScopedSdlType<SDL_Surface, SDL_FreeSurface>;

class OpenGlContext {
 public:
  OpenGlContext(SDL_Window* window);
  ~OpenGlContext() { SDL_GL_DeleteContext(context_); }

  SDL_GLContext context() const { return context_; }

 private:
  SDL_GLContext context_;
};

// Utility wrapper around an OpenGL shader.
class GlShader {
 public:
  GlShader(GLenum type) { shader_ = glCreateShader(type); }

  ~GlShader() { glDeleteShader(shader_); }

  GlShader(const GlShader&) = delete;
  GlShader& operator=(const GlShader&) = delete;

  bool Compile(const char* source);
  std::string GetCompileInfoLog();
  void CompileOrDie(const char* source);

  GLuint operator*() const { return shader_; }

 private:
  GLuint shader_;
};

class GlProgram {
 public:
  GlProgram() { program_ = glCreateProgram(); }

  ~GlProgram() { glDeleteProgram(program_); }

  GlProgram(const GlProgram&) = delete;
  GlProgram& operator=(const GlProgram&) = delete;

  void Attach(const GlShader& shader) { glAttachShader(program_, *shader); }

  bool Link() {
    glLinkProgram(program_);

    GLint link_status = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &link_status);
    return link_status;
  }

  void SetUniform(const char* name, float value) const {
    GLint var = glGetUniformLocation(program_, name);
    DCHECK_NE(var, -1) << "Failed to locate a uniform " << name;
    glUniform1f(var, value);
  }

  void SetUniform(const char* name, float x, float y) const {
    GLint var = glGetUniformLocation(program_, name);
    DCHECK_NE(var, -1) << "Failed to locate a uniform " << name;
    glUniform2f(var, x, y);
  }

  void SetUniform(const char* name, int value) const {
    GLint var = glGetUniformLocation(program_, name);
    DCHECK_NE(var, -1) << "Failed to locate a uniform " << name;
    glUniform1i(var, value);
  }

  GLuint operator*() const { return program_; }

 private:
  GLuint program_;
};

template <GLenum BufferType>
class GlBuffer {
 public:
  GlBuffer() {
    glGenBuffers(1, &buffer_);
    glBindBuffer(BufferType, buffer_);
  }
  ~GlBuffer() { glDeleteBuffers(1, &buffer_); }

  GlBuffer(const GlBuffer&) = delete;
  GlBuffer& operator=(const GlBuffer&) = delete;

  GLuint operator*() const { return buffer_; }

 private:
  GLuint buffer_;
};

using GlVertexBuffer = GlBuffer<GL_ARRAY_BUFFER>;
using GlUniformBuffer = GlBuffer<GL_UNIFORM_BUFFER>;
using GlElementBuffer = GlBuffer<GL_ELEMENT_ARRAY_BUFFER>;

class GlVertexArray {
 public:
  GlVertexArray() {
    glGenVertexArrays(1, &array_);
    glBindVertexArray(array_);
  }
  ~GlVertexArray() { glDeleteVertexArrays(1, &array_); }

  GlVertexArray(const GlVertexArray&) = delete;
  GlVertexArray& operator=(const GlVertexArray&) = delete;

  GLuint operator*() const { return array_; }

 private:
  GLuint array_;
};

class GlVertexArrayAttrib {
 public:
  GlVertexArrayAttrib(const GlProgram& program, const char* name) {
    attribute_ = glGetAttribLocation(*program, name);
    DCHECK_NE(attribute_, -1) << "Failed to locate attribute " << name;
    glEnableVertexAttribArray(attribute_);
  }

  ~GlVertexArrayAttrib() { glDisableVertexAttribArray(attribute_); }

  GlVertexArrayAttrib(const GlVertexArrayAttrib&) = delete;
  GlVertexArrayAttrib& operator=(const GlVertexArrayAttrib&) = delete;

  GLint operator*() const { return attribute_; }

 private:
  GLint attribute_;
};

class GlTexture {
 public:
  GlTexture() {
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
  }
  ~GlTexture() { glDeleteTextures(1, &texture_); }

  GlTexture(const GlTexture&) = delete;
  GlTexture& operator=(const GlTexture&) = delete;

  GLuint operator*() const { return texture_; }

 private:
  GLuint texture_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_SDL_UTIL_H_
