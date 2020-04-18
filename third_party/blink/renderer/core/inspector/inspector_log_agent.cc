// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/inspector_log_agent.h"

#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/frame/performance_monitor.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/console_message_storage.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_agent.h"
#include "third_party/blink/renderer/core/inspector/resolve_node.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"

namespace blink {

using protocol::Response;

namespace LogAgentState {
static const char kLogEnabled[] = "logEnabled";
static const char kLogViolations[] = "logViolations";
}  // namespace LogAgentState

namespace {

String MessageSourceValue(MessageSource source) {
  DCHECK(source != kConsoleAPIMessageSource);
  switch (source) {
    case kXMLMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Xml;
    case kJSMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Javascript;
    case kNetworkMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Network;
    case kStorageMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Storage;
    case kAppCacheMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Appcache;
    case kRenderingMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Rendering;
    case kSecurityMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Security;
    case kOtherMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Other;
    case kDeprecationMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Deprecation;
    case kWorkerMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Worker;
    case kViolationMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Violation;
    case kInterventionMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Intervention;
    case kRecommendationMessageSource:
      return protocol::Log::LogEntry::SourceEnum::Recommendation;
    default:
      return protocol::Log::LogEntry::SourceEnum::Other;
  }
}

String MessageLevelValue(MessageLevel level) {
  switch (level) {
    case kVerboseMessageLevel:
      return protocol::Log::LogEntry::LevelEnum::Verbose;
    case kInfoMessageLevel:
      return protocol::Log::LogEntry::LevelEnum::Info;
    case kWarningMessageLevel:
      return protocol::Log::LogEntry::LevelEnum::Warning;
    case kErrorMessageLevel:
      return protocol::Log::LogEntry::LevelEnum::Error;
  }
  return protocol::Log::LogEntry::LevelEnum::Info;
}

}  // namespace

using protocol::Log::ViolationSetting;

InspectorLogAgent::InspectorLogAgent(
    ConsoleMessageStorage* storage,
    PerformanceMonitor* performance_monitor,
    v8_inspector::V8InspectorSession* v8_session)
    : enabled_(false),
      storage_(storage),
      performance_monitor_(performance_monitor),
      v8_session_(v8_session) {}

InspectorLogAgent::~InspectorLogAgent() = default;

void InspectorLogAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(storage_);
  visitor->Trace(performance_monitor_);
  InspectorBaseAgent::Trace(visitor);
  PerformanceMonitor::Client::Trace(visitor);
}

void InspectorLogAgent::Restore() {
  if (!state_->booleanProperty(LogAgentState::kLogEnabled, false))
    return;
  enable();
  protocol::Value* config = state_->get(LogAgentState::kLogViolations);
  if (config) {
    protocol::ErrorSupport errors;
    startViolationsReport(
        protocol::Array<ViolationSetting>::fromValue(config, &errors));
  }
}

void InspectorLogAgent::ConsoleMessageAdded(ConsoleMessage* message) {
  DCHECK(enabled_);

  std::unique_ptr<protocol::Log::LogEntry> entry =
      protocol::Log::LogEntry::create()
          .setSource(MessageSourceValue(message->Source()))
          .setLevel(MessageLevelValue(message->Level()))
          .setText(message->Message())
          .setTimestamp(message->Timestamp())
          .build();
  if (!message->Location()->Url().IsEmpty())
    entry->setUrl(message->Location()->Url());
  std::unique_ptr<v8_inspector::protocol::Runtime::API::StackTrace>
      stack_trace = message->Location()->BuildInspectorObject();
  if (stack_trace)
    entry->setStackTrace(std::move(stack_trace));
  if (message->Location()->LineNumber())
    entry->setLineNumber(message->Location()->LineNumber() - 1);
  if (message->Source() == kWorkerMessageSource &&
      !message->WorkerId().IsEmpty())
    entry->setWorkerId(message->WorkerId());
  if (message->Source() == kNetworkMessageSource &&
      !message->RequestIdentifier().IsNull()) {
    entry->setNetworkRequestId(message->RequestIdentifier());
  }

  if (v8_session_ && message->Frame() && !message->Nodes().IsEmpty()) {
    ScriptForbiddenScope::AllowUserAgentScript allow_script;
    std::unique_ptr<
        protocol::Array<v8_inspector::protocol::Runtime::API::RemoteObject>>
        remote_objects = protocol::Array<
            v8_inspector::protocol::Runtime::API::RemoteObject>::create();
    for (DOMNodeId node_id : message->Nodes()) {
      std::unique_ptr<v8_inspector::protocol::Runtime::API::RemoteObject>
          remote_object = nullptr;
      Node* node = DOMNodeIds::NodeForId(node_id);
      if (node)
        remote_object = ResolveNode(v8_session_, node, "console");
      if (!remote_object) {
        remote_object =
            NullRemoteObject(v8_session_, message->Frame(), "console");
      }
      if (remote_object) {
        remote_objects->addItem(std::move(remote_object));
      } else {
        // If a null object could not be referenced, we do not send the message
        // at all, to avoid situations in which the arguments are misleading.
        return;
      }
    }
    entry->setArgs(std::move(remote_objects));
  }

  GetFrontend()->entryAdded(std::move(entry));
  GetFrontend()->flush();
}

