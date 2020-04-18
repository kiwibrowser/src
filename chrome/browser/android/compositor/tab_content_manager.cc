// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/compositor/tab_content_manager.h"

#include <android/bitmap.h>
#include <stddef.h>

#include <memory>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "cc/layers/layer.h"
#include "chrome/browser/android/compositor/layer/thumbnail_layer.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/android/thumbnail/thumbnail.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "jni/TabContentManager_jni.h"
#include "ui/android/resources/ui_resource_provider.h"
#include "ui/android/view_android.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

using base::android::JavaParamRef;
using base::android::JavaRef;

namespace {

const size_t kMaxReadbacks = 1;
typedef base::Callback<void(float, const SkBitmap&)> TabReadbackCallback;

}  // namespace

namespace android {

class TabContentManager::TabReadbackRequest {
 public:
  TabReadbackRequest(content::RenderWidgetHostView* rwhv,
                     float thumbnail_scale,
                     const TabReadbackCallback& end_callback)
      : thumbnail_scale_(thumbnail_scale),
        end_callback_(end_callback),
        drop_after_readback_(false),
        weak_factory_(this) {
    DCHECK(rwhv);
    auto result_callback =
        base::BindOnce(&TabReadbackRequest::OnFinishGetTabThumbnailBitmap,
                       weak_factory_.GetWeakPtr());

    gfx::Size view_size_in_pixels =
        rwhv->GetNativeView()->GetPhysicalBackingSize();
    if (view_size_in_pixels.IsEmpty()) {
      std::move(result_callback).Run(SkBitmap());
      return;
    }
    gfx::Size thumbnail_size(
        gfx::ScaleToCeiledSize(view_size_in_pixels, thumbnail_scale_));
    rwhv->CopyFromSurface(gfx::Rect(), thumbnail_size,
                          std::move(result_callback));
  }

  virtual ~TabReadbackRequest() {}

  void OnFinishGetTabThumbnailBitmap(const SkBitmap& bitmap) {
    if (bitmap.drawsNothing() || drop_after_readback_) {
      end_callback_.Run(0.f, SkBitmap());
      return;
    }

    SkBitmap result_bitmap = bitmap;
    result_bitmap.setImmutable();
    end_callback_.Run(thumbnail_scale_, bitmap);
  }

  void SetToDropAfterReadback() { drop_after_readback_ = true; }

 private:
  const float thumbnail_scale_;
  TabReadbackCallback end_callback_;
  bool drop_after_readback_;

  base::WeakPtrFactory<TabReadbackRequest> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TabReadbackRequest);
};

// static
TabContentManager* TabContentManager::FromJavaObject(
    const JavaRef<jobject>& jobj) {
  if (jobj.is_null())
    return nullptr;
  return reinterpret_cast<TabContentManager*>(
      Java_TabContentManager_getNativePtr(base::android::AttachCurrentThread(),
                                          jobj));
}

TabContentManager::TabContentManager(JNIEnv* env,
                                     jobject obj,
                                     jint default_cache_size,
                                     jint approximation_cache_size,
                                     jint compression_queue_max_size,
                                     jint write_queue_max_size,
                                     jboolean use_approximation_thumbnail)
    : weak_java_tab_content_manager_(env, obj), weak_factory_(this) {
  thumbnail_cache_ = std::make_unique<ThumbnailCache>(
      static_cast<size_t>(default_cache_size),
      static_cast<size_t>(approximation_cache_size),
      static_cast<size_t>(compression_queue_max_size),
      static_cast<size_t>(write_queue_max_size), use_approximation_thumbnail);
  thumbnail_cache_->AddThumbnailCacheObserver(this);
}

TabContentManager::~TabContentManager() {
}

void TabContentManager::Destroy(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  thumbnail_cache_->RemoveThumbnailCacheObserver(this);
  delete this;
}

