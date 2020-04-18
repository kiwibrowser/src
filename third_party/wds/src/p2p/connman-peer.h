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

#include <gio/gio.h>
#include "information-element.h"

#ifndef CONNMAN_PEER_H_
#define CONNMAN_PEER_H_

namespace P2P {

class Peer {
    public:
        class Observer {
            public:
                virtual void on_availability_changed(Peer *peer) {}
                virtual void on_initialized(Peer *peer) {}

            protected:
                virtual ~Observer() {}
        };

        Peer(const char *object_path, GVariantIter *property_iterator);
        virtual ~Peer();

        void set_observer(Observer* observer) {
            observer_ = observer;
        }

        /* TODO add error handling for these -- maybe through observer.on_error? */
        void connect();
        void disconnect();

        const P2P::DeviceType device_type() const { return ie_->get_device_type(); }
        const std::string& name() const { return name_; }
        const std::string& remote_host() const {return remote_host_; }
        const int remote_port() const { return ie_->get_rtsp_port(); }
        const std::string& local_host() const {return local_host_; }
        bool is_available() const { return ready_ && !remote_host_.empty() && !local_host_.empty(); }

    private:
        static void proxy_signal_cb (GDBusProxy *proxy, const char *sender, const char *signal, GVariant *params, gpointer data_ptr);
        static void proxy_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void connect_cb (GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void disconnect_cb (GObject *object, GAsyncResult *res, gpointer data_ptr);

        void ips_changed (const char *remote, const char *local);
        void state_changed (const char *state);
        void name_changed (const char *name);
        void proxy_cb (GAsyncResult *res);
        void handle_property_change (const char *name, GVariant *property);

        Observer *observer_;
        std::string name_;
        std::string remote_host_;
        std::string local_host_;
        bool ready_;
        GDBusProxy *proxy_;
        std::shared_ptr<P2P::InformationElement> ie_;
};

}
#endif // CONNMAN_PEER_H_
