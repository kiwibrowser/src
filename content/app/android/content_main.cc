// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/lazy_instance.h"
#include "base/trace_event/trace_event.h"
#include "content/app/content_service_manager_main_delegate.h"
#include "content/public/app/content_main.h"
#include "content/public/app/content_main_delegate.h"
#include "jni/ContentMain_jni.h"
#include "services/service_manager/embedder/main.h"

using base::LazyInstance;
using base::android::JavaParamRef;

namespace content {

namespace {

LazyInstance<std::unique_ptr<service_manager::MainDelegate>>::DestructorAtExit
    g_service_manager_main_delegate = LAZY_INSTANCE_INITIALIZER;

LazyInstance<std::unique_ptr<ContentMainDelegate>>::DestructorAtExit
    g_content_main_delegate = LAZY_INSTANCE_INITIALIZER;

}  // namespace

static jint JNI_ContentMain_Start(JNIEnv* env,
                                  const JavaParamRef<jclass>& clazz) {
  TRACE_EVENT0("startup", "content::Start");

  DCHECK(!g_service_manager_main_delegate.Get());
  g_service_manager_main_delegate.Get() =
      std::make_unique<ContentServiceManagerMainDelegate>(
          ContentMainParams(g_content_main_delegate.Get().get()));

  service_manager::MainParams main_params(
      g_service_manager_main_delegate.Get().get());
  return service_manager::Main(main_params);
}

void SetContentMainDelegate(ContentMainDelegate* delegate) {
  DCHECK(!g_content_main_delegate.Get().get());
  g_content_main_delegate.Get().reset(delegate);
}

}  // namespace content
