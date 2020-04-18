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

#ifndef MIRAC_BROKER_SOURCE_H_
#define MIRAC_BROKER_SOURCE_H_

#include <memory>

#include "mirac-broker.hpp"

namespace wds {
class SourceMediaManager;
class Source;
}

class MiracBrokerSource : public MiracBroker {
 public:
  explicit MiracBrokerSource(int rtsp_port);
  ~MiracBrokerSource();

  wds::Source* wfd_source() { return wfd_source_.get(); }

 private:
  virtual void got_message(const std::string& message) override;
  virtual void on_connected() override;
  void on_connection_failure(ConnectionFailure failure) override;
  virtual wds::Peer* Peer() const override;

  std::unique_ptr<wds::SourceMediaManager> media_manager_;
  std::unique_ptr<wds::Source> wfd_source_;
};

#endif // MIRAC_BROKER_SOURCE_H_
