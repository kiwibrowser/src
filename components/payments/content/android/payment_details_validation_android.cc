// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "base/android/jni_android.h"
#include "components/payments/content/payment_request_converter.h"
#include "components/payments/core/payment_details.h"
#include "components/payments/core/payment_details_validation.h"
#include "jni/PaymentValidator_jni.h"
#include "third_party/blink/public/platform/modules/payments/payment_request.mojom.h"

namespace payments {

jboolean JNI_PaymentValidator_ValidatePaymentDetailsAndroid(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const base::android::JavaParamRef<jobject>& buffer) {
  jbyte* buf_in = static_cast<jbyte*>(env->GetDirectBufferAddress(buffer));
  jlong buf_size = env->GetDirectBufferCapacity(buffer);
  std::vector<uint8_t> mojo_buffer(buf_size);
  memcpy(&mojo_buffer[0], buf_in, buf_size);
  mojom::PaymentDetailsPtr details;
  if (!mojom::PaymentDetails::Deserialize(std::move(mojo_buffer), &details))
    return false;
  std::string unused_error_message;
  return ValidatePaymentDetails(ConvertPaymentDetails(details),
                                &unused_error_message);
}

}  // namespace payments
