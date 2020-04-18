// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_stats_report.h"

namespace blink {

namespace {

template <typename T>
bool AddPropertyValue(v8::Local<v8::Object>& v8_object,
                      v8::Isolate* isolate,
                      T name,
                      v8::Local<v8::Value> value) {
  return V8CallBoolean(v8_object->CreateDataProperty(
      isolate->GetCurrentContext(), V8String(isolate, name), value));
}

bool AddPropertySequenceOfBooleans(v8::Local<v8::Object>& v8_object,
                                   v8::Isolate* isolate,
                                   WebString name,
                                   const WebVector<int>& web_vector) {
  v8::Local<v8::Array> v8_array = v8::Array::New(isolate, web_vector.size());
  for (size_t i = 0; i < web_vector.size(); ++i) {
    if (!V8CallBoolean(v8_array->CreateDataProperty(
            isolate->GetCurrentContext(), static_cast<uint32_t>(i),
            v8::Boolean::New(isolate, static_cast<bool>(web_vector[i])))))
      return false;
  }
  return AddPropertyValue(v8_object, isolate, name, v8_array);
}

template <typename T>
bool AddPropertySequenceOfNumbers(v8::Local<v8::Object>& v8_object,
                                  v8::Isolate* isolate,
                                  WebString name,
                                  const WebVector<T>& web_vector) {
  v8::Local<v8::Array> v8_array = v8::Array::New(isolate, web_vector.size());
  for (size_t i = 0; i < web_vector.size(); ++i) {
    if (!V8CallBoolean(v8_array->CreateDataProperty(
            isolate->GetCurrentContext(), static_cast<uint32_t>(i),
            v8::Number::New(isolate, static_cast<double>(web_vector[i])))))
      return false;
  }
  return AddPropertyValue(v8_object, isolate, name, v8_array);
}

bool AddPropertySequenceOfStrings(v8::Local<v8::Object>& v8_object,
                                  v8::Isolate* isolate,
                                  WebString name,
                                  const WebVector<WebString>& web_vector) {
  v8::Local<v8::Array> v8_array = v8::Array::New(isolate, web_vector.size());
  for (size_t i = 0; i < web_vector.size(); ++i) {
    if (!V8CallBoolean(v8_array->CreateDataProperty(
            isolate->GetCurrentContext(), static_cast<uint32_t>(i),
            V8String(isolate, web_vector[i]))))
      return false;
  }
  return AddPropertyValue(v8_object, isolate, name, v8_array);
}

v8::Local<v8::Value> WebRTCStatsToValue(ScriptState* script_state,
                                        const WebRTCStats* stats) {
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Object> v8_object = v8::Object::New(isolate);

  bool success = true;
  success &= AddPropertyValue(v8_object, isolate, "id",
                              V8String(isolate, stats->Id()));
  success &= AddPropertyValue(v8_object, isolate, "timestamp",
                              v8::Number::New(isolate, stats->Timestamp()));
  success &= AddPropertyValue(v8_object, isolate, "type",
                              V8String(isolate, stats->GetType()));
  for (size_t i = 0; i < stats->MembersCount() && success; ++i) {
    std::unique_ptr<WebRTCStatsMember> member = stats->GetMember(i);
    if (!member->IsDefined())
      continue;
    WebString name = member->GetName();
    switch (member->GetType()) {
      case kWebRTCStatsMemberTypeBool:
        success &=
            AddPropertyValue(v8_object, isolate, name,
                             v8::Boolean::New(isolate, member->ValueBool()));
        break;
      case kWebRTCStatsMemberTypeInt32:
        success &= AddPropertyValue(
            v8_object, isolate, name,
            v8::Number::New(isolate,
                            static_cast<double>(member->ValueInt32())));
        break;
      case kWebRTCStatsMemberTypeUint32:
        success &= AddPropertyValue(
            v8_object, isolate, name,
            v8::Number::New(isolate,
                            static_cast<double>(member->ValueUint32())));
        break;
      case kWebRTCStatsMemberTypeInt64:
        success &= AddPropertyValue(
            v8_object, isolate, name,
            v8::Number::New(isolate,
                            static_cast<double>(member->ValueInt64())));
        break;
      case kWebRTCStatsMemberTypeUint64:
        success &= AddPropertyValue(
            v8_object, isolate, name,
            v8::Number::New(isolate,
                            static_cast<double>(member->ValueUint64())));
        break;
      case kWebRTCStatsMemberTypeDouble:
        success &=
            AddPropertyValue(v8_object, isolate, name,
                             v8::Number::New(isolate, member->ValueDouble()));
        break;
      case kWebRTCStatsMemberTypeString:
        success &= AddPropertyValue(v8_object, isolate, name,
                                    V8String(isolate, member->ValueString()));
        break;
      case kWebRTCStatsMemberTypeSequenceBool:
        success &= AddPropertySequenceOfBooleans(v8_object, isolate, name,
                                                 member->ValueSequenceBool());
        break;
      case kWebRTCStatsMemberTypeSequenceInt32:
        success &= AddPropertySequenceOfNumbers(v8_object, isolate, name,
                                                member->ValueSequenceInt32());
        break;
      case kWebRTCStatsMemberTypeSequenceUint32:
        success &= AddPropertySequenceOfNumbers(v8_object, isolate, name,
                                                member->ValueSequenceUint32());
        break;
      case kWebRTCStatsMemberTypeSequenceInt64:
        success &= AddPropertySequenceOfNumbers(v8_object, isolate, name,
                                                member->ValueSequenceInt64());
        break;
      case kWebRTCStatsMemberTypeSequenceUint64:
        success &= AddPropertySequenceOfNumbers(v8_object, isolate, name,
                                                member->ValueSequenceUint64());
        break;
      case kWebRTCStatsMemberTypeSequenceDouble:
        success &= AddPropertySequenceOfNumbers(v8_object, isolate, name,
                                                member->ValueSequenceDouble());
        break;
      case kWebRTCStatsMemberTypeSequenceString:
        success &= AddPropertySequenceOfStrings(v8_object, isolate, name,
                                                member->ValueSequenceString());
        break;
      default:
        NOTREACHED();
    }
  }
  if (!success) {
    NOTREACHED();
    return v8::Undefined(isolate);
  }
  return v8_object;
}

class RTCStatsReportIterationSource final
    : public PairIterable<String, v8::Local<v8::Value>>::IterationSource {
 public:
  RTCStatsReportIterationSource(std::unique_ptr<WebRTCStatsReport> report)
      : report_(std::move(report)) {}

  bool Next(ScriptState* script_state,
            String& key,
            v8::Local<v8::Value>& value,
            ExceptionState& exception_state) override {
    std::unique_ptr<WebRTCStats> stats = report_->Next();
    if (!stats)
      return false;
    key = stats->Id();
    value = WebRTCStatsToValue(script_state, stats.get());
    return true;
  }

 private:
  std::unique_ptr<WebRTCStatsReport> report_;
};

}  // namespace

RTCStatsReport::RTCStatsReport(std::unique_ptr<WebRTCStatsReport> report)
    : report_(std::move(report)) {}

PairIterable<String, v8::Local<v8::Value>>::IterationSource*
RTCStatsReport::StartIteration(ScriptState*, ExceptionState&) {
  return new RTCStatsReportIterationSource(report_->CopyHandle());
}

bool RTCStatsReport::GetMapEntry(ScriptState* script_state,
                                 const String& key,
                                 v8::Local<v8::Value>& value,
                                 ExceptionState&) {
  std::unique_ptr<WebRTCStats> stats = report_->GetStats(key);
  if (!stats)
    return false;
  value = WebRTCStatsToValue(script_state, stats.get());
  return true;
}

}  // namespace blink