void TabContentManager::SetUIResourceProvider(
    ui::UIResourceProvider* ui_resource_provider) {
  thumbnail_cache_->SetUIResourceProvider(ui_resource_provider);
}

scoped_refptr<cc::Layer> TabContentManager::GetLiveLayer(int tab_id) {
  return live_layer_list_[tab_id];
}

scoped_refptr<ThumbnailLayer> TabContentManager::GetStaticLayer(int tab_id) {
  return static_layer_cache_[tab_id];
}

scoped_refptr<ThumbnailLayer> TabContentManager::GetOrCreateStaticLayer(
    int tab_id,
    bool force_disk_read) {
  Thumbnail* thumbnail = thumbnail_cache_->Get(tab_id, force_disk_read, true);
  scoped_refptr<ThumbnailLayer> static_layer = static_layer_cache_[tab_id];

  if (!thumbnail || !thumbnail->ui_resource_id()) {
    if (static_layer.get()) {
      static_layer->layer()->RemoveFromParent();
      static_layer_cache_.erase(tab_id);
    }
    return nullptr;
  }

  if (!static_layer.get()) {
    static_layer = ThumbnailLayer::Create();
    static_layer_cache_[tab_id] = static_layer;
  }

  static_layer->SetThumbnail(thumbnail);
  return static_layer;
}

void TabContentManager::AttachLiveLayer(int tab_id,
                                        scoped_refptr<cc::Layer> layer) {
  if (!layer.get())
    return;

  scoped_refptr<cc::Layer> cached_layer = live_layer_list_[tab_id];
  if (cached_layer != layer)
    live_layer_list_[tab_id] = layer;
}

void TabContentManager::DetachLiveLayer(int tab_id,
                                        scoped_refptr<cc::Layer> layer) {
  scoped_refptr<cc::Layer> current_layer = live_layer_list_[tab_id];
  if (!current_layer.get()) {
    // Empty cached layer should not exist but it is ok if it happens.
    return;
  }

  // We need to remove if we're getting a detach for our current layer or we're
  // getting a detach with NULL and we have a current layer, which means remove
  //  all layers.
  if (current_layer.get() &&
      (layer.get() == current_layer.get() || !layer.get())) {
    live_layer_list_.erase(tab_id);
  }
}

jboolean TabContentManager::HasFullCachedThumbnail(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint tab_id) {
  return thumbnail_cache_->Get(tab_id, false, false) != nullptr;
}

void TabContentManager::CacheTab(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj,
                                 const JavaParamRef<jobject>& tab,
                                 jfloat thumbnail_scale) {
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab);
  DCHECK(tab_android);
  const int tab_id = tab_android->GetAndroidId();
  if (pending_tab_readbacks_.find(tab_id) != pending_tab_readbacks_.end() ||
      pending_tab_readbacks_.size() >= kMaxReadbacks) {
    return;
  }

  content::WebContents* web_contents = tab_android->web_contents();
  DCHECK(web_contents);

  content::RenderViewHost* rvh = web_contents->GetRenderViewHost();
  if (web_contents->ShowingInterstitialPage()) {
    if (!web_contents->GetInterstitialPage()->GetMainFrame())
      return;

    rvh = web_contents->GetInterstitialPage()->GetMainFrame()->
        GetRenderViewHost();
  }
  if (!rvh)
    return;

  content::RenderWidgetHost* rwh = rvh->GetWidget();
  content::RenderWidgetHostView* rwhv = rwh ? rwh->GetView() : nullptr;
  if (!rwhv || !rwhv->IsSurfaceAvailableForCopy())
    return;

  if (thumbnail_cache_->CheckAndUpdateThumbnailMetaData(
          tab_id, tab_android->GetURL())) {
    TabReadbackCallback readback_done_callback =
        base::Bind(&TabContentManager::PutThumbnailIntoCache,
                   weak_factory_.GetWeakPtr(), tab_id);
    pending_tab_readbacks_[tab_id] = std::make_unique<TabReadbackRequest>(
        rwhv, thumbnail_scale, readback_done_callback);
  }
}

