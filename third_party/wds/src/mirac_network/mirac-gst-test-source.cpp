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

#include <iostream>
#include <gio/gio.h>

#include "mirac-gst-test-source.hpp"
#include "mirac-gst-bus-handler.hpp"
#include "libwds/public/logging.h"

MiracGstTestSource::MiracGstTestSource (wfd_test_stream_t wfd_stream_type, std::string hostname, int port)
{
    std::string gst_pipeline;

    std::string hostname_port = (!hostname.empty() ? "host=" + hostname + " ": " ") + (port > 0 ? "port=" + std::to_string(port) : "");

    if (wfd_stream_type == WFD_TEST_BOTH) {
        gst_pipeline = "videotestsrc ! videoconvert ! video/x-raw,format=I420 ! x264enc ! muxer.  audiotestsrc ! avenc_ac3 ! muxer.  mpegtsmux name=muxer ! rtpmp2tpay ! udpsink name=sink " +
            hostname_port;
    } else if (wfd_stream_type == WFD_TEST_AUDIO) {
        gst_pipeline = "audiotestsrc ! avenc_ac3 ! mpegtsmux ! rtpmp2tpay ! udpsink name=sink " + hostname_port;
    } else if (wfd_stream_type == WFD_TEST_VIDEO) {
        gst_pipeline = "videotestsrc ! videoconvert ! video/x-raw,format=I420 ! x264enc ! mpegtsmux ! rtpmp2tpay ! udpsink name=sink " + hostname_port;
    } else if (wfd_stream_type == WFD_DESKTOP) {
        gst_pipeline = "ximagesrc ! videoconvert ! video/x-raw,format=I420 ! x264enc tune=zerolatency ! mpegtsmux ! rtpmp2tpay ! udpsink name=sink " + hostname_port;
    }

    GError *err = NULL;
    gst_elem = gst_parse_launch(gst_pipeline.c_str(), &err);
    if (err != NULL) {
        WDS_ERROR("Cannot initialize gstreamer pipeline: [%s] %s", g_quark_to_string(err->domain), err->message);
    }

    if (gst_elem) {
        GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (gst_elem));
        bus_watch_id = gst_bus_add_watch (bus, mirac_gstbus_callback, this);
        gst_object_unref (bus);
    }
}

void MiracGstTestSource::SetState(GstState state)
{
    if (gst_elem) {
        gst_element_set_state (gst_elem, state);
    }
}

GstState MiracGstTestSource::GetState() const
{
    if (!gst_elem)
        return GST_STATE_NULL;
    GstState result;
    gst_element_get_state (gst_elem, &result, NULL, GST_CLOCK_TIME_NONE);
    return result;
}

int MiracGstTestSource::UdpSourcePort()
{
    if (gst_elem == NULL)
        return 0;

    GstElement* sink = NULL;
    sink = gst_bin_get_by_name(GST_BIN(gst_elem), "sink");

    if (sink == NULL)
        return 0;

    GSocket* socket = NULL;
    g_object_get(sink, "used-socket", &socket, NULL);
    if (socket == NULL)
        return 0;

    guint16 port = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(g_socket_get_local_address(socket, NULL)));

    return port;
}

MiracGstTestSource::~MiracGstTestSource ()
{
    if (gst_elem) {
        gst_element_set_state (gst_elem, GST_STATE_NULL);
        g_source_remove (bus_watch_id);
        gst_object_unref (GST_OBJECT (gst_elem));
    }
}
