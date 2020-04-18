// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_GUEST_VIEW_GUEST_VIEW_INTERNAL_CUSTOM_BINDINGS_H_
#define EXTENSIONS_RENDERER_GUEST_VIEW_GUEST_VIEW_INTERNAL_CUSTOM_BINDINGS_H_

#include <map>

#include "extensions/renderer/object_backed_native_handler.h"

namespace extensions {

// Implements custom bindings for the guestViewInternal API.
class GuestViewInternalCustomBindings : public ObjectBackedNativeHandler {
 public:
  explicit GuestViewInternalCustomBindings(ScriptContext* context);
  ~GuestViewInternalCustomBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

 private:
  // ResetMapEntry is called as a callback to SetWeak(). It resets the
  // weak view reference held in |view_map_|.
  static void ResetMapEntry(const v8::WeakCallbackInfo<int>& data);

  // AttachGuest attaches a GuestView to a provided container element. Once
  // attached, the GuestView will participate in layout of the container page
  // and become visible on screen.
  // AttachGuest takes four parameters:
  // |element_instance_id| uniquely identifies a container within the content
  // module is able to host GuestViews.
  // |guest_instance_id| uniquely identifies an unattached GuestView.
  // |attach_params| is typically used to convey the current state of the
  // container element at the time of attachment. These parameters are passed
  // down to the GuestView. The GuestView may use these parameters to update the
  // state of the guest hosted in another process.
  // |callback| is an optional callback that is called once attachment is
  // complete. The callback takes in a parameter for the WindowProxy of the
  // guest identified by |guest_instance_id|.
  void AttachGuest(const v8::FunctionCallbackInfo<v8::Value>& args);

  // DetachGuest detaches the container container specified from the associated
  // GuestViewBase. DetachGuest takes two parameters:
  // |element_instance_id| uniquely identifies a container within the content
  // module is able to host GuestViews.
  // |callback| is an optional callback that is called once the container has
  // been detached.
  void DetachGuest(const v8::FunctionCallbackInfo<v8::Value>& args);

  // AttachIframeGuest is --site-per-process variant of AttachGuest().
  //
  // AttachIframeGuest takes a |contentWindow| parameter in addition to the
  // parameters to AttachGuest. That parameter is used to identify the
  // RenderFrame of the <iframe> container element.
  void AttachIframeGuest(const v8::FunctionCallbackInfo<v8::Value>& args);

  // GetContentWindow takes in a RenderView routing ID and returns the
  // Window JavaScript object for that RenderView.
  void GetContentWindow(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Destroys the GuestViewContainer given an element instance ID in |args|.
  void DestroyContainer(const v8::FunctionCallbackInfo<v8::Value>& args);

  // GetViewFromID takes a view ID, and returns the GuestView element associated
  // with that ID, if one exists. Otherwise, null is returned.
  void GetViewFromID(const v8::FunctionCallbackInfo<v8::Value>& args);

  // RegisterDestructionCallback registers a JavaScript callback function to be
  // called when the guestview's container is destroyed.
  // RegisterDestructionCallback takes in a single paramater, |callback|.
  void RegisterDestructionCallback(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  // RegisterElementResizeCallback registers a JavaScript callback function to
  // be called when the element is resized. RegisterElementResizeCallback takes
  // a single parameter, |callback|.
  void RegisterElementResizeCallback(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  // RegisterView takes in a view ID and a GuestView element, and stores the
  // pair as an entry in |view_map_|. The view can then be retrieved using
  // GetViewFromID.
  void RegisterView(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Runs a JavaScript function with user gesture.
  //
  // This is used to request webview element to enter fullscreen (from the
  // embedder).
  // Note that the guest requesting fullscreen means it has already been
  // triggered by a user gesture and we get to this point if embedder allows
  // the fullscreen request to proceed.
  void RunWithGesture(
      const v8::FunctionCallbackInfo<v8::Value>& args);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_GUEST_VIEW_GUEST_VIEW_INTERNAL_CUSTOM_BINDINGS_H_
