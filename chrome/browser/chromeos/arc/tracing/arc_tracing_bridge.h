// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_TRACING_ARC_TRACING_BRIDGE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_TRACING_ARC_TRACING_BRIDGE_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/ring_buffer.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/trace_event/trace_event.h"
#include "components/arc/common/tracing.mojom.h"
#include "components/arc/connection_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "services/tracing/public/cpp/base_agent.h"
#include "services/tracing/public/mojom/tracing.mojom.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

// This class provides the interface to trigger tracing in the container.
class ArcTracingBridge : public KeyedService,
                         public tracing::BaseAgent,
                         public ConnectionObserver<mojom::TracingInstance> {
 public:
  // Returns singleton instance for the given BrowserContext,
  // or nullptr if the browser |context| is not allowed to use ARC.
  static ArcTracingBridge* GetForBrowserContext(
      content::BrowserContext* context);

  ArcTracingBridge(content::BrowserContext* context,
                   ArcBridgeService* bridge_service);
  ~ArcTracingBridge() override;

  // ConnectionObserver<mojom::TracingInstance> overrides:
  void OnConnectionReady() override;

 private:
  // A helper class for reading trace data from the client side. We separate
  // this from |ArcTracingAgentImpl| to isolate the logic that runs on browser's
  // IO thread. All the functions in this class except for constructor,
  // destructor, and |GetWeakPtr| are expected to be run on browser's IO thread.
  class ArcTracingReader {
   public:
    ArcTracingReader();
    ~ArcTracingReader();

    void StartTracing(base::ScopedFD read_fd);
    void OnTraceDataAvailable();
    void StopTracing(base::OnceCallback<void(const std::string&)> callback);
    base::WeakPtr<ArcTracingReader> GetWeakPtr();

   private:
    // Number of events for the ring buffer.
    static constexpr size_t kTraceEventBufferSize = 64000;

    base::ScopedFD read_fd_;
    std::unique_ptr<base::FileDescriptorWatcher::Controller> fd_watcher_;
    base::RingBuffer<std::string, kTraceEventBufferSize> ring_buffer_;
    // NOTE: Weak pointers must be invalidated before all other member variables
    // so it must be the last member.
    base::WeakPtrFactory<ArcTracingReader> weak_ptr_factory_;

    DISALLOW_COPY_AND_ASSIGN(ArcTracingReader);
  };

  struct Category;

  // tracing::mojom::Agent.
  void StartTracing(const std::string& config,
                    base::TimeTicks coordinator_time,
                    Agent::StartTracingCallback callback) override;
  void StopAndFlush(tracing::mojom::RecorderPtr recorder) override;

  // Callback for QueryAvailableCategories.
  void OnCategoriesReady(const std::vector<std::string>& categories);

  void OnArcTracingStopped(bool success);
  void OnTracingReaderStopped(const std::string& data);

  ArcBridgeService* const arc_bridge_service_;  // Owned by ArcServiceManager.

  // List of available categories.
  std::vector<Category> categories_;

  // We use |reader_.GetWeakPtr()| when binding callbacks with its functions.
  // Notes that the weak pointer returned by it can only be deferenced or
  // invalided in the same task runner to avoid racing condition. The
  // destruction of |reader_| is also a source of invalidation. However, we're
  // lucky as we're using |ArcTracingAgentImpl| as a singleton, the
  // destruction is always performed after all MessageLoops are destroyed, and
  // thus there are no race conditions in such situation.
  ArcTracingReader reader_;
  bool is_stopping_ = false;
  tracing::mojom::RecorderPtr recorder_;

  // NOTE: Weak pointers must be invalidated before all other member variables
  // so it must be the last member.
  base::WeakPtrFactory<ArcTracingBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTracingBridge);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_TRACING_ARC_TRACING_BRIDGE_H_
