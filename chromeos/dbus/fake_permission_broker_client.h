// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_PERMISSION_BROKER_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_PERMISSION_BROKER_CLIENT_H_

#include <stdint.h>

#include <set>
#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chromeos/dbus/permission_broker_client.h"

namespace chromeos {

class CHROMEOS_EXPORT FakePermissionBrokerClient
    : public PermissionBrokerClient {
 public:
  FakePermissionBrokerClient();
  ~FakePermissionBrokerClient() override;

  void Init(dbus::Bus* bus) override;
  void CheckPathAccess(const std::string& path,
                       const ResultCallback& callback) override;
  void OpenPath(const std::string& path,
                const OpenPathCallback& callback,
                const ErrorCallback& error_callback) override;
  void RequestTcpPortAccess(uint16_t port,
                            const std::string& interface,
                            int lifeline_fd,
                            const ResultCallback& callback) override;
  void RequestUdpPortAccess(uint16_t port,
                            const std::string& interface,
                            int lifeline_fd,
                            const ResultCallback& callback) override;
  void ReleaseTcpPort(uint16_t port,
                      const std::string& interface,
                      const ResultCallback& callback) override;
  void ReleaseUdpPort(uint16_t port,
                      const std::string& interface,
                      const ResultCallback& callback) override;

  // Add a rule to have RequestTcpPortAccess fail.
  void AddTcpDenyRule(uint16_t port, const std::string& interface);

  // Add a rule to have RequestTcpPortAccess fail.
  void AddUdpDenyRule(uint16_t port, const std::string& interface);

  // Returns true if TCP port has a hole.
  bool HasTcpHole(uint16_t port, const std::string& interface);

  // Returns true if UDP port has a hole.
  bool HasUdpHole(uint16_t port, const std::string& interface);

 private:
  using RuleSet =
      std::set<std::pair<uint16_t /* port */, std::string /* interface */>>;

  bool RequestPortImpl(uint16_t port,
                       const std::string& interface,
                       const RuleSet& deny_rule_set,
                       RuleSet* hole_set);

  RuleSet tcp_hole_set_;
  RuleSet udp_hole_set_;

  RuleSet tcp_deny_rule_set_;
  RuleSet udp_deny_rule_set_;

  DISALLOW_COPY_AND_ASSIGN(FakePermissionBrokerClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_PERMISSION_BROKER_CLIENT_H_
