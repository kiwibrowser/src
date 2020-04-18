/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef CALL_FAKE_NETWORK_PIPE_H_
#define CALL_FAKE_NETWORK_PIPE_H_

#include <deque>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "api/call/transport.h"
#include "call/call.h"
#include "common_types.h"  // NOLINT(build/include)
#include "modules/include/module.h"
#include "rtc_base/constructormagic.h"
#include "rtc_base/criticalsection.h"
#include "rtc_base/random.h"
#include "rtc_base/thread_annotations.h"
#include "typedefs.h"  // NOLINT(build/include)

namespace webrtc {

class Clock;
class PacketReceiver;
enum class MediaType;

class NetworkPacket {
 public:
  NetworkPacket(rtc::CopyOnWriteBuffer packet,
                int64_t send_time,
                int64_t arrival_time,
                rtc::Optional<PacketOptions> packet_options,
                bool is_rtcp,
                MediaType media_type_,
                rtc::Optional<PacketTime> packet_time_);
  // Disallow copy constructor and copy assignment (no deep copies of |data_|).
  NetworkPacket(const NetworkPacket&) = delete;
  NetworkPacket& operator=(const NetworkPacket&) = delete;
  // Allow move constructor/assignment, so that we can use in stl containers.
  NetworkPacket(NetworkPacket&&);
  NetworkPacket& operator=(NetworkPacket&&);

  const uint8_t* data() const { return packet_.data(); }
  size_t data_length() const { return packet_.size(); }
  rtc::CopyOnWriteBuffer* raw_packet() { return &packet_; }
  int64_t send_time() const { return send_time_; }
  int64_t arrival_time() const { return arrival_time_; }
  void IncrementArrivalTime(int64_t extra_delay) {
    arrival_time_ += extra_delay;
  }
  PacketOptions packet_options() const {
    return packet_options_.value_or(PacketOptions());
  }
  bool is_rtcp() const { return is_rtcp_; }
  MediaType media_type() const { return media_type_; }
  PacketTime packet_time() const { return packet_time_.value_or(PacketTime()); }

 private:
  rtc::CopyOnWriteBuffer packet_;
  // The time the packet was sent out on the network.
  int64_t send_time_;
  // The time the packet should arrive at the receiver.
  int64_t arrival_time_;
  // If using a Transport for outgoing degradation, populate with
  // PacketOptions (transport-wide sequence number) for RTP.
  rtc::Optional<PacketOptions> packet_options_;
  bool is_rtcp_;
  // If using a PacketReceiver for incoming degradation, populate with
  // appropriate MediaType and PacketTime. This type/timing will be kept and
  // forwarded. The PacketTime might be altered to reflect time spent in fake
  // network pipe.
  MediaType media_type_;
  rtc::Optional<PacketTime> packet_time_;
};

struct PacketInFlightInfo {
  PacketInFlightInfo(size_t size, int64_t send_time_us, uint64_t packet_id)
      : size(size), send_time_us(send_time_us), packet_id(packet_id) {}

  size_t size;
  int64_t send_time_us;
  // Unique identifier for the packet in relation to other packets in flight.
  uint64_t packet_id;
};

struct PacketDeliveryInfo {
  static constexpr int kNotReceived = -1;
  PacketDeliveryInfo(PacketInFlightInfo source, int64_t receive_time_us)
      : receive_time_us(receive_time_us), packet_id(source.packet_id) {}
  int64_t receive_time_us;
  uint64_t packet_id;
};

class NetworkSimulationInterface {
 public:
  virtual bool EnqueuePacket(PacketInFlightInfo packet_info) = 0;
  // Retrieves all packets that should be delivered by the given receive time.
  virtual std::vector<PacketDeliveryInfo> DequeueDeliverablePackets(
      int64_t receive_time_us) = 0;
  virtual rtc::Optional<int64_t> NextDeliveryTimeUs() const = 0;
  virtual ~NetworkSimulationInterface() = default;
};

// Class simulating a network link. This is a simple and naive solution just
// faking capacity and adding an extra transport delay in addition to the
// capacity introduced delay.
class SimulatedNetwork : public NetworkSimulationInterface {
 public:
  struct Config {
    Config() {}
    // Queue length in number of packets.
    size_t queue_length_packets = 0;
    // Delay in addition to capacity induced delay.
    int queue_delay_ms = 0;
    // Standard deviation of the extra delay.
    int delay_standard_deviation_ms = 0;
    // Link capacity in kbps.
    int link_capacity_kbps = 0;
    // Random packet loss.
    int loss_percent = 0;
    // If packets are allowed to be reordered.
    bool allow_reordering = false;
    // The average length of a burst of lost packets.
    int avg_burst_loss_length = -1;
  };
  explicit SimulatedNetwork(Config config, uint64_t random_seed = 1);

  // Sets a new configuration. This won't affect packets already in the pipe.
  void SetConfig(const Config& config);

  // NetworkSimulationInterface
  bool EnqueuePacket(PacketInFlightInfo packet) override;
  std::vector<PacketDeliveryInfo> DequeueDeliverablePackets(
      int64_t receive_time_us) override;

  rtc::Optional<int64_t> NextDeliveryTimeUs() const override;

 private:
  struct PacketInfo {
    PacketInFlightInfo packet;
    int64_t arrival_time_us;
  };
  rtc::CriticalSection config_lock_;

  // |process_lock| guards the data structures involved in delay and loss
  // processes, such as the packet queues.
  rtc::CriticalSection process_lock_;
  std::queue<PacketInfo> capacity_link_ RTC_GUARDED_BY(process_lock_);
  Random random_;

  std::deque<PacketInfo> delay_link_;

