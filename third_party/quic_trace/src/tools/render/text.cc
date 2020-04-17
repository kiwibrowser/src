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

#include "tools/render/text.h"

#include "absl/strings/str_cat.h"
#include "tools/render/font_data.h"

namespace quic_trace {
namespace render {

namespace {
// The following shader renders the text as specified in window coordinates and
// assigns appropriate texture
const char* kVertexShader = R"(
uniform float texture_w;
uniform float texture_h;

in vec2 coord;
out vec2 texcoord;
void main(void) {
  gl_Position = windowToGl(coord);

  // Textures the squares based on the following assumed ordering of vertices:
  //   0  1
  //   2  3
  texcoord = vec2((gl_VertexID % 2) * texture_w, (gl_VertexID % 4) < 2 ? 0 : texture_h);
}
)";

const char* kFragmentShader = R"(
in vec2 texcoord;
out vec4 color;

uniform sampler2D tex;

void main(void) {
  color = texture(tex, texcoord);
}
)";
}  // namespace

TextRenderer::TextRenderer(const ProgramState* state)
    : state_(state), shader_(kVertexShader, kFragmentShader) {
  CHECK_EQ(TTF_Init(), 0) << "Failed to initialize SDL_ttf: " << TTF_GetError();

  SDL_RWops* font_data =
      SDL_RWFromConstMem(::kDroidSansBlob.data(), ::kDroidSansBlob.size());
  font_ = TTF_OpenFontRW(font_data, /*freesrc=*/1, state->ScaleForDpi(14));
  CHECK(font_ != nullptr) << "Failed to load regular font";
}

TextRenderer::~TextRenderer() {
  if (font_ != nullptr) {
    TTF_CloseFont(font_);
  }

  TTF_Quit();
}

static size_t NearestPowerOfTwo(size_t x) {
  size_t result = 1;
  while (result < x) {
    result *= 2;
  }
  return result;
}

void TextRenderer::AddText(std::shared_ptr<const Text> text, int x, int y) {
  texts_to_draw_.push_back(TextToDraw{
      .text = std::move(text),
      .x = x,
      .y = y,
  });
}

void TextRenderer::DrawAll() {
  // Common setup for drawing text.
  glUseProgram(*shader_);
  state_->Bind(shader_);

  for (const TextToDraw& text_to_draw : texts_to_draw_) {
    text_to_draw.text->Draw(text_to_draw.x, text_to_draw.y);
  }
  texts_to_draw_.clear();

  // Clean up the cache.
  for (auto it = cache_.begin(); it != cache_.end();) {
    if (it->second.live) {
      it->second.live = false;
      it++;
    } else {
      cache_.erase(it++);
    }
  }
  DCHECK(cache_.size() < 4096) << "The text cache is too big, which indicates "
                                  "that it's most likely leaking";
}

std::shared_ptr<const Text> TextRenderer::RenderTextInner(
    const std::string& text,
    int style) {
  // Check if we have the text in the cache.
  std::string cache_key = absl::StrCat(style, "/", text);
  auto cache_it = cache_.find(cache_key);
  if (cache_it != cache_.end()) {
    cache_it->second.live = true;
    return cache_it->second.text;
  }

  if (TTF_GetFontStyle(font_) != style) {
    TTF_SetFontStyle(font_, style);
  }

  // Render the text.
  ScopedSdlSurface surface;
  if (text.empty()) {
    // SDL2_ttf returns error when asked to render an empty string.  Return a
    // 1x1 white square instead.
    surface = ScopedSdlSurface(
        SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_RGBA32));
    CHECK(!SDL_MUSTLOCK(*surface));
    memset(surface->pixels, 0xff, surface->h * surface->pitch);
  } else {
    surface = TTF_RenderUTF8_Blended(font_, text.c_str(), SDL_Color{0, 0, 0});
  }
  if (*surface == nullptr) {
    LOG(FATAL) << "Failed to render text: \"" << text.size() << "\"";
  }

  std::unique_ptr<Text> result(new Text(this));
  // Find the correct size for the texture, which is required to be a power of
  // two.
  size_t size =
      std::max(NearestPowerOfTwo(surface->w), NearestPowerOfTwo(surface->h));
  result->width_ = surface->w;
  result->height_ = surface->h;
  result->texture_size_ = size;

  // Create a |size|x|size| texture buffer in RAM, and move the texture there.
  ScopedSdlSurface scaled_surface(SDL_CreateRGBSurfaceWithFormat(
      0, size, size, 32, SDL_PIXELFORMAT_RGBA32));
  SDL_Rect target{0, 0, surface->w, surface->h};
  SDL_BlitSurface(*surface, nullptr, *scaled_surface, &target);

  CHECK_EQ(scaled_surface->format->format, SDL_PIXELFORMAT_RGBA32);
  CHECK_EQ(scaled_surface->pitch, size * 4);
  CHECK(!SDL_MUSTLOCK(*scaled_surface));

  // Upload the texture onto the GPU.
  glBindTexture(GL_TEXTURE_2D, *result->texture_);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, scaled_surface->pixels);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  std::shared_ptr<const Text> cached_result(result.release());
  cache_.emplace(cache_key, CacheEntry{cached_result, true});
  return cached_result;
}

void Text::Draw(int x, int y) const {
  const float fx = x, fy = y, fw = width_, fh = height_;
  const float vertices[] = {
      fx,      fy + fh,  // upper left
      fx + fw, fy + fh,  // upper right
      fx,      fy,       // lower left
      fx + fw, fy,       // lower right
  };

  GlVertexBuffer buffer;
  glBindBuffer(GL_ARRAY_BUFFER, *buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

  GlVertexArray array;
  glBindVertexArray(*array);

  glBindTexture(GL_TEXTURE_2D, *texture_);
  factory_->shader_.SetUniform("texture_w", width_ / texture_size_);
  factory_->shader_.SetUniform("texture_h", height_ / texture_size_);

  GlVertexArrayAttrib coord(factory_->shader_, "coord");
  glVertexAttribPointer(*coord, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}  // namespace render
}  // namespace quic_trace