void TabContentManager::CacheTabWithBitmap(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj,
                                           const JavaParamRef<jobject>& tab,
                                           const JavaParamRef<jobject>& bitmap,
                                           jfloat thumbnail_scale) {
  TabAndroid* tab_android = TabAndroid::GetNativeTab(env, tab);
  DCHECK(tab_android);
  int tab_id = tab_android->GetAndroidId();
  GURL url = tab_android->GetURL();

  gfx::JavaBitmap java_bitmap_lock(bitmap);
  SkBitmap skbitmap = gfx::CreateSkBitmapFromJavaBitmap(java_bitmap_lock);
  skbitmap.setImmutable();

  if (thumbnail_cache_->CheckAndUpdateThumbnailMetaData(tab_id, url))
    PutThumbnailIntoCache(tab_id, thumbnail_scale, skbitmap);
}

void TabContentManager::InvalidateIfChanged(JNIEnv* env,
                                            const JavaParamRef<jobject>& obj,
                                            jint tab_id,
                                            const JavaParamRef<jstring>& jurl) {
  thumbnail_cache_->InvalidateThumbnailIfChanged(
      tab_id, GURL(base::android::ConvertJavaStringToUTF8(env, jurl)));
}

void TabContentManager::UpdateVisibleIds(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jintArray>& priority,
    jint primary_tab_id) {
  std::list<int> priority_ids;
  jsize length = env->GetArrayLength(priority);
  jint* ints = env->GetIntArrayElements(priority, nullptr);
  for (jsize i = 0; i < length; ++i)
    priority_ids.push_back(static_cast<int>(ints[i]));

  env->ReleaseIntArrayElements(priority, ints, JNI_ABORT);
  thumbnail_cache_->UpdateVisibleIds(priority_ids, primary_tab_id);
}

void TabContentManager::NativeRemoveTabThumbnail(int tab_id) {
  TabReadbackRequestMap::iterator readback_iter =
      pending_tab_readbacks_.find(tab_id);
  if (readback_iter != pending_tab_readbacks_.end())
    readback_iter->second->SetToDropAfterReadback();
  thumbnail_cache_->Remove(tab_id);
}

void TabContentManager::RemoveTabThumbnail(JNIEnv* env,
                                           const JavaParamRef<jobject>& obj,
                                           jint tab_id) {
  NativeRemoveTabThumbnail(tab_id);
}

void TabContentManager::OnUIResourcesWereEvicted() {
  thumbnail_cache_->OnUIResourcesWereEvicted();
}

void TabContentManager::OnFinishedThumbnailRead(int tab_id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_TabContentManager_notifyListenersOfThumbnailChange(
      env, weak_java_tab_content_manager_.get(env), tab_id);
}

void TabContentManager::PutThumbnailIntoCache(int tab_id,
                                              float thumbnail_scale,
                                              const SkBitmap& bitmap) {
  TabReadbackRequestMap::iterator readback_iter =
      pending_tab_readbacks_.find(tab_id);

  if (readback_iter != pending_tab_readbacks_.end())
    pending_tab_readbacks_.erase(tab_id);

  if (thumbnail_scale > 0 && !bitmap.empty())
    thumbnail_cache_->Put(tab_id, bitmap, thumbnail_scale);
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

jlong JNI_TabContentManager_Init(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj,
                                 jint default_cache_size,
                                 jint approximation_cache_size,
                                 jint compression_queue_max_size,
                                 jint write_queue_max_size,
                                 jboolean use_approximation_thumbnail) {
  TabContentManager* manager = new TabContentManager(
      env, obj, default_cache_size, approximation_cache_size,
      compression_queue_max_size, write_queue_max_size,
      use_approximation_thumbnail);
  return reinterpret_cast<intptr_t>(manager);
}

}  // namespace android