  // Link configuration.
  Config config_ RTC_GUARDED_BY(config_lock_);

  // Are we currently dropping a burst of packets?
  bool bursting_;

  // The probability to drop the packet if we are currently dropping a
  // burst of packet
  double prob_loss_bursting_ RTC_GUARDED_BY(config_lock_);

  // The probability to drop a burst of packets.
  double prob_start_bursting_ RTC_GUARDED_BY(config_lock_);
  int64_t capacity_delay_error_bytes_ = 0;
};

// Class faking a network link, internally is uses an implementation of a
// SimulatedNetworkInterface to simulate network behavior.
class FakeNetworkPipe : public Transport, public PacketReceiver, public Module {
 public:
  using Config = SimulatedNetwork::Config;

  // Use these constructors if you plan to insert packets using DeliverPacket().
  FakeNetworkPipe(Clock* clock, const FakeNetworkPipe::Config& config);
  FakeNetworkPipe(Clock* clock,
                  const FakeNetworkPipe::Config& config,
                  PacketReceiver* receiver);
  FakeNetworkPipe(Clock* clock,
                  const FakeNetworkPipe::Config& config,
                  PacketReceiver* receiver,
                  uint64_t seed);

  // Use this constructor if you plan to insert packets using SendRt[c?]p().
  FakeNetworkPipe(Clock* clock,
                  const FakeNetworkPipe::Config& config,
                  Transport* transport);

  virtual ~FakeNetworkPipe();

  void SetClockOffset(int64_t offset_ms);

  // Sets a new configuration. This won't affect packets already in the pipe.
  void SetConfig(const FakeNetworkPipe::Config& config);

  // Must not be called in parallel with DeliverPacket or Process.
  void SetReceiver(PacketReceiver* receiver);

  // Implements Transport interface. When/if packets are delivered, they will
  // be passed to the transport instance given in SetReceiverTransport(). These
  // methods should only be called if a Transport instance was provided in the
  // constructor.
  bool SendRtp(const uint8_t* packet,
               size_t length,
               const PacketOptions& options) override;
  bool SendRtcp(const uint8_t* packet, size_t length) override;

  // Implements the PacketReceiver interface. When/if packets are delivered,
  // they will be passed directly to the receiver instance given in
  // SetReceiver(), without passing through a Demuxer. The receive time in
  // PacketTime will be increased by the amount of time the packet spent in the
  // fake network pipe.
  PacketReceiver::DeliveryStatus DeliverPacket(
      MediaType media_type,
      rtc::CopyOnWriteBuffer packet,
      const PacketTime& packet_time) override;

  // Processes the network queues and trigger PacketReceiver::IncomingPacket for
  // packets ready to be delivered.
  void Process() override;
  int64_t TimeUntilNextProcess() override;

  // Get statistics.
  float PercentageLoss();
  int AverageDelay();
  size_t DroppedPackets();
  size_t SentPackets();
  void ResetStats();

 protected:
  void DeliverPacketWithLock(NetworkPacket* packet);
  void AddToPacketDropCount();
  void AddToPacketSentCount(int count);
  void AddToTotalDelay(int delay_us);
  int64_t GetTimeInMicroseconds() const;
  bool ShouldProcess(int64_t time_now_us) const;
  void SetTimeToNextProcess(int64_t skip_us);

 private:
  struct StoredPacket {
    NetworkPacket packet;
    bool removed = false;
    explicit StoredPacket(NetworkPacket&& packet);
    StoredPacket(StoredPacket&&) = default;
    StoredPacket(const StoredPacket&) = delete;
    StoredPacket& operator=(const StoredPacket&) = delete;
    StoredPacket() = delete;
  };

  // Returns true if enqueued, or false if packet was dropped.
  virtual bool EnqueuePacket(rtc::CopyOnWriteBuffer packet,
                     rtc::Optional<PacketOptions> options,
                     bool is_rtcp,
                     MediaType media_type,
                     rtc::Optional<PacketTime> packet_time);
  void DeliverPacket(NetworkPacket* packet)
      RTC_EXCLUSIVE_LOCKS_REQUIRED(config_lock_);
  bool HasTransport() const;
  bool HasReceiver() const;

  Clock* const clock_;
  // |config_lock| guards the mostly constant things like the callbacks.
  rtc::CriticalSection config_lock_;
  const std::unique_ptr<SimulatedNetwork> network_simulation_;
  PacketReceiver* receiver_ RTC_GUARDED_BY(config_lock_);
  Transport* const transport_ RTC_GUARDED_BY(config_lock_);

  // |process_lock| guards the data structures involved in delay and loss
  // processes, such as the packet queues.
  rtc::CriticalSection process_lock_;

  // Packets  are added at the back of the deque, this makes the deque ordered
  // by increasing send time. The common case when removing packets from the
  // deque is removing early packets, which will be close to the front of the
  // deque. This makes finding the packets in the deque efficient in the common
  // case.
  std::deque<StoredPacket> packets_in_flight_ RTC_GUARDED_BY(process_lock_);

  int64_t clock_offset_ms_ RTC_GUARDED_BY(config_lock_);

  // Statistics.
  size_t dropped_packets_ RTC_GUARDED_BY(process_lock_);
  size_t sent_packets_ RTC_GUARDED_BY(process_lock_);
  int64_t total_packet_delay_us_ RTC_GUARDED_BY(process_lock_);

  int64_t next_process_time_us_;

  int64_t last_log_time_us_;

  RTC_DISALLOW_COPY_AND_ASSIGN(FakeNetworkPipe);
};

}  // namespace webrtc

#endif  // CALL_FAKE_NETWORK_PIPE_H_
