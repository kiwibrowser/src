// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_CHANGE_H__
#define COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_CHANGE_H__

#include <string>
#include <vector>

#include "base/logging.h"
#include "components/autofill/core/browser/webdata/autofill_entry.h"

namespace autofill {

class AutofillProfile;
class CreditCard;

// For classic Autofill form fields, the KeyType is AutofillKey.
// Autofill++ types such as AutofillProfile and CreditCard simply use a string.
template <typename KeyType>
class GenericAutofillChange {
 public:
  enum Type {
    ADD,
    UPDATE,
    REMOVE
  };

  virtual ~GenericAutofillChange() {}

  Type type() const { return type_; }
  const KeyType& key() const { return key_; }

 protected:
  GenericAutofillChange(Type type, const KeyType& key)
      : type_(type),
        key_(key) {}
 private:
  Type type_;
  KeyType key_;
};

class AutofillChange : public GenericAutofillChange<AutofillKey> {
 public:
  AutofillChange(Type type, const AutofillKey& key);
  ~AutofillChange() override;
  bool operator==(const AutofillChange& change) const {
    return type() == change.type() && key() == change.key();
  }
};

typedef std::vector<AutofillChange> AutofillChangeList;

// Change notification details for Autofill profile or credit card changes.
template <typename DataType>
class AutofillDataModelChange : public GenericAutofillChange<std::string> {
 public:
  // The |type| input specifies the change type.  The |key| input is the key,
  // which is expected to be the GUID identifying the |data_model|.
  // When |type| == ADD, |data_model| should be non-NULL.
  // When |type| == UPDATE, |data_model| should be non-NULL.
  // When |type| == REMOVE, |data_model| should be NULL.
  AutofillDataModelChange(Type type,
                          const std::string& key,
                          const DataType* data_model)
      : GenericAutofillChange<std::string>(type, key), data_model_(data_model) {
    DCHECK(type == REMOVE ? !data_model
                          : data_model && data_model->guid() == key);
  }

  ~AutofillDataModelChange() override {}

  const DataType* data_model() const { return data_model_; }

  bool operator==(const AutofillDataModelChange<DataType>& change) const {
    return type() == change.type() && key() == change.key() &&
           (type() == REMOVE || *data_model() == *change.data_model());
  }

 private:
  // Weak reference, can be NULL.
  const DataType* data_model_;
};

typedef AutofillDataModelChange<AutofillProfile> AutofillProfileChange;
typedef AutofillDataModelChange<CreditCard> CreditCardChange;

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_WEBDATA_AUTOFILL_CHANGE_H__
