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
#include <stdexcept>
#include <gio/gio.h>

#include "connman-peer.h"

namespace P2P {

/* static C callback */
void Peer::proxy_cb (GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    auto client = static_cast<Peer*> (data_ptr);
    client->proxy_cb (res);
}

/* static C callback */
void Peer::proxy_signal_cb (GDBusProxy *proxy, const char *sender, const char *signal, GVariant *params, gpointer data_ptr)
{
    GVariant *property;
    char *name;
    auto peer = static_cast<Peer*> (data_ptr);

    if (g_strcmp0(signal, "PropertyChanged") != 0)
        return;

    g_variant_get (params, "(sv)", &name, &property);

    peer->handle_property_change (name, property);
}

void Peer::handle_property_change (const char *name, GVariant *property)
{
    if (g_strcmp0(name, "State") == 0) {
        state_changed (g_variant_get_string (property, NULL));
    } else if (g_strcmp0(name, "Name") == 0) {
        name_changed (g_variant_get_string (property, NULL));
    } else if (g_strcmp0(name, "Services") == 0) {
        GVariantIter *service_array, *services;
        GVariant *spec_val;

        g_variant_get (property, "a(a{sv})", &service_array);
        while (g_variant_iter_loop (service_array, "(a{sv})", &services)) {
            const char *service_name;
            while (g_variant_iter_loop (services, "{sv}", &service_name, &spec_val)) {
                if (g_strcmp0 (service_name, "WiFiDisplayIEs") == 0) {
                    uint8_t *bytes;
                    gsize length;

                    bytes = (uint8_t*)g_variant_get_fixed_array (spec_val, &length, 1);
                    std::unique_ptr<P2P::InformationElementArray> array
                            (new P2P::InformationElementArray(length, bytes));
                    ie_.reset(new P2P::InformationElement (array));
                }
            }
        }
        g_variant_iter_free (service_array);
    } else if (g_strcmp0(name, "IPv4") == 0) {
        GVariantIter *ips;
        GVariant *spec_val;
        char *name;
        const char *remote = "";
        const char *local = "";

        g_variant_get (property, "a{sv}", &ips);
        while (g_variant_iter_loop (ips, "{sv}", &name, &spec_val)) {
            if (g_strcmp0 (name, "Remote") == 0) {
                remote = g_variant_get_string (spec_val, NULL);
            } else if (g_strcmp0 (name, "Local") == 0) {
                local = g_variant_get_string (spec_val, NULL);
            }
        }
        ips_changed (remote, local);

        g_variant_iter_free (ips);
    }
}

/* static C callback */
void Peer::connect_cb (GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GError *error = NULL;
    GDBusProxy *proxy = G_DBUS_PROXY (object);

    g_dbus_proxy_call_finish (proxy, res, &error);
    if (error) {
        std::cout << "connect error " << error->message << std::endl;
        g_clear_error (&error);
        return;
    }

    std::cout << "* connected "<< std::endl;
}

/* static C callback */
void Peer::disconnect_cb (GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GError *error = NULL;
    GDBusProxy *proxy = G_DBUS_PROXY (object);

    g_dbus_proxy_call_finish (proxy, res, &error);
    if (error) {
        std::cout << "disconnect error " << error->message << std::endl;
        g_clear_error (&error);
        return;
    }

    std::cout << "* disconnected "<< std::endl;
}

void Peer::proxy_cb (GAsyncResult *result)
{
    GError *error = NULL;

    proxy_ = g_dbus_proxy_new_for_bus_finish(result, &error);
    if (error) {
        std::cout << "Peer proxy error "<< std::endl;
        g_clear_error (&error);
        return;
    }

    g_signal_connect (proxy_, "g-signal",
                      G_CALLBACK (Peer::proxy_signal_cb), this);

    /* TODO check the ip address in case it's up to date already */

    if (observer_)
        observer_->on_initialized(this);
}

void Peer::ips_changed (const char *remote, const char *local)
{
    if (g_strcmp0 (remote, remote_host_.c_str()) == 0 &&
        g_strcmp0 (local, local_host_.c_str()) == 0)
        return;

    auto was_available = is_available();

    if (g_strcmp0 (remote, "0.0.0.0") == 0)
        remote_host_.clear();
    else
        remote_host_ = std::string(remote);

    if (g_strcmp0 (local, "0.0.0.0") == 0)
        local_host_.clear();
    else
        local_host_ = std::string(local);

    if (!observer_)
        return;

    if (was_available != is_available())
        observer_->on_availability_changed(this);
}

void Peer::state_changed (const char *state)
{
    bool ready = (g_strcmp0 (state, "ready") == 0);

    if (ready_ == ready)
        return;
    
    auto was_available = is_available();
    ready_ = ready;

    if (!observer_)
        return;

    if (was_available != is_available())
        observer_->on_availability_changed(this);
}

void Peer::name_changed (const char *name)
{
    if (g_strcmp0 (name, name_.c_str()) == 0)
        return;

    name_ = std::string (name);
}


Peer::Peer(const char *object_path, GVariantIter *props):
    observer_(NULL)
{
    GVariant *val;
    const char *prop_name;

    while (g_variant_iter_loop (props, "{&sv}", &prop_name, &val)) {
        handle_property_change (prop_name, val);
    }

    if (!ie_)
        throw std::invalid_argument("WiFiDisplayIEs is a required property");

    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                              G_DBUS_PROXY_FLAGS_NONE,
                              NULL,
                              "net.connman",
                              object_path,
                              "net.connman.Peer",
                              NULL,
                              Peer::proxy_cb,
                              this);
}

void Peer::connect()
{
    g_dbus_proxy_call (proxy_,
                       "Connect",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       60 * 1000, // is 1 minute too long?
                       NULL,
                       Peer::connect_cb,
                       this);
}

void Peer::disconnect()
{
    g_dbus_proxy_call (proxy_,
                       "Disconnect",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       Peer::disconnect_cb,
                       this);
}


Peer::~Peer()
{
    if (proxy_)
        g_clear_object (&proxy_);
}

}
