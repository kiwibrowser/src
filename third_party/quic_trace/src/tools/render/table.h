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

#ifndef THIRD_PARTY_QUIC_TRACE_TOOLS_TABLE_H_
#define THIRD_PARTY_QUIC_TRACE_TOOLS_TABLE_H_

#include "tools/render/geometry_util.h"
#include "tools/render/rectangle_renderer.h"
#include "tools/render/text.h"

namespace quic_trace {
namespace render {

// Represents a simple key-value tables with optional section headers.
class Table {
 public:
  Table(const ProgramState* state,
        TextRenderer* text_factory,
        RectangleRenderer* rectangles);

  void AddHeader(const std::string& text);
  void AddRow(const std::string& label, const std::string& value);
  // Compute the layout parameters and return the size of the table.  Has to be
  // caleld before Draw().
  vec2 Layout();
  // Draw the table at the specified position.  Layout() MUST be called
  // beforehand.
  void Draw(vec2 position);

 private:
  struct Row {
    std::shared_ptr<const Text> header;
    std::shared_ptr<const Text> label;
    std::shared_ptr<const Text> value;

    int y_offset;
    int height;
  };

  const ProgramState* state_;
  TextRenderer* text_renderer_;
  RectangleRenderer* rectangles_;

  std::vector<Row> rows_;
  int width_;
  int height_;
  int spacing_;
};

}  // namespace render
}  // namespace quic_trace

#endif  // THIRD_PARTY_QUIC_TRACE_TOOLS_TABLE_H_
