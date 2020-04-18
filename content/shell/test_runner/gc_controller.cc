// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/gc_controller.h"

#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"

namespace test_runner {

gin::WrapperInfo GCController::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
void GCController::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<GCController> controller =
      gin::CreateHandle(isolate, new GCController());
  if (controller.IsEmpty())
    return;
  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "GCController"), controller.ToV8());
}

GCController::GCController() {}

GCController::~GCController() {}

gin::ObjectTemplateBuilder GCController::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<GCController>::GetObjectTemplateBuilder(isolate)
      .SetMethod("collect", &GCController::Collect)
      .SetMethod("collectAll", &GCController::CollectAll)
      .SetMethod("minorCollect", &GCController::MinorCollect);
}

void GCController::Collect(const gin::Arguments& args) {
  args.isolate()->RequestGarbageCollectionForTesting(
      v8::Isolate::kFullGarbageCollection);
}

void GCController::CollectAll(const gin::Arguments& args) {
  // In order to collect a DOM wrapper, two GC cycles are needed.
  // In the first GC cycle, a weak callback of the DOM wrapper is called back
  // and the weak callback disposes a persistent handle to the DOM wrapper.
  // In the second GC cycle, the DOM wrapper is reclaimed.
  // Given that two GC cycles are needed to collect one DOM wrapper,
  // more than two GC cycles are needed to collect all DOM wrappers
  // that are chained. Seven GC cycles look enough in most tests.
  for (int i = 0; i < 7; i++) {
    args.isolate()->RequestGarbageCollectionForTesting(
        v8::Isolate::kFullGarbageCollection);
  }
}

void GCController::MinorCollect(const gin::Arguments& args) {
  args.isolate()->RequestGarbageCollectionForTesting(
      v8::Isolate::kMinorGarbageCollection);
}

}  // namespace test_runner
