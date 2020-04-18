/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <iostream>

#include "sink.h"
#include "gst_sink_media_manager.h"

Sink::Sink(const std::string& remote_host, int remote_rtsp_port, const std::string& local_host)
  : MiracBroker(remote_host, std::to_string(remote_rtsp_port)),
    local_host_(local_host) {
}

Sink::~Sink() {}

void Sink::got_message(const std::string& message) {
  wfd_sink_->RTSPDataReceived(message);
}

void Sink::on_connected() {
  media_manager_.reset(new GstSinkMediaManager(local_host_));
  wfd_sink_.reset(wds::Sink::Create(this, media_manager_.get()));
  wfd_sink_->Start();
}

void Sink::on_connection_failure(ConnectionFailure failure) {
  switch (failure) {
      case CONNECTION_LOST:
          std::cout << "* RTSP connection lost" << std::endl;
      case CONNECTION_TIMEOUT:
          ;
  }
}

void Sink::Play() {
  wfd_sink_->Play();
}

void Sink::Pause() {
  wfd_sink_->Pause();
}

void Sink::Teardown() {
  wfd_sink_->Teardown();
}

wds::Peer* Sink::Peer() const {
  return wfd_sink_.get();
}

