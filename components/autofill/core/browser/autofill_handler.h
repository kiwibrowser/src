// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_HANDLER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_HANDLER_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autofill_driver.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/submission_source.h"

namespace gfx {
class RectF;
}

namespace autofill {

struct FormData;
struct FormFieldData;

// This class defines the interface should be implemented by autofill
// implementation in browser side to interact with AutofillDriver.
class AutofillHandler {
 public:
  enum AutofillDownloadManagerState {
    ENABLE_AUTOFILL_DOWNLOAD_MANAGER,
    DISABLE_AUTOFILL_DOWNLOAD_MANAGER,
  };

  virtual ~AutofillHandler();

  // Invoked when the value of textfield is changed.
  void OnTextFieldDidChange(const FormData& form,
                            const FormFieldData& field,
                            const gfx::RectF& bounding_box,
                            const base::TimeTicks timestamp);

  // Invoked when the textfield is scrolled.
  void OnTextFieldDidScroll(const FormData& form,
                            const FormFieldData& field,
                            const gfx::RectF& bounding_box);

  // Invoked when the value of select is changed.
  void OnSelectControlDidChange(const FormData& form,
                                const FormFieldData& field,
                                const gfx::RectF& bounding_box);

  // Invoked when the |form| needs to be autofilled, the |bounding_box| is
  // a window relative value of |field|.
  void OnQueryFormFieldAutofill(int query_id,
                                const FormData& form,
                                const FormFieldData& field,
                                const gfx::RectF& bounding_box);

  // Invoked when |form|'s |field| has focus.
  void OnFocusOnFormField(const FormData& form,
                          const FormFieldData& field,
                          const gfx::RectF& bounding_box);

  // Invoked when |form| has been submitted.
  // Processes the submitted |form|, saving any new Autofill data to the user's
  // personal profile.
  void OnFormSubmitted(const FormData& form,
                       bool known_success,
                       SubmissionSource source,
                       base::TimeTicks timestamp);

  // Invoked when focus is no longer on form.
  virtual void OnFocusNoLongerOnForm() = 0;

  // Invoked when |form| has been filled with the value given by
  // SendFormDataToRenderer.
  virtual void OnDidFillAutofillFormData(const FormData& form,
                                         const base::TimeTicks timestamp) = 0;

  // Invoked when preview autofill value has been shown.
  virtual void OnDidPreviewAutofillFormData() = 0;

  // Invoked when |forms| has been detected.
  virtual void OnFormsSeen(const std::vector<FormData>& forms,
                           const base::TimeTicks timestamp) = 0;

  // Invoked when textfeild editing ended
  virtual void OnDidEndTextFieldEditing() = 0;

  // Invoked when popup window should be hidden.
  virtual void OnHidePopup() = 0;

  // Invoked when data list need to be set.
  virtual void OnSetDataList(const std::vector<base::string16>& values,
                             const std::vector<base::string16>& labels) = 0;

  // Invoked when the options of a select element in the |form| changed.
  virtual void SelectFieldOptionsDidChange(const FormData& form) = 0;

  // Resets cache.
  virtual void Reset() = 0;

  // Send the form |data| to renderer for the specified |action|.
  void SendFormDataToRenderer(int query_id,
                              AutofillDriver::RendererFormDataAction action,
                              const FormData& data);

 protected:
  AutofillHandler(AutofillDriver* driver);

  virtual void OnFormSubmittedImpl(const FormData& form,
                                   bool known_success,
                                   SubmissionSource source,
                                   base::TimeTicks timestamp) = 0;

  virtual void OnTextFieldDidChangeImpl(const FormData& form,
                                        const FormFieldData& field,
                                        const gfx::RectF& bounding_box,
                                        const base::TimeTicks timestamp) = 0;

  virtual void OnTextFieldDidScrollImpl(const FormData& form,
                                        const FormFieldData& field,
                                        const gfx::RectF& bounding_box) = 0;

  virtual void OnQueryFormFieldAutofillImpl(int query_id,
                                            const FormData& form,
                                            const FormFieldData& field,
                                            const gfx::RectF& bounding_box) = 0;

  virtual void OnFocusOnFormFieldImpl(const FormData& form,
                                      const FormFieldData& field,
                                      const gfx::RectF& bounding_box) = 0;

  virtual void OnSelectControlDidChangeImpl(const FormData& form,
                                            const FormFieldData& field,
                                            const gfx::RectF& bounding_box) = 0;

  AutofillDriver* driver() { return driver_; }

 private:
  // Provides driver-level context to the shared code of the component. Must
  // outlive this object.
  AutofillDriver* driver_;

  DISALLOW_COPY_AND_ASSIGN(AutofillHandler);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_HANDLER_H_
