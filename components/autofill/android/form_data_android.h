// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_ANDROID_FORM_DATA_ANDROID_H_
#define COMPONENTS_AUTOFILL_ANDROID_FORM_DATA_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "components/autofill/core/common/form_data.h"

namespace autofill {

class FormFieldDataAndroid;

// This class is native peer of FormData.java, to make autofill::FormData
// available in Java.
class FormDataAndroid {
 public:
  FormDataAndroid(const FormData& form);
  virtual ~FormDataAndroid();

  base::android::ScopedJavaLocalRef<jobject> GetJavaPeer();

  // Get autofill values from Java side and return FormData.
  const FormData& GetAutofillValues();

  base::android::ScopedJavaLocalRef<jobject> GetNextFormFieldData(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

  // Get index of given field, return True and index of focus field if found.
  bool GetFieldIndex(const FormFieldData& field, size_t* index);

  // Get index of given field, return True and index of focus field if
  // similar field is found. This method compares less attributes than
  // GetFieldIndex() does, and should be used when field could be changed
  // dynamically, but the changed has no impact on autofill purpose, e.g. css
  // style change, see FormFieldData::SimilarFieldAs() for details.
  bool GetSimilarFieldIndex(const FormFieldData& field, size_t* index);

  // Return true if this form is similar to the given form.
  bool SimilarFormAs(const FormData& form);

  // Invoked when form field which specified by |index| is charged to new
  // |value|.
  void OnFormFieldDidChange(size_t index, const base::string16& value);

  const FormData& form_for_testing() { return form_; }

 private:
  FormData form_;
  std::vector<std::unique_ptr<FormFieldDataAndroid>> fields_;
  JavaObjectWeakGlobalRef java_ref_;
  // keep track of index when popping up fields to Java.
  size_t index_;

  DISALLOW_COPY_AND_ASSIGN(FormDataAndroid);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_ANDROID_FORM_DATA_ANDROID_H_