Response InspectorLogAgent::enable() {
  if (enabled_)
    return Response::OK();
  instrumenting_agents_->addInspectorLogAgent(this);
  state_->setBoolean(LogAgentState::kLogEnabled, true);
  enabled_ = true;

  if (storage_->ExpiredCount()) {
    std::unique_ptr<protocol::Log::LogEntry> expired =
        protocol::Log::LogEntry::create()
            .setSource(protocol::Log::LogEntry::SourceEnum::Other)
            .setLevel(protocol::Log::LogEntry::LevelEnum::Warning)
            .setText(String::Number(storage_->ExpiredCount()) +
                     String(" log entries are not shown."))
            .setTimestamp(0)
            .build();
    GetFrontend()->entryAdded(std::move(expired));
    GetFrontend()->flush();
  }
  for (size_t i = 0; i < storage_->size(); ++i)
    ConsoleMessageAdded(storage_->at(i));
  return Response::OK();
}

Response InspectorLogAgent::disable() {
  if (!enabled_)
    return Response::OK();
  state_->setBoolean(LogAgentState::kLogEnabled, false);
  stopViolationsReport();
  enabled_ = false;
  instrumenting_agents_->removeInspectorLogAgent(this);
  return Response::OK();
}

Response InspectorLogAgent::clear() {
  storage_->Clear();
  return Response::OK();
}

static PerformanceMonitor::Violation ParseViolation(const String& name) {
  if (name == ViolationSetting::NameEnum::DiscouragedAPIUse)
    return PerformanceMonitor::kDiscouragedAPIUse;
  if (name == ViolationSetting::NameEnum::LongTask)
    return PerformanceMonitor::kLongTask;
  if (name == ViolationSetting::NameEnum::LongLayout)
    return PerformanceMonitor::kLongLayout;
  if (name == ViolationSetting::NameEnum::BlockedEvent)
    return PerformanceMonitor::kBlockedEvent;
  if (name == ViolationSetting::NameEnum::BlockedParser)
    return PerformanceMonitor::kBlockedParser;
  if (name == ViolationSetting::NameEnum::Handler)
    return PerformanceMonitor::kHandler;
  if (name == ViolationSetting::NameEnum::RecurringHandler)
    return PerformanceMonitor::kRecurringHandler;
  return PerformanceMonitor::kAfterLast;
}

Response InspectorLogAgent::startViolationsReport(
    std::unique_ptr<protocol::Array<ViolationSetting>> settings) {
  if (!enabled_)
    return Response::Error("Log is not enabled");
  state_->setValue(LogAgentState::kLogViolations, settings->toValue());
  if (!performance_monitor_)
    return Response::Error("Violations are not supported for this target");
  performance_monitor_->UnsubscribeAll(this);
  for (size_t i = 0; i < settings->length(); ++i) {
    PerformanceMonitor::Violation violation =
        ParseViolation(settings->get(i)->getName());
    if (violation == PerformanceMonitor::kAfterLast)
      continue;
    performance_monitor_->Subscribe(
        violation, settings->get(i)->getThreshold() / 1000, this);
  }
  return Response::OK();
}

Response InspectorLogAgent::stopViolationsReport() {
  state_->remove(LogAgentState::kLogViolations);
  if (!performance_monitor_)
    return Response::Error("Violations are not supported for this target");
  performance_monitor_->UnsubscribeAll(this);
  return Response::OK();
}

void InspectorLogAgent::ReportLongLayout(double duration) {
  String message_text =
      String::Format("Forced reflow while executing JavaScript took %ldms",
                     lround(duration * 1000));
  ConsoleMessage* message = ConsoleMessage::Create(
      kViolationMessageSource, kVerboseMessageLevel, message_text);
  ConsoleMessageAdded(message);
}

void InspectorLogAgent::ReportGenericViolation(PerformanceMonitor::Violation,
                                               const String& text,
                                               double time,
                                               SourceLocation* location) {
  ConsoleMessage* message = ConsoleMessage::Create(
      kViolationMessageSource, kVerboseMessageLevel, text, location->Clone());
  ConsoleMessageAdded(message);
};

}  // namespace blink
