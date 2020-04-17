/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "restart_worker.h"
#include "jni/RestartWorker_jni.h"
#include "chrome/browser/lifetime/application_lifetime.h"


static void JNI_RestartWorker_Restart(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj) {
    chrome::AttemptRestart();
}
