/* Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This is EXPERIMENTAL interface for media transport.
//
// The goal is to refactor WebRTC code so that audio and video frames
// are sent / received through the media transport interface. This will
// enable different media transport implementations, including QUIC-based
// media transport.

#ifndef API_MEDIA_TRANSPORT_INTERFACE_H_
#define API_MEDIA_TRANSPORT_INTERFACE_H_

#include <api/transport/network_control.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "api/array_view.h"
#include "api/rtc_error.h"
#include "api/units/data_rate.h"
#include "api/video/encoded_image.h"
#include "rtc_base/copy_on_write_buffer.h"
#include "rtc_base/network_route.h"

namespace rtc {
class PacketTransportInternal;
class Thread;
}  // namespace rtc

namespace webrtc {

class RtcEventLog;

class AudioPacketReceivedObserver {
 public:
  virtual ~AudioPacketReceivedObserver() = default;

  // Invoked for the first received audio packet on a given channel id.
  // It will be invoked once for each channel id.
  virtual void OnFirstAudioPacketReceived(int64_t channel_id) = 0;
};

struct MediaTransportAllocatedBitrateLimits {
  DataRate min_pacing_rate = DataRate::Zero();
  DataRate max_padding_bitrate = DataRate::Zero();
  DataRate max_total_allocated_bitrate = DataRate::Zero();
};

// A collection of settings for creation of media transport.
struct MediaTransportSettings final {
  MediaTransportSettings();
  MediaTransportSettings(const MediaTransportSettings&);
  MediaTransportSettings& operator=(const MediaTransportSettings&);
  ~MediaTransportSettings();

  // Group calls are not currently supported, in 1:1 call one side must set
  // is_caller = true and another is_caller = false.
  bool is_caller;

  // Must be set if a pre-shared key is used for the call.
  // TODO(bugs.webrtc.org/9944): This should become zero buffer in the distant
  // future.
  absl::optional<std::string> pre_shared_key;

  // If present, provides the event log that media transport should use.
  // Media transport does not own it. The lifetime of |event_log| will exceed
  // the lifetime of the instance of MediaTransportInterface instance.
  RtcEventLog* event_log = nullptr;
};

// Represents encoded audio frame in any encoding (type of encoding is opaque).
// To avoid copying of encoded data use move semantics when passing by value.
class MediaTransportEncodedAudioFrame final {
 public:
  enum class FrameType {
    // Normal audio frame (equivalent to webrtc::kAudioFrameSpeech).
    kSpeech,

    // DTX frame (equivalent to webrtc::kAudioFrameCN).
    // DTX frame (equivalent to webrtc::kAudioFrameCN).
    kDiscontinuousTransmission,
    // TODO(nisse): Mis-spelled version, update users, then delete.
    kDiscountinuousTransmission = kDiscontinuousTransmission,
  };

  MediaTransportEncodedAudioFrame(
      // Audio sampling rate, for example 48000.
      int sampling_rate_hz,

      // Starting sample index of the frame, i.e. how many audio samples were
      // before this frame since the beginning of the call or beginning of time
      // in one channel (the starting point should not matter for NetEq). In
      // WebRTC it is used as a timestamp of the frame.
      // TODO(sukhanov): Starting_sample_index is currently adjusted on the
      // receiver side in RTP path. Non-RTP implementations should preserve it.
      // For NetEq initial offset should not matter so we should consider fixing
      // RTP path.
      int starting_sample_index,

      // Number of audio samples in audio frame in 1 channel.
      int samples_per_channel,

      // Sequence number of the frame in the order sent, it is currently
      // required by NetEq, but we can fix NetEq, because starting_sample_index
      // should be enough.
      int sequence_number,

      // If audio frame is a speech or discontinued transmission.
      FrameType frame_type,

      // Opaque payload type. In RTP codepath payload type is stored in RTP
      // header. In other implementations it should be simply passed through the
      // wire -- it's needed for decoder.
      int payload_type,

      // Vector with opaque encoded data.
      std::vector<uint8_t> encoded_data);

  ~MediaTransportEncodedAudioFrame();
  MediaTransportEncodedAudioFrame(const MediaTransportEncodedAudioFrame&);
  MediaTransportEncodedAudioFrame& operator=(
      const MediaTransportEncodedAudioFrame& other);
  MediaTransportEncodedAudioFrame& operator=(
      MediaTransportEncodedAudioFrame&& other);
  MediaTransportEncodedAudioFrame(MediaTransportEncodedAudioFrame&&);

  // Getters.
  int sampling_rate_hz() const { return sampling_rate_hz_; }
  int starting_sample_index() const { return starting_sample_index_; }
  int samples_per_channel() const { return samples_per_channel_; }
  int sequence_number() const { return sequence_number_; }

  int payload_type() const { return payload_type_; }
  FrameType frame_type() const { return frame_type_; }

  rtc::ArrayView<const uint8_t> encoded_data() const { return encoded_data_; }

 private:
  int sampling_rate_hz_;
  int starting_sample_index_;
  int samples_per_channel_;

  // TODO(sukhanov): Refactor NetEq so we don't need sequence number.
  // Having sample_index and samples_per_channel should be enough.
  int sequence_number_;

  FrameType frame_type_;

  int payload_type_;

  std::vector<uint8_t> encoded_data_;
};

// Callback to notify about network route changes.
class MediaTransportNetworkChangeCallback {
 public:
  virtual ~MediaTransportNetworkChangeCallback() = default;

  // Called when the network route is changed, with the new network route.
  virtual void OnNetworkRouteChanged(
      const rtc::NetworkRoute& new_network_route) = 0;
};

// Interface for receiving encoded audio frames from MediaTransportInterface
// implementations.
class MediaTransportAudioSinkInterface {
 public:
  virtual ~MediaTransportAudioSinkInterface() = default;

  // Called when new encoded audio frame is received.
  virtual void OnData(uint64_t channel_id,
                      MediaTransportEncodedAudioFrame frame) = 0;
};

// Represents encoded video frame, along with the codec information.
class MediaTransportEncodedVideoFrame final {
 public:
  MediaTransportEncodedVideoFrame(int64_t frame_id,
                                  std::vector<int64_t> referenced_frame_ids,
                                  int payload_type,
                                  const webrtc::EncodedImage& encoded_image);
  ~MediaTransportEncodedVideoFrame();
  MediaTransportEncodedVideoFrame(const MediaTransportEncodedVideoFrame&);
  MediaTransportEncodedVideoFrame& operator=(
      const MediaTransportEncodedVideoFrame& other);
  MediaTransportEncodedVideoFrame& operator=(
      MediaTransportEncodedVideoFrame&& other);
  MediaTransportEncodedVideoFrame(MediaTransportEncodedVideoFrame&&);

  int payload_type() const { return payload_type_; }
  const webrtc::EncodedImage& encoded_image() const { return encoded_image_; }

  int64_t frame_id() const { return frame_id_; }
  const std::vector<int64_t>& referenced_frame_ids() const {
    return referenced_frame_ids_;
  }

  // Hack to workaround lack of ownership of the encoded_image_._buffer. If we
  // don't already own the underlying data, make a copy.
  void Retain();

 private:
  MediaTransportEncodedVideoFrame();

  int payload_type_;

  // The buffer is not owned by the encoded image. On the sender it means that
  // it will need to make a copy using the Retain() method, if it wants to
  // deliver it asynchronously.
  webrtc::EncodedImage encoded_image_;

  // If non-empty, this is the data for the encoded image.
  std::vector<uint8_t> encoded_data_;

  // Frame id uniquely identifies a frame in a stream. It needs to be unique in
  // a given time window (i.e. technically unique identifier for the lifetime of
  // the connection is not needed, but you need to guarantee that remote side
  // got rid of the previous frame_id if you plan to reuse it).
  //
  // It is required by a remote jitter buffer, and is the same as
  // EncodedFrame::id::picture_id.
  //
  // This data must be opaque to the media transport, and media transport should
  // itself not make any assumptions about what it is and its uniqueness.
  int64_t frame_id_;

  // A single frame might depend on other frames. This is set of identifiers on
  // which the current frame depends.
  std::vector<int64_t> referenced_frame_ids_;
};

// Interface for receiving encoded video frames from MediaTransportInterface
// implementations.
class MediaTransportVideoSinkInterface {
 public:
  virtual ~MediaTransportVideoSinkInterface() = default;

  // Called when new encoded video frame is received.
  virtual void OnData(uint64_t channel_id,
                      MediaTransportEncodedVideoFrame frame) = 0;
};

// Interface for video sender to be notified of received key frame request.
class MediaTransportKeyFrameRequestCallback {
 public:
  virtual ~MediaTransportKeyFrameRequestCallback() = default;

  // Called when a key frame request is received on the transport.
  virtual void OnKeyFrameRequested(uint64_t channel_id) = 0;
};

// State of the media transport.  Media transport begins in the pending state.
// It transitions to writable when it is ready to send media.  It may transition
// back to pending if the connection is blocked.  It may transition to closed at
// any time.  Closed is terminal: a transport will never re-open once closed.
enum class MediaTransportState {
  kPending,
  kWritable,
  kClosed,
};

// Callback invoked whenever the state of the media transport changes.
class MediaTransportStateCallback {
 public:
  virtual ~MediaTransportStateCallback() = default;

  // Invoked whenever the state of the media transport changes.
  virtual void OnStateChanged(MediaTransportState state) = 0;
};

// Callback for RTT measurements on the receive side.
// TODO(nisse): Related interfaces: CallStatsObserver and RtcpRttStats. It's
// somewhat unclear what type of measurement is needed. It's used to configure
// NACK generation and playout buffer. Either raw measurement values or recent
// maximum would make sense for this use. Need consolidation of RTT signalling.
class MediaTransportRttObserver {
 public:
  virtual ~MediaTransportRttObserver() = default;

  // Invoked when a new RTT measurement is available, typically once per ACK.
  virtual void OnRttUpdated(int64_t rtt_ms) = 0;
};

// Supported types of application data messages.
enum class DataMessageType {
  // Application data buffer with the binary bit unset.
  kText,

  // Application data buffer with the binary bit set.
  kBinary,

  // Transport-agnostic control messages, such as open or open-ack messages.
  kControl,
};

// Parameters for sending data.  The parameters may change from message to
// message, even within a single channel.  For example, control messages may be
// sent reliably and in-order, even if the data channel is configured for
// unreliable delivery.
struct SendDataParams {
  SendDataParams();
  SendDataParams(const SendDataParams&);

  DataMessageType type = DataMessageType::kText;

  // Whether to deliver the message in order with respect to other ordered
  // messages with the same channel_id.
  bool ordered = false;

  // If set, the maximum number of times this message may be
  // retransmitted by the transport before it is dropped.
  // Setting this value to zero disables retransmission.
  // Must be non-negative. |max_rtx_count| and |max_rtx_ms| may not be set
  // simultaneously.
  absl::optional<int> max_rtx_count;

  // If set, the maximum number of milliseconds for which the transport
  // may retransmit this message before it is dropped.
  // Setting this value to zero disables retransmission.
  // Must be non-negative. |max_rtx_count| and |max_rtx_ms| may not be set
  // simultaneously.
  absl::optional<int> max_rtx_ms;
};

// Sink for callbacks related to a data channel.
class DataChannelSink {
 public:
  virtual ~DataChannelSink() = default;

  // Callback issued when data is received by the transport.
  virtual void OnDataReceived(int channel_id,
                              DataMessageType type,
                              const rtc::CopyOnWriteBuffer& buffer) = 0;

  // Callback issued when a remote data channel begins the closing procedure.
  // Messages sent after the closing procedure begins will not be transmitted.
  virtual void OnChannelClosing(int channel_id) = 0;

  // Callback issued when a (remote or local) data channel completes the closing
  // procedure.  Closing channels become closed after all pending data has been
  // transmitted.
  virtual void OnChannelClosed(int channel_id) = 0;
};

// Media transport interface for sending / receiving encoded audio/video frames
// and receiving bandwidth estimate update from congestion control.
class MediaTransportInterface {
 public:
  MediaTransportInterface();
  virtual ~MediaTransportInterface();

  // Start asynchronous send of audio frame. The status returned by this method
  // only pertains to the synchronous operations (e.g.
  // serialization/packetization), not to the asynchronous operation.

  virtual RTCError SendAudioFrame(uint64_t channel_id,
                                  MediaTransportEncodedAudioFrame frame) = 0;

  // Start asynchronous send of video frame. The status returned by this method
  // only pertains to the synchronous operations (e.g.
  // serialization/packetization), not to the asynchronous operation.
  virtual RTCError SendVideoFrame(
      uint64_t channel_id,
      const MediaTransportEncodedVideoFrame& frame) = 0;

  // Used by video sender to be notified on key frame requests.
  virtual void SetKeyFrameRequestCallback(
      MediaTransportKeyFrameRequestCallback* callback);

  // Requests a keyframe for the particular channel (stream). The caller should
  // check that the keyframe is not present in a jitter buffer already (i.e.
  // don't request a keyframe if there is one that you will get from the jitter
  // buffer in a moment).
  virtual RTCError RequestKeyFrame(uint64_t channel_id) = 0;

  // Sets audio sink. Sink must be unset by calling SetReceiveAudioSink(nullptr)
  // before the media transport is destroyed or before new sink is set.
  virtual void SetReceiveAudioSink(MediaTransportAudioSinkInterface* sink) = 0;

  // Registers a video sink. Before destruction of media transport, you must
  // pass a nullptr.
  virtual void SetReceiveVideoSink(MediaTransportVideoSinkInterface* sink) = 0;

  // Adds a target bitrate observer. Before media transport is destructed
  // the observer must be unregistered (by calling
  // RemoveTargetTransferRateObserver).
  // A newly registered observer will be called back with the latest recorded
  // target rate, if available.
  virtual void AddTargetTransferRateObserver(
      TargetTransferRateObserver* observer);

  // Removes an existing |observer| from observers. If observer was never
  // registered, an error is logged and method does nothing.
  virtual void RemoveTargetTransferRateObserver(
      TargetTransferRateObserver* observer);

  // Sets audio packets observer, which gets informed about incoming audio
  // packets. Before destruction, the observer must be unregistered by setting
  // nullptr.
  //
  // This method may be temporary, when the multiplexer is implemented (or
  // multiplexer may use it to demultiplex channel ids).
  virtual void SetFirstAudioPacketReceivedObserver(
      AudioPacketReceivedObserver* observer);

  // Intended for receive side. AddRttObserver registers an observer to be
  // called for each RTT measurement, typically once per ACK. Before media
  // transport is destructed the observer must be unregistered.
  virtual void AddRttObserver(MediaTransportRttObserver* observer);
  virtual void RemoveRttObserver(MediaTransportRttObserver* observer);

  // Returns the last known target transfer rate as reported to the above
  // observers.
  virtual absl::optional<TargetTransferRate> GetLatestTargetTransferRate();

  // Gets the audio packet overhead in bytes. Returned overhead does not include
  // transport overhead (ipv4/6, turn channeldata, tcp/udp, etc.).
  // If the transport is capable of fusing packets together, this overhead
  // might not be a very accurate number.
  virtual size_t GetAudioPacketOverhead() const;

  // Sets an observer for network change events. If the network route is already
  // established when the callback is set, |callback| will be called immediately
  // with the current network route.
  // Before media transport is destroyed, the callback must be unregistered by
  // setting it to nullptr.
  // TODO(nisse): The SetNetworkChangeCallback is being replaced with Add/Remove
  // methods, because both video channel and voice channel need to register.
  virtual void SetNetworkChangeCallback(
      MediaTransportNetworkChangeCallback* callback);
  virtual void AddNetworkChangeCallback(
      MediaTransportNetworkChangeCallback* callback);
  virtual void RemoveNetworkChangeCallback(
      MediaTransportNetworkChangeCallback* callback);

  // Sets a state observer callback. Before media transport is destroyed, the
  // callback must be unregistered by setting it to nullptr.
  // A newly registered callback will be called with the current state.
  // Media transport does not invoke this callback concurrently.
  virtual void SetMediaTransportStateCallback(
      MediaTransportStateCallback* callback) = 0;

  // Updates allocation limits.
  // TODO(psla): Make abstract when downstream implementation implement it.
  virtual void SetAllocatedBitrateLimits(
      const MediaTransportAllocatedBitrateLimits& limits);

  // Sends a data buffer to the remote endpoint using the given send parameters.
  // |buffer| may not be larger than 256 KiB. Returns an error if the send
  // fails.
  virtual RTCError SendData(int channel_id,
                            const SendDataParams& params,
                            const rtc::CopyOnWriteBuffer& buffer) = 0;

  // Closes |channel_id| gracefully.  Returns an error if |channel_id| is not
  // open.  Data sent after the closing procedure begins will not be
  // transmitted. The channel becomes closed after pending data is transmitted.
  virtual RTCError CloseChannel(int channel_id) = 0;

  // Sets a sink for data messages and channel state callbacks. Before media
  // transport is destroyed, the sink must be unregistered by setting it to
  // nullptr.
  virtual void SetDataSink(DataChannelSink* sink) = 0;

  // TODO(sukhanov): RtcEventLogs.
};

// If media transport factory is set in peer connection factory, it will be
// used to create media transport for sending/receiving encoded frames and
// this transport will be used instead of default RTP/SRTP transport.
//
// Currently Media Transport negotiation is not supported in SDP.
// If application is using media transport, it must negotiate it before
// setting media transport factory in peer connection.
class MediaTransportFactory {
 public:
  virtual ~MediaTransportFactory() = default;

  // Creates media transport.
  // - Does not take ownership of packet_transport or network_thread.
  // - Does not support group calls, in 1:1 call one side must set
  //   is_caller = true and another is_caller = false.
  // TODO(bugs.webrtc.org/9938) This constructor will be removed and replaced
  // with the one below.
  virtual RTCErrorOr<std::unique_ptr<MediaTransportInterface>>
  CreateMediaTransport(rtc::PacketTransportInternal* packet_transport,
                       rtc::Thread* network_thread,
                       bool is_caller);

  // Creates media transport.
  // - Does not take ownership of packet_transport or network_thread.
  // TODO(bugs.webrtc.org/9938): remove default implementation once all children
  // override it.
  virtual RTCErrorOr<std::unique_ptr<MediaTransportInterface>>
  CreateMediaTransport(rtc::PacketTransportInternal* packet_transport,
                       rtc::Thread* network_thread,
                       const MediaTransportSettings& settings);
};

}  // namespace webrtc
#endif  // API_MEDIA_TRANSPORT_INTERFACE_H_
