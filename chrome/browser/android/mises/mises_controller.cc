#include "chrome/browser/android/mises/mises_controller.h"
#include <jni.h>
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/MisesController_jni.h"
#include "base/memory/singleton.h"

namespace android {

// static
MisesController* MisesController::GetInstance() {
  return base::Singleton<MisesController>::get();
}

MisesController::MisesController() {}

MisesController::~MisesController() {}

void MisesController::setMisesUserInfo(const std::string& info) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> j_mises_json = base::android::ConvertUTF8ToJavaString(env, info);
  Java_MisesController_setMisesUserInfo(env, j_mises_json);
}

} // namespace android