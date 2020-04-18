// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_ANDROID_FORM_FIELD_DATA_ANDROID_H_
#define COMPONENTS_AUTOFILL_ANDROID_FORM_FIELD_DATA_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "components/autofill/core/common/form_field_data.h"

namespace autofill {

// This class is native peer of FormFieldData.java, makes
// autofill::FormFieldData available in Java.
class FormFieldDataAndroid {
 public:
  FormFieldDataAndroid(FormFieldData* field);
  virtual ~FormFieldDataAndroid() {}

  base::android::ScopedJavaLocalRef<jobject> GetJavaPeer();
  void GetValue();
  void OnFormFieldDidChange(const base::string16& value);

 private:
  // Not owned.
  FormFieldData* field_ptr_;
  JavaObjectWeakGlobalRef java_ref_;

  DISALLOW_COPY_AND_ASSIGN(FormFieldDataAndroid);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_ANDROID_FORM_FIELD_DATA_ANDROID_H_
