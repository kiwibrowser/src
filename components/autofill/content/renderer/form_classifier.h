// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_CLASSIFIER_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_CLASSIFIER_H_

#include "third_party/blink/public/web/web_form_element.h"

namespace autofill {

// Checks if |form| is eligible for password generation. If yes, sets the name
// of the generation field to |generation_field| and returns true. Otherwise,
// returns false (w/o any changes of |generation_field|).
bool ClassifyFormAndFindGenerationField(const blink::WebFormElement& form,
                                        base::string16* generation_field);
}

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_CLASSIFIER_H_
