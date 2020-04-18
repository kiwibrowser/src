// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_handler.h"

#include "components/autofill/core/common/autofill_data_validation.h"
#include "ui/gfx/geometry/rect_f.h"

namespace autofill {

using base::TimeTicks;

AutofillHandler::AutofillHandler(AutofillDriver* driver) : driver_(driver) {}

AutofillHandler::~AutofillHandler() {}

void AutofillHandler::OnFormSubmitted(const FormData& form,
                                      bool known_success,
                                      SubmissionSource source,
                                      base::TimeTicks timestamp) {
  if (IsValidFormData(form))
    OnFormSubmittedImpl(form, known_success, source, timestamp);
}

void AutofillHandler::OnTextFieldDidChange(const FormData& form,
                                           const FormFieldData& field,
                                           const gfx::RectF& bounding_box,
                                           const TimeTicks timestamp) {
  if (!IsValidFormData(form) || !IsValidFormFieldData(field))
    return;

  gfx::RectF transformed_box =
      driver_->TransformBoundingBoxToViewportCoordinates(bounding_box);

  OnTextFieldDidChangeImpl(form, field, transformed_box, timestamp);
}

void AutofillHandler::OnTextFieldDidScroll(const FormData& form,
                                           const FormFieldData& field,
                                           const gfx::RectF& bounding_box) {
  if (!IsValidFormData(form) || !IsValidFormFieldData(field))
    return;

  gfx::RectF transformed_box =
      driver_->TransformBoundingBoxToViewportCoordinates(bounding_box);

  OnTextFieldDidScrollImpl(form, field, transformed_box);
}

void AutofillHandler::OnSelectControlDidChange(const FormData& form,
                                               const FormFieldData& field,
                                               const gfx::RectF& bounding_box) {
  if (!IsValidFormData(form) || !IsValidFormFieldData(field))
    return;

  gfx::RectF transformed_box =
      driver_->TransformBoundingBoxToViewportCoordinates(bounding_box);

  OnSelectControlDidChangeImpl(form, field, transformed_box);
}

void AutofillHandler::OnQueryFormFieldAutofill(int query_id,
                                               const FormData& form,
                                               const FormFieldData& field,
                                               const gfx::RectF& bounding_box) {
  if (!IsValidFormData(form) || !IsValidFormFieldData(field))
    return;

  gfx::RectF transformed_box =
      driver_->TransformBoundingBoxToViewportCoordinates(bounding_box);

  OnQueryFormFieldAutofillImpl(query_id, form, field, transformed_box);
}

void AutofillHandler::OnFocusOnFormField(const FormData& form,
                                         const FormFieldData& field,
                                         const gfx::RectF& bounding_box) {
  if (!IsValidFormData(form) || !IsValidFormFieldData(field))
    return;

  gfx::RectF transformed_box =
      driver_->TransformBoundingBoxToViewportCoordinates(bounding_box);

  OnFocusOnFormFieldImpl(form, field, transformed_box);
}

void AutofillHandler::SendFormDataToRenderer(
    int query_id,
    AutofillDriver::RendererFormDataAction action,
    const FormData& data) {
  driver_->SendFormDataToRenderer(query_id, action, data);
}

}  // namespace autofill
