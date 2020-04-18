// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_AEC_DUMP_MESSAGE_FILTER_H_
#define CONTENT_RENDERER_MEDIA_STREAM_AEC_DUMP_MESSAGE_FILTER_H_

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "content/common/content_export.h"
#include "content/renderer/render_thread_impl.h"
#include "ipc/ipc_platform_file.h"
#include "ipc/message_filter.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

// MessageFilter that handles AEC dump messages and forwards them to an
// observer.
// TODO(hlundin): Rename class to reflect expanded use; http://crbug.com/709919.
class CONTENT_EXPORT AecDumpMessageFilter : public IPC::MessageFilter {
 public:
  class AecDumpDelegate {
   public:
    virtual void OnAecDumpFile(
        const IPC::PlatformFileForTransit& file_handle) = 0;
    virtual void OnDisableAecDump() = 0;
    virtual void OnIpcClosing() = 0;
  };

  AecDumpMessageFilter(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner);

  // Getter for the one AecDumpMessageFilter object.
  static scoped_refptr<AecDumpMessageFilter> Get();

  // Adds a delegate that receives the enable and disable notifications. Must be
  // called on the main task runner (|main_task_runner| in constructor). All
  // calls on |delegate| are done on the main task runner.
  void AddDelegate(AecDumpMessageFilter::AecDumpDelegate* delegate);

  // Removes a delegate. Must be called on the main task runner
  // (|main_task_runner| in constructor).
  void RemoveDelegate(AecDumpMessageFilter::AecDumpDelegate* delegate);

  // IO task runner associated with this message filter.
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner() const {
    return io_task_runner_;
  }

  // Returns the AEC3 setting. Must be called on the main task runner
  // (|main_task_runner| in constructor).
  base::Optional<bool> GetOverrideAec3() const;

 protected:
  ~AecDumpMessageFilter() override;

  // When this variable is not set, the use of AEC3 is governed by the Finch
  // experiment and/or WebRTC's own default. When set to true/false, Finch and
  // WebRTC defaults will be overridden, and AEC3/AEC2 (respectively) will be
  // used.
  base::Optional<bool> override_aec3_;

 private:
  // Sends an IPC message using |sender_|.
  void Send(IPC::Message* message);

  // Registers a consumer of AEC dump in the browser process. This consumer will
  // get a file handle when the AEC dump is enabled and a notification when it
  // is disabled.
  void RegisterAecDumpConsumer(int id);

  // Unregisters a consumer of AEC dump in the browser process.
  void UnregisterAecDumpConsumer(int id);

  // IPC::MessageFilter override. Called on |io_task_runner|.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnFilterAdded(IPC::Channel* channel) override;
  void OnFilterRemoved() override;
  void OnChannelClosing() override;

  // Accessed on |io_task_runner_|.
  void OnEnableAecDump(int id, IPC::PlatformFileForTransit file_handle);
  void OnDisableAecDump();
  void OnEnableAec3(bool enable);

  // Accessed on |main_task_runner_|.
  void DoEnableAecDump(int id, IPC::PlatformFileForTransit file_handle);
  void DoDisableAecDump();
  void DoChannelClosingOnDelegates();
  int GetIdForDelegate(AecDumpMessageFilter::AecDumpDelegate* delegate);
  void DoEnableAec3(bool enable);

  // Accessed on |io_task_runner_|.
  IPC::Sender* sender_;

  // The delgates for this filter. Must only be accessed on
  // |main_task_runner_|.
  typedef std::map<int, AecDumpMessageFilter::AecDumpDelegate*> DelegateMap;
  DelegateMap delegates_;

  // Counter for generating unique IDs to delegates. Accessed on
  // |main_task_runner_|.
  int delegate_id_counter_;

  // Task runner which IPC calls are executed.
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Main task runner.
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // The singleton instance for this filter.
  static AecDumpMessageFilter* g_filter;

  DISALLOW_COPY_AND_ASSIGN(AecDumpMessageFilter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_AEC_DUMP_MESSAGE_FILTER_H_
