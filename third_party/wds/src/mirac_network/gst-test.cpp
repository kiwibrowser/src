/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
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
#include <iostream>
#include <memory>

#include <glib-unix.h>

#include "mirac-gst-test-source.hpp"
#include "mirac-gst-sink.hpp"
#include "mirac-glib-logging.hpp"

static gboolean _sig_handler (gpointer data_ptr)
{
    GMainLoop *ml = (GMainLoop *) data_ptr;

    g_main_loop_quit(ml);

    return G_SOURCE_CONTINUE;
}


int main (int argc, char *argv[])
{
    InitGlibLogging();

    GError *error = NULL;
    GOptionContext *context;
    GMainLoop* ml = NULL;
    
    gchar* wfd_device_option = NULL;
    gchar* wfd_stream_option = NULL;
    gchar* hostname_option = NULL;
    gint port = 0;
    
    GOptionEntry main_entries[] =
    {
        { "device", 0, 0, G_OPTION_ARG_STRING, &wfd_device_option, "Specify WFD device type: testsource or sink", "(testsource|sink)"},
        { "stream", 0, 0, G_OPTION_ARG_STRING, &wfd_stream_option, "Specify WFD stream type for testsource: audio, video, both or desktop capture", "(audio|video|both|desktop)"},
        { "hostname", 0, 0, G_OPTION_ARG_STRING, &hostname_option, "Specify optional hostname or ip address to stream to or listen on", "host"},
        { "port", 0, 0, G_OPTION_ARG_INT, &port, "Specify optional UDP port number to stream to or listen on", "port"},
        { NULL }
    };

    context = g_option_context_new ("- WFD source/sink demo application\n\nExample:\ngst-test --device=testsource --stream=both --hostname=127.0.0.1 --port=5000\ngst-test --device=sink --port=5000");
    g_option_context_add_main_entries (context, main_entries, NULL);
    
   if (!g_option_context_parse (context, &argc, &argv, &error)) {
        WDS_ERROR ("option parsing failed: %s", error->message);
        g_option_context_free(context);
        exit (1);
    }
    g_option_context_free(context);

    wfd_test_stream_t wfd_stream = WFD_UNKNOWN_STREAM;
    if (g_strcmp0(wfd_stream_option, "audio") == 0)
        wfd_stream = WFD_TEST_AUDIO;
    else if (g_strcmp0(wfd_stream_option, "video") == 0)
        wfd_stream = WFD_TEST_VIDEO;
    else if (g_strcmp0(wfd_stream_option, "both") == 0)
        wfd_stream = WFD_TEST_BOTH;
    else if (g_strcmp0(wfd_stream_option, "desktop") == 0)
        wfd_stream = WFD_DESKTOP;

    std::string hostname;
    if (hostname_option)
        hostname = hostname_option;

    g_free(wfd_stream_option);
    g_free(hostname_option);

    gst_init (&argc, &argv);

    std::unique_ptr<MiracGstSink> sink_pipeline;
    std::unique_ptr<MiracGstTestSource> source_pipeline;

    if (g_strcmp0(wfd_device_option, "testsource") == 0) {
        source_pipeline.reset(new MiracGstTestSource(wfd_stream, hostname, port));
        source_pipeline->SetState(GST_STATE_PLAYING);
        WDS_LOG("Source UDP port: %d", source_pipeline->UdpSourcePort());
    } else if (g_strcmp0(wfd_device_option, "sink") == 0) {
        sink_pipeline.reset(new MiracGstSink(hostname, port));
        WDS_LOG("Listening on port %d", sink_pipeline->sink_udp_port());
    }

    g_free(wfd_device_option);

    ml = g_main_loop_new(NULL, TRUE);
    g_unix_signal_add(SIGINT, _sig_handler, ml);
    g_unix_signal_add(SIGTERM, _sig_handler, ml);

    g_main_loop_run(ml);

    g_main_loop_unref(ml);
    
    return 0;
}

  