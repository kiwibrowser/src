/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Michelangelo De Simone <micdesim@gmail.com>
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/html/forms/validity_state.h"

namespace blink {

String ValidityState::ValidationMessage() const {
  return control_->validationMessage();
}

bool ValidityState::valueMissing() const {
  return control_->ValueMissing();
}

bool ValidityState::typeMismatch() const {
  return control_->TypeMismatch();
}

bool ValidityState::patternMismatch() const {
  return control_->PatternMismatch();
}

bool ValidityState::tooLong() const {
  return control_->TooLong();
}

bool ValidityState::tooShort() const {
  return control_->TooShort();
}

bool ValidityState::rangeUnderflow() const {
  return control_->RangeUnderflow();
}

bool ValidityState::rangeOverflow() const {
  return control_->RangeOverflow();
}

bool ValidityState::stepMismatch() const {
  return control_->StepMismatch();
}

bool ValidityState::badInput() const {
  return control_->HasBadInput();
}

bool ValidityState::customError() const {
  return control_->CustomError();
}

bool ValidityState::valid() const {
  return control_->Valid();
}

}  // namespace blink
