// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_STRUCTURE_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_STRUCTURE_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/form_types.h"
#include "components/autofill/core/browser/proto/server.pb.h"
#include "url/gurl.h"
#include "url/origin.h"

enum UploadRequired { UPLOAD_NOT_REQUIRED, UPLOAD_REQUIRED, USE_UPLOAD_RATES };

namespace base {
class TimeTicks;
}

namespace ukm {
class UkmRecorder;
}

namespace autofill {

// Password attributes (whether a password has special symbols, numeric, etc.)
enum class PasswordAttribute {
  kHasLowercaseLetter,
  kHasUppercaseLetter,
  kHasNumeric,
  kHasSpecialSymbol,
  kPasswordAttributesCount
};

struct FormData;
struct FormDataPredictions;

// FormStructure stores a single HTML form together with the values entered
// in the fields along with additional information needed by Autofill.
class FormStructure {
 public:
  explicit FormStructure(const FormData& form);
  virtual ~FormStructure();

  // Runs several heuristics against the form fields to determine their possible
  // types. If |ukm_recorder| is specified, logs UKM for the form structure
  // corresponding to |source_url_|.
  void DetermineHeuristicTypes(ukm::UkmRecorder* ukm_recorder);

  // Encodes the proto |upload| request from this FormStructure.
  // In some cases, a |login_form_signature| is included as part of the upload.
  // This field is empty when sending upload requests for non-login forms.
  bool EncodeUploadRequest(const ServerFieldTypeSet& available_field_types,
                           bool form_was_autofilled,
                           const std::string& login_form_signature,
                           bool observed_submission,
                           autofill::AutofillUploadContents* upload) const;

  // Encodes the proto |query| request for the set of |forms| that are valid
  // (see implementation for details on which forms are not included in the
  // query). The form signatures used in the Query request are output in
  // |encoded_signatures|. All valid fields are encoded in |query|.
  static bool EncodeQueryRequest(const std::vector<FormStructure*>& forms,
                                 std::vector<std::string>* encoded_signatures,
                                 autofill::AutofillQueryContents* query);

  // Parses the field types from the server query response. |forms| must be the
  // same as the one passed to EncodeQueryRequest when constructing the query.
  static void ParseQueryResponse(std::string response,
                                 const std::vector<FormStructure*>& forms);

  // Returns predictions using the details from the given |form_structures| and
  // their fields' predicted types.
  static std::vector<FormDataPredictions> GetFieldTypePredictions(
      const std::vector<FormStructure*>& form_structures);

  // Returns whether sending autofill field metadata to the server is enabled.
  static bool IsAutofillFieldMetadataEnabled();

  // Return the form signature as string.
  std::string FormSignatureAsStr() const;

  // Runs a quick heuristic to rule out forms that are obviously not
  // auto-fillable, like google/yahoo/msn search, etc.
  bool IsAutofillable() const;

  // Returns whether |this| form represents a complete Credit Card form, which
  // consists in having at least a credit card number field and an expiration
  // field.
  bool IsCompleteCreditCardForm() const;

  // Resets |autofill_count_| and counts the number of auto-fillable fields.
  // This is used when we receive server data for form fields.  At that time,
  // we may have more known fields than just the number of fields we matched
  // heuristically.
  void UpdateAutofillCount();

  // Returns true if this form matches the structural requirements for Autofill.
  bool ShouldBeParsed() const;

  // Returns true if heuristic autofill type detection should be attempted for
  // this form.
  bool ShouldRunHeuristics() const;

  // Returns true if we should query the crowd-sourcing server to determine this
  // form's field types. If the form includes author-specified types, this will
  // return false unless there are password fields in the form. If there are no
  // password fields the assumption is that the author has expressed their
  // intent and crowdsourced data should not be used to override this. Password
  // fields are different because there is no way to specify password generation
  // directly.
  bool ShouldBeQueried() const;

  // Returns true if we should upload votes for this form to the crowd-sourcing
  // server.
  bool ShouldBeUploaded() const;

  // Sets the field types to be those set for |cached_form|.
  void RetrieveFromCache(const FormStructure& cached_form,
                         const bool apply_is_autofilled,
                         const bool only_server_and_autofill_state);

  // Logs quality metrics for |this|, which should be a user-submitted form.
  // This method should only be called after the possible field types have been
  // set for each field.  |interaction_time| should be a timestamp corresponding
  // to the user's first interaction with the form.  |submission_time| should be
  // a timestamp corresponding to the form's submission. |observed_submission|
  // indicates whether this method is called as a result of observing a
  // submission event (otherwise, it may be that an upload was triggered after
  // a form was unfocused or a navigation occurred).
  // TODO(sebsg): We log more than quality metrics. Maybe rename or split
  // function?
  void LogQualityMetrics(
      const base::TimeTicks& load_time,
      const base::TimeTicks& interaction_time,
      const base::TimeTicks& submission_time,
      AutofillMetrics::FormInteractionsUkmLogger* form_interactions_ukm_logger,
      bool did_show_suggestions,
      bool observed_submission) const;

