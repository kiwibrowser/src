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

#include "mirac-gst-sink.hpp"
#include "mirac-gst-bus-handler.hpp"
#include "libwds/public/logging.h"

#include <cassert>

void _set_udp_caps(GstElement *playbin, GstElement *source, gpointer    user_data)
{
    GstCaps* caps = gst_caps_new_simple ("application/x-rtp",
        "media", G_TYPE_STRING, "video",
        "clock-rate", G_TYPE_INT, 1,
        "encoding-name", G_TYPE_STRING, "MP2T",
        NULL);

    g_object_set(source, "caps", caps, NULL);
    gst_caps_unref(caps);
}

MiracGstSink::MiracGstSink (std::string hostname, int port) {
  // todo (shalamov): move out pipeline initialization
  // from constructor, otherwise we can't check error conditions
  std::string gst_pipeline;

  std::string url =  "udp://" + (!hostname.empty() ? hostname  : "::") + (port > 0 ? ":" + std::to_string(port) : ":");
  gst_pipeline = "playbin uri=" + url;

  GError *err = NULL;
  gst_elem = gst_parse_launch(gst_pipeline.c_str(), &err);
  if (err != NULL) {
      WDS_ERROR("Cannot initialize gstreamer pipeline: [%s] %s", g_quark_to_string(err->domain), err->message);
  }

  if (gst_elem) {
      GstBus* bus = gst_pipeline_get_bus (GST_PIPELINE (gst_elem));
      bus_watch_id = gst_bus_add_watch (bus, mirac_gstbus_callback, this);
      gst_object_unref (bus);

      g_signal_connect(gst_elem, "source-setup", G_CALLBACK(_set_udp_caps), NULL);
      gst_element_set_state (gst_elem, GST_STATE_PLAYING);
  }
}

int MiracGstSink::sink_udp_port() {
    if (gst_elem == NULL)
        return 0;

    GstElement* source = NULL;
    g_object_get(gst_elem, "source", &source, NULL);

    if (source == NULL)
        return 0;

    gint port = 0;
    g_object_get(source, "port", &port, NULL);
    return port;
}

void MiracGstSink::Play() {
  assert(gst_elem);
  if(!IsInState(GST_STATE_PLAYING)) {
    gst_element_set_state(gst_elem, GST_STATE_PLAYING);
    IsInState(GST_STATE_PLAYING);
  }
}

void MiracGstSink::Pause() {
  assert(gst_elem);
  if(!IsPaused()) {
    gst_element_set_state(gst_elem, GST_STATE_PAUSED);
    IsPaused();
  }
}

void MiracGstSink::Teardown() {
  assert(gst_elem);
  if(!IsInState(GST_STATE_READY)) {
    gst_element_set_state(gst_elem, GST_STATE_READY);
    IsInState(GST_STATE_READY);
  }
}

bool MiracGstSink::IsPaused() const {
  return IsInState(GST_STATE_PAUSED);
}

bool MiracGstSink::IsInState(GstState state) const {
  assert(gst_elem);
  GstState current_state;
  gst_element_get_state(gst_elem, &current_state, NULL, GST_CLOCK_TIME_NONE);
  return current_state == state;
}

MiracGstSink::~MiracGstSink () {
  if (gst_elem) {
    gst_element_set_state (gst_elem, GST_STATE_NULL);
    g_source_remove (bus_watch_id);
    gst_object_unref (GST_OBJECT (gst_elem));
  }
}
