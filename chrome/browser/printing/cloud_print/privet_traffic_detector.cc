// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_traffic_detector.h"

#include <stddef.h>

#include "base/location.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/sys_byteorder.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/base/network_interfaces.h"
#include "net/dns/dns_protocol.h"
#include "net/dns/dns_response.h"
#include "net/dns/mdns_client.h"
#include "net/log/net_log_source.h"
#include "net/socket/datagram_server_socket.h"
#include "net/socket/udp_server_socket.h"

namespace {

const int kMaxRestartAttempts = 10;
const char kPrivetDeviceTypeDnsString[] = "\x07_privet";

void GetNetworkListInBackground(
    const base::Callback<void(const net::NetworkInterfaceList&)> callback) {
  base::AssertBlockingAllowed();
  net::NetworkInterfaceList networks;
  if (!GetNetworkList(&networks, net::INCLUDE_HOST_SCOPE_VIRTUAL_INTERFACES))
    return;

  net::NetworkInterfaceList ip4_networks;
  for (size_t i = 0; i < networks.size(); ++i) {
    net::AddressFamily address_family =
        net::GetAddressFamily(networks[i].address);
    if (address_family == net::ADDRESS_FAMILY_IPV4 &&
        networks[i].prefix_length >= 24) {
      ip4_networks.push_back(networks[i]);
    }
  }

  net::IPAddress localhost_prefix(127, 0, 0, 0);
  ip4_networks.push_back(
      net::NetworkInterface("lo",
                            "lo",
                            0,
                            net::NetworkChangeNotifier::CONNECTION_UNKNOWN,
                            localhost_prefix,
                            8,
                            net::IP_ADDRESS_ATTRIBUTE_NONE));
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(callback, ip4_networks));
}

}  // namespace

namespace cloud_print {

PrivetTrafficDetector::PrivetTrafficDetector(
    net::AddressFamily address_family,
    const base::Closure& on_traffic_detected)
    : on_traffic_detected_(on_traffic_detected),
      callback_runner_(base::ThreadTaskRunnerHandle::Get()),
      address_family_(address_family),
      io_buffer_(
          new net::IOBufferWithSize(net::dns_protocol::kMaxMulticastSize)),
      restart_attempts_(kMaxRestartAttempts),
      weak_ptr_factory_(this) {}

void PrivetTrafficDetector::Start() {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PrivetTrafficDetector::StartOnIOThread,
                     weak_ptr_factory_.GetWeakPtr()));
}

PrivetTrafficDetector::~PrivetTrafficDetector() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void PrivetTrafficDetector::StartOnIOThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  ScheduleRestart();
}

void PrivetTrafficDetector::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  restart_attempts_ = kMaxRestartAttempts;
  if (type != net::NetworkChangeNotifier::CONNECTION_NONE)
    ScheduleRestart();
}

void PrivetTrafficDetector::ScheduleRestart() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  socket_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
  base::PostDelayedTaskWithTraits(
      FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
      base::BindOnce(&GetNetworkListInBackground,
                     base::Bind(&PrivetTrafficDetector::Restart,
                                weak_ptr_factory_.GetWeakPtr())),
      base::TimeDelta::FromSeconds(3));
}

void PrivetTrafficDetector::Restart(const net::NetworkInterfaceList& networks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  networks_ = networks;
  if (Bind() < net::OK || DoLoop(0) < net::OK) {
    if ((restart_attempts_--) > 0)
      ScheduleRestart();
  } else {
    // Reset on success.
    restart_attempts_ = kMaxRestartAttempts;
  }
}

int PrivetTrafficDetector::Bind() {
  if (!start_time_.is_null()) {
    base::TimeDelta time_delta = base::Time::Now() - start_time_;
    UMA_HISTOGRAM_LONG_TIMES("LocalDiscovery.DetectorRestartTime", time_delta);
  }
  start_time_ = base::Time::Now();
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  socket_.reset(new net::UDPServerSocket(NULL, net::NetLogSource()));
  net::IPEndPoint multicast_addr = net::GetMDnsIPEndPoint(address_family_);
  net::IPEndPoint bind_endpoint(
      net::IPAddress::AllZeros(multicast_addr.address().size()),
      multicast_addr.port());
  socket_->AllowAddressReuse();
  int rv = socket_->Listen(bind_endpoint);
  if (rv < net::OK)
    return rv;
  socket_->SetMulticastLoopbackMode(false);
  return socket_->JoinGroup(multicast_addr.address());
}

bool PrivetTrafficDetector::IsSourceAcceptable() const {
  for (size_t i = 0; i < networks_.size(); ++i) {
    if (net::IPAddressMatchesPrefix(recv_addr_.address(), networks_[i].address,
                                    networks_[i].prefix_length)) {
      return true;
    }
  }
  return false;
}

bool PrivetTrafficDetector::IsPrivetPacket(int rv) const {
  if (rv <= static_cast<int>(sizeof(net::dns_protocol::Header)) ||
      !IsSourceAcceptable()) {
    return false;
  }

  const char* buffer_begin = io_buffer_->data();
  const char* buffer_end = buffer_begin + rv;
  const net::dns_protocol::Header* header =
      reinterpret_cast<const net::dns_protocol::Header*>(buffer_begin);
  // Check if response packet.
  if (!(header->flags & base::HostToNet16(net::dns_protocol::kFlagResponse)))
    return false;
  const char* substring_begin = kPrivetDeviceTypeDnsString;
  const char* substring_end = substring_begin +
                              arraysize(kPrivetDeviceTypeDnsString) - 1;
  // Check for expected substring, any Privet device must include this.
  return std::search(buffer_begin, buffer_end, substring_begin,
                     substring_end) != buffer_end;
}

int PrivetTrafficDetector::DoLoop(int rv) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  do {
    if (IsPrivetPacket(rv)) {
      socket_.reset();
      callback_runner_->PostTask(FROM_HERE, on_traffic_detected_);
      base::TimeDelta time_delta = base::Time::Now() - start_time_;
      UMA_HISTOGRAM_LONG_TIMES("LocalDiscovery.DetectorTriggerTime",
                               time_delta);
      return net::OK;
    }

    rv = socket_->RecvFrom(
        io_buffer_.get(),
        io_buffer_->size(),
        &recv_addr_,
        base::Bind(base::IgnoreResult(&PrivetTrafficDetector::DoLoop),
                   base::Unretained(this)));
  } while (rv > 0);

  if (rv != net::ERR_IO_PENDING)
    return rv;

  return net::OK;
}

}  // namespace cloud_print
