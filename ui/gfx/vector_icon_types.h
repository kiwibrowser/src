// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_VECTOR_ICON_TYPES_H_
#define UI_GFX_VECTOR_ICON_TYPES_H_

#include "base/macros.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "ui/gfx/animation/tween.h"

namespace gfx {

// A command to Skia.
enum CommandType {
  // A new <path> element. For the first path, this is assumed.
  NEW_PATH,
  // Sets the alpha for the current path.
  PATH_COLOR_ALPHA,
  // Sets the color for the current path.
  PATH_COLOR_ARGB,
  // Sets the path to clear mode (Skia's kClear_Mode).
  PATH_MODE_CLEAR,
  // By default, the path will be filled. This changes the paint action to
  // stroke at the given width.
  STROKE,
  // By default, a stroke has a round cap. This sets it to square.
  CAP_SQUARE,
  // These correspond to pathing commands.
  MOVE_TO,
  R_MOVE_TO,
  ARC_TO,
  R_ARC_TO,
  LINE_TO,
  R_LINE_TO,
  H_LINE_TO,
  R_H_LINE_TO,
  V_LINE_TO,
  R_V_LINE_TO,
  CUBIC_TO,
  R_CUBIC_TO,
  CUBIC_TO_SHORTHAND,
  CIRCLE,
  ROUND_RECT,
  CLOSE,
  // Sets the dimensions of the canvas in dip.
  CANVAS_DIMENSIONS,
  // Sets a bounding rect for the path. This allows fine adjustment because it
  // can tweak edge anti-aliasing. Args are x, y, w, h.
  CLIP,
  // Disables anti-aliasing for this path.
  DISABLE_AA,
  // Flips the x-axis in RTL locales. Default is false, this command sets it to
  // true.
  FLIPS_IN_RTL,
  // Defines a timed transition for other elements.
  TRANSITION_FROM,
  TRANSITION_TO,
  // Parameters are delay (ms), duration (ms), and tween type
  // (gfx::Tween::Type).
  TRANSITION_END,
};

// A POD that describes either a path command or an argument for it.
struct PathElement {
  constexpr PathElement(CommandType value) : command(value) {}
  constexpr PathElement(SkScalar value) : arg(value) {}

  union {
    CommandType command;
    SkScalar arg;
  };
};

// Describes the drawing commands for a single vector icon at a particular pixel
// size or range of sizes.
struct VectorIconRep {
  VectorIconRep() = default;

  const PathElement* path = nullptr;

  // The length of |path|.
  size_t path_size = 0u;

 private:
  DISALLOW_COPY_AND_ASSIGN(VectorIconRep);
};

// A vector icon that stores one or more representations to be used for various
// scale factors and pixel dimensions.
struct VectorIcon {
  VectorIcon() = default;

  bool is_empty() const { return !reps; }

  const VectorIconRep* const reps = nullptr;
  size_t reps_size = 0u;

  // A human-readable name, useful for debugging, derived from the name of the
  // icon file. This can also be used as an identifier, but vector icon targets
  // should be careful to ensure this is unique.
  const char* name = nullptr;

  bool operator<(const VectorIcon& other) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(VectorIcon);
};

}  // namespace gfx

#endif  // UI_GFX_VECTOR_ICON_TYPES_H_
