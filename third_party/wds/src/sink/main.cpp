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

#include <glib.h>
#include <glib-unix.h>
#include <gst/gst.h>

#include <iostream>

#include "mirac-glib-logging.hpp"

#include "sink-app.h"
#include "sink.h"


static gboolean _sig_handler (gpointer data_ptr)
{
    GMainLoop *main_loop = (GMainLoop *) data_ptr;

    g_main_loop_quit(main_loop);

    return G_SOURCE_CONTINUE;
}

static void parse_input_and_call_sink(
    const std::string& command, Sink *sink) {
    if (command == "teardown\n") {
        sink->Teardown();
        return;
    }
    if (command == "pause\n") {
        sink->Pause();
        return;
    }
    if (command == "play\n") {
        sink->Play();
        return;
    }
    std::cout << "Received unknown command: " << command << std::endl;
}

static gboolean _user_input_handler (
    GIOChannel* channel, GIOCondition /*condition*/, gpointer data_ptr)
{
    GError* error = NULL;
    char* str = NULL;
    size_t len;
    SinkApp* app = static_cast<SinkApp*>(data_ptr);

    switch (g_io_channel_read_line(channel, &str, &len, NULL, &error)) {
    case G_IO_STATUS_NORMAL:
        parse_input_and_call_sink(str, &app->sink());
        g_free(str);
        return true;
    case G_IO_STATUS_ERROR:
        std::cout << "User input error: " << error->message << std::endl;
        g_error_free(error);
        return false;
    case G_IO_STATUS_EOF:
    case G_IO_STATUS_AGAIN:
        return true;
    default:
        return false;
    }
    return false;
}

int main (int argc, char *argv[])
{
    InitGlibLogging();
    char* hostname = NULL;
    int port = 7236;
    std::unique_ptr<SinkApp> app;

    GOptionEntry main_entries[] = {
        { "hostname", 0, 0, G_OPTION_ARG_STRING, &hostname, "Specify remote hostname (for debugging purposes)", "host"},
        { "rtsp_port", 0, 0, G_OPTION_ARG_INT, &port, "Specify remote RTSP port number (for debugging purposes), 7236 by default", "rtsp_port"},
        { NULL }
    };

    GOptionContext* context = g_option_context_new ("- WFD sink demo application\n");
    g_option_context_add_main_entries (context, main_entries, NULL);
    g_option_context_add_group (context, gst_init_get_option_group ());

    GError* error = NULL;
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        WDS_ERROR ("option parsing failed: %s", error->message);
        g_option_context_free(context);
        exit (1);
    }
    g_option_context_free(context);

    if (hostname) {
        app.reset(new SinkApp(std::string(hostname), port));
        g_free (hostname);
    } else {
        app.reset(new SinkApp());
    }

    GMainLoop *main_loop =  g_main_loop_new(NULL, TRUE);
    g_unix_signal_add(SIGINT, _sig_handler, main_loop);
    g_unix_signal_add(SIGTERM, _sig_handler, main_loop);

    GIOChannel* io_channel = g_io_channel_unix_new (STDIN_FILENO);
    g_io_add_watch(io_channel, G_IO_IN, _user_input_handler, app.get());
    g_io_channel_unref(io_channel);

    g_main_loop_run (main_loop);

    g_main_loop_unref (main_loop);

    return 0;
}

