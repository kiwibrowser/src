// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_MATRIX_COMPONENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_MATRIX_COMPONENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/css/cssom/css_transform_component.h"
#include "third_party/blink/renderer/core/geometry/dom_matrix.h"
#include "third_party/blink/renderer/core/geometry/dom_matrix_read_only.h"

namespace blink {

class CSSMatrixComponentOptions;

// Represents a matrix value in a CSSTransformValue used for properties like
// "transform".
// See CSSMatrixComponent.idl for more information about this class.
class CORE_EXPORT CSSMatrixComponent final : public CSSTransformComponent {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Constructors defined in the IDL.
  static CSSMatrixComponent* Create(DOMMatrixReadOnly*,
                                    const CSSMatrixComponentOptions&);

  // Blink-internal ways of creating CSSMatrixComponents.
  static CSSMatrixComponent* FromCSSValue(const CSSFunctionValue&);

  // Getters and setters for attributes defined in the IDL.
  DOMMatrix* matrix() { return matrix_.Get(); }
  void setMatrix(DOMMatrix* matrix) { matrix_ = matrix; }

  DOMMatrix* toMatrix(ExceptionState&) const final;

  // Internal methods - from CSSTransformComponent.
  TransformComponentType GetType() const final { return kMatrixType; }
  const CSSFunctionValue* ToCSSValue() const final;

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(matrix_);
    CSSTransformComponent::Trace(visitor);
  }

 private:
  CSSMatrixComponent(DOMMatrixReadOnly* matrix, bool is2D)
      : CSSTransformComponent(is2D), matrix_(DOMMatrix::Create(matrix)) {}

  Member<DOMMatrix> matrix_;
  DISALLOW_COPY_AND_ASSIGN(CSSMatrixComponent);
};

}  // namespace blink

#endif
