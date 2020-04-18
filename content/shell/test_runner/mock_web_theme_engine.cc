// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_web_theme_engine.h"

#if !defined(OS_MACOSX)

#include "base/logging.h"
#include "build/build_config.h"
#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_flags.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkRect.h"

using blink::WebCanvas;
using blink::WebRect;
using blink::WebThemeEngine;

namespace test_runner {

namespace {

const SkColor edgeColor = SK_ColorBLACK;
const SkColor readOnlyColor = SkColorSetRGB(0xe9, 0xc2, 0xa6);

}  // namespace

SkColor bgColors(WebThemeEngine::State state) {
  switch (state) {
    case WebThemeEngine::kStateDisabled:
      return SkColorSetRGB(0xc9, 0xc9, 0xc9);
    case WebThemeEngine::kStateHover:
      return SkColorSetRGB(0x43, 0xf9, 0xff);
    case WebThemeEngine::kStateNormal:
      return SkColorSetRGB(0x89, 0xc4, 0xff);
    case WebThemeEngine::kStatePressed:
      return SkColorSetRGB(0xa9, 0xff, 0x12);
    case WebThemeEngine::kStateFocused:
      return SkColorSetRGB(0x00, 0xf3, 0xac);
    case WebThemeEngine::kStateReadonly:
      return SkColorSetRGB(0xf3, 0xe0, 0xd0);
    default:
      NOTREACHED();
  }
  return SkColorSetRGB(0x00, 0x00, 0xff);
}

blink::WebSize MockWebThemeEngine::GetSize(WebThemeEngine::Part part) {
  // FIXME: We use this constant to indicate we are being asked for the size of
  // a part that we don't expect to be asked about. We return a garbage value
  // rather than just asserting because this code doesn't have access to either
  // WTF or base to raise an assertion or do any logging :(.
  const blink::WebSize invalidPartSize = blink::WebSize(100, 100);

  switch (part) {
    case WebThemeEngine::kPartScrollbarLeftArrow:
      return blink::WebSize(17, 15);
    case WebThemeEngine::kPartScrollbarRightArrow:
      return invalidPartSize;
    case WebThemeEngine::kPartScrollbarUpArrow:
      return blink::WebSize(15, 17);
    case WebThemeEngine::kPartScrollbarDownArrow:
      return invalidPartSize;
    case WebThemeEngine::kPartScrollbarHorizontalThumb:
      return blink::WebSize(15, 15);
    case WebThemeEngine::kPartScrollbarVerticalThumb:
      return blink::WebSize(15, 15);
    case WebThemeEngine::kPartScrollbarHorizontalTrack:
      return blink::WebSize(0, 15);
    case WebThemeEngine::kPartScrollbarVerticalTrack:
      return blink::WebSize(15, 0);
    case WebThemeEngine::kPartCheckbox:
    case WebThemeEngine::kPartRadio:
      return blink::WebSize(13, 13);
    case WebThemeEngine::kPartSliderThumb:
      return blink::WebSize(11, 21);
    case WebThemeEngine::kPartInnerSpinButton:
      return blink::WebSize(15, 8);
    default:
      return invalidPartSize;
  }
}

static SkIRect webRectToSkIRect(const WebRect& webRect) {
  SkIRect irect;
  irect.set(webRect.x, webRect.y, webRect.x + webRect.width - 1,
            webRect.y + webRect.height - 1);
  return irect;
}

static SkIRect validate(const SkIRect& rect, WebThemeEngine::Part part) {
  switch (part) {
    case WebThemeEngine::kPartCheckbox:
    case WebThemeEngine::kPartRadio: {
      SkIRect retval = rect;

      // The maximum width and height is 13.
      // Center the square in the passed rectangle.
      const int maxControlSize = 13;
      int controlSize = std::min(rect.width(), rect.height());
      controlSize = std::min(controlSize, maxControlSize);

      retval.fLeft = rect.fLeft + (rect.width() / 2) - (controlSize / 2);
      retval.fRight = retval.fLeft + controlSize - 1;
      retval.fTop = rect.fTop + (rect.height() / 2) - (controlSize / 2);
      retval.fBottom = retval.fTop + controlSize - 1;

      return retval;
    }
    default:
      return rect;
  }
}

void box(cc::PaintCanvas* canvas, const SkIRect& rect, SkColor fillColor) {
  cc::PaintFlags flags;

  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(fillColor);
  canvas->drawIRect(rect, flags);

  flags.setColor(edgeColor);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  canvas->drawIRect(rect, flags);
}

void line(cc::PaintCanvas* canvas,
          int x0,
          int y0,
          int x1,
          int y1,
          SkColor color) {
  cc::PaintFlags flags;
  flags.setColor(color);
  canvas->drawLine(SkIntToScalar(x0), SkIntToScalar(y0), SkIntToScalar(x1),
                   SkIntToScalar(y1), flags);
}

void triangle(cc::PaintCanvas* canvas,
              int x0,
              int y0,
              int x1,
              int y1,
              int x2,
              int y2,
              SkColor color) {
  SkPath path;
  cc::PaintFlags flags;

  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  path.incReserve(4);
  path.moveTo(SkIntToScalar(x0), SkIntToScalar(y0));
  path.lineTo(SkIntToScalar(x1), SkIntToScalar(y1));
  path.lineTo(SkIntToScalar(x2), SkIntToScalar(y2));
  path.close();
  canvas->drawPath(path, flags);

  flags.setColor(edgeColor);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  canvas->drawPath(path, flags);
}

void roundRect(cc::PaintCanvas* canvas, SkIRect irect, SkColor color) {
  SkRect rect;
  SkScalar radius = SkIntToScalar(5);
  cc::PaintFlags flags;

  rect.set(irect);
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->drawRoundRect(rect, radius, radius, flags);

  flags.setColor(edgeColor);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  canvas->drawRoundRect(rect, radius, radius, flags);
}

void oval(cc::PaintCanvas* canvas, SkIRect irect, SkColor color) {
  SkRect rect;
  cc::PaintFlags flags;

  rect.set(irect);
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->drawOval(rect, flags);

  flags.setColor(edgeColor);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  canvas->drawOval(rect, flags);
}

void circle(cc::PaintCanvas* canvas,
            SkIRect irect,
            SkScalar radius,
            SkColor color) {
  int left = irect.fLeft;
  int width = irect.width();
  int height = irect.height();
  int top = irect.fTop;

  SkScalar cy = SkIntToScalar(top + height / 2);
  SkScalar cx = SkIntToScalar(left + width / 2);
  cc::PaintFlags flags;

  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  canvas->drawOval(
      SkRect::MakeLTRB(cx - radius, cy - radius, cx + radius, cy + radius),
      flags);

  flags.setColor(edgeColor);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  canvas->drawOval(
      SkRect::MakeLTRB(cx - radius, cy - radius, cx + radius, cy + radius),
      flags);
}

void nestedBoxes(cc::PaintCanvas* canvas,
                 SkIRect irect,
                 int indentLeft,
                 int indentTop,
                 int indentRight,
                 int indentBottom,
                 SkColor outerColor,
                 SkColor innerColor) {
  SkIRect lirect;
  box(canvas, irect, outerColor);
  lirect.set(irect.fLeft + indentLeft, irect.fTop + indentTop,
             irect.fRight - indentRight, irect.fBottom - indentBottom);
  box(canvas, lirect, innerColor);
}

void insetBox(cc::PaintCanvas* canvas,
              SkIRect irect,
              int indentLeft,
              int indentTop,
              int indentRight,
              int indentBottom,
              SkColor color) {
  SkIRect lirect;
  lirect.set(irect.fLeft + indentLeft, irect.fTop + indentTop,
             irect.fRight - indentRight, irect.fBottom - indentBottom);
  box(canvas, lirect, color);
}

void markState(cc::PaintCanvas* canvas,
               SkIRect irect,
               WebThemeEngine::State state) {
  int left = irect.fLeft;
  int right = irect.fRight;
  int top = irect.fTop;
  int bottom = irect.fBottom;

  // The length of a triangle side for the corner marks.
  const int triangleSize = 5;

  switch (state) {
    case WebThemeEngine::kStateDisabled:
    case WebThemeEngine::kStateNormal:
      // Don't visually mark these states (color is enough).
      break;

    case WebThemeEngine::kStateReadonly: {
      // The horizontal lines in a read only control are spaced by this amount.
      const int readOnlyLineOffset = 5;

      // Drawing lines across the control.
      for (int i = top + readOnlyLineOffset; i < bottom;
           i += readOnlyLineOffset)
        line(canvas, left + 1, i, right - 1, i, readOnlyColor);
      break;
    }
    case WebThemeEngine::kStateHover:
      // Draw a triangle in the upper left corner of the control. (Win's "hot")
      triangle(canvas, left, top, left + triangleSize, top, left,
               top + triangleSize, edgeColor);
      break;

    case WebThemeEngine::kStateFocused:
      // Draw a triangle in the bottom right corner of the control.
      triangle(canvas, right, bottom, right - triangleSize, bottom, right,
               bottom - triangleSize, edgeColor);
      break;

    case WebThemeEngine::kStatePressed:
      // Draw a triangle in the bottom left corner of the control.
      triangle(canvas, left, bottom, left, bottom - triangleSize,
               left + triangleSize, bottom, edgeColor);
      break;

    default:
      // FIXME: Should we do something here to indicate that we got an invalid
      // state?
      // Unfortunately, we can't assert because we don't have access to WTF or
      // base.
      break;
  }
}

void MockWebThemeEngine::Paint(blink::WebCanvas* canvas,
                               WebThemeEngine::Part part,
                               WebThemeEngine::State state,
                               const blink::WebRect& rect,
                               const WebThemeEngine::ExtraParams* extraParams) {
  SkIRect irect = webRectToSkIRect(rect);
  cc::PaintFlags flags;

  // Indent amounts for the check in a checkbox or radio button.
  const int checkIndent = 3;

  // Indent amounts for short and long sides of the scrollbar notches.
  const int notchLongOffset = 1;
  const int notchShortOffset = 4;
  const int noOffset = 0;

  // Indent amounts for the short and long sides of a scroll thumb box.
  const int thumbLongIndent = 0;
  const int thumbShortIndent = 2;

  // Indents for the crosshatch on a scroll grip.
  const int gripLongIndent = 3;
  const int gripShortIndent = 5;

  // Indents for the the slider track.
  const int sliderIndent = 2;

  int halfHeight = irect.height() / 2;
  int halfWidth = irect.width() / 2;
  int quarterHeight = irect.height() / 4;
  int quarterWidth = irect.width() / 4;
  int left = irect.fLeft;
  int right = irect.fRight;
  int top = irect.fTop;
  int bottom = irect.fBottom;

  switch (part) {
    case WebThemeEngine::kPartScrollbarDownArrow:
      box(canvas, irect, bgColors(state));
      triangle(canvas, left + quarterWidth, top + quarterHeight,
               right - quarterWidth, top + quarterHeight, left + halfWidth,
               bottom - quarterHeight, edgeColor);
      markState(canvas, irect, state);
      break;

    case WebThemeEngine::kPartScrollbarLeftArrow:
      box(canvas, irect, bgColors(state));
      triangle(canvas, right - quarterWidth, top + quarterHeight,
               right - quarterWidth, bottom - quarterHeight,
               left + quarterWidth, top + halfHeight, edgeColor);
      break;

    case WebThemeEngine::kPartScrollbarRightArrow:
      box(canvas, irect, bgColors(state));
      triangle(canvas, left + quarterWidth, top + quarterHeight,
               right - quarterWidth, top + halfHeight, left + quarterWidth,
               bottom - quarterHeight, edgeColor);
      break;

    case WebThemeEngine::kPartScrollbarUpArrow:
      box(canvas, irect, bgColors(state));
      triangle(canvas, left + quarterWidth, bottom - quarterHeight,
               left + halfWidth, top + quarterHeight, right - quarterWidth,
               bottom - quarterHeight, edgeColor);
      markState(canvas, irect, state);
      break;

    case WebThemeEngine::kPartScrollbarHorizontalThumb: {
      // Draw a narrower box on top of the outside box.
      nestedBoxes(canvas, irect, thumbLongIndent, thumbShortIndent,
                  thumbLongIndent, thumbShortIndent, bgColors(state),
                  bgColors(state));
      // Draw a horizontal crosshatch for the grip.
      int longOffset = halfWidth - gripLongIndent;
      line(canvas, left + gripLongIndent, top + halfHeight,
           right - gripLongIndent, top + halfHeight, edgeColor);
      line(canvas, left + longOffset, top + gripShortIndent, left + longOffset,
           bottom - gripShortIndent, edgeColor);
      line(canvas, right - longOffset, top + gripShortIndent,
           right - longOffset, bottom - gripShortIndent, edgeColor);
      markState(canvas, irect, state);
      break;
    }

    case WebThemeEngine::kPartScrollbarVerticalThumb: {
      // Draw a shorter box on top of the outside box.
      nestedBoxes(canvas, irect, thumbShortIndent, thumbLongIndent,
                  thumbShortIndent, thumbLongIndent, bgColors(state),
                  bgColors(state));
      // Draw a vertical crosshatch for the grip.
      int longOffset = halfHeight - gripLongIndent;
      line(canvas, left + halfWidth, top + gripLongIndent, left + halfWidth,
           bottom - gripLongIndent, edgeColor);
      line(canvas, left + gripShortIndent, top + longOffset,
           right - gripShortIndent, top + longOffset, edgeColor);
      line(canvas, left + gripShortIndent, bottom - longOffset,
           right - gripShortIndent, bottom - longOffset, edgeColor);
      markState(canvas, irect, state);
      break;
    }

    case WebThemeEngine::kPartScrollbarHorizontalTrack: {
      int longOffset = halfHeight - notchLongOffset;
      int shortOffset = irect.width() - notchShortOffset;
      box(canvas, irect, bgColors(state));
      // back, notch on right
      insetBox(canvas, irect, noOffset, longOffset, shortOffset, longOffset,
               edgeColor);
      // forward, notch on right
      insetBox(canvas, irect, shortOffset, longOffset, noOffset, longOffset,
               edgeColor);
      markState(canvas, irect, state);
      break;
    }

    case WebThemeEngine::kPartScrollbarVerticalTrack: {
      int longOffset = halfWidth - notchLongOffset;
      int shortOffset = irect.height() - notchShortOffset;
      box(canvas, irect, bgColors(state));
      // back, notch at top
      insetBox(canvas, irect, longOffset, noOffset, longOffset, shortOffset,
               edgeColor);
      // forward, notch at bottom
      insetBox(canvas, irect, longOffset, shortOffset, longOffset, noOffset,
               edgeColor);
      markState(canvas, irect, state);
      break;
    }

    case WebThemeEngine::kPartScrollbarCorner: {
      SkIRect cornerRect = {rect.x, rect.y, rect.x + rect.width,
                            rect.y + rect.height};
      flags.setColor(SK_ColorWHITE);
      flags.setStyle(cc::PaintFlags::kFill_Style);
      flags.setBlendMode(SkBlendMode::kSrc);
      flags.setAntiAlias(true);
      canvas->drawIRect(cornerRect, flags);
      break;
    }

    case WebThemeEngine::kPartCheckbox:
      if (extraParams->button.indeterminate) {
        nestedBoxes(canvas, irect, checkIndent, halfHeight, checkIndent,
                    halfHeight, bgColors(state), edgeColor);
      } else if (extraParams->button.checked) {
        irect = validate(irect, part);
        nestedBoxes(canvas, irect, checkIndent, checkIndent, checkIndent,
                    checkIndent, bgColors(state), edgeColor);
      } else {
        irect = validate(irect, part);
        box(canvas, irect, bgColors(state));
      }
      break;

    case WebThemeEngine::kPartRadio:
      irect = validate(irect, part);
      halfHeight = irect.height() / 2;
      if (extraParams->button.checked) {
        circle(canvas, irect, SkIntToScalar(halfHeight), bgColors(state));
        circle(canvas, irect, SkIntToScalar(halfHeight - checkIndent),
               edgeColor);
      } else {
        circle(canvas, irect, SkIntToScalar(halfHeight), bgColors(state));
      }
      break;

    case WebThemeEngine::kPartButton:
      roundRect(canvas, irect, bgColors(state));
      markState(canvas, irect, state);
      break;

    case WebThemeEngine::kPartTextField:
      flags.setColor(extraParams->text_field.background_color);
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->drawIRect(irect, flags);

      flags.setColor(edgeColor);
      flags.setStyle(cc::PaintFlags::kStroke_Style);
      canvas->drawIRect(irect, flags);

      markState(canvas, irect, state);
      break;

    case WebThemeEngine::kPartMenuList:
      if (extraParams->menu_list.fill_content_area) {
        box(canvas, irect, extraParams->menu_list.background_color);
      } else {
        cc::PaintFlags flags;
        flags.setColor(edgeColor);
        flags.setStyle(cc::PaintFlags::kStroke_Style);
        canvas->drawIRect(irect, flags);
      }

      // clip the drop-down arrow to be inside the select box
      irect.fLeft = std::max(irect.fLeft,
                             extraParams->menu_list.arrow_x -
                                 (extraParams->menu_list.arrow_size + 1) / 2);
      irect.fRight = std::min(irect.fLeft + extraParams->menu_list.arrow_size,
                              irect.fRight);

      irect.fTop = extraParams->menu_list.arrow_y -
                   (extraParams->menu_list.arrow_size) / 2;
      irect.fBottom = irect.fTop + (extraParams->menu_list.arrow_size);

      halfWidth = irect.width() / 2;
      quarterWidth = irect.width() / 4;

      if (state == WebThemeEngine::kStateFocused)  // FIXME: draw differenty?
        state = WebThemeEngine::kStateNormal;
      box(canvas, irect, bgColors(state));
      triangle(canvas, irect.fLeft + quarterWidth, irect.fTop + quarterWidth,
               irect.fRight - quarterWidth, irect.fTop + quarterWidth,
               irect.fLeft + halfWidth, irect.fBottom - quarterWidth,
               edgeColor);

      break;

    case WebThemeEngine::kPartSliderTrack: {
      SkIRect lirect = irect;

      // Draw a narrow rect for the track plus box hatches on the ends.
      if (state == WebThemeEngine::kStateFocused)  // FIXME: draw differently?
        state = WebThemeEngine::kStateNormal;
      if (extraParams->slider.vertical) {
        lirect.inset(halfWidth - sliderIndent, noOffset);
        box(canvas, lirect, bgColors(state));
        line(canvas, left, top, right, top, edgeColor);
        line(canvas, left, bottom, right, bottom, edgeColor);
      } else {
        lirect.inset(noOffset, halfHeight - sliderIndent);
        box(canvas, lirect, bgColors(state));
        line(canvas, left, top, left, bottom, edgeColor);
        line(canvas, right, top, right, bottom, edgeColor);
      }
      break;
    }

    case WebThemeEngine::kPartSliderThumb:
      if (state == WebThemeEngine::kStateFocused)  // FIXME: draw differently?
        state = WebThemeEngine::kStateNormal;
      oval(canvas, irect, bgColors(state));
      break;

    case WebThemeEngine::kPartInnerSpinButton: {
      // stack half-height up and down arrows on top of each other
      SkIRect lirect;
      int halfHeight = rect.height / 2;
      if (extraParams->inner_spin.read_only)
        state = blink::WebThemeEngine::kStateDisabled;

      lirect.set(rect.x, rect.y, rect.x + rect.width - 1,
                 rect.y + halfHeight - 1);
      box(canvas, lirect, bgColors(state));
      bottom = lirect.fBottom;
      quarterHeight = lirect.height() / 4;
      triangle(canvas, left + quarterWidth, bottom - quarterHeight,
               right - quarterWidth, bottom - quarterHeight, left + halfWidth,
               top + quarterHeight, edgeColor);

      lirect.set(rect.x, rect.y + halfHeight, rect.x + rect.width - 1,
                 rect.y + 2 * halfHeight - 1);
      top = lirect.fTop;
      bottom = lirect.fBottom;
      quarterHeight = lirect.height() / 4;
      box(canvas, lirect, bgColors(state));
      triangle(canvas, left + quarterWidth, top + quarterHeight,
               right - quarterWidth, top + quarterHeight, left + halfWidth,
               bottom - quarterHeight, edgeColor);
      markState(canvas, irect, state);
      break;
    }
    case WebThemeEngine::kPartProgressBar: {
      flags.setColor(bgColors(state));
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->drawIRect(irect, flags);

      // Emulate clipping
      SkIRect tofill = irect;
      if (extraParams->progress_bar.determinate) {
        tofill.set(extraParams->progress_bar.value_rect_x,
                   extraParams->progress_bar.value_rect_y,
                   extraParams->progress_bar.value_rect_x +
                       extraParams->progress_bar.value_rect_width - 1,
                   extraParams->progress_bar.value_rect_y +
                       extraParams->progress_bar.value_rect_height);
      }

      if (!tofill.intersect(irect))
        tofill.setEmpty();

      flags.setColor(edgeColor);
      flags.setStyle(cc::PaintFlags::kFill_Style);
      canvas->drawIRect(tofill, flags);

      markState(canvas, irect, state);
      break;
    }
    default:
      // FIXME: Should we do something here to indicate that we got an invalid
      // part?
      // Unfortunately, we can't assert because we don't have access to WTF or
      // base.
      break;
  }
}

}  // namespace test_runner

#endif  // !defined(OS_MACOSX)
