// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_permission_broker_client.h"

#include <fcntl.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

namespace {

const char kOpenFailedError[] = "open_failed";

// So that real devices can be accessed by tests and "Chromium OS on Linux" this
// function implements a simplified version of the method implemented by the
// permission broker by opening the path specified and returning the resulting
// file descriptor.
void OpenPath(const std::string& path,
              const PermissionBrokerClient::OpenPathCallback& callback,
              const PermissionBrokerClient::ErrorCallback& error_callback,
              scoped_refptr<base::TaskRunner> task_runner) {
  base::ScopedFD fd(HANDLE_EINTR(open(path.c_str(), O_RDWR)));
  if (!fd.is_valid()) {
    int error_code = logging::GetLastSystemErrorCode();
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            error_callback, kOpenFailedError,
            base::StringPrintf(
                "Failed to open '%s': %s", path.c_str(),
                logging::SystemErrorCodeToString(error_code).c_str())));
    return;
  }

  task_runner->PostTask(FROM_HERE, base::BindOnce(callback, std::move(fd)));
}

}  // namespace

FakePermissionBrokerClient::FakePermissionBrokerClient() = default;

FakePermissionBrokerClient::~FakePermissionBrokerClient() = default;

void FakePermissionBrokerClient::Init(dbus::Bus* bus) {}

void FakePermissionBrokerClient::CheckPathAccess(
    const std::string& path,
    const ResultCallback& callback) {
  callback.Run(true);
}

void FakePermissionBrokerClient::OpenPath(const std::string& path,
                                          const OpenPathCallback& callback,
                                          const ErrorCallback& error_callback) {
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&chromeos::OpenPath, path, callback, error_callback,
                     base::ThreadTaskRunnerHandle::Get()));
}

void FakePermissionBrokerClient::RequestTcpPortAccess(
    uint16_t port,
    const std::string& interface,
    int lifeline_fd,
    const ResultCallback& callback) {
  callback.Run(
      RequestPortImpl(port, interface, tcp_deny_rule_set_, &tcp_hole_set_));
}

void FakePermissionBrokerClient::RequestUdpPortAccess(
    uint16_t port,
    const std::string& interface,
    int lifeline_fd,
    const ResultCallback& callback) {
  callback.Run(
      RequestPortImpl(port, interface, udp_deny_rule_set_, &udp_hole_set_));
}

void FakePermissionBrokerClient::ReleaseTcpPort(
    uint16_t port,
    const std::string& interface,
    const ResultCallback& callback) {
  callback.Run(tcp_hole_set_.erase(std::make_pair(port, interface)));
}

void FakePermissionBrokerClient::ReleaseUdpPort(
    uint16_t port,
    const std::string& interface,
    const ResultCallback& callback) {
  callback.Run(udp_hole_set_.erase(std::make_pair(port, interface)));
}

void FakePermissionBrokerClient::AddTcpDenyRule(uint16_t port,
                                                const std::string& interface) {
  tcp_deny_rule_set_.insert(std::make_pair(port, interface));
}

void FakePermissionBrokerClient::AddUdpDenyRule(uint16_t port,
                                                const std::string& interface) {
  udp_deny_rule_set_.insert(std::make_pair(port, interface));
}

bool FakePermissionBrokerClient::HasTcpHole(uint16_t port,
                                            const std::string& interface) {
  auto rule = std::make_pair(port, interface);
  return tcp_hole_set_.find(rule) != tcp_hole_set_.end();
}

bool FakePermissionBrokerClient::HasUdpHole(uint16_t port,
                                            const std::string& interface) {
  auto rule = std::make_pair(port, interface);
  return udp_hole_set_.find(rule) != udp_hole_set_.end();
}

bool FakePermissionBrokerClient::RequestPortImpl(uint16_t port,
                                                 const std::string& interface,
                                                 const RuleSet& deny_rule_set,
                                                 RuleSet* hole_set) {
  auto rule = std::make_pair(port, interface);

  // If there is already a hole, returns true.
  if (hole_set->find(rule) != hole_set->end())
    return true;

  // If it is denied to make a hole, returns false.
  if (deny_rule_set.find(rule) != deny_rule_set.end())
    return false;

  hole_set->insert(rule);
  return true;
}

}  // namespace chromeos
