// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_CACHE_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_CACHE_H_

#include <stddef.h>

#include <map>
#include <set>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/autofill/core/common/form_data.h"

namespace blink {
class WebFormControlElement;
class WebInputElement;
class WebLocalFrame;
class WebSelectElement;
}

namespace autofill {

struct FormDataPredictions;

// Manages the forms in a single RenderFrame.
class FormCache {
 public:
  explicit FormCache(blink::WebLocalFrame* frame);
  ~FormCache();

  // Scans the DOM in |frame_| extracting and storing forms that have not been
  // seen before. Returns the extracted forms.
  std::vector<FormData> ExtractNewForms();

  // Resets the forms.
  void Reset();

  // Clears the values of all input elements in the section of the form that
  // contains |element|.  Returns false if the form is not found.
  bool ClearSectionWithElement(const blink::WebFormControlElement& element);

  // For each field in the |form|, if |attach_predictions_to_dom| is true, sets
  // the title to include the field's heuristic type, server type, and
  // signature; as well as the form's signature and the experiment id for the
  // server predictions. In all cases, may emit console warnings regarding the
  // use of autocomplete attributes.
  bool ShowPredictions(const FormDataPredictions& form,
                       bool attach_predictions_to_dom);

 private:
  // Scans |control_elements| and returns the number of editable elements.
  // Also remembers the initial <select> and <input> element states, and
  // logs warning messages for deprecated attribute if
  // |log_deprecation_messages| is set.
  size_t ScanFormControlElements(
      const std::vector<blink::WebFormControlElement>& control_elements,
      bool log_deprecation_messages);

  // Saves initial state of checkbox and select elements.
  void SaveInitialValues(
      const std::vector<blink::WebFormControlElement>& control_elements);

  // The frame this FormCache is associated with. Weak reference.
  blink::WebLocalFrame* frame_;

  // The cached forms. Used to prevent re-extraction of forms.
  std::set<FormData> parsed_forms_;

  // The synthetic FormData is for all the fieldsets in the document without a
  // form owner.
  FormData synthetic_form_;

  // The cached initial values for <select> elements.
  std::map<const blink::WebSelectElement, base::string16>
      initial_select_values_;

  // The cached initial values for checkable <input> elements.
  std::map<const blink::WebInputElement, bool> initial_checked_state_;

  DISALLOW_COPY_AND_ASSIGN(FormCache);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_CACHE_H_
