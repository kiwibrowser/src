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

#ifndef MIRAC_GST_TEST_SOURCE_HPP
#define MIRAC_GST_TEST_SOURCE_HPP

#include <gst/gst.h>

enum wfd_test_stream_t {WFD_TEST_AUDIO, WFD_TEST_VIDEO, WFD_TEST_BOTH, WFD_DESKTOP, WFD_UNKNOWN_STREAM};


class MiracGstTestSource
{
public:
    MiracGstTestSource(wfd_test_stream_t wfd_stream, std::string hostname, int port);
    ~MiracGstTestSource ();

    void SetState(GstState state);
    GstState GetState() const;

    int UdpSourcePort();

private:
    GstElement* gst_elem;
    guint bus_watch_id;
};

#endif
