// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_AX_RANGE_H_
#define UI_ACCESSIBILITY_AX_RANGE_H_

#include <memory>
#include <utility>

#include "base/strings/string16.h"

namespace ui {

// A range of ax positions.
//
// In order to avoid any confusion regarding whether a deep or a shallow copy is
// being performed, this class can be moved but not copied.
template <class AXPositionType>
class AXRange {
 public:
  AXRange()
      : anchor_(AXPositionType::CreateNullPosition()),
        focus_(AXPositionType::CreateNullPosition()) {}

  AXRange(std::unique_ptr<AXPositionType> anchor,
          std::unique_ptr<AXPositionType> focus) {
    if (anchor) {
      anchor_ = std::move(anchor);
    } else {
      anchor_ = AXPositionType::CreateNullPosition();
    }
    if (focus) {
      focus_ = std::move(focus);
    } else {
      focus = AXPositionType::CreateNullPosition();
    }
  }

  AXRange(const AXRange& other) = delete;

  AXRange(AXRange&& other) : AXRange() {
    anchor_.swap(other.anchor_);
    focus_.swap(other.focus_);
  }

  AXRange& operator=(const AXRange& other) = delete;

  AXRange& operator=(const AXRange&& other) {
    if (this != other) {
      anchor_ = AXPositionType::CreateNullPosition();
      focus_ = AXPositionType::CreateNullPosition();
      anchor_.swap(other.anchor_);
      focus_.swap(other.focus_);
    }
    return *this;
  }

  virtual ~AXRange() {}

  bool IsNull() const {
    return !anchor_ || !focus_ || anchor_->IsNullPosition() ||
           focus_->IsNullPosition();
  }

  AXPositionType* anchor() const {
    DCHECK(anchor_);
    return anchor_.get();
  }

  AXPositionType* focus() const {
    DCHECK(focus_);
    return focus_.get();
  }

  base::string16 GetText() const {
    base::string16 text;
    if (IsNull())
      return text;

    std::unique_ptr<AXPositionType> start, end;
    if (*anchor_ < *focus_) {
      start = anchor_->AsLeafTextPosition();
      end = focus_->AsLeafTextPosition();
    } else {
      start = focus_->AsLeafTextPosition();
      end = anchor_->AsLeafTextPosition();
    }

    int start_offset = start->text_offset();
    DCHECK_GE(start_offset, 0);
    int end_offset = end->text_offset();
    DCHECK_GE(end_offset, 0);

    do {
      text += start->GetInnerText();
      start = start->CreateNextTextAnchorPosition();
    } while (!start->IsNullPosition() && *start <= *end);

    if (static_cast<size_t>(start_offset) > text.length())
      return base::string16();

    text = text.substr(start_offset, base::string16::npos);
    size_t text_length = text.length() - end->GetInnerText().length() +
                         static_cast<size_t>(end_offset);
    return text.substr(0, text_length);
  }

 private:
  std::unique_ptr<AXPositionType> anchor_;
  std::unique_ptr<AXPositionType> focus_;
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_AX_RANGE_H_
