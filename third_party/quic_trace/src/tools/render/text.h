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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_TEXT_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_TEXT_H_

#include <memory>
#include <unordered_map>

#include "absl/container/node_hash_map.h"
#include "external/sdl2_ttf/SDL_ttf.h"
#include "tools/render/program_state.h"
#include "tools/render/sdl_util.h"
#include "tools/render/shader.h"

namespace quic_trace {
namespace render {

class TextRenderer;

// Represents a rendered text object.
class Text {
 public:
  size_t width() const { return width_; }
  size_t height() const { return height_; }

 private:
  friend class TextRenderer;

  Text(TextRenderer* factory) : factory_(factory) {}

  void Draw(int x, int y) const;

  TextRenderer* factory_;
  GlTexture texture_;
  float texture_size_;

  size_t width_;
  size_t height_;
};

// Factory for Text objects.  Upon cosntruction, loads all of the fonts and
// shaders used for text rendering into memory.
class TextRenderer {
 public:
  TextRenderer(const ProgramState* state);
  ~TextRenderer();

  // Render the supplied text.  This converts the text into pixels (making the
  // dimensions of the text available), but does not actually draw anything on
  // screen.
  std::shared_ptr<const Text> RenderText(const std::string& text) {
    return RenderTextInner(text, TTF_STYLE_NORMAL);
  }
  // The equivalent of the method above, except it renders bold text.
  std::shared_ptr<const Text> RenderTextBold(const std::string& text) {
    return RenderTextInner(text, TTF_STYLE_BOLD);
  }
  // Schedules specified text to be drawn at specified window coordinates.
  void AddText(std::shared_ptr<const Text> text, int x, int y);
  // Draws all of the scheduled text on-screen.
  void DrawAll();

 private:
  friend class Text;
  struct CacheEntry {
    std::shared_ptr<const Text> text;
    // Was this entry accessed within last frame?
    bool live;
  };
  struct TextToDraw {
    std::shared_ptr<const Text> text;
    int x;
    int y;
  };

  std::shared_ptr<const Text> RenderTextInner(const std::string& text,
                                              int style);

  TTF_Font* font_ = nullptr;

  const ProgramState* state_;
  Shader shader_;

  // Text that will be drawn at the end of the frame.
  std::vector<TextToDraw> texts_to_draw_;
  // The text cache.  Used to avoid having to render the text into texture every
  // frame.  All entries that are not rendered within the past frame are
  // evicted.
  absl::node_hash_map<std::string, CacheEntry> cache_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_TEXT_H_
