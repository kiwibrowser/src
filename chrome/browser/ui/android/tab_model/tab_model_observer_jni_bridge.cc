// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/tab_model/tab_model_observer_jni_bridge.h"

#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_jni_bridge.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/ui/android/tab_model/tab_model_observer.h"
#include "jni/TabModelObserverJniBridge_jni.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/extensions/api/tabs/tabs_event_router.h"
#include "chrome/browser/extensions/api/tabs/tabs_windows_api.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list_observer.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "content/public/browser/web_contents.h"

#include "chrome/browser/sessions/session_tab_helper.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace {

// Gets the size of the provided enum.
int GetTabModelStaticIntField(JNIEnv* env, const char* field_name) {
  jclass clazz =
      env->FindClass("org/chromium/chrome/browser/tabmodel/TabModel");
  jfieldID field_id = env->GetStaticFieldID(clazz, field_name, "I");
  jint value = env->GetStaticIntField(clazz, field_id);
  return value;
}

void EnsureEnumSizesConsistent(JNIEnv* env) {
  DCHECK_EQ(static_cast<int>(TabModel::TabLaunchType::SIZE),
            GetTabModelStaticIntField(env, "TabLaunchTypeSize"));
  DCHECK_EQ(static_cast<int>(TabModel::TabSelectionType::SIZE),
            GetTabModelStaticIntField(env, "TabSelectionTypeSize"));
}

// Converts from a Java TabModel.TabLaunchType to a C++ TabModel::TabLaunchType.
TabModel::TabLaunchType GetTabLaunchType(JNIEnv* env, int type) {
  return static_cast<TabModel::TabLaunchType>(type);
}

// Converts from a Java TabModel.TabSelectionType to a C++
// TabModel::TabSelectionType.
TabModel::TabSelectionType GetTabSelectionType(JNIEnv* env, int type) {
  return static_cast<TabModel::TabSelectionType>(type);
}

}  // namespace

TabModelObserverJniBridge::TabModelObserverJniBridge(
    JNIEnv* env,
    const JavaRef<jobject>& tab_model, Profile* profile) {
  profile_ = profile;
  // TODO(chrisha): Clean up these enums so that the Java ones are generated
  // from them.
  // https://chromium.googlesource.com/chromium/src/+/lkcr/docs/android_accessing_cpp_enums_in_java.md
  EnsureEnumSizesConsistent(env);

  // Create the Java object. This immediately adds it as an observer on the
  // corresponding TabModel.
  java_object_.Reset(Java_TabModelObserverJniBridge_create(
      env, reinterpret_cast<uintptr_t>(this), tab_model));
}

TabModelObserverJniBridge::~TabModelObserverJniBridge() {
  JNIEnv* env = AttachCurrentThread();
  Java_TabModelObserverJniBridge_detachFromTabModel(env, java_object_);
}

void TabModelObserverJniBridge::DidSelectTab(JNIEnv* env,
                                             const JavaParamRef<jobject>& jobj,
                                             const JavaParamRef<jobject>& jtab,
                                             int jtype,
                                             int last_id) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  TabModel::TabSelectionType type = GetTabSelectionType(env, jtype);
  for (auto& observer : observers_)
    observer.DidSelectTab(tab, type);

  Profile *profile = profile_;
  if (profile) {
    extensions::TabsWindowsAPI* tabs_window_api = extensions::TabsWindowsAPI::Get(profile);
    if (tabs_window_api) {
      tabs_window_api->tabs_event_router()->ActiveTabChanged(nullptr, tab->web_contents(), 0, 0);
    }
  }
}

void TabModelObserverJniBridge::WillCloseTab(JNIEnv* env,
                                             const JavaParamRef<jobject>& jobj,
                                             const JavaParamRef<jobject>& jtab,
                                             bool animate) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  if (tab) {
      Profile *profile = profile_;
      if (profile) {
        extensions::TabsWindowsAPI* tabs_window_api = extensions::TabsWindowsAPI::Get(profile);
        if (tabs_window_api) {
           TabModel *tab_strip = nullptr;
           if (!TabModelList::empty())
             tab_strip = *(TabModelList::begin());
           if (tab_strip) {
             for (int i = 0; i < tab_strip->GetTabCount(); ++i) {
               if (tab_strip->GetWebContentsAt(i) == tab->web_contents()) {
                 tabs_window_api->tabs_event_router()->TabClosedAt(nullptr, tab->web_contents(), i, profile);
               }
             }
          }
       }
     }
  }
  for (auto& observer : observers_)
    observer.WillCloseTab(tab, animate);
}