  // Log the quality of the heuristics and server predictions for this form
  // structure, if autocomplete attributes are present on the fields (they are
  // used as golden truths).
  void LogQualityMetricsBasedOnAutocomplete(
      AutofillMetrics::FormInteractionsUkmLogger* form_interactions_ukm_logger)
      const;

  // Classifies each field in |fields_| based upon its |autocomplete| attribute,
  // if the attribute is available.  The association is stored into the field's
  // |heuristic_type|.
  // Fills |has_author_specified_types_| with |true| if the attribute is
  // available and neither empty nor set to the special values "on" or "off" for
  // at least one field.
  // Fills |has_author_specified_sections_| with |true| if the attribute
  // specifies a section for at least one field.
  void ParseFieldTypesFromAutocompleteAttributes();

  // Returns the values that can be filled into the form structure for the
  // given type. For example, there's no way to fill in a value of "The Moon"
  // into ADDRESS_HOME_STATE if the form only has a
  // <select autocomplete="region"> with no "The Moon" option. Returns an
  // empty set if the form doesn't reference the given type or if all inputs
  // are accepted (e.g., <input type="text" autocomplete="region">).
  // All returned values are standardized to upper case.
  std::set<base::string16> PossibleValues(ServerFieldType type);

  // Gets the form's current value for |type|. For example, it may return
  // the contents of a text input or the currently selected <option>.
  base::string16 GetUniqueValue(HtmlFieldType type) const;

  // Rationalize phone number fields in a given section, that is only fill
  // the fields that are considered composing a first complete phone number.
  void RationalizePhoneNumbersInSection(std::string section);

  const AutofillField* field(size_t index) const;
  AutofillField* field(size_t index);
  size_t field_count() const;

  // Returns the number of fields that are part of the form signature and that
  // are included in queries to the Autofill server.
  size_t active_field_count() const;

  // Returns the number of fields that are able to be autofilled.
  size_t autofill_count() const { return autofill_count_; }

  // Used for iterating over the fields.
  std::vector<std::unique_ptr<AutofillField>>::const_iterator begin() const {
    return fields_.begin();
  }
  std::vector<std::unique_ptr<AutofillField>>::const_iterator end() const {
    return fields_.end();
  }

  const base::string16& form_name() const { return form_name_; }

  const GURL& source_url() const { return source_url_; }

  const GURL& target_url() const { return target_url_; }

  const url::Origin& main_frame_origin() const { return main_frame_origin_; }

  bool has_author_specified_types() const {
    return has_author_specified_types_;
  }

  bool has_author_specified_sections() const {
    return has_author_specified_sections_;
  }

  bool has_author_specified_upi_vpa_hint() const {
    return has_author_specified_upi_vpa_hint_;
  }

  void set_upload_required(UploadRequired required) {
    upload_required_ = required;
  }
  UploadRequired upload_required() const { return upload_required_; }

  void set_form_parsed_timestamp(const base::TimeTicks form_parsed_timestamp) {
    form_parsed_timestamp_ = form_parsed_timestamp;
  }
  base::TimeTicks form_parsed_timestamp() const {
    return form_parsed_timestamp_;
  }

  bool all_fields_are_passwords() const { return all_fields_are_passwords_; }

  bool is_signin_upload() const { return is_signin_upload_; }
  void set_is_signin_upload(bool is_signin_upload) {
    is_signin_upload_ = is_signin_upload;
  }

  FormSignature form_signature() const { return form_signature_; }

  // Returns a FormData containing the data this form structure knows about.
  FormData ToFormData() const;

  // Returns the possible form types.
  std::set<FormType> GetFormTypes() const;

  bool passwords_were_revealed() const { return passwords_were_revealed_; }
  void set_passwords_were_revealed(bool passwords_were_revealed) {
    passwords_were_revealed_ = passwords_were_revealed;
  }

  void set_password_attributes_vote(
      const std::pair<PasswordAttribute, bool>& vote) {
    password_attributes_vote_ = vote;
  }
#if defined(UNIT_TEST)
  base::Optional<std::pair<PasswordAttribute, bool>>
  get_password_attributes_vote_for_testing() const {
    return password_attributes_vote_;
  }
#endif

  bool operator==(const FormData& form) const;
  bool operator!=(const FormData& form) const;

  // Returns an identifier that is used by the refill logic. Takes the first non
  // empty of these or returns an empty string:
  // - Form name
  // - Name for Autofill of first field
  base::string16 GetIdentifierForRefill() const;

