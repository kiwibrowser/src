// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_PROCESS_PROXY_PROCESS_PROXY_REGISTRY_H_
#define CHROMEOS_PROCESS_PROXY_PROCESS_PROXY_REGISTRY_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/process_proxy/process_proxy.h"

namespace chromeos {

// Keeps track of all created ProcessProxies. It is created lazily and should
// live on a single thread (where all methods must be called).
class CHROMEOS_EXPORT ProcessProxyRegistry {
 public:
  using OutputCallback = base::Callback<void(int terminal_id,
                                             const std::string& output_type,
                                             const std::string& output_data)>;

  // Info we need about a ProcessProxy instance.
  struct ProcessProxyInfo {
    scoped_refptr<ProcessProxy> proxy;
    OutputCallback callback;

    ProcessProxyInfo();
    // This is to make map::insert happy, we don't init anything.
    ProcessProxyInfo(const ProcessProxyInfo& other);
    ~ProcessProxyInfo();
  };

  static ProcessProxyRegistry* Get();

  // Returns a SequencedTaskRunner where the singleton instance of
  // ProcessProxyRegistry lives.
  static scoped_refptr<base::SequencedTaskRunner> GetTaskRunner();

  // Starts new ProcessProxy (which starts new process).
  // Returns ID used for the created process. Returns -1 on failure.
  int OpenProcess(const base::CommandLine& cmdline,
                  const std::string& user_id_hash,
                  const OutputCallback& callback);
  // Sends data to the process identified by |id|.
  bool SendInput(int id, const std::string& data);
  // Stops the process identified by |id|.
  bool CloseProcess(int id);
  // Reports terminal resize to process proxy.
  bool OnTerminalResize(int id, int width, int height);
  // Notifies process proxy identified by |id| that previously reported output
  // has been handled.
  void AckOutput(int id);

  // Shuts down registry, closing all associated processed.
  void ShutDown();

 private:
  friend struct ::base::LazyInstanceTraitsBase<ProcessProxyRegistry>;

  ProcessProxyRegistry();
  ~ProcessProxyRegistry();

  // Gets called when output gets detected.
  void OnProcessOutput(int id, ProcessOutputType type, const std::string& data);

  bool EnsureWatcherThreadStarted();

  // Map of all existing ProcessProxies.
  std::map<int, ProcessProxyInfo> proxy_map_;

  std::unique_ptr<base::Thread> watcher_thread_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ProcessProxyRegistry);
};

}  // namespace chromeos

#endif  // CHROMEOS_PROCESS_PROXY_PROCESS_PROXY_REGISTRY_H_
