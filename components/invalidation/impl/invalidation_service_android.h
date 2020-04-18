// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_ANDROID_H_
#define COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_ANDROID_H_

#include <jni.h>
#include <stdint.h>

#include <map>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/sequence_checker.h"
#include "components/invalidation/impl/invalidation_logger.h"
#include "components/invalidation/impl/invalidator_registrar.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/keyed_service/core/keyed_service.h"

namespace invalidation {

class InvalidationLogger;

// This InvalidationService is used to deliver invalidations on Android.  The
// Android operating system has its own mechanisms for delivering invalidations.
class InvalidationServiceAndroid : public InvalidationService {
 public:
  explicit InvalidationServiceAndroid();
  ~InvalidationServiceAndroid() override;

  // InvalidationService implementation.
  //
  // Note that this implementation does not properly support Ack-tracking,
  // fetching the invalidator state, or querying the client's ID.  Support for
  // exposing the client ID should be available soon; see crbug.com/172391.
  void RegisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  bool UpdateRegisteredInvalidationIds(syncer::InvalidationHandler* handler,
                                       const syncer::ObjectIdSet& ids) override;
  void UnregisterInvalidationHandler(
      syncer::InvalidationHandler* handler) override;
  syncer::InvalidatorState GetInvalidatorState() const override;
  std::string GetInvalidatorClientId() const override;
  InvalidationLogger* GetInvalidationLogger() override;
  void RequestDetailedStatus(
      base::Callback<void(const base::DictionaryValue&)> caller) const override;

  void Invalidate(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj,
                  jint object_source,
                  const base::android::JavaParamRef<jstring>& object_id,
                  jlong version,
                  const base::android::JavaParamRef<jstring>& state);

  // The InvalidationServiceAndroid always reports that it is enabled.
  // This is used only by unit tests.
  void TriggerStateChangeForTest(syncer::InvalidatorState state);

 private:
  typedef std::map<invalidation::ObjectId, int64_t, syncer::ObjectIdLessThan>
      ObjectIdVersionMap;

  // Friend class so that InvalidationServiceFactoryAndroid has access to
  // private member object java_ref_.
  friend class InvalidationServiceFactoryAndroid;

  // Points to a Java instance of InvalidationService.
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  syncer::InvalidatorRegistrar invalidator_registrar_;
  syncer::InvalidatorState invalidator_state_;

  // The invalidation API spec allows for the possibility of redundant
  // invalidations, so keep track of the max versions and drop
  // invalidations with old versions.
  ObjectIdVersionMap max_invalidation_versions_;

  // The invalidation logger object we use to record state changes
  // and invalidations.
  InvalidationLogger logger_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(InvalidationServiceAndroid);
};

}  // namespace invalidation

#endif  // COMPONENTS_INVALIDATION_IMPL_INVALIDATION_SERVICE_ANDROID_H_
