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

#include "tools/render/table.h"

namespace quic_trace {
namespace render {

Table::Table(const ProgramState* state,
             TextRenderer* text_factory,
             RectangleRenderer* rectangles)
    : state_(state), text_renderer_(text_factory), rectangles_(rectangles) {}

void Table::AddHeader(const std::string& text) {
  rows_.push_back(Row{/*.header=*/text_renderer_->RenderTextBold(text),
                      /*.label=*/nullptr,
                      /*.value=*/nullptr});
}

void Table::AddRow(const std::string& label, const std::string& value) {
  rows_.push_back(Row{
      /*.header=*/nullptr,
      /*.label=*/text_renderer_->RenderTextBold(label),
      /*.value=*/text_renderer_->RenderText(value),
  });
}

vec2 Table::Layout() {
  spacing_ = state_->ScaleForDpi(8);

  int y_offset = 0;
  width_ = height_ = 0;
  for (Row& row : rows_) {
    int row_width = 2 * spacing_;
    int row_height = spacing_;
    if (row.header != nullptr) {
      row_width += row.header->width();
      row_height += row.header->height();

      // Add extra margin for all headers except the first one.
      if (y_offset > 0) {
        y_offset += spacing_ / 2;
      }
    } else {
      // A label-value combination
      row_width += row.label->width() + spacing_ + row.value->width();
      row_height += std::max(row.label->height(), row.value->height());
    }
    width_ = std::max(row_width, width_);

    row.y_offset = y_offset;
    row.height = row_height;
    y_offset += row_height;
  }
  height_ = y_offset + spacing_;
  return vec2(width_, height_);
}

void Table::Draw(vec2 position) {
  vec2 base_offset = position + vec2(spacing_, spacing_);
  rectangles_->AddRectangleWithBorder(Box{position, vec2(width_, height_)},
                                      0xffffffff);
  for (const Row& row : rows_) {
    vec2 row_offset =
        base_offset + vec2(0, height_ - row.y_offset - row.height - spacing_);
    if (row.header != nullptr) {
      row_offset += vec2((width_ - spacing_ * 2 - row.header->width()) / 2, 0);
      text_renderer_->AddText(row.header, row_offset.x, row_offset.y);
    } else {
      text_renderer_->AddText(row.label, row_offset.x, row_offset.y);
      text_renderer_->AddText(row.value,
                              row_offset.x + row.label->width() + spacing_,
                              row_offset.y);
    }
  }
}

}  // namespace render
}  // namespace quic_trace
