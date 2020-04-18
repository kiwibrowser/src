/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/svg/svg_matrix_tear_off.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg/svg_transform_tear_off.h"

namespace blink {

SVGMatrixTearOff::SVGMatrixTearOff(const AffineTransform& static_value)
    : static_value_(static_value) {}

SVGMatrixTearOff::SVGMatrixTearOff(SVGTransformTearOff* transform)
    : context_transform_(transform) {
  DCHECK(transform);
}

void SVGMatrixTearOff::Trace(blink::Visitor* visitor) {
  visitor->Trace(context_transform_);
  ScriptWrappable::Trace(visitor);
}

void SVGMatrixTearOff::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(context_transform_);
  ScriptWrappable::TraceWrappers(visitor);
}

const AffineTransform& SVGMatrixTearOff::Value() const {
  return context_transform_ ? context_transform_->Target()->Matrix()
                            : static_value_;
}

AffineTransform* SVGMatrixTearOff::MutableValue() {
  return context_transform_ ? context_transform_->Target()->MutableMatrix()
                            : &static_value_;
}

void SVGMatrixTearOff::CommitChange() {
  if (!context_transform_)
    return;

  context_transform_->Target()->OnMatrixChange();
  context_transform_->CommitChange();
}

#define DEFINE_SETTER(ATTRIBUTE)                                          \
  void SVGMatrixTearOff::set##ATTRIBUTE(double f,                         \
                                        ExceptionState& exceptionState) { \
    if (context_transform_ && context_transform_->IsImmutable()) {        \
      SVGPropertyTearOffBase::ThrowReadOnly(exceptionState);              \
      return;                                                             \
    }                                                                     \
    MutableValue()->Set##ATTRIBUTE(f);                                    \
    CommitChange();                                                       \
  }

DEFINE_SETTER(A);
DEFINE_SETTER(B);
DEFINE_SETTER(C);
DEFINE_SETTER(D);
DEFINE_SETTER(E);
DEFINE_SETTER(F);

#undef DEFINE_SETTER

SVGMatrixTearOff* SVGMatrixTearOff::translate(double tx, double ty) {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->Translate(tx, ty);
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::scale(double s) {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->Scale(s, s);
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::scaleNonUniform(double sx, double sy) {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->Scale(sx, sy);
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::rotate(double d) {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->Rotate(d);
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::flipX() {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->FlipX();
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::flipY() {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->FlipY();
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::skewX(double angle) {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->SkewX(angle);
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::skewY(double angle) {
  SVGMatrixTearOff* matrix = Create(Value());
  matrix->MutableValue()->SkewY(angle);
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::multiply(SVGMatrixTearOff* other) {
  SVGMatrixTearOff* matrix = Create(Value());
  *matrix->MutableValue() *= other->Value();
  return matrix;
}

SVGMatrixTearOff* SVGMatrixTearOff::inverse(ExceptionState& exception_state) {
  if (!Value().IsInvertible()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "The matrix is not invertible.");
    return nullptr;
  }
  return Create(Value().Inverse());
}

SVGMatrixTearOff* SVGMatrixTearOff::rotateFromVector(
    double x,
    double y,
    ExceptionState& exception_state) {
  if (!x || !y) {
    exception_state.ThrowDOMException(kInvalidAccessError,
                                      "Arguments cannot be zero.");
    return nullptr;
  }
  AffineTransform copy = Value();
  copy.RotateFromVector(x, y);
  return Create(copy);
}

}  // namespace blink
