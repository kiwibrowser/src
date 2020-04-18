// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container.h"

#include <map>
#include <set>

#include "base/guid.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "components/guest_view/common/guest_view_constants.h"
#include "components/guest_view/common/guest_view_messages.h"
#include "content/public/common/url_loader_throttle.h"
#include "content/public/common/webplugininfo.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/guest_view/extensions_guest_view_messages.h"
#include "extensions/common/mojo/guest_view.mojom.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "gin/arguments.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "ipc/ipc_sync_channel.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_associated_url_loader.h"
#include "third_party/blink/public/web/web_associated_url_loader_options.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_remote_frame.h"
#include "third_party/blink/public/web/web_view.h"

namespace extensions {

namespace {

const char kPostMessageName[] = "postMessage";

base::LazyInstance<mojom::GuestViewAssociatedPtr>::Leaky g_guest_view;

mojom::GuestView* GetGuestView() {
  if (!g_guest_view.Get()) {
    content::RenderThread::Get()->GetChannel()->GetRemoteAssociatedInterface(
        &g_guest_view.Get());
  }

  return g_guest_view.Get().get();
}

// The gin-backed scriptable object which is exposed by the BrowserPlugin for
// MimeHandlerViewContainer. This currently only implements "postMessage".
class ScriptableObject : public gin::Wrappable<ScriptableObject>,
                         public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static v8::Local<v8::Object> Create(
      v8::Isolate* isolate,
      base::WeakPtr<MimeHandlerViewContainer> container) {
    ScriptableObject* scriptable_object =
        new ScriptableObject(isolate, container);
    return gin::CreateHandle(isolate, scriptable_object)
        .ToV8()
        .As<v8::Object>();
  }

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(
      v8::Isolate* isolate,
      const std::string& identifier) override {
    if (identifier == kPostMessageName) {
      if (post_message_function_template_.IsEmpty()) {
        post_message_function_template_.Reset(
            isolate,
            gin::CreateFunctionTemplate(
                isolate,
                base::Bind(&MimeHandlerViewContainer::PostJavaScriptMessage,
                           container_, isolate)));
      }
      v8::Local<v8::FunctionTemplate> function_template =
          v8::Local<v8::FunctionTemplate>::New(isolate,
                                               post_message_function_template_);
      v8::Local<v8::Function> function;
      if (function_template->GetFunction(isolate->GetCurrentContext())
              .ToLocal(&function))
        return function;
    }
    return v8::Local<v8::Value>();
  }

 private:
  ScriptableObject(v8::Isolate* isolate,
                   base::WeakPtr<MimeHandlerViewContainer> container)
    : gin::NamedPropertyInterceptor(isolate, this),
      container_(container) {}

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<ScriptableObject>::GetObjectTemplateBuilder(isolate)
        .AddNamedPropertyInterceptor();
  }

  base::WeakPtr<MimeHandlerViewContainer> container_;
  v8::Persistent<v8::FunctionTemplate> post_message_function_template_;
};

// static
gin::WrapperInfo ScriptableObject::kWrapperInfo = { gin::kEmbedderNativeGin };

// Maps from content::RenderFrame to the set of MimeHandlerViewContainers within
// it.
base::LazyInstance<
    std::map<content::RenderFrame*, std::set<MimeHandlerViewContainer*>>>::
    DestructorAtExit g_mime_handler_view_container_map =
        LAZY_INSTANCE_INITIALIZER;

}  // namespace

// Stores a raw pointer to MimeHandlerViewContainer since this throttle's
// lifetime is shorter (it matches |container|'s loader_).
class MimeHandlerViewContainer::PluginResourceThrottle
    : public content::URLLoaderThrottle {
 public:
  explicit PluginResourceThrottle(MimeHandlerViewContainer* container)
      : container_(container) {}
  ~PluginResourceThrottle() override {}

 private:
  // content::URLLoaderThrottle overrides;
  void WillProcessResponse(const GURL& response_url,
                           const network::ResourceResponseHead& response_head,
                           bool* defer) override {
    network::mojom::URLLoaderPtr dummy_new_loader;
    mojo::MakeRequest(&dummy_new_loader);
    network::mojom::URLLoaderClientPtr new_client;
    network::mojom::URLLoaderClientRequest new_client_request =
        mojo::MakeRequest(&new_client);

    network::mojom::URLLoaderPtr original_loader;
    network::mojom::URLLoaderClientRequest original_client;
    delegate_->InterceptResponse(std::move(dummy_new_loader),
                                 std::move(new_client_request),
                                 &original_loader, &original_client);

    auto transferrable_loader = content::mojom::TransferrableURLLoader::New();
    transferrable_loader->url_loader = original_loader.PassInterface();
    transferrable_loader->url_loader_client = std::move(original_client);

    // Make a deep copy of ResourceResponseHead before passing it cross-thread.
    auto resource_response = base::MakeRefCounted<network::ResourceResponse>();
    resource_response->head = response_head;
    auto deep_copied_response = resource_response->DeepCopy();
    transferrable_loader->head = std::move(deep_copied_response->head);
    container_->SetEmbeddedLoader(std::move(transferrable_loader));
  }

  MimeHandlerViewContainer* container_;

  DISALLOW_COPY_AND_ASSIGN(PluginResourceThrottle);
};

