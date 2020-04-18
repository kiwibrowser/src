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
#include <netinet/in.h> // htons()

#include "sink-app.h"
#include "sink.h"
#include "connman-client.h"

void SinkApp::on_peer_added(P2P::Client *client, std::shared_ptr<P2P::Peer> peer)
{
    std::cout << "* New peer: " << peer->name() << std::endl;
    peer->set_observer (this);
}

void SinkApp::on_peer_removed(P2P::Client *client, std::shared_ptr<P2P::Peer> peer)
{
    std::cout << "* Peer removed: " << peer->name() << std::endl;
    if (peer.get() == peer_) {
        sink_.reset(NULL);
        peer_ = NULL;
    }
}

void SinkApp::on_availability_changed(P2P::Peer *peer)
{
    if (!sink_ && peer->is_available() && peer->device_type() == P2P::SOURCE) {
        std::cout << "* Connecting to source at " << peer->remote_host() << ":" << ntohs(peer->remote_port()) << std::endl;

        sink_.reset(new Sink (peer->remote_host(), ntohs(peer->remote_port()), peer->local_host()));
        peer_ = peer;
    } else if (sink_ && !peer->is_available() && peer == peer_) {
        std::cout << "* Source unavailable" << std::endl;

        sink_.reset(NULL);
        peer_ = NULL;
    }
}

SinkApp::SinkApp(){
    // Create a information element for a simple WFD Sink
    P2P::InformationElement ie;
    auto sub_element = P2P::new_subelement(P2P::DEVICE_INFORMATION);
    auto dev_info = (P2P::DeviceInformationSubelement*)sub_element;

    // TODO port number is a lie -- we should start the sink first, then 
    // use the port from there (and sink should probably default to 7236)

    // TODO InformationElement could have constructors for this stuff...
    dev_info->session_management_control_port = htons(7236);
    dev_info->maximum_throughput = htons(50);
    dev_info->field1.device_type = P2P::PRIMARY_SINK;
    dev_info->field1.session_availability = true;
    ie.add_subelement (sub_element);

    std::cout << "* Registering Wifi Display with IE " << ie.to_string() <<  std::endl;

    // register the P2P service with connman
    auto array = ie.serialize ();
    p2p_client_.reset(new P2P::Client(array, this));
}

SinkApp::SinkApp(const std::string& hostname, int port)
{
    std::cout << "* Connecting to peer at " << hostname << ":" << port << std::endl;

    sink_.reset(new Sink (hostname, port, ""));
}

SinkApp::~SinkApp() {
}
