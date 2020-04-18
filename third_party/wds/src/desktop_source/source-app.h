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

#ifndef SOURCE_APP_H
#define SOURCE_APP_H

#include <memory>

#include "source.h"
#include "connman-client.h"
#include "mirac_broker_source.h"

class SourceApp: public P2P::Client::Observer, public P2P::Peer::Observer {
  public:
    SourceApp(int port);
    ~SourceApp();

    MiracBrokerSource* source() { return source_.get(); }

    void on_peer_added(P2P::Client *client, std::shared_ptr<P2P::Peer> peer) override;
    void on_peer_removed(P2P::Client *client, std::shared_ptr<P2P::Peer> peer) override;
    void on_availability_changed(P2P::Client *client) override;

    void on_availability_changed(P2P::Peer *peer) override;
    void on_initialized(P2P::Peer *peer) override;

    void scan();
    bool connect(uint peer_index);

  private:
    std::unique_ptr<P2P::Client> p2p_client_;
    std::unique_ptr<MiracBrokerSource> source_;
    std::map<uint, P2P::Peer*>peers_;
    uint peer_index_;
};

#endif // SOURCE_APP_H
