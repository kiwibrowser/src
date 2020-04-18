// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ANDROID_ROUTER_MEDIA_ROUTER_ANDROID_BRIDGE_H_
#define CHROME_BROWSER_MEDIA_ANDROID_ROUTER_MEDIA_ROUTER_ANDROID_BRIDGE_H_

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/common/media_router/media_route.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"
#include "content/public/browser/media_controller.h"
#include "url/origin.h"

namespace media_router {

class MediaRouterAndroid;

// Wraps the JNI layer between MediaRouterAndroid and ChromeMediaRouter.
class MediaRouterAndroidBridge {
 public:
  explicit MediaRouterAndroidBridge(MediaRouterAndroid* router);
  virtual ~MediaRouterAndroidBridge();

  // Implement the corresponding calls for the MediaRouterAndroid class.
  // Virtual so could be overridden by tests.
  virtual void CreateRoute(const MediaSource::Id& source_id,
                           const MediaSink::Id& sink_id,
                           const std::string& presentation_id,
                           const url::Origin& origin,
                           int tab_id,
                           bool is_incognito,
                           int route_request_id);
  virtual void JoinRoute(const MediaSource::Id& source_id,
                         const std::string& presentation_id,
                         const url::Origin& origin,
                         int tab_id,
                         int route_request_id);
  virtual void TerminateRoute(const MediaRoute::Id& route_id);
  virtual void SendRouteMessage(const MediaRoute::Id& route_id,
                                const std::string& message,
                                int callback_id);
  virtual void DetachRoute(const MediaRoute::Id& route_id);
  virtual bool StartObservingMediaSinks(const MediaSource::Id& source_id);
  virtual void StopObservingMediaSinks(const MediaSource::Id& source_id);
  virtual std::unique_ptr<content::MediaController> GetMediaController(
      const MediaRoute::Id& route_id);

  // Methods called by the Java counterpart.
  void OnSinksReceived(JNIEnv* env,
                       const base::android::JavaRef<jobject>& obj,
                       const base::android::JavaRef<jstring>& jsource_urn,
                       jint jcount);
  void OnRouteCreated(JNIEnv* env,
                      const base::android::JavaRef<jobject>& obj,
                      const base::android::JavaRef<jstring>& jmedia_route_id,
                      const base::android::JavaRef<jstring>& jmedia_sink_id,
                      jint jroute_request_id,
                      jboolean jis_local);
  void OnRouteRequestError(JNIEnv* env,
                           const base::android::JavaRef<jobject>& obj,
                           const base::android::JavaRef<jstring>& jerror_text,
                           jint jroute_request_id);
  void OnRouteClosed(JNIEnv* env,
                     const base::android::JavaRef<jobject>& obj,
                     const base::android::JavaRef<jstring>& jmedia_route_id);
  void OnRouteClosedWithError(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& obj,
      const base::android::JavaRef<jstring>& jmedia_route_id,
      const base::android::JavaRef<jstring>& jmessage);
  void OnMessageSentResult(JNIEnv* env,
                           const base::android::JavaRef<jobject>& obj,
                           jboolean jsuccess,
                           jint jcallback_id);
  void OnMessage(JNIEnv* env,
                 const base::android::JavaRef<jobject>& obj,
                 const base::android::JavaRef<jstring>& jmedia_route_id,
                 const base::android::JavaRef<jstring>& jmessage);

 private:
  MediaRouterAndroid* native_media_router_;
  base::android::ScopedJavaGlobalRef<jobject> java_media_router_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterAndroidBridge);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ANDROID_ROUTER_MEDIA_ROUTER_ANDROID_BRIDGE_H_
