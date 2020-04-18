// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/stats_collection_controller.h"

#include "base/json/json_writer.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "base/strings/string_util.h"
#include "content/common/renderer_host.mojom.h"
#include "content/public/common/service_names.mojom.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_view_impl.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"

namespace content {

namespace {

bool CurrentRenderViewImpl(RenderViewImpl** out) {
  blink::WebLocalFrame* web_frame =
      blink::WebLocalFrame::FrameForCurrentContext();
  if (!web_frame)
    return false;

  blink::WebView* web_view = web_frame->View();
  if (!web_view)
    return false;

  RenderViewImpl* render_view_impl =
      RenderViewImpl::FromWebView(web_view);
  if (!render_view_impl)
    return false;

  *out = render_view_impl;
  return true;
}

// Encodes a WebContentsLoadTime as JSON.
// Input:
// - |load_start_time| - time at which page load started.
// - |load_stop_time| - time at which page load stopped.
// - |result| - returned JSON.
// Example return value:
// {'load_start_ms': 1, 'load_duration_ms': 2.5}
// either value may be null if a web contents hasn't fully loaded.
// load_start_ms is represented as milliseconds since the unix epoch.
void ConvertLoadTimeToJSON(
    const base::Time& load_start_time,
    const base::Time& load_stop_time,
    std::string *result) {
  base::DictionaryValue item;

  if (load_start_time.is_null()) {
    item.Set("load_start_ms", std::make_unique<base::Value>());
  } else {
    item.SetDouble("load_start_ms", (load_start_time - base::Time::UnixEpoch())
                   .InMillisecondsF());
  }
  if (load_start_time.is_null() || load_stop_time.is_null()) {
    item.Set("load_duration_ms", std::make_unique<base::Value>());
  } else {
    item.SetDouble("load_duration_ms",
        (load_stop_time - load_start_time).InMillisecondsF());
  }
  base::JSONWriter::Write(item, result);
}

}  // namespace

// static
gin::WrapperInfo StatsCollectionController::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

// static
void StatsCollectionController::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<StatsCollectionController> controller =
      gin::CreateHandle(isolate, new StatsCollectionController());
  if (controller.IsEmpty())
    return;
  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "statsCollectionController"),
              controller.ToV8());
}

StatsCollectionController::StatsCollectionController() {}

StatsCollectionController::~StatsCollectionController() {}

gin::ObjectTemplateBuilder StatsCollectionController::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<StatsCollectionController>::GetObjectTemplateBuilder(
      isolate)
      .SetMethod("getHistogram", &StatsCollectionController::GetHistogram)
      .SetMethod("getBrowserHistogram",
                 &StatsCollectionController::GetBrowserHistogram)
      .SetMethod("tabLoadTiming", &StatsCollectionController::GetTabLoadTiming);
}

std::string StatsCollectionController::GetHistogram(
    const std::string& histogram_name) {
  base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(histogram_name);
  std::string output;
  if (!histogram) {
    output = "{}";
  } else {
    histogram->WriteJSON(&output, base::JSON_VERBOSITY_LEVEL_FULL);
  }
  return output;
}

std::string StatsCollectionController::GetBrowserHistogram(
    const std::string& histogram_name) {
  std::string histogram_json;
  RenderThreadImpl::current()->GetRendererHost()->GetBrowserHistogram(
      histogram_name, &histogram_json);

  return histogram_json;
}

std::string StatsCollectionController::GetTabLoadTiming() {
  RenderViewImpl* render_view_impl = nullptr;
  bool result = CurrentRenderViewImpl(&render_view_impl);
  DCHECK(result);

  StatsCollectionObserver* observer =
      render_view_impl->GetStatsCollectionObserver();
  DCHECK(observer);

  std::string tab_timing_json;
  ConvertLoadTimeToJSON(
      observer->load_start_time(), observer->load_stop_time(),
      &tab_timing_json);
  return tab_timing_json;
}

}  // namespace content
