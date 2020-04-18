// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_TOOLBAR_SCENE_LAYER_H_
#define CHROME_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_TOOLBAR_SCENE_LAYER_H_

#include <memory>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/android/compositor/scene_layer/scene_layer.h"
#include "ui/android/resources/resource_manager_impl.h"

namespace cc {
class Layer;
}

namespace android {

class ToolbarLayer;

class ToolbarSceneLayer : public SceneLayer {
 public:
  ToolbarSceneLayer(JNIEnv* env, const base::android::JavaRef<jobject>& jobj);
  ~ToolbarSceneLayer() override;

  // Update the compositor version of the toolbar.
  void UpdateToolbarLayer(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& object,
      const base::android::JavaParamRef<jobject>& jresource_manager,
      jint toolbar_resource_id,
      jint toolbar_background_color,
      jint url_bar_resource_id,
      jfloat url_bar_alpha,
      jint url_bar_color,
      jfloat y_offset,
      jfloat view_height,
      bool visible,
      bool show_shadow,
      bool modern_design_enabled);

  // Update the progress bar.
  void UpdateProgressBar(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& object,
      jint progress_bar_x,
      jint progress_bar_y,
      jint progress_bar_width,
      jint progress_bar_height,
      jint progress_bar_color,
      jint progress_bar_background_x,
      jint progress_bar_background_y,
      jint progress_bar_background_width,
      jint progress_bar_background_height,
      jint progress_bar_background_color);

  void SetContentTree(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jobj,
      const base::android::JavaParamRef<jobject>& jcontent_tree);

  SkColor GetBackgroundColor() override;

  bool ShouldShowBackground() override;

 private:
  bool should_show_background_;
  SkColor background_color_;
  scoped_refptr<cc::Layer> content_container_;
  scoped_refptr<ToolbarLayer> toolbar_layer_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarSceneLayer);
};

}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_COMPOSITOR_SCENE_LAYER_TOOLBAR_SCENE_LAYER_H_
