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

#include "tools/render/sdl_util.h"

#include "gflags/gflags.h"

#if !defined(NDEBUG)
#define DEFAULT_OPENGL_DEBUG_FLAG_VALUE true
#else
#define DEFAULT_OPENGL_DEBUG_FLAG_VALUE false
#endif  // !defined(NDEBUG)

DEFINE_bool(opengl_debug,
            DEFAULT_OPENGL_DEBUG_FLAG_VALUE,
            "Show OpenGL debug messages");

namespace quic_trace {
namespace render {

OpenGlContext::OpenGlContext(SDL_Window* window) {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  context_ = SDL_GL_CreateContext(window);
  CHECK(context_ != nullptr) << "GL context is null: " << SDL_GetError();

  GLenum err = glewInit();
  if (err != GLEW_OK) {
    LOG(FATAL) << "Failed to initalize GLEW: " << glewGetErrorString(err);
  }

  if (GLEW_KHR_debug && FLAGS_opengl_debug) {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
        +[](GLenum source, GLenum type, GLuint id, GLenum severity,
            GLsizei length, const GLchar* message, const void* userParam) {
          switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
              LOG(ERROR) << "[GL]: " << std::string(message, length);
              break;
            case GL_DEBUG_SEVERITY_MEDIUM:
              LOG(WARNING) << "[GL]: " << std::string(message, length);
              break;
            case GL_DEBUG_SEVERITY_LOW:
              LOG(INFO) << "[GL]: " << std::string(message, length);
              break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
              VLOG(1) << "[GL]: " << std::string(message, length);
              break;
          }
        },
        nullptr);
  }
}

bool GlShader::Compile(const char* source) {
  glShaderSource(shader_, 1, &source, nullptr);
  glCompileShader(shader_);

  GLint compile_status = GL_FALSE;
  glGetShaderiv(shader_, GL_COMPILE_STATUS, &compile_status);
  return compile_status;
}

std::string GlShader::GetCompileInfoLog() {
  GLint buffer_size = 0;
  GLint actual_size = 0;

  glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &buffer_size);
  auto buffer = absl::make_unique<char[]>(buffer_size);

  glGetShaderInfoLog(shader_, buffer_size, &actual_size, buffer.get());
  return std::string(buffer.get(), actual_size);
}

void GlShader::CompileOrDie(const char* source) {
  if (!Compile(source)) {
    LOG(FATAL) << "Failed to compile a shader: " << GetCompileInfoLog();
  }
}

}  // namespace render
}  // namespace quic_trace
