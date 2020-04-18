// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/android/data_usage/data_use_ui_tab_model.h"
#include "chrome/browser/android/data_usage/data_use_ui_tab_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/grit/generated_resources.h"
#include "components/sessions/core/session_id.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "jni/DataUseTabUIManager_jni.h"
#include "net/android/network_library.h"
#include "net/base/network_change_notifier.h"
#include "ui/base/l10n/l10n_util.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace {

// Represents the IDs for string messages used by data use snackbar and
// interstitial dialog. A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.datausage
enum DataUseUIMessage {
  DATA_USE_TRACKING_STARTED_SNACKBAR_MESSAGE,
  DATA_USE_TRACKING_SNACKBAR_ACTION,
  DATA_USE_TRACKING_ENDED_SNACKBAR_MESSAGE,
  DATA_USE_TRACKING_ENDED_TITLE,
  DATA_USE_TRACKING_ENDED_MESSAGE,
  DATA_USE_TRACKING_ENDED_CHECKBOX_MESSAGE,
  DATA_USE_TRACKING_ENDED_CONTINUE,
  DATA_USE_LEARN_MORE_TITLE,
  DATA_USE_LEARN_MORE_LINK_URL,
  DATA_USE_UI_MESSAGE_MAX
};

// Represents the mapping between DataUseUIMessage ID and UI message IDs in
// generated_resources.grd
uint32_t data_use_ui_message_id_map[DATA_USE_UI_MESSAGE_MAX] = {
        [DATA_USE_TRACKING_STARTED_SNACKBAR_MESSAGE] =
            IDS_DATA_USE_TRACKING_STARTED_SNACKBAR_MESSAGE,
        [DATA_USE_TRACKING_SNACKBAR_ACTION] =
            IDS_DATA_USE_TRACKING_SNACKBAR_ACTION,
        [DATA_USE_TRACKING_ENDED_SNACKBAR_MESSAGE] =
            IDS_DATA_USE_TRACKING_ENDED_SNACKBAR_MESSAGE,
        [DATA_USE_TRACKING_ENDED_TITLE] = IDS_DATA_USE_TRACKING_ENDED_TITLE,
        [DATA_USE_TRACKING_ENDED_MESSAGE] = IDS_DATA_USE_TRACKING_ENDED_MESSAGE,
        [DATA_USE_TRACKING_ENDED_CHECKBOX_MESSAGE] =
            IDS_DATA_USE_TRACKING_ENDED_CHECKBOX_MESSAGE,
        [DATA_USE_TRACKING_ENDED_CONTINUE] =
            IDS_DATA_USE_TRACKING_ENDED_CONTINUE,
        [DATA_USE_LEARN_MORE_TITLE] = IDS_LEARN_MORE,
        [DATA_USE_LEARN_MORE_LINK_URL] = IDS_DATA_USE_LEARN_MORE_LINK_URL,
};

}  // namespace

// static
jboolean JNI_DataUseTabUIManager_CheckAndResetDataUseTrackingStarted(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jint tab_id,
    const JavaParamRef<jobject>& jprofile) {
  SessionID casted_tab_id = SessionID::FromSerializedValue(tab_id);
  DCHECK(casted_tab_id.is_valid());

  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(profile);
  if (!data_use_ui_tab_model)
    return false;

  return data_use_ui_tab_model->CheckAndResetDataUseTrackingStarted(
      casted_tab_id);
}

// static
jboolean JNI_DataUseTabUIManager_CheckAndResetDataUseTrackingEnded(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jint tab_id,
    const JavaParamRef<jobject>& jprofile) {
  SessionID casted_tab_id = SessionID::FromSerializedValue(tab_id);
  DCHECK(casted_tab_id.is_valid());

  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(profile);
  if (!data_use_ui_tab_model)
    return false;

  return data_use_ui_tab_model->CheckAndResetDataUseTrackingEnded(
      casted_tab_id);
}

// static
void JNI_DataUseTabUIManager_UserClickedContinueOnDialogBox(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jint tab_id,
    const JavaParamRef<jobject>& jprofile) {
  SessionID casted_tab_id = SessionID::FromSerializedValue(tab_id);
  DCHECK(casted_tab_id.is_valid());

  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(profile);
  if (!data_use_ui_tab_model)
    return;

  data_use_ui_tab_model->UserClickedContinueOnDialogBox(casted_tab_id);
}

// static
jboolean JNI_DataUseTabUIManager_WouldDataUseTrackingEnd(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jobject>& j_web_contents,
    jint tab_id,
    const JavaParamRef<jstring>& url,
    jint transition_type,
    const JavaParamRef<jobject>& jprofile) {
  SessionID casted_tab_id = SessionID::FromSerializedValue(tab_id);
  DCHECK(casted_tab_id.is_valid());
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(j_web_contents);
  DCHECK(web_contents);

  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(profile);
  if (!data_use_ui_tab_model)
    return false;

  return data_use_ui_tab_model->WouldDataUseTrackingEnd(
      ConvertJavaStringToUTF8(env, url), transition_type, casted_tab_id,
      web_contents->GetController().GetPendingEntry());
}

// static
void JNI_DataUseTabUIManager_OnCustomTabInitialNavigation(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jint tab_id,
    const JavaParamRef<jstring>& jpackage_name,
    const JavaParamRef<jstring>& jurl,
    const JavaParamRef<jobject>& jprofile) {
  SessionID casted_tab_id = SessionID::FromSerializedValue(tab_id);
  DCHECK(casted_tab_id.is_valid());

  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  android::DataUseUITabModel* data_use_ui_tab_model =
      android::DataUseUITabModelFactory::GetForBrowserContext(profile);
  if (!data_use_ui_tab_model)
    return;

  std::string url;
  if (!jurl.is_null())
    ConvertJavaStringToUTF8(env, jurl, &url);

  std::string package_name;
  if (!jpackage_name.is_null())
    ConvertJavaStringToUTF8(env, jpackage_name, &package_name);

  data_use_ui_tab_model->ReportCustomTabInitialNavigation(casted_tab_id,
                                                          package_name, url);
}

// static
ScopedJavaLocalRef<jstring> JNI_DataUseTabUIManager_GetDataUseUIString(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    int message_id) {
  DCHECK_GE(message_id, 0);
  DCHECK_LT(message_id, DATA_USE_UI_MESSAGE_MAX);
  return base::android::ConvertUTF8ToJavaString(
      env, l10n_util::GetStringUTF8(data_use_ui_message_id_map[message_id]));
}

// static
jboolean JNI_DataUseTabUIManager_IsNonRoamingCellularConnection(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return net::NetworkChangeNotifier::IsConnectionCellular(
             net::NetworkChangeNotifier::GetConnectionType()) &&
         !net::android::GetIsRoaming();
}
