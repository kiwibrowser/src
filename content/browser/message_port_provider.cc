// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/message_port_provider.h"

#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/common/message_port/string_message_codec.h"

#if defined(OS_ANDROID)
#include "base/android/jni_string.h"
#include "content/browser/android/app_web_message_port.h"
#endif

using blink::MessagePortChannel;

namespace content {
namespace {

void PostMessageToFrameInternal(WebContents* web_contents,
                                const base::string16& source_origin,
                                const base::string16& target_origin,
                                const base::string16& data,
                                std::vector<MessagePortChannel> channels) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  FrameMsg_PostMessage_Params params;
  params.message = new base::RefCountedData<blink::TransferableMessage>();
  params.message->data.owned_encoded_message = blink::EncodeStringMessage(data);
  params.message->data.encoded_message =
      params.message->data.owned_encoded_message;
  params.message->data.ports = std::move(channels);
  params.source_routing_id = MSG_ROUTING_NONE;
  params.source_origin = source_origin;
  params.target_origin = target_origin;

  RenderFrameHost* rfh = web_contents->GetMainFrame();
  rfh->Send(new FrameMsg_PostMessageEvent(rfh->GetRoutingID(), params));
}

#if defined(OS_ANDROID)
base::string16 ToString16(JNIEnv* env,
                          const base::android::JavaParamRef<jstring>& s) {
  if (s.is_null())
    return base::string16();
  return base::android::ConvertJavaStringToUTF16(env, s);
}
#endif

}  // namespace

// static
void MessagePortProvider::PostMessageToFrame(
    WebContents* web_contents,
    const base::string16& source_origin,
    const base::string16& target_origin,
    const base::string16& data) {
  PostMessageToFrameInternal(web_contents, source_origin, target_origin, data,
                             std::vector<MessagePortChannel>());
}

#if defined(OS_ANDROID)
void MessagePortProvider::PostMessageToFrame(
    WebContents* web_contents,
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& source_origin,
    const base::android::JavaParamRef<jstring>& target_origin,
    const base::android::JavaParamRef<jstring>& data,
    const base::android::JavaParamRef<jobjectArray>& ports) {
  PostMessageToFrameInternal(
      web_contents,
      ToString16(env, source_origin),
      ToString16(env, target_origin),
      ToString16(env, data),
      AppWebMessagePort::UnwrapJavaArray(env, ports));
}
#endif

} // namespace content
