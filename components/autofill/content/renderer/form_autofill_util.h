// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_AUTOFILL_UTIL_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_AUTOFILL_UTIL_H_

#include <stddef.h>

#include <set>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/autofill/content/renderer/password_form_conversion_utils.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/password_form_field_prediction_map.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "ui/gfx/geometry/rect_f.h"

class GURL;

namespace blink {
class WebDocument;
class WebElement;
class WebFormControlElement;
class WebFormElement;
class WebInputElement;
class WebLocalFrame;
class WebNode;
}

namespace autofill {

struct FormData;
struct FormFieldData;

namespace form_util {

// A bit field mask to extract data from WebFormControlElement.
// Copied to components/autofill/ios/browser/resources/autofill_controller.js.
enum ExtractMask {
  EXTRACT_NONE        = 0,
  EXTRACT_VALUE       = 1 << 0,  // Extract value from WebFormControlElement.
  EXTRACT_OPTION_TEXT = 1 << 1,  // Extract option text from
                                 // WebFormSelectElement. Only valid when
                                 // |EXTRACT_VALUE| is set.
                                 // This is used for form submission where
                                 // human readable value is captured.
  EXTRACT_OPTIONS     = 1 << 2,  // Extract options from
                                 // WebFormControlElement.
};

// The maximum number of form fields we are willing to parse, due to
// computational costs.  Several examples of forms with lots of fields that are
// not relevant to Autofill: (1) the Netflix queue; (2) the Amazon wishlist;
// (3) router configuration pages; and (4) other configuration pages, e.g. for
// Google code project settings.
// Copied to components/autofill/ios/browser/resources/autofill_controller.js.
extern const size_t kMaxParseableFields;

// Create an instance of ScopedLayoutPreventer to stop form_util code from
// triggering layout as a side-effect. For example, when creating a
// FormFieldData, a call to WebNode::isFocusable is normally made, which may
// trigger a layout computation. When an instance of ScopedLayoutPreventer is
// alive, that call will not be made. On destruction, this class allows
// layout-triggering calls again. It is not thread safe and multiple instances
// should not be created at the same time in the same process.
class ScopedLayoutPreventer {
 public:
  ScopedLayoutPreventer();
  ~ScopedLayoutPreventer();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedLayoutPreventer);
};

// Helper function that strips any authentication data, as well as query and
// ref portions of URL
GURL StripAuthAndParams(const GURL& gurl);

// Extract FormData from the form element and return whether the operation was
// successful.
bool ExtractFormData(const blink::WebFormElement& form_element, FormData* data);

// Helper function to check if there exist any visible form on |frame| which
// equals |form_element|. If |form_element| is null, checks if forms action
// equals |action|. Returns true if so. For forms with empty or unspecified
// actions, all form data are used for comparison.
bool IsFormVisible(blink::WebLocalFrame* frame,
                   const blink::WebFormElement& form_element,
                   const GURL& action,
                   const GURL& origin,
                   const FormData& form_data);

// Returns true if at least one element from |control_elements| is visible.
bool IsSomeControlElementVisible(
    const blink::WebVector<blink::WebFormControlElement>& control_elements);

// Returns true if some control elements of |form| are visible.
bool AreFormContentsVisible(const blink::WebFormElement& form);

// Helper functions to assist in getting the canonical form of the action and
// origin. The action will proplerly take into account <BASE>, and both will
// strip unnecessary data (e.g. query params and HTTP credentials).
GURL GetCanonicalActionForForm(const blink::WebFormElement& form);
GURL GetCanonicalOriginForDocument(const blink::WebDocument& document);

// Returns true if |element| is a month input element.
bool IsMonthInput(const blink::WebInputElement* element);

// Returns true if |element| is a text input element.
bool IsTextInput(const blink::WebInputElement* element);

// Returns true if |element| is a select element.
bool IsSelectElement(const blink::WebFormControlElement& element);

// Returns true if |element| is a textarea element.
bool IsTextAreaElement(const blink::WebFormControlElement& element);

// Returns true if |element| is a checkbox or a radio button element.
bool IsCheckableElement(const blink::WebInputElement* element);

// Returns true if |element| is one of the input element types that can be
// autofilled. {Text, Radiobutton, Checkbox}.
bool IsAutofillableInputElement(const blink::WebInputElement* element);

// Returns true if |element| is one of the element types that can be autofilled.
// {Text, Radiobutton, Checkbox, Select, TextArea}.
bool IsAutofillableElement(const blink::WebFormControlElement& element);

// True if this node can take focus. If layout is blocked, then the function
// checks if the element takes up space in the layout, ie. this element or a
// descendant has a non-empty bounding bounding client rect.
bool IsWebElementVisible(const blink::WebElement& element);

// Returns the form's |name| attribute if non-empty; otherwise the form's |id|
// attribute.
const base::string16 GetFormIdentifier(const blink::WebFormElement& form);

// Returns all the auto-fillable form control elements in |control_elements|.
std::vector<blink::WebFormControlElement> ExtractAutofillableElementsFromSet(
    const blink::WebVector<blink::WebFormControlElement>& control_elements);

// Returns all the auto-fillable form control elements in |form_element|.
std::vector<blink::WebFormControlElement> ExtractAutofillableElementsInForm(
    const blink::WebFormElement& form_element);

// Fills out a FormField object from a given WebFormControlElement.
// |extract_mask|: See the enum ExtractMask above for details. Field properties
// will be copied from |field_value_and_properties_map|, if the argument is not
// null and has entry for |element| (see properties in FieldPropertiesFlags).
void WebFormControlElementToFormField(
    const blink::WebFormControlElement& element,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    ExtractMask extract_mask,
    FormFieldData* field);

// Fills |form| with the FormData object corresponding to the |form_element|.
// If |field| is non-NULL, also fills |field| with the FormField object
// corresponding to the |form_control_element|. |extract_mask| controls what
// data is extracted. Returns true if |form| is filled out.  Also returns false
// if there are no fields or too many fields in the |form|. Field properties
// will be copied from |field_value_and_properties_map|, if the argument is not
// null and has entry for |element| (see properties in FieldPropertiesFlags).
bool WebFormElementToFormData(
    const blink::WebFormElement& form_element,
    const blink::WebFormControlElement& form_control_element,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    ExtractMask extract_mask,
    FormData* form,
    FormFieldData* field);

// Get all form control elements from |elements| that are not part of a form.
// If |fieldsets| is not NULL, also append the fieldsets encountered that are
// not part of a form.
std::vector<blink::WebFormControlElement> GetUnownedFormFieldElements(
    const blink::WebElementCollection& elements,
    std::vector<blink::WebElement>* fieldsets);

// A shorthand for filtering the results of GetUnownedFormFieldElements with
// ExtractAutofillableElementsFromSet.
std::vector<blink::WebFormControlElement>
GetUnownedAutofillableFormFieldElements(
    const blink::WebElementCollection& elements,
    std::vector<blink::WebElement>* fieldsets);

// Fills |form| with the form data derived from |fieldsets|, |control_elements|
// and |origin|. If |field| is non-NULL, fill it with the FormField
// representation for |element|.
// |extract_mask| usage and the return value are the same as
// WebFormElementToFormData() above.
// This function will return false and not perform any extraction if
// |document| does not pertain to checkout.
bool UnownedCheckoutFormElementsAndFieldSetsToFormData(
    const std::vector<blink::WebElement>& fieldsets,
    const std::vector<blink::WebFormControlElement>& control_elements,
    const blink::WebFormControlElement* element,
    const blink::WebDocument& document,
    ExtractMask extract_mask,
    FormData* form,
    FormFieldData* field);

// Same as above, but without the requirement that the elements only be
// related to checkout. Field properties of |control_elements| will be copied
// from |field_value_and_properties_map|, if the argument is not null and has
// corresponding entries (see properties in FieldPropertiesFlags).
bool UnownedPasswordFormElementsAndFieldSetsToFormData(
    const std::vector<blink::WebElement>& fieldsets,
    const std::vector<blink::WebFormControlElement>& control_elements,
    const blink::WebFormControlElement* element,
    const blink::WebDocument& document,
    const FieldValueAndPropertiesMaskMap* field_value_and_properties_map,
    ExtractMask extract_mask,
    FormData* form,
    FormFieldData* field);

// Finds the form that contains |element| and returns it in |form|.  If |field|
// is non-NULL, fill it with the FormField representation for |element|.
// Returns false if the form is not found or cannot be serialized.
bool FindFormAndFieldForFormControlElement(
    const blink::WebFormControlElement& element,
    FormData* form,
    FormFieldData* field);

// Fills the form represented by |form|.  |element| is the input element that
// initiated the auto-fill process.
void FillForm(const FormData& form,
              const blink::WebFormControlElement& element);

// Fills focusable and non-focusable form control elements within |form_element|
// with field data from |form_data|.
void FillFormIncludingNonFocusableElements(
    const FormData& form_data,
    const blink::WebFormElement& form_element);

// Previews the form represented by |form|.  |element| is the input element that
// initiated the preview process.
void PreviewForm(const FormData& form,
                 const blink::WebFormControlElement& element);

// Clears the placeholder values and the auto-filled background for any fields
// in the form containing |node| that have been previewed.  Resets the
// autofilled state of |node| to |was_autofilled|.  Returns false if the form is
// not found.
bool ClearPreviewedFormWithElement(const blink::WebFormControlElement& element,
                                   bool was_autofilled);

// Checks if the webpage is empty.
// This kind of webpage is considered as empty:
// <html>
//    <head>
//    <head/>
//    <body>
//    <body/>
// <html/>
// Meta, script and title tags don't influence the emptiness of a webpage.
bool IsWebpageEmpty(const blink::WebLocalFrame* frame);

// This function checks whether the children of |element|
// are of the type <script>, <meta>, or <title>.
bool IsWebElementEmpty(const blink::WebElement& element);

// Previews |suggestion| in |input_element| and highlights the suffix of
// |suggestion| not included in the |input_element| text. |input_element| must
// not be null. |user_input| should be the text typed by the user into
// |input_element|. Note that |user_input| cannot be easily derived from
// |input_element| by calling value(), because of http://crbug.com/507714.
void PreviewSuggestion(const base::string16& suggestion,
                       const base::string16& user_input,
                       blink::WebFormControlElement* input_element);

// Returns the aggregated values of the descendants of |element| that are
// non-empty text nodes.  This is a faster alternative to |innerText()| for
// performance critical operations.  It does a full depth-first search so can be
// used when the structure is not directly known.  However, unlike with
// |innerText()|, the search depth and breadth are limited to a fixed threshold.
// Whitespace is trimmed from text accumulated at descendant nodes.
base::string16 FindChildText(const blink::WebNode& node);

// Exposed for testing purpose
base::string16 FindChildTextWithIgnoreListForTesting(
    const blink::WebNode& node,
    const std::set<blink::WebNode>& divs_to_skip);
bool InferLabelForElementForTesting(const blink::WebFormControlElement& element,
                                    const std::vector<base::char16>& stop_words,
                                    base::string16* label,
                                    FormFieldData::LabelSource* label_source);

}  // namespace form_util
}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_FORM_AUTOFILL_UTIL_H_
