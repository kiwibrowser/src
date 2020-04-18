// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_CAST_IPC_DISPATCHER_H_
#define CHROME_RENDERER_MEDIA_CAST_IPC_DISPATCHER_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/message_filter.h"
#include "media/cast/cast_sender.h"
#include "media/cast/logging/logging_defines.h"
#include "media/cast/net/cast_transport.h"

class CastTransportIPC;

// This dispatcher listens to incoming IPC messages and sends
// the call to the correct CastTransportIPC instance.
class CastIPCDispatcher : public IPC::MessageFilter {
 public:
  explicit CastIPCDispatcher(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);

  static CastIPCDispatcher* Get();
  void Send(IPC::Message* message);
  int32_t AddSender(CastTransportIPC* sender);
  void RemoveSender(int32_t channel_id);

  // IPC::MessageFilter implementation
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnFilterAdded(IPC::Channel* channel) override;
  void OnFilterRemoved() override;
  void OnChannelClosing() override;

 protected:
  ~CastIPCDispatcher() override;

 private:
  void OnNotifyStatusChange(int32_t channel_id,
                            media::cast::CastTransportStatus status);
  void OnRtpStatistics(int32_t channel_id,
                       bool audio,
                       const media::cast::RtcpSenderInfo& sender_info,
                       base::TimeTicks time_sent,
                       uint32_t rtp_timestamp);
  void OnRawEvents(int32_t channel_id,
                   const std::vector<media::cast::PacketEvent>& packet_events,
                   const std::vector<media::cast::FrameEvent>& frame_events);
  void OnRtt(int32_t channel_id, uint32_t ssrc, base::TimeDelta rtt);
  void OnRtcpCastMessage(int32_t channel_id,
                         uint32_t ssrc,
                         const media::cast::RtcpCastMessage& cast_message);
  void OnReceivedPli(int32_t channel_id, int32_t ssrc);
  void OnReceivedPacket(int32_t channel_id, const media::cast::Packet& packet);

  static CastIPCDispatcher* global_instance_;

  // For IPC Send(); must only be accesed on |io_message_loop_|.
  IPC::Sender* sender_;

  // Task runner on which IPC calls are driven.
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // A map of stream ids to delegates; must only be accessed on
  // |io_message_loop_|.
  base::IDMap<CastTransportIPC*> id_map_;
  DISALLOW_COPY_AND_ASSIGN(CastIPCDispatcher);
};

#endif  // CHROME_RENDERER_MEDIA_CAST_IPC_DISPATCHER_H_
