// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/cast_ipc_dispatcher.h"

#include "base/single_thread_task_runner.h"
#include "chrome/common/cast_messages.h"
#include "chrome/renderer/media/cast_transport_ipc.h"
#include "ipc/ipc_message_macros.h"

CastIPCDispatcher* CastIPCDispatcher::global_instance_ = NULL;

CastIPCDispatcher::CastIPCDispatcher(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : sender_(NULL),
      io_task_runner_(io_task_runner) {
  DCHECK(io_task_runner_.get());
  DCHECK(!global_instance_);
}

CastIPCDispatcher::~CastIPCDispatcher() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(!global_instance_);
}

CastIPCDispatcher* CastIPCDispatcher::Get() {
  return global_instance_;
}

void CastIPCDispatcher::Send(IPC::Message* message) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (sender_) {
    sender_->Send(message);
  } else {
    delete message;
  }
}

int32_t CastIPCDispatcher::AddSender(CastTransportIPC* sender) {
  return id_map_.Add(sender);
}

void CastIPCDispatcher::RemoveSender(int32_t channel_id) {
  return id_map_.Remove(channel_id);
}

bool CastIPCDispatcher::OnMessageReceived(const IPC::Message& message) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(CastIPCDispatcher, message)
    IPC_MESSAGE_HANDLER(CastMsg_NotifyStatusChange, OnNotifyStatusChange)
    IPC_MESSAGE_HANDLER(CastMsg_RawEvents, OnRawEvents)
    IPC_MESSAGE_HANDLER(CastMsg_Rtt, OnRtt)
    IPC_MESSAGE_HANDLER(CastMsg_RtcpCastMessage, OnRtcpCastMessage)
    IPC_MESSAGE_HANDLER(CastMsg_Pli, OnReceivedPli);
    IPC_MESSAGE_HANDLER(CastMsg_ReceivedPacket, OnReceivedPacket)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void CastIPCDispatcher::OnFilterAdded(IPC::Channel* channel) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK(!global_instance_);
  global_instance_ = this;
  sender_ = channel;
}

void CastIPCDispatcher::OnFilterRemoved() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(this, global_instance_);
  global_instance_ = NULL;
  sender_ = NULL;
}

void CastIPCDispatcher::OnChannelClosing() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(this, global_instance_);
}

void CastIPCDispatcher::OnNotifyStatusChange(
    int32_t channel_id,
    media::cast::CastTransportStatus status) {
  CastTransportIPC* sender = id_map_.Lookup(channel_id);
  if (sender) {
    sender->OnNotifyStatusChange(status);
  } else {
    DVLOG(1)
        << "CastIPCDispatcher::OnNotifystatusChange on non-existing channel.";
  }
}

void CastIPCDispatcher::OnRawEvents(
    int32_t channel_id,
    const std::vector<media::cast::PacketEvent>& packet_events,
    const std::vector<media::cast::FrameEvent>& frame_events) {
  CastTransportIPC* sender = id_map_.Lookup(channel_id);
  if (sender) {
    sender->OnRawEvents(packet_events, frame_events);
  } else {
    DVLOG(1) << "CastIPCDispatcher::OnRawEvents on non-existing channel.";
  }
}

void CastIPCDispatcher::OnRtt(int32_t channel_id,
                              uint32_t ssrc,
                              base::TimeDelta rtt) {
  CastTransportIPC* sender = id_map_.Lookup(channel_id);
  if (sender) {
    sender->OnRtt(ssrc, rtt);
  } else {
    DVLOG(1) << "CastIPCDispatcher::OnRtt on non-existing channel.";
  }
}

void CastIPCDispatcher::OnRtcpCastMessage(
    int32_t channel_id,
    uint32_t ssrc,
    const media::cast::RtcpCastMessage& cast_message) {
  CastTransportIPC* sender = id_map_.Lookup(channel_id);
  if (sender) {
    sender->OnRtcpCastMessage(ssrc, cast_message);
  } else {
    DVLOG(1) << "CastIPCDispatcher::OnRtt on non-existing channel.";
  }
}

void CastIPCDispatcher::OnReceivedPli(int32_t channel_id, int32_t ssrc) {
  CastTransportIPC* sender = id_map_.Lookup(channel_id);
  if (sender) {
    sender->OnReceivedPli(ssrc);
  } else {
    DVLOG(1) << "CastIPCDispatcher::OnReceivedPli on non-existing "
                "channel.";
  }
}

void CastIPCDispatcher::OnReceivedPacket(int32_t channel_id,
                                         const media::cast::Packet& packet) {
  CastTransportIPC* sender = id_map_.Lookup(channel_id);
  if (sender) {
    sender->OnReceivedPacket(packet);
  } else {
    DVLOG(1) << "CastIPCDispatcher::OnReceievdPacket on non-existing channel.";
  }
}
