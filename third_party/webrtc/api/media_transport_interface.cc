/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
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

#include "api/media_transport_interface.h"

#include <cstdint>
#include <utility>

namespace webrtc {

MediaTransportSettings::MediaTransportSettings() = default;
MediaTransportSettings::MediaTransportSettings(const MediaTransportSettings&) =
    default;
MediaTransportSettings& MediaTransportSettings::operator=(
    const MediaTransportSettings&) = default;
MediaTransportSettings::~MediaTransportSettings() = default;

MediaTransportEncodedAudioFrame::~MediaTransportEncodedAudioFrame() {}

MediaTransportEncodedAudioFrame::MediaTransportEncodedAudioFrame(
    int sampling_rate_hz,
    int starting_sample_index,
    int samples_per_channel,
    int sequence_number,
    FrameType frame_type,
    int payload_type,
    std::vector<uint8_t> encoded_data)
    : sampling_rate_hz_(sampling_rate_hz),
      starting_sample_index_(starting_sample_index),
      samples_per_channel_(samples_per_channel),
      sequence_number_(sequence_number),
      frame_type_(frame_type),
      payload_type_(payload_type),
      encoded_data_(std::move(encoded_data)) {}

MediaTransportEncodedAudioFrame& MediaTransportEncodedAudioFrame::operator=(
    const MediaTransportEncodedAudioFrame&) = default;

MediaTransportEncodedAudioFrame& MediaTransportEncodedAudioFrame::operator=(
    MediaTransportEncodedAudioFrame&&) = default;

MediaTransportEncodedAudioFrame::MediaTransportEncodedAudioFrame(
    const MediaTransportEncodedAudioFrame&) = default;

MediaTransportEncodedAudioFrame::MediaTransportEncodedAudioFrame(
    MediaTransportEncodedAudioFrame&&) = default;

MediaTransportEncodedVideoFrame::MediaTransportEncodedVideoFrame() = default;

MediaTransportEncodedVideoFrame::~MediaTransportEncodedVideoFrame() = default;

MediaTransportEncodedVideoFrame::MediaTransportEncodedVideoFrame(
    int64_t frame_id,
    std::vector<int64_t> referenced_frame_ids,
    int payload_type,
    const webrtc::EncodedImage& encoded_image)
    : payload_type_(payload_type),
      encoded_image_(encoded_image),
      frame_id_(frame_id),
      referenced_frame_ids_(std::move(referenced_frame_ids)) {}

MediaTransportEncodedVideoFrame& MediaTransportEncodedVideoFrame::operator=(
    const MediaTransportEncodedVideoFrame& o) {
  payload_type_ = o.payload_type_;
  encoded_image_ = o.encoded_image_;
  encoded_data_ = o.encoded_data_;
  frame_id_ = o.frame_id_;
  referenced_frame_ids_ = o.referenced_frame_ids_;
  if (!encoded_data_.empty()) {
    // We own the underlying data.
    encoded_image_.set_buffer(encoded_data_.data(), encoded_data_.size());
  }
  return *this;
}

MediaTransportEncodedVideoFrame& MediaTransportEncodedVideoFrame::operator=(
    MediaTransportEncodedVideoFrame&& o) {
  payload_type_ = o.payload_type_;
  encoded_image_ = o.encoded_image_;
  encoded_data_ = std::move(o.encoded_data_);
  frame_id_ = o.frame_id_;
  referenced_frame_ids_ = std::move(o.referenced_frame_ids_);
  if (!encoded_data_.empty()) {
    // We take over ownership of the underlying data.
    encoded_image_.set_buffer(encoded_data_.data(), encoded_data_.size());
    o.encoded_image_.set_buffer(nullptr, 0);
  }
  return *this;
}

MediaTransportEncodedVideoFrame::MediaTransportEncodedVideoFrame(
    const MediaTransportEncodedVideoFrame& o)
    : MediaTransportEncodedVideoFrame() {
  *this = o;
}

MediaTransportEncodedVideoFrame::MediaTransportEncodedVideoFrame(
    MediaTransportEncodedVideoFrame&& o)
    : MediaTransportEncodedVideoFrame() {
  *this = std::move(o);
}

void MediaTransportEncodedVideoFrame::Retain() {
  if (encoded_image_.data() && encoded_data_.empty()) {
    encoded_data_ = std::vector<uint8_t>(
        encoded_image_.data(), encoded_image_.data() + encoded_image_.size());
    encoded_image_.set_buffer(encoded_data_.data(), encoded_image_.size());
  }
}

SendDataParams::SendDataParams() = default;
SendDataParams::SendDataParams(const SendDataParams&) = default;

RTCErrorOr<std::unique_ptr<MediaTransportInterface>>
MediaTransportFactory::CreateMediaTransport(
    rtc::PacketTransportInternal* packet_transport,
    rtc::Thread* network_thread,
    bool is_caller) {
  MediaTransportSettings settings;
  settings.is_caller = is_caller;
  return CreateMediaTransport(packet_transport, network_thread, settings);
}

RTCErrorOr<std::unique_ptr<MediaTransportInterface>>
MediaTransportFactory::CreateMediaTransport(
    rtc::PacketTransportInternal* packet_transport,
    rtc::Thread* network_thread,
    const MediaTransportSettings& settings) {
  return std::unique_ptr<MediaTransportInterface>(nullptr);
}

MediaTransportInterface::MediaTransportInterface() = default;
MediaTransportInterface::~MediaTransportInterface() = default;

void MediaTransportInterface::SetKeyFrameRequestCallback(
    MediaTransportKeyFrameRequestCallback* callback) {}

absl::optional<TargetTransferRate>
MediaTransportInterface::GetLatestTargetTransferRate() {
  return absl::nullopt;
}

void MediaTransportInterface::SetNetworkChangeCallback(
    MediaTransportNetworkChangeCallback* callback) {}

void MediaTransportInterface::AddNetworkChangeCallback(
    MediaTransportNetworkChangeCallback* callback) {}

void MediaTransportInterface::RemoveNetworkChangeCallback(
    MediaTransportNetworkChangeCallback* callback) {}

void MediaTransportInterface::SetFirstAudioPacketReceivedObserver(
    AudioPacketReceivedObserver* observer) {}

void MediaTransportInterface::AddTargetTransferRateObserver(
    TargetTransferRateObserver* observer) {}
void MediaTransportInterface::RemoveTargetTransferRateObserver(
    TargetTransferRateObserver* observer) {}

void MediaTransportInterface::AddRttObserver(
    MediaTransportRttObserver* observer) {}
void MediaTransportInterface::RemoveRttObserver(
    MediaTransportRttObserver* observer) {}

size_t MediaTransportInterface::GetAudioPacketOverhead() const {
  return 0;
}

void MediaTransportInterface::SetAllocatedBitrateLimits(
    const MediaTransportAllocatedBitrateLimits& limits) {}

}  // namespace webrtc