 private:
  friend class AutofillMergeTest;
  friend class FormStructureTest;
  FRIEND_TEST_ALL_PREFIXES(AutofillDownloadTest, QueryAndUploadTest);
  FRIEND_TEST_ALL_PREFIXES(FormStructureTest, FindLongestCommonPrefix);
  FRIEND_TEST_ALL_PREFIXES(FormStructureTest,
                           RationalizePhoneNumber_RunsOncePerSection);
  // A function to fine tune the credit cards related predictions. For example:
  // lone credit card fields in an otherwise non-credit-card related form is
  // unlikely to be correct, the function will override that prediction.
  void RationalizeCreditCardFieldPredictions();

  // A helper function to review the predictions and do appropriate adjustments
  // when it considers neccessary.
  void RationalizeFieldTypePredictions();

  // Encodes information about this form and its fields into |query_form|.
  void EncodeFormForQuery(
      autofill::AutofillQueryContents::Form* query_form) const;

  // Encodes information about this form and its fields into |upload|.
  void EncodeFormForUpload(autofill::AutofillUploadContents* upload) const;

  // Returns true if the form has no fields, or too many.
  bool IsMalformed() const;

  // Classifies each field in |fields_| into a logical section.
  // Sections are identified by the heuristic that a logical section should not
  // include multiple fields of the same autofill type (with some exceptions, as
  // described in the implementation). Credit card fields also, have a single
  // separate section from address fields.
  // If |has_author_specified_sections| is true, only the second pass --
  // distinguishing credit card sections from non-credit card ones -- is made.
  void IdentifySections(bool has_author_specified_sections);

  // Returns true if field should be skipped when talking to Autofill server.
  bool ShouldSkipField(const FormFieldData& field) const;

  // Further processes the extracted |fields_|.
  void ProcessExtractedFields();

  // Returns the longest common prefix found within |strings|. Strings below a
  // threshold length are excluded when performing this check; this is needed
  // because an exceptional field may be missing a prefix which is otherwise
  // consistently applied--for instance, a framework may only apply a prefix
  // to those fields which are bound when POSTing.
  static base::string16 FindLongestCommonPrefix(
      const std::vector<base::string16>& strings);

  // The name of the form.
  base::string16 form_name_;

  // The source URL.
  GURL source_url_;

  // The target URL.
  GURL target_url_;

  // The origin of the main frame of this form.
  url::Origin main_frame_origin_;

  // The number of fields able to be auto-filled.
  size_t autofill_count_;

  // A vector of all the input fields in the form.
  std::vector<std::unique_ptr<AutofillField>> fields_;

  // The number of fields that are part of the form signature and that are
  // included in queries to the Autofill server.
  size_t active_field_count_;

  // Whether the server expects us to always upload, never upload, or default
  // to the stored upload rates.
  UploadRequired upload_required_;

  // Whether the form includes any field types explicitly specified by the site
  // author, via the |autocompletetype| attribute.
  bool has_author_specified_types_;

  // Whether the form includes any sections explicitly specified by the site
  // author, via the autocomplete attribute.
  bool has_author_specified_sections_;

  // Whether the form includes a field that explicitly sets it autocomplete
  // type to "upi-vpa".
  bool has_author_specified_upi_vpa_hint_;

  // Whether the form was parsed for autocomplete attribute, thus assigning
  // the real values of |has_author_specified_types_| and
  // |has_author_specified_sections_|.
  bool was_parsed_for_autocomplete_attributes_;

  // True if the form contains at least one password field.
  bool has_password_field_;

  // True if the form is a <form>.
  bool is_form_tag_;

  // True if the form is made of unowned fields (i.e., not within a <form> tag)
  // in what appears to be a checkout flow. This attribute is only calculated
  // and used if features::kAutofillRestrictUnownedFieldsToFormlessCheckout is
  // enabled, to prevent heuristics from running on formless non-checkout.
  bool is_formless_checkout_;

  // True if all form fields are password fields.
  bool all_fields_are_passwords_;

  // True if the form is submitted and has 2 fields: one text and one password
  // field.
  bool is_signin_upload_;

  // The unique signature for this form, composed of the target url domain,
  // the form name, and the form field names in a 64-bit hash.
  FormSignature form_signature_;

  // When a form is parsed on this page.
  base::TimeTicks form_parsed_timestamp_;

  // If phone number rationalization has been performed for a given section.
  std::map<std::string, bool> phone_rationalized_;

  // True iff the form is a password form and the user has seen the password
  // value before accepting the prompt to save. Used for crowdsourcing.
  bool passwords_were_revealed_;

  // The vote about password attributes (e.g. whether the password has a numeric
  // character).
  base::Optional<std::pair<PasswordAttribute, bool>> password_attributes_vote_;

  DISALLOW_COPY_AND_ASSIGN(FormStructure);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_STRUCTURE_H_
