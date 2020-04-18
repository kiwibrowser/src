/*
 * Copyright (C) 2006, 2007, 2009, 2010, 2011, 2012 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2012, 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_CANVAS_PATH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_CANVAS_PATH_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/graphics/path.h"

namespace blink {

class ExceptionState;

class MODULES_EXPORT CanvasPath {
 public:
  virtual ~CanvasPath() = default;

  void closePath();
  void moveTo(float x, float y);
  void lineTo(float x, float y);
  void quadraticCurveTo(float cpx, float cpy, float x, float y);
  void bezierCurveTo(float cp1x,
                     float cp1y,
                     float cp2x,
                     float cp2y,
                     float x,
                     float y);
  void arcTo(float x0,
             float y0,
             float x1,
             float y1,
             float radius,
             ExceptionState&);
  void arc(float x,
           float y,
           float radius,
           float start_angle,
           float end_angle,
           bool anticlockwise,
           ExceptionState&);
  void ellipse(float x,
               float y,
               float radius_x,
               float radius_y,
               float rotation,
               float start_angle,
               float end_angle,
               bool anticlockwise,
               ExceptionState&);
  void rect(float x, float y, float width, float height);

  virtual bool IsTransformInvertible() const { return true; }

 protected:
  CanvasPath() { path_.SetIsVolatile(true); }
  CanvasPath(const Path& path) : path_(path) { path_.SetIsVolatile(true); }
  Path path_;
};
}  // namespace blink

#endif
