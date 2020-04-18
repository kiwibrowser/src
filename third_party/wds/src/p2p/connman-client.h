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

#ifndef CONNMAN_CLIENT_H_
#define CONNMAN_CLIENT_H_

#include <memory>
#include <gio/gio.h>

#include "information-element.h"
#include "connman-peer.h"

namespace P2P {

class Client {
    public:
        class Observer {
            public:
                virtual void on_peer_added(Client *client, std::shared_ptr<P2P::Peer> peer) {}
                virtual void on_peer_removed(Client *client, std::shared_ptr<P2P::Peer> peer) {}
                virtual void on_availability_changed(Client *client) {}

            protected:
                virtual ~Observer() {}
        };

        Client(std::unique_ptr<P2P::InformationElementArray> &take_array, Observer *observer = NULL);
        virtual ~Client();

        void set_information_element(std::unique_ptr<P2P::InformationElementArray> &take_array);
        void set_observer(Observer* observer) {
            observer_ = observer;
        }

        bool is_available() const;
        /* TODO error / finished handling */
        void scan();

    private:
        static void connman_appeared_cb(GDBusConnection *connection, const char *name, const char *owner, gpointer data_ptr);
        static void connman_disappeared_cb(GDBusConnection *connection, const char *name, gpointer data_ptr);
        static void proxy_signal_cb (GDBusProxy *proxy, const char *sender, const char *signal, GVariant *params, gpointer data_ptr);
        static void proxy_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void technology_proxy_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void get_technologies_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void register_peer_service_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void scan_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void get_peers_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);

        void connman_disappeared();
        void peers_changed (GVariant *params);
        void proxy_cb(GAsyncResult *res);
        void technology_proxy_cb(GAsyncResult *res);
        void handle_new_peers(GVariantIter *added);

        void initialize_peers();
        void register_peer_service();
        void unregister_peer_service();

        uint connman_watcher_;
        GDBusProxy *proxy_;
        GDBusProxy *technology_proxy_;

        Observer* observer_;
        std::unique_ptr<P2P::InformationElementArray>array_;
        std::map<std::string, std::shared_ptr<P2P::Peer>> peers_;
};

}
#endif // CONNMAN_CLIENT_H_