MimeHandlerViewContainer::MimeHandlerViewContainer(
    content::RenderFrame* render_frame,
    const content::WebPluginInfo& info,
    const std::string& mime_type,
    const GURL& original_url)
    : GuestViewContainer(render_frame),
      plugin_path_(info.path.MaybeAsASCII()),
      mime_type_(mime_type),
      original_url_(original_url),
      guest_proxy_routing_id_(-1),
      guest_loaded_(false),
      weak_factory_(this) {
  DCHECK(!mime_type_.empty());
  is_embedded_ = !render_frame->GetWebFrame()->GetDocument().IsPluginDocument();
  g_mime_handler_view_container_map.Get()[render_frame].insert(this);
}

MimeHandlerViewContainer::~MimeHandlerViewContainer() {
  if (loader_) {
    DCHECK(is_embedded_);
    loader_->Cancel();
  }

  if (render_frame()) {
    g_mime_handler_view_container_map.Get()[render_frame()].erase(this);
    if (g_mime_handler_view_container_map.Get()[render_frame()].empty())
      g_mime_handler_view_container_map.Get().erase(render_frame());
  }
}

// static
std::vector<MimeHandlerViewContainer*>
MimeHandlerViewContainer::FromRenderFrame(content::RenderFrame* render_frame) {
  auto it = g_mime_handler_view_container_map.Get().find(render_frame);
  if (it == g_mime_handler_view_container_map.Get().end())
    return std::vector<MimeHandlerViewContainer*>();

  return std::vector<MimeHandlerViewContainer*>(it->second.begin(),
                                                it->second.end());
}

void MimeHandlerViewContainer::OnReady() {
  if (!render_frame() || !is_embedded_)
    return;

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  blink::WebAssociatedURLLoaderOptions options;
  DCHECK(!loader_);
  loader_.reset(frame->CreateAssociatedURLLoader(options));

  // The embedded plugin is allowed to be cross-origin and we should always
  // send credentials/cookies with the request. So, use the default mode
  // "no-cors" and credentials mode "include".
  blink::WebURLRequest request(original_url_);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextObject);
  // The plugin resource request should skip service workers since "plug-ins
  // may get their security origins from their own urls".
  // https://w3c.github.io/ServiceWorker/#implementer-concerns
  request.SetSkipServiceWorker(true);

  waiting_to_create_throttle_ = true;
  loader_->LoadAsynchronously(request, this);
}

bool MimeHandlerViewContainer::OnMessage(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MimeHandlerViewContainer, message)
  IPC_MESSAGE_HANDLER(ExtensionsGuestViewMsg_CreateMimeHandlerViewGuestACK,
                      OnCreateMimeHandlerViewGuestACK)
  IPC_MESSAGE_HANDLER(
      ExtensionsGuestViewMsg_MimeHandlerViewGuestOnLoadCompleted,
      OnMimeHandlerViewGuestOnLoadCompleted)
  IPC_MESSAGE_HANDLER(GuestViewMsg_GuestAttached, OnGuestAttached)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void MimeHandlerViewContainer::PluginDidFinishLoading() {
  DCHECK(!is_embedded_);
  CreateMimeHandlerViewGuestIfNecessary();
}

void MimeHandlerViewContainer::OnRenderFrameDestroyed() {
  g_mime_handler_view_container_map.Get().erase(render_frame());
}

void MimeHandlerViewContainer::PluginDidReceiveData(const char* data,
                                                    int data_length) {
  view_id_ += std::string(data, data_length);
}


void MimeHandlerViewContainer::DidResizeElement(const gfx::Size& new_size) {
  element_size_ = new_size;

  CreateMimeHandlerViewGuestIfNecessary();

  // Don't try to resize a guest that hasn't been created yet. It is enough to
  // initialise |element_size_| here and then we'll send that to the browser
  // during guest creation.
  if (!guest_created_)
    return;

  render_frame()->Send(new ExtensionsGuestViewHostMsg_ResizeGuest(
      render_frame()->GetRoutingID(), element_instance_id(), new_size));
}

v8::Local<v8::Object> MimeHandlerViewContainer::V8ScriptableObject(
    v8::Isolate* isolate) {
  if (scriptable_object_.IsEmpty()) {
    v8::Local<v8::Object> object =
        ScriptableObject::Create(isolate, weak_factory_.GetWeakPtr());
    scriptable_object_.Reset(isolate, object);
  }
  return v8::Local<v8::Object>::New(isolate, scriptable_object_);
}

void MimeHandlerViewContainer::DidReceiveData(const char* data,
                                              int data_length) {
  view_id_ += std::string(data, data_length);
}

void MimeHandlerViewContainer::DidFinishLoading() {
  DCHECK(is_embedded_);
  CreateMimeHandlerViewGuestIfNecessary();
}

