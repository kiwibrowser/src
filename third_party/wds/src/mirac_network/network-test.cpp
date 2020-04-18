/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * Contact: Jussi Laako <jussi.laako@linux.intel.com>
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


#include <glib.h>
#include <glib-unix.h>

#include <memory>

#include "mirac-network.hpp"


static gboolean _sig_handler (gpointer data_ptr)
{
    GMainLoop *ml = (GMainLoop *) data_ptr;

    g_main_loop_quit(ml);

    return G_SOURCE_CONTINUE;
}


static gboolean _send_cb (gint fd, GIOCondition condition,
    gpointer data_ptr)
{
    MiracNetwork *ctx = reinterpret_cast<MiracNetwork *> (data_ptr);

    try
    {
        return ctx->Send() ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
    }
    catch (std::exception &x)
    {
        g_warning("exception: %s", x.what());
        delete ctx;
    }
    return G_SOURCE_REMOVE;
}


static gboolean _receive_cb (gint fd, GIOCondition condition,
    gpointer data_ptr)
{
    MiracNetwork *ctx = reinterpret_cast<MiracNetwork *> (data_ptr);

    try
    {
        std::string msg;
        if (ctx->Receive(msg))
        {
            g_message("message: %s", msg.c_str());
            if (!ctx->Send(msg))
                g_unix_fd_add(ctx->GetHandle(), G_IO_OUT, _send_cb, ctx);
        }
    }
    catch (std::exception &x)
    {
        g_warning("exception: %s", x.what());
        delete ctx;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}


static gboolean _listen_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    try
    {
        MiracNetwork *listener = reinterpret_cast<MiracNetwork *> (data_ptr);
        MiracNetwork *ctx;

        ctx = listener->Accept();
        g_message("connection from: %s", ctx->GetPeerAddress().c_str());
        g_unix_fd_add(ctx->GetHandle(), G_IO_IN, _receive_cb, ctx);
    }
    catch (std::exception &x)
    {
        g_warning("exception: %s", x.what());
    }
    return G_SOURCE_CONTINUE;
}


static gboolean _sendmsg_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    try
    {
        MiracNetwork *ctx = reinterpret_cast<MiracNetwork *> (data_ptr);

        if (!ctx->Send (std::string("Hello world!\r\n\r\n")))
            g_unix_fd_add(ctx->GetHandle(), G_IO_OUT, _send_cb, ctx);

    }
    catch (std::exception &x)
    {
        g_warning("exception %s", x.what());
    }
    return G_SOURCE_REMOVE;
}


static gboolean _connect_cb (gint fd, GIOCondition condition, gpointer data_ptr)
{
    try
    {
        MiracNetwork *ctx = reinterpret_cast<MiracNetwork *> (data_ptr);

        if (!ctx->Connect(NULL, NULL))
            return G_SOURCE_CONTINUE;
        g_message("connection success to: %s", ctx->GetPeerAddress().c_str());
        g_unix_fd_add(ctx->GetHandle(), G_IO_OUT, _sendmsg_cb, ctx);
    }
    catch (std::exception &x)
    {
        g_warning("exception: %s", x.what());
    }
    return G_SOURCE_REMOVE;
}


int main (int argc, char *argv[])
{
    GMainLoop *ml = NULL;

    try
    {
        ml = g_main_loop_new(NULL, TRUE);
        g_unix_signal_add(SIGINT, _sig_handler, ml);
        g_unix_signal_add(SIGTERM, _sig_handler, ml);

        std::unique_ptr<MiracNetwork> net_listen(new MiracNetwork);
        std::unique_ptr<MiracNetwork> net_conn(new MiracNetwork);

        net_listen->Bind("127.0.0.1", "8080");
        g_message("bound to port %hu", net_listen->GetHostPort());
        g_unix_fd_add(net_listen->GetHandle(), G_IO_IN, _listen_cb,
            net_listen.get());

        if (net_conn->Connect("127.0.0.1", "8080"))
            g_unix_fd_add(net_conn->GetHandle(), G_IO_OUT, _sendmsg_cb,
                net_conn.get());
        else
            g_unix_fd_add(net_conn->GetHandle(), G_IO_OUT, _connect_cb,
                net_conn.get());

        g_main_loop_run(ml);
    }
    catch (std::exception &x)
    {
        g_error("exception: %s", x.what());
    }

    if (ml)
        g_main_loop_unref(ml);

    return 0;
}

