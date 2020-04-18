// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_OBSERVER_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_OBSERVER_BRIDGE_H_

#include <jni.h>
#include <stdint.h>

#include <string>

#include "base/android/jni_array.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "net/base/network_change_notifier.h"

namespace base {
class SingleThreadTaskRunner;
class Time;
class TimeTicks;
}

namespace android {

class DataUseTabModel;
class ExternalDataUseObserver;

// ExternalDataUseObserverBridge creates and owns a Java listener object
// that is notified of the data usage observations of Chromium. This class
// receives regular expressions from the Java listener object. Objects of this
// class may may be constructed on any thread safe but are immediately moved to
// the UI thread, and afterwards are accessible only on the UI thread.
class ExternalDataUseObserverBridge {
 public:
  ExternalDataUseObserverBridge();
  virtual ~ExternalDataUseObserverBridge();

  // Initializes |this| on UI thread by constructing the
  // |j_external_data_use_observer_|, and fetches matching rules from
  // |j_external_data_use_observer_|.
  void Init(scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
            base::WeakPtr<ExternalDataUseObserver> external_data_use_observer,
            DataUseTabModel* data_use_tab_model);

  // Fetches matching rules from Java. Returns result asynchronously via
  // FetchMatchingRulesDone. FetchMatchingRules should not be called if a
  // fetch to matching rules is already in progress.
  virtual void FetchMatchingRules() const;

  // Called by Java when new matching rules have been fetched.
  // |app_package_name| is the package name of the app that should be matched.
  // |domain_path_regex| is the regex to be used for matching URLs. |label| is
  // the label that must be applied to data reports corresponding to the
  // matching rule, and must uniquely identify the matching rule. Each element
  // in |label| must have non-zero length. The three vectors should have equal
  // length. The vectors may be empty which implies that no matching rules are
  // active.
  void FetchMatchingRulesDone(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobjectArray>& app_package_name,
      const base::android::JavaParamRef<jobjectArray>& domain_path_regex,
      const base::android::JavaParamRef<jobjectArray>& label);

  // Reports data use to Java. Returns result asynchronously via
  // OnReportDataUseDone. ReportDataUse should not be called if a
  // request to submit data use is already in progress.
  void ReportDataUse(const std::string& label,
                     const std::string& tag,
                     net::NetworkChangeNotifier::ConnectionType connection_type,
                     const std::string& mcc_mnc,
                     base::Time start_time,
                     base::Time end_time,
                     int64_t bytes_downloaded,
                     int64_t bytes_uploaded) const;

  // Called by Java when the reporting of data usage has finished. |success|
  // is true if the request was successfully submitted to the external data
  // use observer by Java.
  void OnReportDataUseDone(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& obj,
                           bool success);

  // Notifies the ExternalDataUseObserverBridge that the external control app is
  // installed or uninstalled. |is_control_app_installed| is true if app is
  // installed.
  void OnControlAppInstallStateChange(JNIEnv* env,
                                      jobject obj,
                                      bool is_control_app_installed) const;

  // Called by DataUseMatcher to notify |external_data_use_observer_| if it
  // should register as a data use observer.
  virtual void ShouldRegisterAsDataUseObserver(bool should_register) const;

  void RegisterGoogleVariationID(bool should_register);

 private:
  // Java listener that provides regular expressions to |this|. Data use
  // reports are submitted to |j_external_data_use_observer_|.
  base::android::ScopedJavaGlobalRef<jobject> j_external_data_use_observer_;

  // |external_data_use_observer_| owns |this|. |external_data_use_observer_| is
  // notified of results from Java code by |this|.
  base::WeakPtr<ExternalDataUseObserver> external_data_use_observer_;

  // |data_use_tab_model_| is notified of the matching rules on UI thread.
  // |data_use_tab_model_| may be null.
  base::WeakPtr<DataUseTabModel> data_use_tab_model_;

  // The construction time of |this|.
  const base::TimeTicks construct_time_;

  // True if matching rules are fetched for the first time.
  bool is_first_matching_rule_fetch_;

  // |io_task_runner_| accesses ExternalDataUseObserver members on IO thread.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ExternalDataUseObserverBridge);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_EXTERNAL_DATA_USE_OBSERVER_BRIDGE_H_
