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

#include "source-app.h"
#include "mirac_broker_source.h"
#include "mirac-glib-logging.hpp"

#include "libwds/public/source.h"

static gboolean _sig_handler (gpointer data_ptr)
{
    GMainLoop *main_loop = (GMainLoop *) data_ptr;

    g_main_loop_quit(main_loop);

    return G_SOURCE_CONTINUE;
}

static void parse_input_and_call_source(
    const std::string& command, SourceApp *app) {

    auto source = app->source()->wfd_source();
    bool status = true;

    if (command == "scan\n") {
        app->scan();
    } else if (command.find("connect ") == 0) {
        app->connect(std::stoi(command.substr(8)));
    } else if (command == "teardown\n") {
        status = source && source->Teardown();
    } else if (command == "pause\n") {
        status = source && source->Pause();
    } else if (command == "play\n") {
        status = source && source->Play();
    } else {
        std::cout << "Received unknown command: " << command << std::endl;
    }

    if (!status)
        std::cout << "This command cannot be executed now." << std::endl;
}

static gboolean _user_input_handler (
    GIOChannel* channel, GIOCondition /*condition*/, gpointer data_ptr)
{
    GError* error = NULL;
    char* str = NULL;
    size_t len;
    SourceApp* app = static_cast<SourceApp*>(data_ptr);

    switch (g_io_channel_read_line(channel, &str, &len, NULL, &error)) {
    case G_IO_STATUS_NORMAL:
        parse_input_and_call_source(str, app);
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
    int port = 7236;

    GOptionEntry main_entries[] =
    {
        { "rtsp_port", 0, 0, G_OPTION_ARG_INT, &(port), "Specify optional RTSP port number, 7236 by default", "rtsp_port"},
        { NULL }
    };

    GOptionContext* context = g_option_context_new ("- WFD desktop source demo application\n");
    g_option_context_add_main_entries (context, main_entries, NULL);
    g_option_context_add_group (context, gst_init_get_option_group ());

    GError* error = NULL;
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        WDS_ERROR ("option parsing failed: %s", error->message);
        g_option_context_free(context);
        exit (1);
    }
    g_option_context_free(context);

    SourceApp app(port);

    GMainLoop *main_loop =  g_main_loop_new(NULL, TRUE);
    g_unix_signal_add(SIGINT, _sig_handler, main_loop);
    g_unix_signal_add(SIGTERM, _sig_handler, main_loop);

    GIOChannel* io_channel = g_io_channel_unix_new (STDIN_FILENO);
    g_io_add_watch(io_channel, G_IO_IN, _user_input_handler, &app);
    g_io_channel_unref(io_channel);


    g_main_loop_run (main_loop);

    g_main_loop_unref (main_loop);

    return 0;
}

