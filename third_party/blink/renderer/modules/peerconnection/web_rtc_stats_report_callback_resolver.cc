#include "third_party/blink/renderer/modules/peerconnection/web_rtc_stats_report_callback_resolver.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"

namespace blink {

// static
std::unique_ptr<WebRTCStatsReportCallback>
WebRTCStatsReportCallbackResolver::Create(ScriptPromiseResolver* resolver) {
  return std::unique_ptr<WebRTCStatsReportCallback>(
      new WebRTCStatsReportCallbackResolver(resolver));
}

WebRTCStatsReportCallbackResolver::WebRTCStatsReportCallbackResolver(
    ScriptPromiseResolver* resolver)
    : resolver_(resolver) {}

WebRTCStatsReportCallbackResolver::~WebRTCStatsReportCallbackResolver() {
  DCHECK(
      ExecutionContext::From(resolver_->GetScriptState())->IsContextThread());
}

void WebRTCStatsReportCallbackResolver::OnStatsDelivered(
    std::unique_ptr<WebRTCStatsReport> report) {
  DCHECK(
      ExecutionContext::From(resolver_->GetScriptState())->IsContextThread());
  resolver_->Resolve(new RTCStatsReport(std::move(report)));
}

}  // namespace blink