void MimeHandlerViewContainer::PostJavaScriptMessage(
    v8::Isolate* isolate,
    v8::Local<v8::Value> message) {
  if (!guest_loaded_) {
    pending_messages_.push_back(v8::Global<v8::Value>(isolate, message));
    return;
  }

  content::RenderView* guest_proxy_render_view =
      content::RenderView::FromRoutingID(guest_proxy_routing_id_);
  if (!guest_proxy_render_view)
    return;
  blink::WebFrame* guest_proxy_frame =
      guest_proxy_render_view->GetWebView()->MainFrame();
  if (!guest_proxy_frame)
    return;

  v8::Context::Scope context_scope(
      render_frame()->GetWebFrame()->MainWorldScriptContext());

  v8::Local<v8::Object> guest_proxy_window = guest_proxy_frame->GlobalProxy();
  gin::Dictionary window_object(isolate, guest_proxy_window);
  v8::Local<v8::Function> post_message;
  if (!window_object.Get(std::string(kPostMessageName), &post_message))
    return;

  v8::Local<v8::Value> args[] = {
      message,
      // Post the message to any domain inside the browser plugin. The embedder
      // should already know what is embedded.
      gin::StringToV8(isolate, "*")};
  render_frame()->GetWebFrame()->CallFunctionEvenIfScriptDisabled(
      post_message.As<v8::Function>(), guest_proxy_window, arraysize(args),
      args);
}

void MimeHandlerViewContainer::PostMessageFromValue(
    const base::Value& message) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(frame->MainWorldScriptContext());
  PostJavaScriptMessage(isolate,
                        content::V8ValueConverter::Create()->ToV8Value(
                            &message, frame->MainWorldScriptContext()));
}

std::unique_ptr<content::URLLoaderThrottle>
MimeHandlerViewContainer::MaybeCreatePluginThrottle(const GURL& url) {
  if (!waiting_to_create_throttle_ || url != original_url_)
    return nullptr;

  waiting_to_create_throttle_ = false;
  return std::make_unique<PluginResourceThrottle>(this);
}

void MimeHandlerViewContainer::OnCreateMimeHandlerViewGuestACK(
    int element_instance_id) {
  DCHECK_NE(this->element_instance_id(), guest_view::kInstanceIDNone);
  DCHECK_EQ(this->element_instance_id(), element_instance_id);

  if (!render_frame())
    return;

  render_frame()->AttachGuest(element_instance_id);
}

void MimeHandlerViewContainer::OnGuestAttached(int /* unused */,
                                               int guest_proxy_routing_id) {
  // Save the RenderView routing ID of the guest here so it can be used to route
  // PostMessage calls.
  guest_proxy_routing_id_ = guest_proxy_routing_id;
}

void MimeHandlerViewContainer::OnMimeHandlerViewGuestOnLoadCompleted(
    int /* unused */) {
  if (!render_frame())
    return;

  guest_loaded_ = true;
  if (pending_messages_.empty())
    return;

  // Now that the guest has loaded, flush any unsent messages.
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  if (!frame)
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(frame->MainWorldScriptContext());
  for (const auto& pending_message : pending_messages_)
    PostJavaScriptMessage(isolate,
                          v8::Local<v8::Value>::New(isolate, pending_message));

  pending_messages_.clear();
}

void MimeHandlerViewContainer::SetEmbeddedLoader(
    content::mojom::TransferrableURLLoaderPtr transferrable_url_loader) {
  transferrable_url_loader_ = std::move(transferrable_url_loader);
  transferrable_url_loader_->url = GURL(plugin_path_ + base::GenerateGUID());
  CreateMimeHandlerViewGuestIfNecessary();
}

void MimeHandlerViewContainer::CreateMimeHandlerViewGuestIfNecessary() {
  if (guest_created_ || !element_size_.has_value())
    return;

  // When the network service is enabled, subresource requests like plugins are
  // made directly from the renderer to the network service. So we need to
  // intercept the URLLoader and send it to the browser so that it can forward
  // it to the plugin.
  if (base::FeatureList::IsEnabled(network::features::kNetworkService) &&
      is_embedded_) {
    if (transferrable_url_loader_.is_null())
      return;

    GetGuestView()->CreateEmbeddedMimeHandlerViewGuest(
        render_frame()->GetRoutingID(),
        ExtensionFrameHelper::Get(render_frame())->tab_id(), original_url_,
        element_instance_id(), *element_size_,
        std::move(transferrable_url_loader_));
    guest_created_ = true;
    return;
  }

  if (view_id_.empty())
    return;

  // The loader has completed loading |view_id_| so we can dispose it.
  if (loader_) {
    DCHECK(is_embedded_);
    loader_.reset();
  }

  DCHECK_NE(element_instance_id(), guest_view::kInstanceIDNone);

  if (!render_frame())
    return;

  render_frame()->Send(
      new ExtensionsGuestViewHostMsg_CreateMimeHandlerViewGuest(
          render_frame()->GetRoutingID(), view_id_, element_instance_id(),
          *element_size_));

  guest_created_ = true;
}

}  // namespace extensions
