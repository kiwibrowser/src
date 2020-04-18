// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/process_proxy/process_proxy_registry.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/lazy_task_runner.h"

namespace chromeos {

namespace {

const char kWatcherThreadName[] = "ProcessWatcherThread";

const char kStdoutOutputType[] = "stdout";
const char kExitOutputType[] = "exit";

const char* ProcessOutputTypeToString(ProcessOutputType type) {
  switch (type) {
    case PROCESS_OUTPUT_TYPE_OUT:
      return kStdoutOutputType;
    case PROCESS_OUTPUT_TYPE_EXIT:
      return kExitOutputType;
    default:
      return NULL;
  }
}

static base::LazyInstance<ProcessProxyRegistry>::DestructorAtExit
    g_process_proxy_registry = LAZY_INSTANCE_INITIALIZER;

}  // namespace

ProcessProxyRegistry::ProcessProxyInfo::ProcessProxyInfo() = default;

ProcessProxyRegistry::ProcessProxyInfo::ProcessProxyInfo(
    const ProcessProxyInfo& other) {
  // This should be called with empty info only.
  DCHECK(!other.proxy.get());
}

ProcessProxyRegistry::ProcessProxyInfo::~ProcessProxyInfo() = default;

ProcessProxyRegistry::ProcessProxyRegistry() = default;

ProcessProxyRegistry::~ProcessProxyRegistry() {
  // TODO(tbarzic): Fix issue with ProcessProxyRegistry being destroyed
  // on a different thread (it's a LazyInstance).
  // DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  ShutDown();
}

void ProcessProxyRegistry::ShutDown() {
  // Close all proxies we own.
  while (!proxy_map_.empty())
    CloseProcess(proxy_map_.begin()->first);

  if (watcher_thread_) {
    watcher_thread_->Stop();
    watcher_thread_.reset();
  }
}

// static
ProcessProxyRegistry* ProcessProxyRegistry::Get() {
  DCHECK(ProcessProxyRegistry::GetTaskRunner()->RunsTasksInCurrentSequence());
  return g_process_proxy_registry.Pointer();
}

// static
scoped_refptr<base::SequencedTaskRunner> ProcessProxyRegistry::GetTaskRunner() {
  static base::LazySequencedTaskRunner task_runner =
      LAZY_SEQUENCED_TASK_RUNNER_INITIALIZER(
          base::TaskTraits({base::MayBlock(), base::TaskPriority::BACKGROUND}));
  return task_runner.Get();
}

int ProcessProxyRegistry::OpenProcess(const base::CommandLine& cmdline,
                                      const std::string& user_id_hash,
                                      const OutputCallback& output_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureWatcherThreadStarted())
    return -1;

  // Create and open new proxy.
  scoped_refptr<ProcessProxy> proxy(new ProcessProxy());
  // TODO(tbarzic): Use a random int as an id here instead of process pid.
  int terminal_id = proxy->Open(cmdline, user_id_hash);
  if (terminal_id < 0)
    return -1;

  // Kick off watcher.
  // We can use Unretained because proxy will stop calling callback after it is
  // closed, which is done before this object goes away.
  if (!proxy->StartWatchingOutput(
          watcher_thread_->task_runner(), GetTaskRunner(),
          base::Bind(&ProcessProxyRegistry::OnProcessOutput,
                     base::Unretained(this), terminal_id))) {
    proxy->Close();
    return -1;
  }

  ProcessProxyInfo& info = proxy_map_[terminal_id];
  info.proxy.swap(proxy);
  info.callback = output_callback;

  return terminal_id;
}

bool ProcessProxyRegistry::SendInput(int id, const std::string& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::map<int, ProcessProxyInfo>::iterator it = proxy_map_.find(id);
  if (it == proxy_map_.end())
    return false;
  return it->second.proxy->Write(data);
}

bool ProcessProxyRegistry::CloseProcess(int id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::map<int, ProcessProxyInfo>::iterator it = proxy_map_.find(id);
  if (it == proxy_map_.end())
    return false;

  it->second.proxy->Close();
  proxy_map_.erase(it);
  return true;
}

bool ProcessProxyRegistry::OnTerminalResize(int id, int width, int height) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::map<int, ProcessProxyInfo>::iterator it = proxy_map_.find(id);
  if (it == proxy_map_.end())
    return false;

  return it->second.proxy->OnTerminalResize(width, height);
}

void ProcessProxyRegistry::AckOutput(int id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::map<int, ProcessProxyInfo>::iterator it = proxy_map_.find(id);
  if (it == proxy_map_.end())
    return;

  it->second.proxy->AckOutput();
}

void ProcessProxyRegistry::OnProcessOutput(int id,
                                           ProcessOutputType type,
                                           const std::string& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const char* type_str = ProcessOutputTypeToString(type);
  DCHECK(type_str);

  std::map<int, ProcessProxyInfo>::iterator it = proxy_map_.find(id);
  if (it == proxy_map_.end())
    return;
  it->second.callback.Run(id, std::string(type_str), data);

  // Contact with the slave end of the terminal has been lost. We have to close
  // the process.
  if (type == PROCESS_OUTPUT_TYPE_EXIT)
    CloseProcess(id);
}

bool ProcessProxyRegistry::EnsureWatcherThreadStarted() {
  if (watcher_thread_.get())
    return true;

  // TODO(tbarzic): Change process output watcher to watch for fd readability on
  //    FILE thread, and move output reading to worker thread instead of
  //    spinning a new thread.
  watcher_thread_.reset(new base::Thread(kWatcherThreadName));
  return watcher_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
}

}  // namespace chromeos