void TabModelObserverJniBridge::DidCloseTab(JNIEnv* env,
                                            const JavaParamRef<jobject>& jobj,
                                            int tab_id,
                                            bool incognito) {
  for (auto& observer : observers_)
    observer.DidCloseTab(tab_id, incognito);
}

void TabModelObserverJniBridge::WillAddTab(JNIEnv* env,
                                           const JavaParamRef<jobject>& jobj,
                                           const JavaParamRef<jobject>& jtab,
                                           int jtype) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  TabModel::TabLaunchType type = GetTabLaunchType(env, jtype);
  for (auto& observer : observers_)
    observer.WillAddTab(tab, type);
}

void TabModelObserverJniBridge::DidAddTab(JNIEnv* env,
                                          const JavaParamRef<jobject>& jobj,
                                          const JavaParamRef<jobject>& jtab,
                                          int jtype) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  TabModel::TabLaunchType type = GetTabLaunchType(env, jtype);
  for (auto& observer : observers_)
    observer.DidAddTab(tab, type);
  if (tab) {
      Profile *profile = profile_;
      if (profile) {
        extensions::TabsWindowsAPI* tabs_window_api = extensions::TabsWindowsAPI::Get(profile);
        if (tabs_window_api) {
           TabModel *tab_strip = nullptr;
           if (!TabModelList::empty())
             tab_strip = *(TabModelList::begin());
           if (tab_strip) {
             int index = tab_strip->GetTabCount() - 1;
             if (index == -1)
               index = 0;
             if (tab->web_contents() != nullptr) {
               tabs_window_api->tabs_event_router()->TabCreatedAt(tab->web_contents(), index, tab_strip->GetActiveWebContents() == tab->web_contents(), profile);
             }
          }
       }
     }
  }
}

void TabModelObserverJniBridge::DidMoveTab(JNIEnv* env,
                                           const JavaParamRef<jobject>& jobj,
                                           const JavaParamRef<jobject>& jtab,
                                           int new_index,
                                           int cur_index) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  for (auto& observer : observers_)
    observer.DidMoveTab(tab, new_index, cur_index);
}

void TabModelObserverJniBridge::TabPendingClosure(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    const JavaParamRef<jobject>& jtab) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  for (auto& observer : observers_)
    observer.TabPendingClosure(tab);
}

void TabModelObserverJniBridge::TabClosureUndone(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    const JavaParamRef<jobject>& jtab) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  for (auto& observer : observers_)
    observer.TabClosureUndone(tab);
}

void TabModelObserverJniBridge::TabClosureCommitted(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    const JavaParamRef<jobject>& jtab) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  for (auto& observer : observers_)
    observer.TabClosureCommitted(tab);
}

void TabModelObserverJniBridge::AllTabsPendingClosure(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj,
    const JavaParamRef<jobjectArray>& jtabs) {
  std::vector<TabAndroid*> tabs;

  // |jtabs| is actually a Tab[]. Iterate over the array and convert it to
  // a vector of TabAndroid*.
  jint size = env->GetArrayLength(jtabs.obj());
  tabs.reserve(size);
  for (jint i = 0; i < size; ++i) {
    jobject jtab = env->GetObjectArrayElement(jtabs.obj(), 0);
    TabAndroid* tab =
        TabAndroid::GetNativeTab(env, JavaParamRef<jobject>(env, jtab));
    if (!tab)
      return;
    tabs.push_back(tab);
  }

  for (auto& observer : observers_)
    observer.AllTabsPendingClosure(tabs);
}

void TabModelObserverJniBridge::AllTabsClosureCommitted(
    JNIEnv* env,
    const JavaParamRef<jobject>& jobj) {
  for (auto& observer : observers_)
    observer.AllTabsClosureCommitted();
}

void TabModelObserverJniBridge::TabRemoved(JNIEnv* env,
                                           const JavaParamRef<jobject>& jobj,
                                           const JavaParamRef<jobject>& jtab) {
  TabAndroid* tab = TabAndroid::GetNativeTab(env, jtab);
  if (!tab)
    return;
  for (auto& observer : observers_)
    observer.TabRemoved(tab);
}

void TabModelObserverJniBridge::AddObserver(TabModelObserver* observer) {
  observers_.AddObserver(observer);
}

void TabModelObserverJniBridge::RemoveObserver(TabModelObserver* observer) {
  observers_.RemoveObserver(observer);
}
